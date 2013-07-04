#include <GL/gl.h>
#include "yagl_host_gles_calls.h"
#include "yagl_egl_interface.h"
#include "yagl_gles_context.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_texture.h"
#include "yagl_gles_texture_unit.h"
#include "yagl_gles_buffer.h"
#include "yagl_gles_validate.h"
#include "yagl_gles_image.h"
#include "yagl_gles_framebuffer.h"
#include "yagl_gles_renderbuffer.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_mem_gl.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_sharegroup.h"

#define YAGL_SET_ERR(err) \
    yagl_gles_context_set_error(ctx, err); \
    YAGL_LOG_ERROR("error = 0x%X", err)

#define YAGL_GET_CTX_IMPL(func, ret_expr) \
    struct yagl_gles_context *ctx = \
        (struct yagl_gles_context*)cur_ts->ps->egl_iface->get_ctx(cur_ts->ps->egl_iface); \
    YAGL_LOG_FUNC_SET(func); \
    if (!ctx) { \
        YAGL_LOG_WARN("no current context"); \
        ret_expr; \
        return true; \
    }

#define YAGL_GET_CTX_RET(func, ret) YAGL_GET_CTX_IMPL(func, *retval = ret)

#define YAGL_GET_CTX(func) YAGL_GET_CTX_IMPL(func,)

#define YAGL_UNIMPLEMENTED_RET(func, ret) \
    YAGL_GET_CTX_RET(func, ret); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    *retval = ret; \
    return true

#define YAGL_UNIMPLEMENTED(func) \
    YAGL_GET_CTX(func); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    return true

/*
 * We can't include GLES2/gl2ext.h here
 */
#define GL_HALF_FLOAT_OES 0x8D61

static GLenum yagl_get_actual_type(GLenum type)
{
    switch (type) {
    case GL_HALF_FLOAT_OES:
        return GL_HALF_FLOAT;
    default:
        return type;
    }
}

static GLint yagl_get_stride(struct yagl_gles_context *ctx,
                             GLuint alignment_type,
                             GLsizei width,
                             GLenum format,
                             GLenum type,
                             GLsizei *stride)
{
    unsigned num_components;
    GLsizei bpp;
    GLsizei alignment = 0;

    switch (format) {
    case GL_ALPHA:
        num_components = 1;
        break;
    case GL_RGB:
        num_components = 3;
        break;
    case GL_RGBA:
        num_components = 4;
        break;
    case GL_LUMINANCE:
        num_components = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        num_components = 2;
        break;
    case GL_DEPTH_STENCIL_EXT:
        if (!ctx->pack_depth_stencil) {
            return GL_INVALID_ENUM;
        }
        if ((type == GL_FLOAT) || (type == GL_HALF_FLOAT_OES)) {
            return GL_INVALID_OPERATION;
        }
        num_components = 1;
        break;
    case GL_DEPTH_COMPONENT:
        if ((type != GL_UNSIGNED_SHORT) && (type != GL_UNSIGNED_INT)) {
            return GL_INVALID_OPERATION;
        }
        num_components = 1;
        break;
    default:
        return GL_INVALID_ENUM;
    }

    switch (type) {
    case GL_UNSIGNED_BYTE:
        bpp = num_components;
        break;
    case GL_UNSIGNED_SHORT_5_6_5:
        if (format != GL_RGB) {
            return GL_INVALID_OPERATION;
        }
        bpp = 2;
        break;
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        if (format != GL_RGBA) {
            return GL_INVALID_OPERATION;
        }
        bpp = 2;
        break;
    case GL_UNSIGNED_INT_24_8_EXT:
        if (!ctx->pack_depth_stencil) {
            return GL_INVALID_ENUM;
        }
        bpp = num_components * 4;
        break;
    case GL_UNSIGNED_SHORT:
        if (format != GL_DEPTH_COMPONENT) {
            return GL_INVALID_OPERATION;
        }
        bpp = num_components * 2;
        break;
    case GL_UNSIGNED_INT:
        if (format != GL_DEPTH_COMPONENT) {
            return GL_INVALID_OPERATION;
        }
        bpp = num_components * 4;
        break;
    case GL_FLOAT:
        bpp = num_components * 4;
        break;
    case GL_HALF_FLOAT_OES:
        bpp = num_components * 2;
        break;
    default:
        return GL_INVALID_ENUM;
    }

    ctx->driver->GetIntegerv(alignment_type, &alignment);

    if (!alignment) {
        alignment = 1;
    }

    *stride = ((width * bpp) + alignment - 1) & ~(alignment - 1);

    return GL_NO_ERROR;
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

bool yagl_host_glActiveTexture(GLenum texture)
{
    YAGL_GET_CTX(glActiveTexture);

    if (yagl_gles_context_set_active_texture(ctx, texture)) {
        ctx->driver->ActiveTexture(texture);
    } else {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    }

    return true;
}

bool yagl_host_glBindBuffer(GLenum target,
    GLuint buffer)
{
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glBindBuffer);

    if (buffer != 0) {
        buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_BUFFER, buffer);

        if (!buffer_obj) {
            buffer_obj = yagl_gles_buffer_create(ctx->driver);

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

    if (buffer_obj) {
        yagl_gles_buffer_set_bound(buffer_obj);
    }

out:
    yagl_gles_buffer_release(buffer_obj);

    return true;
}

bool yagl_host_glBindTexture(GLenum target,
    GLuint texture)
{
    struct yagl_gles_texture *texture_obj = NULL;
    yagl_gles_texture_target texture_target;

    YAGL_GET_CTX(glBindTexture);

    if (!yagl_gles_validate_texture_target(target, &texture_target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (texture != 0) {
        texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_TEXTURE, texture);

        if (!texture_obj) {
            texture_obj = yagl_gles_texture_create(ctx->driver);

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
        ctx->driver->BindTexture(target, 0);
    }

    yagl_gles_context_bind_texture(ctx, texture_target, texture_obj, texture);

out:
    yagl_gles_texture_release(texture_obj);

    return true;
}

bool yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor)
{
    YAGL_GET_CTX(glBlendFunc);

    ctx->driver->BlendFunc(sfactor, dfactor);

    return true;
}

bool yagl_host_glBlendEquation(GLenum mode)
{
    YAGL_GET_CTX(glBlendEquation);

    ctx->driver->BlendEquation(mode);

    return true;
}

bool yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha)
{
    YAGL_GET_CTX(glBlendFuncSeparate);

    ctx->driver->BlendFuncSeparate(srcRGB,
                                   dstRGB,
                                   srcAlpha,
                                   dstAlpha);

    return true;
}

bool yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha)
{
    YAGL_GET_CTX(glBlendEquationSeparate);

    ctx->driver->BlendEquationSeparate(modeRGB, modeAlpha);

    return true;
}

