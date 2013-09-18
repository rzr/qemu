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
#include "yagl_gles_array.h"
#include "yagl_tls.h"
#include "yagl_log.h"
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
    }

#define YAGL_GET_CTX_RET(func, ret) YAGL_GET_CTX_IMPL(func, return ret)

#define YAGL_GET_CTX(func) YAGL_GET_CTX_IMPL(func, return)

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

static GLint yagl_get_actual_internalformat(GLint internalformat)
{
    switch (internalformat) {
    case GL_BGRA:
        return GL_RGBA;
    default:
        return internalformat;
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
    case GL_BGRA:
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

void yagl_host_glActiveTexture(GLenum texture)
{
    YAGL_GET_CTX(glActiveTexture);

    if (yagl_gles_context_set_active_texture(ctx, texture)) {
        ctx->driver->ActiveTexture(texture);
    } else {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    }
}

void yagl_host_glBindBuffer(GLenum target,
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
}

void yagl_host_glBindTexture(GLenum target,
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
}

void yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor)
{
    YAGL_GET_CTX(glBlendFunc);

    ctx->driver->BlendFunc(sfactor, dfactor);
}

void yagl_host_glBlendEquation(GLenum mode)
{
    YAGL_GET_CTX(glBlendEquation);

    ctx->driver->BlendEquation(mode);
}

void yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha)
{
    YAGL_GET_CTX(glBlendFuncSeparate);

    ctx->driver->BlendFuncSeparate(srcRGB,
                                   dstRGB,
                                   srcAlpha,
                                   dstAlpha);
}

void yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha)
{
    YAGL_GET_CTX(glBlendEquationSeparate);

    ctx->driver->BlendEquationSeparate(modeRGB, modeAlpha);
}

