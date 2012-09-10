#include <GL/gl.h>
#include "yagl_host_gles_calls.h"
#include "yagl_apis/gles2/yagl_gles2_api_ts.h"
#include "yagl_egl_interface.h"
#include "yagl_gles_context.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_texture.h"
#include "yagl_gles_texture_unit.h"
#include "yagl_gles_buffer.h"
#include "yagl_gles_validate.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_mem_gl.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_sharegroup.h"

#define YAGL_SET_ERR(err) \
    yagl_gles_context_set_error(ctx, err); \
    YAGL_LOG_ERROR("error = 0x%X", err)

YAGL_DECLARE_TLS(struct yagl_gles1_api_ts*, gles1_api_ts);
YAGL_DECLARE_TLS(struct yagl_gles2_api_ts*, gles2_api_ts);

#define YAGL_GET_CTX_RET(func, ret) \
    struct yagl_thread_state *ts = NULL; \
    struct yagl_gles_context *ctx = NULL; \
    if (gles2_api_ts) { \
        ts = gles2_api_ts->ts; \
        ctx = (struct yagl_gles_context*)gles2_api_ts->egl_iface->get_ctx(gles2_api_ts->egl_iface); \
    } else if (gles1_api_ts) { \
        assert(0); \
        ts = NULL; \
        ctx = NULL; \
    } else { \
        assert(0); \
    } \
    YAGL_LOG_FUNC_SET_TS(ts, func); \
    if (!ctx || \
        ((ctx->base.client_api != yagl_client_api_gles1) && \
         (ctx->base.client_api != yagl_client_api_gles2))) { \
        YAGL_LOG_WARN("no current context"); \
        return ret; \
    } \
    (void)ts

#define YAGL_GET_CTX(func) YAGL_GET_CTX_RET(func,)

#define YAGL_UNIMPLEMENTED_RET(func, ret) \
    YAGL_GET_CTX_RET(func, ret); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    return ret

#define YAGL_UNIMPLEMENTED(func) \
    YAGL_GET_CTX(func); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!")

static __inline bool yagl_get_stride(struct yagl_gles_driver_ps *driver_ps,
                                     GLuint alignment_type,
                                     GLsizei width,
                                     GLenum format,
                                     GLenum type,
                                     GLsizei *stride)
{
    bool per_byte;
    GLsizei bpp;
    GLsizei alignment = 0;

    switch (type) {
    case GL_UNSIGNED_BYTE:
        per_byte = true;
        break;
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        per_byte = false;
        break;
    default:
        return false;
    }

    switch (format) {
    case GL_ALPHA:
        bpp = 1;
        break;
    case GL_RGB:
        bpp = (per_byte ? 3 : 2);
        break;
    case GL_RGBA:
        bpp = (per_byte ? 4 : 2);
        break;
    case GL_LUMINANCE:
        bpp = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        bpp = 2;
        break;
    default:
        return false;
    }

    driver_ps->GetIntegerv(driver_ps, alignment_type, &alignment);

    if (!alignment) {
        alignment = 1;
    }

    *stride = ((width * bpp) + alignment - 1) & ~(alignment - 1);

    return true;
}

static bool yagl_get_integer(struct yagl_gles_context *ctx,
                             GLenum pname,
                             GLint *param)
{
    switch (pname) {
    case GL_ACTIVE_TEXTURE:
        *param = GL_TEXTURE0 + ctx->active_texture_unit;
        break;
    case GL_TEXTURE_BINDING_2D:
        *param = yagl_gles_context_get_active_texture_target_state(ctx,
            yagl_gles_texture_target_2d)->texture_local_name;
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP:
        *param = yagl_gles_context_get_active_texture_target_state(ctx,
            yagl_gles_texture_target_cubemap)->texture_local_name;
        break;
    case GL_ARRAY_BUFFER_BINDING:
        *param = ctx->vbo_local_name;
        break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        *param = ctx->ebo_local_name;
        break;
    default:
        return false;
    }

    return true;
}

void yagl_host_glActiveTexture(GLenum texture)
{
    YAGL_GET_CTX(glActiveTexture);

    YAGL_BARRIER_ARG_NORET(ts);

    if (!yagl_gles_context_set_active_texture(ctx, texture)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    }
    ctx->driver_ps->ActiveTexture(ctx->driver_ps, texture);
}