bool yagl_host_glBufferData(GLenum target,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_,
    GLenum usage)
{
    bool res = true;
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
        data = yagl_gles_context_malloc(ctx, size);
        if (!yagl_mem_get(data_,
                          size,
                          data)) {
            res = false;
            goto out;
        }
    }

    yagl_gles_buffer_set_data(buffer_obj, size, data, usage);

out:
    yagl_gles_buffer_release(buffer_obj);

    return res;
}

bool yagl_host_glBufferSubData(GLenum target,
    GLintptr offset,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_)
{
    bool res = true;
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

    data = yagl_gles_context_malloc(ctx, size);
    if (!yagl_mem_get(data_,
                      size,
                      data)) {
        res = false;
        goto out;
    }

    if (!yagl_gles_buffer_update_data(buffer_obj, offset, size, data)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

out:
    yagl_gles_buffer_release(buffer_obj);

    return res;
}

bool yagl_host_glClear(GLbitfield mask)
{
    YAGL_GET_CTX(glClear);

    ctx->driver->Clear(mask);

    return true;
}

bool yagl_host_glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    YAGL_GET_CTX(glClearColor);

    ctx->driver->ClearColor(red, green, blue, alpha);

    return true;
}

bool yagl_host_glClearDepthf(GLclampf depth)
{
    YAGL_GET_CTX(glClearDepthf);

    ctx->driver->ClearDepth(depth);

    return true;
}

bool yagl_host_glClearStencil(GLint s)
{
    YAGL_GET_CTX(glClearStencil);

    ctx->driver->ClearStencil(s);

    return true;
}

bool yagl_host_glColorMask(GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha)
{
    YAGL_GET_CTX(glColorMask);

    ctx->driver->ColorMask(red, green, blue, alpha);

    return true;
}

bool yagl_host_glCompressedTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_)
{
    bool res = true;
    GLvoid *data = NULL;
    GLenum err_code;

    YAGL_GET_CTX(glCompressedTexImage2D);

    if (data_ && (imageSize > 0)) {
        data = yagl_gles_context_malloc(ctx, imageSize);
        if (!yagl_mem_get(data_,
                          imageSize,
                          data)) {
            res = false;
            goto out;
        }
    }

    if (target == GL_TEXTURE_2D) {
        struct yagl_gles_texture_target_state *tex_target_state =
            yagl_gles_context_get_active_texture_target_state(ctx,
                                                              yagl_gles_texture_target_2d);
        if (tex_target_state->texture) {
            /*
             * This operation should orphan EGLImage according
             * to OES_EGL_image specs.
             */
            yagl_gles_texture_unset_image(tex_target_state->texture);

            /*
             * This operation should release TexImage according
             * to eglBindTexImage spec.
             */
            yagl_gles_texture_release_tex_image(tex_target_state->texture);
        }
    }

    err_code = ctx->compressed_tex_image(ctx,
                                         target,
                                         level,
                                         internalformat,
                                         width,
                                         height,
                                         border,
                                         imageSize,
                                         data);

    if (err_code != GL_NO_ERROR) {
        YAGL_SET_ERR(err_code);
    }

out:
    return res;
}

bool yagl_host_glCompressedTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_)
{
    bool res = true;
    GLvoid *data = NULL;

    YAGL_GET_CTX(glCompressedTexSubImage2D);

    if (ctx->base.client_api == yagl_client_api_gles1) {
        /* No formats are supported by this call in GLES1 API */
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        return true;
    }

    if (data_ && (imageSize > 0)) {
        data = yagl_gles_context_malloc(ctx, imageSize);
        if (!yagl_mem_get(data_,
                          imageSize,
                          data)) {
            res = false;
            goto out;
        }
    }

    ctx->driver->CompressedTexSubImage2D(target,
                                         level,
                                         xoffset,
                                         yoffset,
                                         width,
                                         height,
                                         format,
                                         imageSize,
                                         data);

out:
    return res;
}