void yagl_host_glBufferData(GLenum target,
    const GLvoid *data, int32_t data_count,
    GLenum usage)
{
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glBufferData);

    if (!yagl_gles_is_buffer_target_valid(target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (data_count < 0) {
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

    yagl_gles_buffer_set_data(buffer_obj, data_count, data, usage);

out:
    yagl_gles_buffer_release(buffer_obj);
}

void yagl_host_glBufferSubData(GLenum target,
    GLsizei offset,
    const GLvoid *data, int32_t data_count)
{
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glBufferSubData);

    if (!yagl_gles_is_buffer_target_valid(target)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if ((offset < 0) || (data_count < 0)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    buffer_obj = yagl_gles_context_acquire_binded_buffer(ctx, target);

    if (!buffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (data_count == 0) {
        goto out;
    }

    if (!yagl_gles_buffer_update_data(buffer_obj, offset, data_count, data)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

out:
    yagl_gles_buffer_release(buffer_obj);
}

void yagl_host_glClear(GLbitfield mask)
{
    YAGL_GET_CTX(glClear);

    ctx->driver->Clear(mask);
}

void yagl_host_glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    YAGL_GET_CTX(glClearColor);

    ctx->driver->ClearColor(red, green, blue, alpha);
}

void yagl_host_glClearDepthf(GLclampf depth)
{
    YAGL_GET_CTX(glClearDepthf);

    ctx->driver->ClearDepth(depth);
}

void yagl_host_glClearStencil(GLint s)
{
    YAGL_GET_CTX(glClearStencil);

    ctx->driver->ClearStencil(s);
}

void yagl_host_glColorMask(GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha)
{
    YAGL_GET_CTX(glColorMask);

    ctx->driver->ColorMask(red, green, blue, alpha);
}

void yagl_host_glCompressedTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    const GLvoid *data, int32_t data_count)
{
    GLenum err_code;

    YAGL_GET_CTX(glCompressedTexImage2D);

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
                                         data_count,
                                         data);

    if (err_code != GL_NO_ERROR) {
        YAGL_SET_ERR(err_code);
    }
}

void yagl_host_glCompressedTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    const GLvoid *data, int32_t data_count)
{
    YAGL_GET_CTX(glCompressedTexSubImage2D);

    if (ctx->base.client_api == yagl_client_api_gles1) {
        /* No formats are supported by this call in GLES1 API */
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        return;
    }

    ctx->driver->CompressedTexSubImage2D(target,
                                         level,
                                         xoffset,
                                         yoffset,
                                         width,
                                         height,
                                         format,
                                         data_count,
                                         data);
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

    if (ctx->base.client_api == yagl_client_api_gles1 &&
        target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    if (border != 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    ctx->driver->CopyTexImage2D(target,
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

    if (ctx->base.client_api == yagl_client_api_gles1 &&
        target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    ctx->driver->CopyTexSubImage2D(target,
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

    ctx->driver->CullFace(mode);
}

void yagl_host_glDeleteBuffers(const GLuint *buffers, int32_t buffers_count)
{
    GLsizei i;
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX(glDeleteBuffers);

    if (buffers_count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (buffers) {
        for (i = 0; i < buffers_count; ++i) {
            buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
                YAGL_NS_BUFFER, buffers[i]);

            if (buffer_obj) {
                yagl_gles_context_unbind_buffer(ctx, buffer_obj);
            }

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_BUFFER,
                                   buffers[i]);

            yagl_gles_buffer_release(buffer_obj);
        }
    }
}

void yagl_host_glDeleteTextures(const GLuint *textures, int32_t textures_count)
{
    GLsizei i;

    YAGL_GET_CTX(glDeleteTextures);

    if (textures_count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (textures) {
        for (i = 0; i < textures_count; ++i) {
            yagl_gles_context_unbind_texture(ctx, textures[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_TEXTURE,
                                   textures[i]);

        }
    }
}

void yagl_host_glDepthFunc(GLenum func)
{
    YAGL_GET_CTX(glDepthFunc);

    ctx->driver->DepthFunc(func);
}

void yagl_host_glDepthMask(GLboolean flag)
{
    YAGL_GET_CTX(glDepthMask);

    ctx->driver->DepthMask(flag);
}

void yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar)
{
    YAGL_GET_CTX(glDepthRangef);

    ctx->driver->DepthRange(zNear, zFar);
}

void yagl_host_glDisable(GLenum cap)
{
    YAGL_GET_CTX(glDisable);

    ctx->driver->Disable(cap);

    if (cap == GL_TEXTURE_2D) {
        yagl_gles_context_active_texture_set_enabled(ctx,
            yagl_gles_texture_target_2d, false);
    }
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

    yagl_gles_context_transfer_arrays_vbo(ctx);

    ctx->draw_arrays(ctx, mode, first, count);
}

void yagl_host_glTransferArrayYAGL(GLuint indx,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    YAGL_GET_CTX(glTransferArrayYAGL);

    if (((int)indx < ctx->num_arrays) &&
        data &&
        (first >= 0) &&
        (data_count > 0)) {
        yagl_gles_array_transfer(&ctx->arrays[indx], first, data, data_count);
    }
}

void yagl_host_glDrawElementsIndicesYAGL(GLenum mode,
    GLenum type,
    const GLvoid *indices, int32_t indices_count)
{
    int index_size = 0;

    YAGL_GET_CTX(glDrawElements);

    if (ctx->ebo) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        return;
    }

    if (indices_count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (indices_count == 0) {
        return;
    }

    if (!yagl_gles_get_index_size(type, &index_size)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (!indices) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    yagl_gles_context_transfer_arrays_vbo(ctx);

    ctx->draw_elements(ctx,
                       mode,
                       (indices_count / index_size),
                       type,
                       indices);
}

void yagl_host_glDrawElementsOffsetYAGL(GLenum mode,
    GLenum type,
    GLsizei offset,
    GLsizei count)
{
    yagl_object_name old_buffer_name = 0;

    YAGL_GET_CTX(glDrawElements);

    if (!ctx->ebo) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        return;
    }

    if (count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (count == 0) {
        return;
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

    yagl_gles_context_transfer_arrays_vbo(ctx);

    ctx->draw_elements(ctx,
                       mode,
                       count,
                       type,
                       (const GLvoid*)offset);

    ctx->driver->BindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                            old_buffer_name);
}

void yagl_host_glGetVertexAttribRangeYAGL(GLenum type,
    GLsizei offset,
    GLsizei count,
    GLint *range_first,
    GLsizei *range_count)
{
    uint32_t first_idx = UINT32_MAX, last_idx = 0;

    YAGL_GET_CTX(glGetVertexAttribRangesYAGL);

    if (!ctx->ebo) {
        return;
    }

    if (!yagl_gles_buffer_get_minmax_index(ctx->ebo,
                                           type,
                                           offset,
                                           count,
                                           &first_idx,
                                           &last_idx)) {
        return;
    }

    if (range_first) {
        *range_first = (GLint)first_idx;
    }

    if (range_count) {
        *range_count = (GLsizei)(last_idx + 1 - first_idx);
    }
}

void yagl_host_glEnable(GLenum cap)
{
    YAGL_GET_CTX(glEnable);

    ctx->driver->Enable(cap);

    if (cap == GL_TEXTURE_2D) {
        yagl_gles_context_active_texture_set_enabled(ctx,
            yagl_gles_texture_target_2d, true);
    }
}

void yagl_host_glFlush(void)
{
    YAGL_GET_CTX(glFlush);

    ctx->driver->Flush();
}

void yagl_host_glFrontFace(GLenum mode)
{
    YAGL_GET_CTX(glFrontFace);

    ctx->driver->FrontFace(mode);
}

void yagl_host_glGenBuffers(GLuint *buffer_names, int32_t buffers_maxcount, int32_t *buffers_count)
{
    struct yagl_gles_buffer **buffers = NULL;
    GLsizei i;

    YAGL_GET_CTX(glGenBuffers);

    if (buffers_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    buffers = g_malloc0(buffers_maxcount * sizeof(*buffers));

    for (i = 0; i < buffers_maxcount; ++i) {
        buffers[i] = yagl_gles_buffer_create(ctx->driver);

        if (!buffers[i]) {
            goto out;
        }
    }

    for (i = 0; i < buffers_maxcount; ++i) {
        yagl_object_name tmp = yagl_sharegroup_add(ctx->base.sg,
                                                   YAGL_NS_BUFFER,
                                                   &buffers[i]->base);

        if (buffer_names) {
            buffer_names[i] = tmp;
        }
    }

    *buffers_count = buffers_maxcount;

out:
    for (i = 0; i < buffers_maxcount; ++i) {
        yagl_gles_buffer_release(buffers[i]);
    }
    g_free(buffers);
}

void yagl_host_glGenTextures(GLuint *texture_names, int32_t textures_maxcount, int32_t *textures_count)
{
    struct yagl_gles_texture **textures = NULL;
    GLsizei i;

    YAGL_GET_CTX(glGenTextures);

    if (textures_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    textures = g_malloc0(textures_maxcount * sizeof(*textures));

    for (i = 0; i < textures_maxcount; ++i) {
        textures[i] = yagl_gles_texture_create(ctx->driver);

        if (!textures[i]) {
            goto out;
        }
    }

    for (i = 0; i < textures_maxcount; ++i) {
        yagl_object_name tmp = yagl_sharegroup_add(ctx->base.sg,
                                                   YAGL_NS_TEXTURE,
                                                   &textures[i]->base);

        if (texture_names) {
            texture_names[i] = tmp;
        }
    }

    *textures_count = textures_maxcount;

out:
    for (i = 0; i < textures_maxcount; ++i) {
        yagl_gles_texture_release(textures[i]);
    }
    g_free(textures);
}

void yagl_host_glGetBooleanv(GLenum pname,
    GLboolean *params, int32_t params_maxcount, int32_t *params_count)
{
    YAGL_GET_CTX(glGetBooleanv);

    if (!ctx->get_param_count(ctx, pname, params_count)) {
        YAGL_LOG_ERROR("Bad pname = 0x%X", pname);
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    if (!params) {
        return;
    }

    if (!ctx->get_booleanv(ctx, pname, params)) {
        GLint param;

        if (yagl_get_integer(ctx, pname, &param)) {
            params[0] = ((param != 0) ? GL_TRUE : GL_FALSE);
        } else {
            ctx->driver->GetBooleanv(pname, params);
        }
    }
}

void yagl_host_glGetBufferParameteriv(GLenum target,
    GLenum pname,
    GLint *param)
{
    struct yagl_gles_buffer *buffer_obj = NULL;
    GLint tmp = 0;

    YAGL_GET_CTX(glGetBufferParameteriv);

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
                                        &tmp)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (param) {
        *param = tmp;
    }

out:
    yagl_gles_buffer_release(buffer_obj);
}

GLenum yagl_host_glGetError(void)
{
    GLenum ret;

    YAGL_GET_CTX_RET(glGetError, GL_NO_ERROR);

    ret = yagl_gles_context_get_error(ctx);

    return (ret == GL_NO_ERROR) ? ctx->driver->GetError() : ret;
}

void yagl_host_glGetFloatv(GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    YAGL_GET_CTX(glGetFloatv);

    if (!ctx->get_param_count(ctx, pname, params_count)) {
        YAGL_LOG_ERROR("Bad pname = 0x%X", pname);
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    if (!params) {
        return;
    }

    if (!ctx->get_floatv(ctx, pname, params)) {
        GLint param;

        if (yagl_get_integer(ctx, pname, &param)) {
            params[0] = (GLfloat)param;
        } else {
            ctx->driver->GetFloatv(pname, params);
        }
    }
}

void yagl_host_glGetIntegerv(GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    YAGL_GET_CTX(glGetIntegerv);

    if (!ctx->get_param_count(ctx, pname, params_count)) {
        YAGL_LOG_ERROR("Bad pname = 0x%X", pname);
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    if (!params) {
        return;
    }

    if (!ctx->get_integerv(ctx, pname, params)) {
        if (!yagl_get_integer(ctx, pname, params)) {
            ctx->driver->GetIntegerv(pname, params);
        }
    }
}

void yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    GLfloat *param)
{
    GLfloat params[10];

    YAGL_GET_CTX(glGetTexParameterfv);

    ctx->driver->GetTexParameterfv(target,
                                   pname,
                                   params);

    if (param) {
        *param = params[0];
    }
}

void yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    GLint *param)
{
    GLint params[10];

    YAGL_GET_CTX(glGetTexParameteriv);

    ctx->driver->GetTexParameteriv(target,
                                   pname,
                                   params);

    if (param) {
        *param = params[0];
    }
}

void yagl_host_glHint(GLenum target,
    GLenum mode)
{
    YAGL_GET_CTX(glHint);

    ctx->driver->Hint(target, mode);
}

GLboolean yagl_host_glIsBuffer(GLuint buffer)
{
    GLboolean res = GL_FALSE;
    struct yagl_gles_buffer *buffer_obj = NULL;

    YAGL_GET_CTX_RET(glIsBuffer, GL_FALSE);

    buffer_obj = (struct yagl_gles_buffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_BUFFER, buffer);

    if (buffer_obj && yagl_gles_buffer_was_bound(buffer_obj)) {
        res = GL_TRUE;
    }

    yagl_gles_buffer_release(buffer_obj);

    return res;
}

GLboolean yagl_host_glIsEnabled(GLenum cap)
{
    GLboolean res = GL_FALSE;

    YAGL_GET_CTX_RET(glIsEnabled, GL_FALSE);

    if (!ctx->is_enabled(ctx, &res, cap)) {
        res = ctx->driver->IsEnabled(cap);
    }

    return res;
}

GLboolean yagl_host_glIsTexture(GLuint texture)
{
    GLboolean res = GL_FALSE;
    struct yagl_gles_texture *texture_obj = NULL;

    YAGL_GET_CTX_RET(glIsTexture, GL_FALSE);

    texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_TEXTURE, texture);

    if (texture_obj && (yagl_gles_texture_get_target(texture_obj) != 0)) {
        res = GL_TRUE;
    }

    yagl_gles_texture_release(texture_obj);

    return res;
}

void yagl_host_glLineWidth(GLfloat width)
{
    YAGL_GET_CTX(glLineWidth);

    ctx->driver->LineWidth(width);
}

void yagl_host_glPixelStorei(GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glPixelStorei);

    ctx->driver->PixelStorei(pname, param);
}

void yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units)
{
    YAGL_GET_CTX(glPolygonOffset);

    ctx->driver->PolygonOffset(factor, units);
}

void yagl_host_glReadPixels(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    GLvoid *pixels, int32_t pixels_maxcount, int32_t *pixels_count)
{
    yagl_object_name current_pbo = 0;
    GLint err;
    GLsizei stride = 0;
    GLenum actual_type = yagl_get_actual_type(type);

    YAGL_GET_CTX(glReadPixels);

    if (!pixels || (width <= 0) || (height <= 0)) {
        return;
    }

    err = yagl_get_stride(ctx,
                          GL_PACK_ALIGNMENT,
                          width,
                          format,
                          type,
                          &stride);

    if (err != GL_NO_ERROR) {
        YAGL_SET_ERR(err);
        return;
    }

    ctx->driver->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB,
                             (GLint*)&current_pbo);
    if (current_pbo != 0) {
        ctx->driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
                                0);
    }

    *pixels_count = stride * height;

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
}

void yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert)
{
    YAGL_GET_CTX(glSampleCoverage);

    ctx->driver->SampleCoverage(value, invert);
}

void yagl_host_glScissor(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glScissor);

    ctx->driver->Scissor(x, y, width, height);
}

void yagl_host_glStencilFunc(GLenum func,
    GLint ref,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilFunc);

    ctx->driver->StencilFunc(func, ref, mask);
}

void yagl_host_glStencilMask(GLuint mask)
{
    YAGL_GET_CTX(glStencilMask);

    ctx->driver->StencilMask(mask);
}

void yagl_host_glStencilOp(GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    YAGL_GET_CTX(glStencilOp);

    ctx->driver->StencilOp(fail, zfail, zpass);
}

void yagl_host_glTexImage2D(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    const GLvoid *pixels, int32_t pixels_count)
{
    GLsizei stride = 0;
    GLenum actual_type = yagl_get_actual_type(type);
    GLint actual_internalformat = yagl_get_actual_internalformat(internalformat);

    YAGL_GET_CTX(glTexImage2D);

    if (pixels && (width > 0) && (height > 0)) {
        GLint err = yagl_get_stride(ctx,
                                    GL_UNPACK_ALIGNMENT,
                                    width,
                                    format,
                                    type,
                                    &stride);

        if (err != GL_NO_ERROR) {
            YAGL_SET_ERR(err);
            return;
        }
    } else {
        pixels = NULL;
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
                            actual_internalformat,
                            width,
                            height,
                            border,
                            format,
                            actual_type,
                            pixels);
}

void yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glTexParameterf);

    ctx->driver->TexParameterf(target, pname, param);
}

void yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    GLfloat tmp[10];

    YAGL_GET_CTX(glTexParameterfv);

    memset(tmp, 0, sizeof(tmp));

    if (params) {
        tmp[0] = *params;
    }

    ctx->driver->TexParameterfv(target,
                                pname,
                                (params ? tmp : NULL));
}

void yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glTexParameteri);

    ctx->driver->TexParameteri(target, pname, param);
}

void yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    const GLint *params, int32_t params_count)
{
    GLint tmp[10];

    YAGL_GET_CTX(glTexParameteriv);

    memset(tmp, 0, sizeof(tmp));

    if (params) {
        tmp[0] = *params;
    }

    ctx->driver->TexParameteriv(target,
                                pname,
                                (params ? tmp : NULL));
}

void yagl_host_glTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    const GLvoid *pixels, int32_t pixels_count)
{
    GLsizei stride = 0;
    GLenum actual_type = yagl_get_actual_type(type);

    YAGL_GET_CTX(glTexSubImage2D);

    if (pixels && (width > 0) && (height > 0)) {
        GLint err = yagl_get_stride(ctx,
                                    GL_UNPACK_ALIGNMENT,
                                    width,
                                    format,
                                    type,
                                    &stride);

        if (err != GL_NO_ERROR) {
            YAGL_SET_ERR(err);
            return;
        }
    } else {
        pixels = NULL;
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
}

void yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glViewport);

    ctx->driver->Viewport(x, y, width, height);
}