void yagl_host_glBindBuffer(GLenum target,
    GLuint buffer)
{
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glBindBuffer);

    YAGL_BARRIER_ARG_NORET(ts);

    if (buffer != 0) {
        buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_BUFFER, buffer);

        if (!buffer_obj) {
            buffer_obj = yagl_gles_buffer_create(ctx->driver_ps);

            if (!buffer_obj) {
                goto out;
            }

            buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_add_named(ctx->base.sg,
               YAGL_NS_BUFFER, buffer, &buffer_obj->base);
        }
    }

    if (!yagl_gles_context_bind_buffer(ctx, target, buffer_obj, buffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

out:
    yagl_gles_buffer_release(buffer_obj);
}

void yagl_host_glBindTexture(GLenum target,
    GLuint texture)
{
    struct yagl_gles_texture *texture_obj = NULL;
    yagl_gles_texture_target texture_target;

    YAGL_GET_CTX(glBindTexture);

    YAGL_BARRIER_ARG_NORET(ts);

    if (!yagl_gles_validate_texture_target(target, &texture_target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (texture != 0) {
        texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_TEXTURE, texture);

        if (!texture_obj) {
            texture_obj = yagl_gles_texture_create(ctx->driver_ps);

            if (!texture_obj) {
                goto out;
            }

            texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_add_named(ctx->base.sg,
               YAGL_NS_TEXTURE, texture, &texture_obj->base);
        }

        if (!yagl_gles_texture_bind(texture_obj, target)) {
            YAGL_SET_ERR(GL_INVALID_OPERATION);
            goto out;
        }
    } else {
        ctx->driver_ps->BindTexture(ctx->driver_ps, target, 0);
    }

    yagl_gles_context_bind_texture(ctx, texture_target, texture);

out:
    yagl_gles_texture_release(texture_obj);
}

void yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor)
{
    YAGL_GET_CTX(glBlendFunc);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->BlendFunc(ctx->driver_ps, sfactor, dfactor);
}

void yagl_host_glBufferData(GLenum target,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_,
    GLenum usage)
{
    struct yagl_gles_buffer *buffer_obj = NULL;
    GLvoid *data = NULL;

    YAGL_GET_CTX(glBufferData);

    if (!yagl_gles_is_buffer_target_valid(target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (size < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!yagl_gles_is_buffer_usage_valid(usage)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    buffer_obj = yagl_gles_context_acquire_binded_buffer(ctx, target);

    if (!buffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (data_ && (size > 0)) {
        data = g_malloc(size);
        if (!yagl_mem_get(ts,
                          data_,
                          size,
                          data)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    yagl_gles_buffer_set_data(buffer_obj, size, data, usage);

out:
    g_free(data);
    yagl_gles_buffer_release(buffer_obj);
}

void yagl_host_glBufferSubData(GLenum target,
    GLintptr offset,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_)
{
    struct yagl_gles_buffer *buffer_obj = NULL;
    GLvoid *data = NULL;

    YAGL_GET_CTX(glBufferSubData);

    if (!yagl_gles_is_buffer_target_valid(target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if ((offset < 0) || (size < 0)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    buffer_obj = yagl_gles_context_acquire_binded_buffer(ctx, target);

    if (!buffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (size == 0) {
        goto out;
    }

    data = g_malloc(size);
    if (!yagl_mem_get(ts,
                      data_,
                      size,
                      data)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    YAGL_BARRIER_ARG_NORET(ts);

    if (!yagl_gles_buffer_update_data(buffer_obj, offset, size, data)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

out:
    g_free(data);
    yagl_gles_buffer_release(buffer_obj);
}

void yagl_host_glClear(GLbitfield mask)
{
    YAGL_GET_CTX(glClear);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Clear(ctx->driver_ps, mask);
}

void yagl_host_glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    YAGL_GET_CTX(glClearColor);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->ClearColor(ctx->driver_ps, red, green, blue, alpha);
}

void yagl_host_glClearDepthf(GLclampf depth)
{
    YAGL_GET_CTX(glClearDepthf);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->ClearDepthf(ctx->driver_ps, depth);
}

void yagl_host_glClearStencil(GLint s)
{
    YAGL_GET_CTX(glClearStencil);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->ClearStencil(ctx->driver_ps, s);
}

void yagl_host_glColorMask(GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha)
{
    YAGL_GET_CTX(glColorMask);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->ColorMask(ctx->driver_ps, red, green, blue, alpha);
}

void yagl_host_glCompressedTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_)
{
    GLvoid *data = NULL;

    YAGL_GET_CTX(glCompressedTexImage2D);

    if (data_ && (imageSize > 0)) {
        data = g_malloc(imageSize);
        if (!yagl_mem_get(ts,
                          data_,
                          imageSize,
                          data)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->CompressedTexImage2D(ctx->driver_ps,
                                         target,
                                         level,
                                         internalformat,
                                         width,
                                         height,
                                         border,
                                         imageSize,
                                         data);

out:
    g_free(data);
}

void yagl_host_glCompressedTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_)
{
    GLvoid *data = NULL;

    YAGL_GET_CTX(glCompressedTexSubImage2D);

    if (data_ && (imageSize > 0)) {
        data = g_malloc(imageSize);
        if (!yagl_mem_get(ts,
                          data_,
                          imageSize,
                          data)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->CompressedTexSubImage2D(ctx->driver_ps,
                                            target,
                                            level,
                                            xoffset,
                                            yoffset,
                                            width,
                                            height,
                                            format,
                                            imageSize,
                                            data);

out:
    g_free(data);
}

void yagl_host_glCopyTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border)
{
    YAGL_GET_CTX(glCopyTexImage2D);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->CopyTexImage2D(ctx->driver_ps,
                                   target,
                                   level,
                                   internalformat,
                                   x,
                                   y,
                                   width,
                                   height,
                                   border);
}

void yagl_host_glCopyTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glCopyTexSubImage2D);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->CopyTexSubImage2D(ctx->driver_ps,
                                      target,
                                      level,
                                      xoffset,
                                      yoffset,
                                      x,
                                      y,
                                      width,
                                      height);
}

void yagl_host_glCullFace(GLenum mode)
{
    YAGL_GET_CTX(glCullFace);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->CullFace(ctx->driver_ps, mode);
}

void yagl_host_glDeleteBuffers(GLsizei n,
    target_ulong /* const GLuint* */ buffers_)
{
    GLuint *buffer_names = NULL;
    GLsizei i;
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glDeleteBuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    buffer_names = g_malloc0(n * sizeof(*buffer_names));

    if (buffers_) {
        if (!yagl_mem_get(ts,
                          buffers_,
                          n * sizeof(*buffer_names),
                          buffer_names)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    if (buffer_names) {
        for (i = 0; i < n; ++i) {
            buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
                YAGL_NS_BUFFER, buffer_names[i]);

            if (buffer_obj) {
                yagl_gles_context_unbind_buffer(ctx, buffer_obj);
            }

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_BUFFER,
                                   buffer_names[i]);

            yagl_gles_buffer_release(buffer_obj);
        }
    }

out:
    g_free(buffer_names);
}

void yagl_host_glDeleteTextures(GLsizei n,
    target_ulong /* const GLuint* */ textures_)
{
    GLuint *texture_names = NULL;
    GLsizei i;

    YAGL_GET_CTX(glDeleteTextures);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    texture_names = g_malloc0(n * sizeof(*texture_names));

    if (textures_) {
        if (!yagl_mem_get(ts,
                          textures_,
                          n * sizeof(*texture_names),
                          texture_names)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    if (texture_names) {
        for (i = 0; i < n; ++i) {
            yagl_gles_context_unbind_texture(ctx, texture_names[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_TEXTURE,
                                   texture_names[i]);

        }
    }

out:
    g_free(texture_names);
}

void yagl_host_glDepthFunc(GLenum func)
{
    YAGL_GET_CTX(glDepthFunc);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->DepthFunc(ctx->driver_ps, func);
}

void yagl_host_glDepthMask(GLboolean flag)
{
    YAGL_GET_CTX(glDepthMask);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->DepthMask(ctx->driver_ps, flag);
}

void yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar)
{
    YAGL_GET_CTX(glDepthRangef);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->DepthRangef(ctx->driver_ps, zNear, zFar);
}

void yagl_host_glDisable(GLenum cap)
{
    YAGL_GET_CTX(glEnable);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Disable(ctx->driver_ps, cap);
}

void yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count)
{
    YAGL_GET_CTX(glDrawArrays);

    if ((first < 0) || (count < 0)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (count == 0) {
        return;
    }

    if (!yagl_gles_context_transfer_arrays(ctx, first, count)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->DrawArrays(ctx->driver_ps, mode, first, count);
}

void yagl_host_glDrawElements(GLenum mode,
    GLsizei count,
    GLenum type,
    target_ulong /* const GLvoid* */ indices_)
{
    GLvoid *indices = NULL;
    int index_size = 0;
    uint32_t first_idx = UINT32_MAX, last_idx = 0;
    int i;
    yagl_object_name old_buffer_name = 0;
    bool ebo_bound = false;

    YAGL_GET_CTX(glDrawElements);

    if (count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (count == 0) {
        goto out;
    }

    if (ctx->ebo) {
        if (!yagl_gles_buffer_get_minmax_index(ctx->ebo,
                                               type,
                                               indices_,
                                               count,
                                               &first_idx,
                                               &last_idx)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
        yagl_gles_buffer_bind(ctx->ebo,
                              type,
                              GL_ELEMENT_ARRAY_BUFFER,
                              &old_buffer_name);
        yagl_gles_buffer_transfer(ctx->ebo, type, GL_ELEMENT_ARRAY_BUFFER);
        ebo_bound = true;
    } else {
        if (!yagl_gles_get_index_size(type, &index_size)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }

        if (indices_) {
            indices = g_malloc(index_size * count);
            if (!yagl_mem_get(ts,
                              indices_,
                              index_size * count,
                              indices)) {
                YAGL_SET_ERR(GL_INVALID_VALUE);
                goto out;
            }
        } else {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }

        for (i = 0; i < count; ++i) {
            uint32_t idx = 0;
            switch (type) {
            case GL_UNSIGNED_BYTE:
                idx = ((uint8_t*)indices)[i];
                break;
            case GL_UNSIGNED_SHORT:
                idx = ((uint16_t*)indices)[i];
                break;
            default:
                assert(0);
                break;
            }
            if (idx < first_idx) {
                first_idx = idx;
            }
            if (idx > last_idx) {
                last_idx = idx;
            }
        }
    }

    if (!yagl_gles_context_transfer_arrays(ctx, first_idx, (last_idx + 1 - first_idx))) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->DrawElements(ctx->driver_ps,
                                 mode,
                                 count,
                                 type,
                                 (ctx->ebo ? (GLvoid*)indices_ : indices));

out:
    if (ebo_bound) {
        ctx->driver_ps->BindBuffer(ctx->driver_ps,
                                   GL_ELEMENT_ARRAY_BUFFER,
                                   old_buffer_name);
    }
    g_free(indices);
}

void yagl_host_glEnable(GLenum cap)
{
    YAGL_GET_CTX(glEnable);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Enable(ctx->driver_ps, cap);
}

void yagl_host_glFinish(void)
{
    YAGL_GET_CTX(glFinish);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Finish(ctx->driver_ps);
}

void yagl_host_glFlush(void)
{
    YAGL_GET_CTX(glFlush);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Flush(ctx->driver_ps);
}

void yagl_host_glFrontFace(GLenum mode)
{
    YAGL_GET_CTX(glFrontFace);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->FrontFace(ctx->driver_ps, mode);
}

void yagl_host_glGenBuffers(GLsizei n,
    target_ulong /* GLuint* */ buffers_)
{
    struct yagl_gles_buffer **buffers = NULL;
    GLsizei i;
    GLuint *buffer_names = NULL;

    YAGL_GET_CTX(glGenBuffers);

    buffers = g_malloc0(n * sizeof(*buffers));

    for (i = 0; i < n; ++i) {
        buffers[i] = yagl_gles_buffer_create(ctx->driver_ps);

        if (!buffers[i]) {
            goto out;
        }
    }

    buffer_names = g_malloc(n * sizeof(*buffer_names));

    for (i = 0; i < n; ++i) {
        buffer_names[i] = yagl_sharegroup_add(ctx->base.sg,
                                              YAGL_NS_BUFFER,
                                              &buffers[i]->base);
    }

    if (buffers_) {
        for (i = 0; i < n; ++i) {
            yagl_mem_put_GLuint(ts,
                                buffers_ + (i * sizeof(GLuint)),
                                buffer_names[i]);
        }
    }

out:
    g_free(buffer_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_buffer_release(buffers[i]);
    }
    g_free(buffers);
}

void yagl_host_glGenTextures(GLsizei n,
    target_ulong /* GLuint* */ textures_)
{
    struct yagl_gles_texture **textures = NULL;
    GLsizei i;
    GLuint *texture_names = NULL;

    YAGL_GET_CTX(glGenTextures);

    textures = g_malloc0(n * sizeof(*textures));

    for (i = 0; i < n; ++i) {
        textures[i] = yagl_gles_texture_create(ctx->driver_ps);

        if (!textures[i]) {
            goto out;
        }
    }

    texture_names = g_malloc(n * sizeof(*texture_names));

    for (i = 0; i < n; ++i) {
        texture_names[i] = yagl_sharegroup_add(ctx->base.sg,
                                               YAGL_NS_TEXTURE,
                                               &textures[i]->base);
    }

    if (textures_) {
        for (i = 0; i < n; ++i) {
            yagl_mem_put_GLuint(ts,
                                textures_ + (i * sizeof(GLuint)),
                                texture_names[i]);
        }
    }

out:
    g_free(texture_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_texture_release(textures[i]);
    }
    g_free(textures);
}

void yagl_host_glGetBooleanv(GLenum pname,
    target_ulong /* GLboolean* */ params_)
{
    int i, count = 0;
    GLboolean *params = NULL;

    YAGL_GET_CTX(glGetBooleanv);

    if (!ctx->get_param_count(ctx, pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    params = g_malloc0(count * sizeof(*params));

    if (!ctx->get_booleanv(ctx, pname, params)) {
        GLint param;

        if (yagl_get_integer(ctx, pname, &param)) {
            params[0] = ((param != 0) ? GL_TRUE : GL_FALSE);
        } else {
            ctx->driver_ps->GetBooleanv(ctx->driver_ps, pname, params);
        }
    }

    if (params_) {
        for (i = 0; i < count; ++i) {
            yagl_mem_put_GLuint(ts,
                                params_ + (i * sizeof(*params)),
                                params[i]);
        }
    }

out:
    g_free(params);
}

void yagl_host_glGetBufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params)
{
    YAGL_UNIMPLEMENTED(glGetBufferParameteriv);
}

GLenum yagl_host_glGetError(void)
{
    GLenum ret;

    YAGL_GET_CTX_RET(glGetError, GL_NO_ERROR);

    ret = yagl_gles_context_get_error(ctx);

    if (ret != GL_NO_ERROR) {
        return ret;
    }

    return ctx->driver_ps->GetError(ctx->driver_ps);
}

void yagl_host_glGetFloatv(GLenum pname,
    target_ulong /* GLfloat* */ params_)
{
    int i, count = 0;
    GLfloat *params = NULL;

    YAGL_GET_CTX(glGetFloatv);

    if (!ctx->get_param_count(ctx, pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    params = g_malloc0(count * sizeof(*params));

    if (!ctx->get_floatv(ctx, pname, params)) {
        GLint param;

        if (yagl_get_integer(ctx, pname, &param)) {
            params[0] = (GLfloat)param;
        } else {
            ctx->driver_ps->GetFloatv(ctx->driver_ps, pname, params);
        }
    }

    if (params_) {
        for (i = 0; i < count; ++i) {
            yagl_mem_put_GLuint(ts,
                                params_ + (i * sizeof(*params)),
                                params[i]);
        }
    }

out:
    g_free(params);
}

void yagl_host_glGetIntegerv(GLenum pname,
    target_ulong /* GLint* */ params_)
{
    int i, count = 0;
    GLint *params = NULL;

    YAGL_GET_CTX(glGetIntegerv);

    if (!ctx->get_param_count(ctx, pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    params = g_malloc0(count * sizeof(*params));

    if (!ctx->get_integerv(ctx, pname, params)) {
        if (!yagl_get_integer(ctx, pname, params)) {
            ctx->driver_ps->GetIntegerv(ctx->driver_ps, pname, params);
        }
    }

    if (params_) {
        for (i = 0; i < count; ++i) {
            yagl_mem_put_GLuint(ts,
                                params_ + (i * sizeof(*params)),
                                params[i]);
        }
    }

out:
    g_free(params);
}

void yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* GLfloat* */ params)
{
    YAGL_UNIMPLEMENTED(glGetTexParameterfv);
}

void yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params)
{
    YAGL_UNIMPLEMENTED(glGetTexParameteriv);
}

void yagl_host_glHint(GLenum target,
    GLenum mode)
{
    YAGL_GET_CTX(glHint);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Hint(ctx->driver_ps, target, mode);
}

GLboolean yagl_host_glIsBuffer(GLuint buffer)
{
    YAGL_UNIMPLEMENTED_RET(glIsBuffer, GL_FALSE);
}

GLboolean yagl_host_glIsEnabled(GLenum cap)
{
    YAGL_GET_CTX_RET(glIsEnabled, GL_FALSE);
    return ctx->driver_ps->IsEnabled(ctx->driver_ps, cap);
}

GLboolean yagl_host_glIsTexture(GLuint texture)
{
    YAGL_UNIMPLEMENTED_RET(glIsTexture, GL_FALSE);
}

void yagl_host_glLineWidth(GLfloat width)
{
    YAGL_GET_CTX(glLineWidth);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->LineWidth(ctx->driver_ps, width);
}

void yagl_host_glPixelStorei(GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glPixelStorei);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->PixelStorei(ctx->driver_ps, pname, param);
}

void yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units)
{
    YAGL_GET_CTX(glPolygonOffset);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->PolygonOffset(ctx->driver_ps, factor, units);
}

void yagl_host_glReadPixels(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* GLvoid* */ pixels)
{
    YAGL_UNIMPLEMENTED(glReadPixels);
}

void yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert)
{
    YAGL_GET_CTX(glSampleCoverage);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->SampleCoverage(ctx->driver_ps, value, invert);
}

void yagl_host_glScissor(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glScissor);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Scissor(ctx->driver_ps, x, y, width, height);
}

void yagl_host_glStencilFunc(GLenum func,
    GLint ref,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilFunc);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->StencilFunc(ctx->driver_ps, func, ref, mask);
}

void yagl_host_glStencilMask(GLuint mask)
{
    YAGL_GET_CTX(glStencilMask);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->StencilMask(ctx->driver_ps, mask);
}

void yagl_host_glStencilOp(GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    YAGL_GET_CTX(glStencilOp);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->StencilOp(ctx->driver_ps, fail, zfail, zpass);
}

void yagl_host_glTexImage2D(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_)
{
    GLvoid *pixels = NULL;
    GLsizei stride = 0;

    YAGL_GET_CTX(glTexImage2D);

    if (pixels_ && (width > 0) && (height > 0)) {
        if (!yagl_get_stride(ctx->driver_ps,
                             GL_UNPACK_ALIGNMENT,
                             width,
                             format,
                             type,
                             &stride)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
        pixels = g_malloc(stride * height);
        if (!yagl_mem_get(ts,
                          pixels_,
                          stride * height,
                          pixels)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->TexImage2D(ctx->driver_ps,
                               target,
                               level,
                               internalformat,
                               width,
                               height,
                               border,
                               format,
                               type,
                               pixels);

out:
    g_free(pixels);
}

void yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glTexParameterf);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->TexParameterf(ctx->driver_ps, target, pname, param);
}

void yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfloat* */ params)
{
    YAGL_UNIMPLEMENTED(glTexParameterfv);
}

void yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glTexParameteri);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->TexParameteri(ctx->driver_ps, target, pname, param);
}

void yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* const GLint* */ params)
{
    YAGL_UNIMPLEMENTED(glTexParameteriv);
}

void yagl_host_glTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_)
{
    GLvoid *pixels = NULL;
    GLsizei stride = 0;

    YAGL_GET_CTX(glTexSubImage2D);

    if (pixels_ && (width > 0) && (height > 0)) {
        if (!yagl_get_stride(ctx->driver_ps,
                             GL_UNPACK_ALIGNMENT,
                             width,
                             format,
                             type,
                             &stride)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
        pixels = g_malloc(stride * height);
        if (!yagl_mem_get(ts,
                          pixels_,
                          stride * height,
                          pixels)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->TexSubImage2D(ctx->driver_ps,
                                  target,
                                  level,
                                  xoffset,
                                  yoffset,
                                  width,
                                  height,
                                  format,
                                  type,
                                  pixels);

out:
    g_free(pixels);
}

void yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glViewport);

    YAGL_BARRIER_ARG_NORET(ts);

    ctx->driver_ps->Viewport(ctx->driver_ps, x, y, width, height);
}