bool yagl_host_glCopyTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border)
{
    YAGL_GET_CTX(glCopyTexImage2D);

    if (ctx->base.client_api == yagl_client_api_gles1 &&
        target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (border != 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    ctx->driver->CopyTexImage2D(target,
                                level,
                                internalformat,
                                x,
                                y,
                                width,
                                height,
                                border);

    return true;
}

bool yagl_host_glCopyTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glCopyTexSubImage2D);

    if (ctx->base.client_api == yagl_client_api_gles1 &&
        target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    ctx->driver->CopyTexSubImage2D(target,
                                   level,
                                   xoffset,
                                   yoffset,
                                   x,
                                   y,
                                   width,
                                   height);

    return true;
}

bool yagl_host_glCullFace(GLenum mode)
{
    YAGL_GET_CTX(glCullFace);

    ctx->driver->CullFace(mode);

    return true;
}

bool yagl_host_glDeleteBuffers(GLsizei n,
    target_ulong /* const GLuint* */ buffers_)
{
    bool res = true;
    GLuint *buffer_names = NULL;
    GLsizei i;
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glDeleteBuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    buffer_names = yagl_gles_context_malloc0(ctx, n * sizeof(*buffer_names));

    if (buffers_) {
        if (!yagl_mem_get(buffers_,
                          n * sizeof(*buffer_names),
                          buffer_names)) {
            res = false;
            goto out;
        }
    }

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
    return res;
}

bool yagl_host_glDeleteTextures(GLsizei n,
    target_ulong /* const GLuint* */ textures_)
{
    bool res = true;
    GLuint *texture_names = NULL;
    GLsizei i;

    YAGL_GET_CTX(glDeleteTextures);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    texture_names = yagl_gles_context_malloc0(ctx, n * sizeof(*texture_names));

    if (textures_) {
        if (!yagl_mem_get(textures_,
                          n * sizeof(*texture_names),
                          texture_names)) {
            res = false;
            goto out;
        }
    }

    if (texture_names) {
        for (i = 0; i < n; ++i) {
            yagl_gles_context_unbind_texture(ctx, texture_names[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_TEXTURE,
                                   texture_names[i]);

        }
    }

out:
    return res;
}

bool yagl_host_glDepthFunc(GLenum func)
{
    YAGL_GET_CTX(glDepthFunc);

    ctx->driver->DepthFunc(func);

    return true;
}

bool yagl_host_glDepthMask(GLboolean flag)
{
    YAGL_GET_CTX(glDepthMask);

    ctx->driver->DepthMask(flag);

    return true;
}

bool yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar)
{
    YAGL_GET_CTX(glDepthRangef);

    ctx->driver->DepthRange(zNear, zFar);

    return true;
}

bool yagl_host_glDisable(GLenum cap)
{
    YAGL_GET_CTX(glDisable);

    ctx->driver->Disable(cap);

    if (cap == GL_TEXTURE_2D) {
        yagl_gles_context_active_texture_set_enabled(ctx,
            yagl_gles_texture_target_2d, false);
    }

    return true;
}

bool yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count)
{
    YAGL_GET_CTX(glDrawArrays);

    if ((first < 0) || (count < 0)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    if (count == 0) {
        return true;
    }

    if (!yagl_gles_context_transfer_arrays(ctx, first, count)) {
        return false;
    }

    ctx->draw_arrays(ctx, mode, first, count);

    return true;
}

/*
 * Be sure to support 'glGetVertexAttribRangeYAGL' together with
 * this function.
 */
bool yagl_host_glDrawElements(GLenum mode,
    GLsizei count,
    GLenum type,
    target_ulong /* const GLvoid* */ indices_)
{
    bool res = true;
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
                              false,
                              GL_ELEMENT_ARRAY_BUFFER,
                              &old_buffer_name);
        yagl_gles_buffer_transfer(ctx->ebo,
                                  type,
                                  GL_ELEMENT_ARRAY_BUFFER,
                                  false);
        ebo_bound = true;
    } else {
        if (!yagl_gles_get_index_size(type, &index_size)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }

        if (indices_) {
            indices = yagl_gles_context_malloc0(ctx, index_size * count);
            if (!yagl_mem_get(indices_,
                              index_size * count,
                              indices)) {
                res = false;
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
        res = false;
        goto out;
    }

    ctx->draw_elements(ctx,
                       mode,
                       count,
                       type,
                       (ctx->ebo ? (GLvoid *)(uintptr_t)indices_ : indices));

out:
    if (ebo_bound) {
        ctx->driver->BindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                old_buffer_name);
    }

    return res;
}

bool yagl_host_glEnable(GLenum cap)
{
    YAGL_GET_CTX(glEnable);

    ctx->driver->Enable(cap);

    if (cap == GL_TEXTURE_2D) {
        yagl_gles_context_active_texture_set_enabled(ctx,
            yagl_gles_texture_target_2d, true);
    }

    return true;
}

bool yagl_host_glFinish(void)
{
    YAGL_GET_CTX(glFinish);

    ctx->driver->Finish();

    return true;
}

bool yagl_host_glFlush(void)
{
    YAGL_GET_CTX(glFlush);

    ctx->driver->Flush();

    return true;
}

bool yagl_host_glFrontFace(GLenum mode)
{
    YAGL_GET_CTX(glFrontFace);

    ctx->driver->FrontFace(mode);

    return true;
}

bool yagl_host_glGenBuffers(GLsizei n,
    target_ulong /* GLuint* */ buffers_)
{
    bool res = true;
    struct yagl_gles_buffer **buffers = NULL;
    GLsizei i;
    GLuint *buffer_names = NULL;

    YAGL_GET_CTX(glGenBuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, buffers_, n * sizeof(*buffer_names))) {
        res = false;
        goto out;
    }

    buffers = g_malloc0(n * sizeof(*buffers));

    for (i = 0; i < n; ++i) {
        buffers[i] = yagl_gles_buffer_create(ctx->driver);

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
        yagl_mem_put(cur_ts->mt1, buffer_names);
    }

out:
    g_free(buffer_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_buffer_release(buffers[i]);
    }
    g_free(buffers);

    return res;
}

bool yagl_host_glGenTextures(GLsizei n,
    target_ulong /* GLuint* */ textures_)
{
    bool res = true;
    struct yagl_gles_texture **textures = NULL;
    GLsizei i;
    GLuint *texture_names = NULL;

    YAGL_GET_CTX(glGenTextures);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, textures_, n * sizeof(*texture_names))) {
        res = false;
        goto out;
    }

    textures = g_malloc0(n * sizeof(*textures));

    for (i = 0; i < n; ++i) {
        textures[i] = yagl_gles_texture_create(ctx->driver);

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
        yagl_mem_put(cur_ts->mt1, texture_names);
    }

out:
    g_free(texture_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_texture_release(textures[i]);
    }
    g_free(textures);

    return res;
}