void yagl_host_glEGLImageTargetTexture2DOES(GLenum target,
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
}

void yagl_host_glGetExtensionStringYAGL(GLchar *str, int32_t str_maxcount, int32_t *str_count)
{
    GLchar *tmp = NULL;

    YAGL_GET_CTX(glGetExtensionStringYAGL);

    tmp = ctx->get_extensions(ctx);

    *str_count = strlen(tmp) + 1;

    if (str) {
        memcpy(str, tmp, MIN(*str_count, str_maxcount));
    }

    g_free(tmp);
}

void yagl_host_glEGLUpdateOffscreenImageYAGL(struct yagl_client_image *image,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    const void *pixels)
{
    struct yagl_gles_image *gles_image = (struct yagl_gles_image*)image;
    GLenum format = 0;
    GLuint cur_tex = 0;
    GLsizei unpack_alignment = 0;

    YAGL_GET_CTX(glEGLUpdateOffscreenImageYAGL);

    switch (bpp) {
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_BGRA;
        break;
    default:
        YAGL_LOG_ERROR("bad bpp - %u", bpp);
        return;
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
}

GLboolean yagl_host_glIsRenderbuffer(GLuint renderbuffer)
{
    GLboolean res = GL_FALSE;
    struct yagl_gles_renderbuffer *renderbuffer_obj = NULL;

    YAGL_GET_CTX_RET(glIsRenderbuffer, GL_FALSE);

    renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_RENDERBUFFER, renderbuffer);

    if (renderbuffer_obj && yagl_gles_renderbuffer_was_bound(renderbuffer_obj)) {
        res = GL_TRUE;
    }

    yagl_gles_renderbuffer_release(renderbuffer_obj);

    return res;
}