bool yagl_host_glGetBooleanv(GLenum pname,
    target_ulong /* GLboolean* */ params_)
{
    bool res = true;
    int count = 0;
    GLboolean *params = NULL;

    YAGL_GET_CTX(glGetBooleanv);

    if (!ctx->get_param_count(ctx, pname, &count)) {
        YAGL_LOG_ERROR("Bad pname = 0x%X", pname);
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(*params))) {
        res = false;
        goto out;
    }

    params = yagl_gles_context_malloc0(ctx, count * sizeof(*params));

    if (!ctx->get_booleanv(ctx, pname, params)) {
        GLint param;

        if (yagl_get_integer(ctx, pname, &param)) {
            params[0] = ((param != 0) ? GL_TRUE : GL_FALSE);
        } else {
            ctx->driver->GetBooleanv(pname, params);
        }
    }

    if (params_) {
        yagl_mem_put(cur_ts->mt1, params);
    }

out:
    return res;
}

bool yagl_host_glGetBufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    bool res = true;
    struct yagl_gles_buffer *buffer_obj = NULL;
    GLint param = 0;

    YAGL_GET_CTX(glGetBufferParameteriv);

    if (!yagl_mem_prepare_GLint(cur_ts->mt1, params_)) {
        res = false;
        goto out;
    }

    if (!yagl_gles_is_buffer_target_valid(target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    buffer_obj = yagl_gles_context_acquire_binded_buffer(ctx, target);

    if (!buffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles_buffer_get_parameter(buffer_obj,
                                        pname,
                                        &param)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (params_) {
        yagl_mem_put_GLint(cur_ts->mt1, param);
    }

out:
    yagl_gles_buffer_release(buffer_obj);

    return res;
}

bool yagl_host_glGetError(GLenum* retval)
{
    YAGL_GET_CTX_RET(glGetError, GL_NO_ERROR);

    *retval = yagl_gles_context_get_error(ctx);

    if (*retval == GL_NO_ERROR) {
        *retval = ctx->driver->GetError();
    }

    return true;
}

bool yagl_host_glGetFloatv(GLenum pname,
    target_ulong /* GLfloat* */ params_)
{
    bool res = true;
    int count = 0;
    GLfloat *params = NULL;

    YAGL_GET_CTX(glGetFloatv);

    if (!ctx->get_param_count(ctx, pname, &count)) {
        YAGL_LOG_ERROR("Bad pname = 0x%X", pname);
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(*params))) {
        res = false;
        goto out;
    }

    params = yagl_gles_context_malloc0(ctx, count * sizeof(*params));

    if (!ctx->get_floatv(ctx, pname, params)) {
        GLint param;

        if (yagl_get_integer(ctx, pname, &param)) {
            params[0] = (GLfloat)param;
        } else {
            ctx->driver->GetFloatv(pname, params);
        }
    }

    if (params_) {
        yagl_mem_put(cur_ts->mt1, params);
    }

out:
    return res;
}

bool yagl_host_glGetIntegerv(GLenum pname,
    target_ulong /* GLint* */ params_)
{
    bool res = true;
    int count = 0;
    GLint *params = NULL;

    YAGL_GET_CTX(glGetIntegerv);

    if (!ctx->get_param_count(ctx, pname, &count)) {
        YAGL_LOG_ERROR("Bad pname = 0x%X", pname);
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(*params))) {
        res = false;
        goto out;
    }

    params = yagl_gles_context_malloc0(ctx, count * sizeof(*params));

    if (!ctx->get_integerv(ctx, pname, params)) {
        if (!yagl_get_integer(ctx, pname, params)) {
            ctx->driver->GetIntegerv(pname, params);
        }
    }

    if (params_) {
        yagl_mem_put(cur_ts->mt1, params);
    }

out:
    return res;
}

bool yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* GLfloat* */ params_)
{
    GLfloat params[10];

    YAGL_GET_CTX(glGetTexParameterfv);

    if (!yagl_mem_prepare_GLfloat(cur_ts->mt1, params_)) {
        return false;
    }

    ctx->driver->GetTexParameterfv(target,
                                   pname,
                                   params);

    if (params_) {
        yagl_mem_put_GLfloat(cur_ts->mt1, params[0]);
    }

    return true;
}

bool yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    GLint params[10];

    YAGL_GET_CTX(glGetTexParameteriv);

    if (!yagl_mem_prepare_GLint(cur_ts->mt1, params_)) {
        return false;
    }

    ctx->driver->GetTexParameteriv(target,
                                   pname,
                                   params);

    if (params_) {
        yagl_mem_put_GLint(cur_ts->mt1, params[0]);
    }

    return true;
}

bool yagl_host_glHint(GLenum target,
    GLenum mode)
{
    YAGL_GET_CTX(glHint);

    ctx->driver->Hint(target, mode);

    return true;
}

bool yagl_host_glIsBuffer(GLboolean* retval, GLuint buffer)
{
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX_RET(glIsBuffer, GL_FALSE);

    *retval = GL_FALSE;

    buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_BUFFER, buffer);

    if (buffer_obj && yagl_gles_buffer_was_bound(buffer_obj)) {
        *retval = GL_TRUE;
    }

    yagl_gles_buffer_release(buffer_obj);

    return true;
}

bool yagl_host_glIsEnabled(GLboolean* retval, GLenum cap)
{
    YAGL_GET_CTX_RET(glIsEnabled, GL_FALSE);

    if (!ctx->is_enabled(ctx, retval, cap)) {
        *retval = ctx->driver->IsEnabled(cap);
    }

    return true;
}

bool yagl_host_glIsTexture(GLboolean* retval, GLuint texture)
{
    struct yagl_gles_texture *texture_obj = NULL;

    YAGL_GET_CTX_RET(glIsTexture, GL_FALSE);

    *retval = GL_FALSE;

    texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_TEXTURE, texture);

    if (texture_obj && (yagl_gles_texture_get_target(texture_obj) != 0)) {
        *retval = GL_TRUE;
    }

    yagl_gles_texture_release(texture_obj);

    return true;
}

bool yagl_host_glLineWidth(GLfloat width)
{
    YAGL_GET_CTX(glLineWidth);

    ctx->driver->LineWidth(width);

    return true;
}

bool yagl_host_glPixelStorei(GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glPixelStorei);

    ctx->driver->PixelStorei(pname, param);

    return true;
}

bool yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units)
{
    YAGL_GET_CTX(glPolygonOffset);

    ctx->driver->PolygonOffset(factor, units);

    return true;
}

bool yagl_host_glReadPixels(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* GLvoid* */ pixels_)
{
    bool res = true;
    yagl_object_name current_pbo = 0;
    GLvoid *pixels = NULL;
    GLsizei stride = 0;
    GLenum actual_type = yagl_get_actual_type(type);

    YAGL_GET_CTX(glReadPixels);

    if (pixels_ && (width > 0) && (height > 0)) {
        GLint err = yagl_get_stride(ctx,
                                    GL_PACK_ALIGNMENT,
                                    width,
                                    format,
                                    type,
                                    &stride);

        if (err != GL_NO_ERROR) {
            YAGL_SET_ERR(err);
            goto out;
        }
        if (!yagl_mem_prepare(cur_ts->mt1, pixels_, stride * height)) {
            res = false;
            goto out;
        }
        pixels = yagl_gles_context_malloc(ctx, stride * height);
        ctx->driver->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB,
                                 (GLint*)&current_pbo);
        if (current_pbo != 0) {
            ctx->driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
                                    0);
        }

        ctx->driver->ReadPixels(x,
                                y,
                                width,
                                height,
                                format,
                                actual_type,
                                pixels);

        if (current_pbo != 0) {
            ctx->driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
                                    current_pbo);
        }

        yagl_mem_put(cur_ts->mt1, pixels);
    }

out:
    return res;
}

bool yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert)
{
    YAGL_GET_CTX(glSampleCoverage);

    ctx->driver->SampleCoverage(value, invert);

    return true;
}

bool yagl_host_glScissor(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glScissor);

    ctx->driver->Scissor(x, y, width, height);

    return true;
}

bool yagl_host_glStencilFunc(GLenum func,
    GLint ref,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilFunc);

    ctx->driver->StencilFunc(func, ref, mask);

    return true;
}

bool yagl_host_glStencilMask(GLuint mask)
{
    YAGL_GET_CTX(glStencilMask);

    ctx->driver->StencilMask(mask);

    return true;
}

bool yagl_host_glStencilOp(GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    YAGL_GET_CTX(glStencilOp);

    ctx->driver->StencilOp(fail, zfail, zpass);

    return true;
}

bool yagl_host_glTexImage2D(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_)
{
    bool res = true;
    GLvoid *pixels = NULL;
    GLsizei stride = 0;
    GLenum actual_type = yagl_get_actual_type(type);

    YAGL_GET_CTX(glTexImage2D);

    if (pixels_ && (width > 0) && (height > 0)) {
        GLint err = yagl_get_stride(ctx,
                                    GL_UNPACK_ALIGNMENT,
                                    width,
                                    format,
                                    type,
                                    &stride);

        if (err != GL_NO_ERROR) {
            YAGL_SET_ERR(err);
            goto out;
        }
        pixels = yagl_gles_context_malloc(ctx, stride * height);
        if (!yagl_mem_get(pixels_,
                          stride * height,
                          pixels)) {
            res = false;
            goto out;
        }
    }

    if (target == GL_TEXTURE_2D) {
        struct yagl_gles_texture_target_state *tex_target_state =
            yagl_gles_context_get_active_texture_target_state(ctx,
                                                              yagl_gles_texture_target_2d);
        if (tex_target_state->texture) {
            /*
             * This operation should orphan EGLImage according
             * to OES_EGL_image specs.
             */
            yagl_gles_texture_unset_image(tex_target_state->texture);

            /*
             * This operation should release TexImage according
             * to eglBindTexImage spec.
             */
            yagl_gles_texture_release_tex_image(tex_target_state->texture);
        }
    }

    ctx->driver->TexImage2D(target,
                            level,
                            internalformat,
                            width,
                            height,
                            border,
                            format,
                            actual_type,
                            pixels);

out:
    return res;
}

bool yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glTexParameterf);

    ctx->driver->TexParameterf(target, pname, param);

    return true;
}

bool yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    bool res = true;
    GLfloat params[10];

    YAGL_GET_CTX(glTexParameterfv);

    memset(params, 0, sizeof(params));

    if (params_) {
        if (!yagl_mem_get_GLfloat(params_, params)) {
            res = false;
            goto out;
        }
    }

    ctx->driver->TexParameterfv(target,
                                pname,
                                (params_ ? params : NULL));

out:
    return res;
}

bool yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glTexParameteri);

    ctx->driver->TexParameteri(target, pname, param);

    return true;
}

bool yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* const GLint* */ params_)
{
    bool res = true;
    GLint params[10];

    YAGL_GET_CTX(glTexParameteriv);

    memset(params, 0, sizeof(params));

    if (params_) {
        if (!yagl_mem_get_GLint(params_, params)) {
            res = false;
            goto out;
        }
    }

    ctx->driver->TexParameteriv(target,
                                pname,
                                (params_ ? params : NULL));

out:
    return res;
}