void yagl_host_glBindRenderbuffer(GLenum target,
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
}

void yagl_host_glDeleteRenderbuffers(const GLuint *renderbuffers, int32_t renderbuffers_count)
{
    GLsizei i;

    YAGL_GET_CTX(glDeleteRenderbuffers);

    if (renderbuffers_count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (renderbuffers) {
        for (i = 0; i < renderbuffers_count; ++i) {
            yagl_gles_context_unbind_renderbuffer(ctx, renderbuffers[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_RENDERBUFFER,
                                   renderbuffers[i]);
        }
    }
}

void yagl_host_glGenRenderbuffers(GLuint *renderbuffer_names, int32_t renderbuffers_maxcount, int32_t *renderbuffers_count)
{
    struct yagl_gles_renderbuffer **renderbuffers = NULL;
    GLsizei i;

    YAGL_GET_CTX(glGenRenderbuffers);

    if (renderbuffers_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    renderbuffers = g_malloc0(renderbuffers_maxcount * sizeof(*renderbuffers));

    for (i = 0; i < renderbuffers_maxcount; ++i) {
        renderbuffers[i] = yagl_gles_renderbuffer_create(ctx->driver);

        if (!renderbuffers[i]) {
            goto out;
        }
    }

    for (i = 0; i < renderbuffers_maxcount; ++i) {
        yagl_object_name tmp = yagl_sharegroup_add(ctx->base.sg,
                                                   YAGL_NS_RENDERBUFFER,
                                                   &renderbuffers[i]->base);

        if (renderbuffer_names) {
            renderbuffer_names[i] = tmp;
        }
    }

    *renderbuffers_count = renderbuffers_maxcount;

out:
    for (i = 0; i < renderbuffers_maxcount; ++i) {
        yagl_gles_renderbuffer_release(renderbuffers[i]);
    }
    g_free(renderbuffers);
}

void yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glRenderbufferStorage);

    ctx->driver->RenderbufferStorage(target,
                                     internalformat,
                                     width,
                                     height);
}

void yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    GLint *param)
{
    GLint params[10];

    YAGL_GET_CTX(glGetRenderbufferParameteriv);

    ctx->driver->GetRenderbufferParameteriv(target,
                                            pname,
                                            params);

    if (param) {
        *param = params[0];
    }
}