bool yagl_host_glTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_)
{
    bool res = true;
    GLvoid *pixels = NULL;
    GLsizei stride = 0;
    GLenum actual_type = yagl_get_actual_type(type);

    YAGL_GET_CTX(glTexSubImage2D);

    if (pixels_ && (width > 0) && (height > 0)) {
        GLint err = yagl_get_stride(ctx,
                                    GL_UNPACK_ALIGNMENT,
                                    width,
                                    format,
                                    type,
                                    &stride);

        if (err != GL_NO_ERROR) {
            YAGL_SET_ERR(err);
            goto out;
        }
        pixels = yagl_gles_context_malloc(ctx, stride * height);
        if (!yagl_mem_get(pixels_,
                          stride * height,
                          pixels)) {
            res = false;
            goto out;
        }
    }

    /*
     * Nvidia Windows openGL drivers doesn't account for GL_UNPACK_ALIGNMENT
     * parameter when glTexSubImage2D function is called with format GL_ALPHA.
     * Work around this by manually setting line stride.
     */
    if (format == GL_ALPHA) {
        ctx->driver->PixelStorei(GL_UNPACK_ROW_LENGTH,
                                 stride);
    }

    ctx->driver->TexSubImage2D(target,
                               level,
                               xoffset,
                               yoffset,
                               width,
                               height,
                               format,
                               actual_type,
                               pixels);

    if (format == GL_ALPHA) {
        ctx->driver->PixelStorei(GL_UNPACK_ROW_LENGTH,
                                 0);
    }

out:
    return res;
}

bool yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glViewport);

    ctx->driver->Viewport(x, y, width, height);

    return true;
}

bool yagl_host_glEGLImageTargetTexture2DOES(GLenum target,
    yagl_host_handle image_)
{
    struct yagl_gles_image *image = NULL;
    struct yagl_gles_texture_target_state *tex_target_state;

    YAGL_GET_CTX(glEGLImageTargetTexture2DOES);

    if (target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    image = (struct yagl_gles_image*)cur_ts->ps->egl_iface->get_image(cur_ts->ps->egl_iface, image_);

    if (!image) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    tex_target_state =
        yagl_gles_context_get_active_texture_target_state(ctx,
                                                          yagl_gles_texture_target_2d);

    if (!tex_target_state->texture) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles_texture_set_image(tex_target_state->texture, image);

out:
    yagl_gles_image_release(image);

    return true;
}

bool yagl_host_glGetExtensionStringYAGL(GLuint* retval, target_ulong /* GLchar* */ str_)
{
    bool res = true;
    GLchar *str = NULL;
    GLuint str_len;

    YAGL_GET_CTX_RET(glGetExtensionStringYAGL, 1);

    str = ctx->get_extensions(ctx);

    str_len = strlen(str);

    if (!yagl_mem_prepare(cur_ts->mt1, str_, str_len + 1)) {
        res = false;
        goto out;
    }

    if (str_) {
        yagl_mem_put(cur_ts->mt1, str);
    }

    *retval = str_len + 1;

out:
    g_free(str);

    return res;
}

bool yagl_host_glGetVertexAttribRangeYAGL(GLsizei count,
    GLenum type,
    target_ulong /* const GLvoid* */ indices_,
    target_ulong /* GLint* */ range_first_,
    target_ulong /* GLsizei* */ range_count_)
{
    bool res = true;
    GLvoid *indices = NULL;
    int index_size = 0;
    uint32_t first_idx = UINT32_MAX, last_idx = 0;
    int i;

    YAGL_GET_CTX(glGetVertexAttribRangesYAGL);

    if (!yagl_mem_prepare_GLint(cur_ts->mt1, range_first_) ||
        !yagl_mem_prepare_GLsizei(cur_ts->mt2, range_count_)) {
        res = false;
        goto out;
    }

    if (ctx->ebo) {
        if (!yagl_gles_buffer_get_minmax_index(ctx->ebo,
                                               type,
                                               indices_,
                                               count,
                                               &first_idx,
                                               &last_idx)) {
            goto out;
        }
    } else {
        if (!yagl_gles_get_index_size(type, &index_size)) {
            goto out;
        }

        if (indices_) {
            indices = yagl_gles_context_malloc0(ctx, index_size * count);
            if (!yagl_mem_get(indices_,
                              index_size * count,
                              indices)) {
                res = false;
                goto out;
            }
        } else {
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

    if (range_first_) {
        yagl_mem_put_GLint(cur_ts->mt1, (GLint)first_idx);
    }

    if (range_count_) {
        yagl_mem_put_GLsizei(cur_ts->mt2, (GLsizei)(last_idx + 1 - first_idx));
    }

out:
    return res;
}

bool yagl_host_glEGLUpdateOffscreenImageYAGL(struct yagl_client_image *image,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* const void* */ pixels_)
{
    struct yagl_gles_image *gles_image = (struct yagl_gles_image*)image;
    bool res = true;
    void *pixels = NULL;
    GLenum format = 0;
    GLuint cur_tex = 0;
    GLsizei unpack_alignment = 0;

    YAGL_GET_CTX(glEGLUpdateOffscreenImageYAGL);

    if (pixels_ && (width > 0) && (height > 0) && (bpp > 0)) {
        pixels = yagl_gles_context_malloc(ctx, width * height * bpp);
        if (!yagl_mem_get(pixels_,
                          width * height * bpp,
                          pixels)) {
            res = false;
            goto out;
        }
    }

    switch (bpp) {
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_BGRA;
        break;
    default:
        YAGL_LOG_ERROR("bad bpp - %u", bpp);
        goto out;
    }

    ctx->driver->GetIntegerv(GL_TEXTURE_BINDING_2D,
                             (GLint*)&cur_tex);

    ctx->driver->GetIntegerv(GL_UNPACK_ALIGNMENT,
                             &unpack_alignment);

    ctx->driver->PixelStorei(GL_UNPACK_ALIGNMENT,
                             1);

    ctx->driver->BindTexture(GL_TEXTURE_2D,
                             gles_image->tex_global_name);

    ctx->driver->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ctx->driver->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    ctx->driver->TexImage2D(GL_TEXTURE_2D,
                            0,
                            GL_RGB,
                            width,
                            height,
                            0,
                            format,
                            GL_UNSIGNED_BYTE,
                            pixels);

    ctx->driver->PixelStorei(GL_UNPACK_ALIGNMENT,
                             unpack_alignment);

    ctx->driver->BindTexture(GL_TEXTURE_2D,
                             cur_tex);

out:
    return res;
}

bool yagl_host_glIsRenderbuffer(GLboolean* retval,
    GLuint renderbuffer)
{
    struct yagl_gles_renderbuffer *renderbuffer_obj = NULL;

    YAGL_GET_CTX_RET(glIsRenderbuffer, GL_FALSE);

    *retval = GL_FALSE;

    renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_RENDERBUFFER, renderbuffer);

    if (renderbuffer_obj && yagl_gles_renderbuffer_was_bound(renderbuffer_obj)) {
        *retval = GL_TRUE;
    }

    yagl_gles_renderbuffer_release(renderbuffer_obj);

    return true;
}

bool yagl_host_glBindRenderbuffer(GLenum target,
    GLuint renderbuffer)
{
    struct yagl_gles_renderbuffer *renderbuffer_obj = NULL;

    YAGL_GET_CTX(glBindRenderbuffer);

    if (renderbuffer != 0) {
        renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_RENDERBUFFER, renderbuffer);

        if (!renderbuffer_obj) {
            renderbuffer_obj = yagl_gles_renderbuffer_create(ctx->driver);

            if (!renderbuffer_obj) {
                goto out;
            }

            renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_add_named(ctx->base.sg,
               YAGL_NS_RENDERBUFFER, renderbuffer, &renderbuffer_obj->base);
        }
    }

    if (!yagl_gles_context_bind_renderbuffer(ctx, target, renderbuffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    ctx->driver->BindRenderbuffer(target,
                                  (renderbuffer_obj ?
                                  renderbuffer_obj->global_name : 0));

    if (renderbuffer_obj) {
        yagl_gles_renderbuffer_set_bound(renderbuffer_obj);
    }

out:
    yagl_gles_renderbuffer_release(renderbuffer_obj);

    return true;
}

bool yagl_host_glDeleteRenderbuffers(GLsizei n,
    target_ulong /* const GLuint* */ renderbuffers_)
{
    bool res = true;
    GLuint *renderbuffer_names = NULL;
    GLsizei i;

    YAGL_GET_CTX(glDeleteRenderbuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    renderbuffer_names = yagl_gles_context_malloc0(ctx, n * sizeof(*renderbuffer_names));

    if (renderbuffers_) {
        if (!yagl_mem_get(renderbuffers_,
                          n * sizeof(*renderbuffer_names),
                          renderbuffer_names)) {
            res = false;
            goto out;
        }
    }

    if (renderbuffer_names) {
        for (i = 0; i < n; ++i) {
            yagl_gles_context_unbind_renderbuffer(ctx, renderbuffer_names[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_RENDERBUFFER,
                                   renderbuffer_names[i]);
        }
    }

out:
    return res;
}

bool yagl_host_glGenRenderbuffers(GLsizei n,
    target_ulong /* GLuint* */ renderbuffers_)
{
    bool res = true;
    struct yagl_gles_renderbuffer **renderbuffers = NULL;
    GLsizei i;
    GLuint *renderbuffer_names = NULL;

    YAGL_GET_CTX(glGenRenderbuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, renderbuffers_, n * sizeof(*renderbuffer_names))) {
        res = false;
        goto out;
    }

    renderbuffers = g_malloc0(n * sizeof(*renderbuffers));

    for (i = 0; i < n; ++i) {
        renderbuffers[i] = yagl_gles_renderbuffer_create(ctx->driver);

        if (!renderbuffers[i]) {
            goto out;
        }
    }

    renderbuffer_names = g_malloc(n * sizeof(*renderbuffer_names));

    for (i = 0; i < n; ++i) {
        renderbuffer_names[i] = yagl_sharegroup_add(ctx->base.sg,
                                                    YAGL_NS_RENDERBUFFER,
                                                    &renderbuffers[i]->base);
    }

    if (renderbuffers_) {
        yagl_mem_put(cur_ts->mt1, renderbuffer_names);
    }

out:
    g_free(renderbuffer_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_renderbuffer_release(renderbuffers[i]);
    }
    g_free(renderbuffers);

    return res;
}

bool yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glRenderbufferStorage);

    ctx->driver->RenderbufferStorage(target,
                                     internalformat,
                                     width,
                                     height);

    return true;
}

bool yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    GLint params[10];

    YAGL_GET_CTX(glGetRenderbufferParameteriv);

    if (!yagl_mem_prepare_GLint(cur_ts->mt1, params_)) {
        return false;
    }

    ctx->driver->GetRenderbufferParameteriv(target,
                                            pname,
                                            params);

    if (params_) {
        yagl_mem_put_GLint(cur_ts->mt1, params[0]);
    }

    return true;
}

bool yagl_host_glIsFramebuffer(GLboolean* retval,
    GLuint framebuffer)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;

    YAGL_GET_CTX_RET(glIsFramebuffer, GL_FALSE);

    *retval = GL_FALSE;

    framebuffer_obj = (struct yagl_gles_framebuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_FRAMEBUFFER, framebuffer);

    if (framebuffer_obj && yagl_gles_framebuffer_was_bound(framebuffer_obj)) {
        *retval = GL_TRUE;
    }

    yagl_gles_framebuffer_release(framebuffer_obj);

    return true;
}