GLboolean yagl_host_glIsFramebuffer(GLuint framebuffer)
{
    GLboolean res = GL_FALSE;
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;

    YAGL_GET_CTX_RET(glIsFramebuffer, GL_FALSE);

    framebuffer_obj = (struct yagl_gles_framebuffer*)yagl_sharegroup_acquire_object(ctx->base.sg,
        YAGL_NS_FRAMEBUFFER, framebuffer);

    if (framebuffer_obj && yagl_gles_framebuffer_was_bound(framebuffer_obj)) {
        res = GL_TRUE;
    }

    yagl_gles_framebuffer_release(framebuffer_obj);

    return res;
}

void yagl_host_glBindFramebuffer(GLenum target,
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
}


void yagl_host_glDeleteFramebuffers(const GLuint *framebuffers, int32_t framebuffers_count)
{
    GLsizei i;

    YAGL_GET_CTX(glDeleteFramebuffers);

    if (framebuffers_count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (framebuffers) {
        for (i = 0; i < framebuffers_count; ++i) {
            yagl_gles_context_unbind_framebuffer(ctx, framebuffers[i]);

            yagl_sharegroup_remove(ctx->base.sg,
                                   YAGL_NS_FRAMEBUFFER,
                                   framebuffers[i]);
        }
    }
}

void yagl_host_glGenFramebuffers(GLuint *framebuffer_names, int32_t framebuffers_maxcount, int32_t *framebuffers_count)
{
    struct yagl_gles_framebuffer **framebuffers = NULL;
    GLsizei i;

    YAGL_GET_CTX(glGenFramebuffers);

    if (framebuffers_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    framebuffers = g_malloc0(framebuffers_maxcount * sizeof(*framebuffers));

    for (i = 0; i < framebuffers_maxcount; ++i) {
        framebuffers[i] = yagl_gles_framebuffer_create(ctx->driver);

        if (!framebuffers[i]) {
            goto out;
        }
    }

    for (i = 0; i < framebuffers_maxcount; ++i) {
        yagl_object_name tmp = yagl_sharegroup_add(ctx->base.sg,
                                                   YAGL_NS_FRAMEBUFFER,
                                                   &framebuffers[i]->base);

        if (framebuffer_names) {
            framebuffer_names[i] = tmp;
        }
    }

    *framebuffers_count = framebuffers_maxcount;

out:
    for (i = 0; i < framebuffers_maxcount; ++i) {
        yagl_gles_framebuffer_release(framebuffers[i]);
    }
    g_free(framebuffers);
}

GLenum yagl_host_glCheckFramebufferStatus(GLenum target)
{
    YAGL_GET_CTX_RET(glCheckFramebufferStatus, 0);

    return ctx->driver->CheckFramebufferStatus(target);
}

void yagl_host_glFramebufferTexture2D(GLenum target,
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
}

void yagl_host_glFramebufferRenderbuffer(GLenum target,
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
}

void yagl_host_glGetFramebufferAttachmentParameteriv(GLenum target,
    GLenum attachment,
    GLenum pname,
    GLint *param)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;
    GLint tmp = 0;

    YAGL_GET_CTX(glGetFramebufferAttachmentParameteriv);

    framebuffer_obj = yagl_gles_context_acquire_binded_framebuffer(ctx, target);

    if (!framebuffer_obj) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (!yagl_gles_framebuffer_get_attachment_parameter(framebuffer_obj,
                                                        attachment,
                                                        pname,
                                                        &tmp)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (param) {
        *param = tmp;
    }

out:
    yagl_gles_framebuffer_release(framebuffer_obj);
}

void yagl_host_glGenerateMipmap(GLenum target)
{
    YAGL_GET_CTX(glGenerateMipmap);

    ctx->driver->GenerateMipmap(target);
}