bool yagl_host_glBindFramebuffer(GLenum target,
    GLuint framebuffer)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;

    YAGL_GET_CTX(glBindFramebuffer);

    if (framebuffer != 0) {
        framebuffer_obj = (struct yagl_gles_framebuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_FRAMEBUFFER, framebuffer);

        if (!framebuffer_obj) {
            framebuffer_obj = yagl_gles_framebuffer_create(ctx->driver);

            if (!framebuffer_obj) {
                goto out;
            }

            framebuffer_obj = (struct yagl_gles_framebuffer*)yagl_sharegroup_add_named(ctx->base.sg,
               YAGL_NS_FRAMEBUFFER, framebuffer, &framebuffer_obj->base);
        }
    }

    if (!yagl_gles_context_bind_framebuffer(ctx, target, framebuffer_obj, framebuffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    ctx->driver->BindFramebuffer(target,
                                 (framebuffer_obj ?
                                 framebuffer_obj->global_name : 0));

    if (framebuffer_obj) {
        yagl_gles_framebuffer_set_bound(framebuffer_obj);
    }

out:
    yagl_gles_framebuffer_release(framebuffer_obj);

    return true;
}


bool yagl_host_glDeleteFramebuffers(GLsizei n,
    target_ulong /* const GLuint* */ framebuffers_)
{
    bool res = true;
    GLuint *framebuffer_names = NULL;
    GLsizei i;

    YAGL_GET_CTX(glDeleteFramebuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    framebuffer_names = yagl_gles_context_malloc0(ctx, n * sizeof(*framebuffer_names));

    if (framebuffers_) {
        if (!yagl_mem_get(framebuffers_,
                          n * sizeof(*framebuffer_names),
                          framebuffer_names)) {
            res = false;
            goto out;
        }
    }

    if (framebuffer_names) {
        for (i = 0; i < n; ++i) {
            yagl_gles_context_unbind_framebuffer(ctx, framebuffer_names[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_FRAMEBUFFER,
                                   framebuffer_names[i]);
        }
    }

out:
    return res;
}

bool yagl_host_glGenFramebuffers(GLsizei n,
    target_ulong /* GLuint* */ framebuffers_)
{
    bool res = true;
    struct yagl_gles_framebuffer **framebuffers = NULL;
    GLsizei i;
    GLuint *framebuffer_names = NULL;

    YAGL_GET_CTX(glGenFramebuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, framebuffers_, n * sizeof(*framebuffer_names))) {
        res = false;
        goto out;
    }

    framebuffers = g_malloc0(n * sizeof(*framebuffers));

    for (i = 0; i < n; ++i) {
        framebuffers[i] = yagl_gles_framebuffer_create(ctx->driver);

        if (!framebuffers[i]) {
            goto out;
        }
    }

    framebuffer_names = g_malloc(n * sizeof(*framebuffer_names));

    for (i = 0; i < n; ++i) {
        framebuffer_names[i] = yagl_sharegroup_add(ctx->base.sg,
                                                   YAGL_NS_FRAMEBUFFER,
                                                   &framebuffers[i]->base);
    }

    if (framebuffers_) {
        yagl_mem_put(cur_ts->mt1, framebuffer_names);
    }

out:
    g_free(framebuffer_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_framebuffer_release(framebuffers[i]);
    }
    g_free(framebuffers);

    return res;
}

bool yagl_host_glCheckFramebufferStatus(GLenum* retval,
    GLenum target)
{
    YAGL_GET_CTX_RET(glCheckFramebufferStatus, 0);

    *retval = ctx->driver->CheckFramebufferStatus(target);

    return true;
}

bool yagl_host_glFramebufferTexture2D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;
    struct yagl_gles_texture *texture_obj = NULL;

    YAGL_GET_CTX(glFramebufferTexture2D);

    framebuffer_obj = yagl_gles_context_acquire_binded_framebuffer(ctx, target);

    if (!framebuffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (texture) {
        texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_TEXTURE, texture);

        if (!texture_obj) {
            YAGL_SET_ERR(GL_INVALID_OPERATION);
            goto out;
        }
    }

    if (!yagl_gles_framebuffer_texture2d(framebuffer_obj,
                                         target,
                                         attachment,
                                         textarget,
                                         level,
                                         texture_obj,
                                         texture)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

out:
    yagl_gles_texture_release(texture_obj);
    yagl_gles_framebuffer_release(framebuffer_obj);

    return true;
}

bool yagl_host_glFramebufferRenderbuffer(GLenum target,
    GLenum attachment,
    GLenum renderbuffertarget,
    GLuint renderbuffer)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;
    struct yagl_gles_renderbuffer *renderbuffer_obj = NULL;

    YAGL_GET_CTX(glFramebufferRenderbuffer);

    framebuffer_obj = yagl_gles_context_acquire_binded_framebuffer(ctx, target);

    if (!framebuffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (renderbuffer) {
        renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
            YAGL_NS_RENDERBUFFER, renderbuffer);

        if (!renderbuffer_obj) {
            YAGL_SET_ERR(GL_INVALID_OPERATION);
            goto out;
        }
    }

    if (!yagl_gles_framebuffer_renderbuffer(framebuffer_obj,
                                            target,
                                            attachment,
                                            renderbuffertarget,
                                            renderbuffer_obj,
                                            renderbuffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

out:
    yagl_gles_renderbuffer_release(renderbuffer_obj);
    yagl_gles_framebuffer_release(framebuffer_obj);

    return true;
}


bool yagl_host_glGetFramebufferAttachmentParameteriv(GLenum target,
    GLenum attachment,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    bool res = true;
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;
    GLint param = 0;

    YAGL_GET_CTX(glGetFramebufferAttachmentParameteriv);

    if (!yagl_mem_prepare_GLint(cur_ts->mt1, params_)) {
        res = false;
        goto out;
    }

    framebuffer_obj = yagl_gles_context_acquire_binded_framebuffer(ctx, target);

    if (!framebuffer_obj) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (!yagl_gles_framebuffer_get_attachment_parameter(framebuffer_obj,
                                                        attachment,
                                                        pname,
                                                        &param)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (params_) {
        yagl_mem_put_GLint(cur_ts->mt1, param);
    }

out:
    yagl_gles_framebuffer_release(framebuffer_obj);

    return res;
}

bool yagl_host_glGenerateMipmap(GLenum target)
{
    YAGL_GET_CTX(glGenerateMipmap);

    ctx->driver->GenerateMipmap(target);

    return true;
}
