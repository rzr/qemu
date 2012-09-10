#include <GL/gl.h>
#include "yagl_gles_context.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_array.h"
#include "yagl_gles_buffer.h"
#include "yagl_gles_framebuffer.h"
#include "yagl_gles_texture_unit.h"
#include "yagl_gles_validate.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_sharegroup.h"
#include <assert.h>

static void yagl_gles_context_flush(struct yagl_client_context *ctx)
{
    struct yagl_gles_context *gles_ctx = (struct yagl_gles_context*)ctx;

    gles_ctx->driver_ps->Flush(gles_ctx->driver_ps);
}

static void yagl_gles_context_finish(struct yagl_client_context *ctx)
{
    struct yagl_gles_context *gles_ctx = (struct yagl_gles_context*)ctx;

    gles_ctx->driver_ps->Finish(gles_ctx->driver_ps);
}

static bool yagl_gles_context_read_pixels(struct yagl_client_context *ctx,
                                          uint32_t width,
                                          uint32_t height,
                                          uint32_t bpp,
                                          void *pixels)
{
    struct yagl_gles_context *gles_ctx = (struct yagl_gles_context*)ctx;
    bool ret = false;
    yagl_object_name current_pbo = 0;
    uint32_t rp_line_size = width * bpp;
    uint32_t rp_size = rp_line_size * height;
    GLenum format = 0;
    bool pop_attrib = false;
    void *mapped_pixels = NULL;
    uint32_t i;

    YAGL_LOG_FUNC_ENTER_TS(gles_ctx->ts,
                           yagl_gles_context_read_pixels,
                           "%ux%ux%u", width, height, bpp);

    if (!gles_ctx->rp_pbo) {
        /*
         * No buffer yet, create one.
         */

        gles_ctx->driver_ps->GenBuffers(gles_ctx->driver_ps,
                                        1,
                                        &gles_ctx->rp_pbo);

        if (!gles_ctx->rp_pbo) {
            YAGL_LOG_ERROR("GenBuffers failed");
            goto out;
        }

        YAGL_LOG_TRACE("Created pbo %u", gles_ctx->rp_pbo);
    }

    gles_ctx->driver_ps->GetIntegerv(gles_ctx->driver_ps,
                                     GL_PIXEL_PACK_BUFFER_BINDING_ARB,
                                     (GLint*)&current_pbo);

    if (current_pbo != gles_ctx->rp_pbo) {
        YAGL_LOG_TRACE("Binding pbo");
        gles_ctx->driver_ps->BindBuffer(gles_ctx->driver_ps,
                                        GL_PIXEL_PACK_BUFFER_ARB,
                                        gles_ctx->rp_pbo);
    }

    if ((width != gles_ctx->rp_pbo_width) ||
        (height != gles_ctx->rp_pbo_height) ||
        (bpp != gles_ctx->rp_pbo_bpp)) {
        /*
         * The surface was resized/changed, recreate pbo data accordingly.
         */

        gles_ctx->rp_pbo_width = width;
        gles_ctx->rp_pbo_height = height;
        gles_ctx->rp_pbo_bpp = bpp;

        YAGL_LOG_TRACE("Recreating pbo storage");

        gles_ctx->driver_ps->BufferData(gles_ctx->driver_ps,
                                        GL_PIXEL_PACK_BUFFER_ARB,
                                        rp_size,
                                        0,
                                        GL_STREAM_READ);
    }

    switch (bpp) {
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_BGRA;
        break;
    default:
        assert(0);
        goto out;
    }

    gles_ctx->driver_ps->PushClientAttrib(gles_ctx->driver_ps,
                                          GL_CLIENT_PIXEL_STORE_BIT);
    pop_attrib = true;

    gles_ctx->driver_ps->PixelStorei(gles_ctx->driver_ps,
                                     GL_PACK_ALIGNMENT,
                                     ((bpp == 4) ? 4 : 1));

    gles_ctx->driver_ps->ReadPixels(gles_ctx->driver_ps,
                                    0, 0,
                                    width, height, format, GL_UNSIGNED_BYTE,
                                    NULL);

    mapped_pixels = gles_ctx->driver_ps->MapBuffer(gles_ctx->driver_ps,
                                                   GL_PIXEL_PACK_BUFFER_ARB,
                                                   GL_READ_ONLY);

    if (!mapped_pixels) {
        YAGL_LOG_ERROR("MapBuffer failed");
        goto out;
    }

    if (height > 0) {
        pixels += (height - 1) * rp_line_size;

        for (i = 0; i < height; ++i)
        {
            memcpy(pixels, mapped_pixels, rp_line_size);
            pixels -= rp_line_size;
            mapped_pixels += rp_line_size;
        }
    }

    ret = true;

out:
    if (mapped_pixels) {
        gles_ctx->driver_ps->UnmapBuffer(gles_ctx->driver_ps,
                                         GL_PIXEL_PACK_BUFFER_ARB);
    }
    if (pop_attrib) {
        gles_ctx->driver_ps->PopClientAttrib(gles_ctx->driver_ps);
    }
    if ((current_pbo != 0) &&
        (current_pbo != gles_ctx->rp_pbo)) {
        YAGL_LOG_ERROR("Target binded a pbo ?");
        gles_ctx->driver_ps->BindBuffer(gles_ctx->driver_ps,
                                        GL_PIXEL_PACK_BUFFER_ARB,
                                        current_pbo);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return ret;
}

void yagl_gles_context_init(struct yagl_gles_context *ctx,
                            struct yagl_gles_driver_ps *driver_ps)
{
    ctx->base.flush = &yagl_gles_context_flush;
    ctx->base.finish = &yagl_gles_context_finish;
    ctx->base.read_pixels = &yagl_gles_context_read_pixels;

    ctx->driver_ps = driver_ps;

    ctx->error = GL_NO_ERROR;

    ctx->arrays = NULL;
    ctx->num_arrays = 0;

    ctx->texture_units = NULL;
    ctx->num_texture_units = 0;

    ctx->num_compressed_texture_formats = 0;

    ctx->active_texture_unit = 0;

    ctx->vbo = NULL;
    ctx->vbo_local_name = 0;

    ctx->ebo = NULL;
    ctx->ebo_local_name = 0;

    ctx->fbo = NULL;
    ctx->fbo_local_name = 0;

    ctx->rbo_local_name = 0;

    ctx->ts = NULL;

    ctx->rp_pbo = 0;
    ctx->rp_pbo_width = 0;
    ctx->rp_pbo_height = 0;
    ctx->rp_pbo_bpp = 0;
}

void yagl_gles_context_prepare(struct yagl_gles_context *ctx,
                               struct yagl_thread_state *ts,
                               struct yagl_gles_array *arrays,
                               int num_arrays,
                               int num_texture_units)
{
    int i;

    if (num_texture_units < 1) {
        num_texture_units = 1;
    }

    YAGL_LOG_FUNC_ENTER_TS(ts,
                           yagl_gles_context_prepare,
                           "num_arrays = %d, num_texture_units = %d",
                           num_arrays,
                           num_texture_units);

    ctx->arrays = arrays;
    ctx->num_arrays = num_arrays;

    ctx->num_texture_units = num_texture_units;
    ctx->texture_units =
        g_malloc(ctx->num_texture_units * sizeof(*ctx->texture_units));

    for (i = 0; i < ctx->num_texture_units; ++i) {
        yagl_gles_texture_unit_init(&ctx->texture_units[i], ctx);
    }

    ctx->driver_ps->GetIntegerv(ctx->driver_ps,
                                GL_NUM_COMPRESSED_TEXTURE_FORMATS,
                                &ctx->num_compressed_texture_formats);

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_gles_context_activate(struct yagl_gles_context *ctx,
                                struct yagl_thread_state *ts)
{
    assert(ctx->ts == NULL);
    ctx->ts = ts;
}

void yagl_gles_context_deactivate(struct yagl_gles_context *ctx)
{
    if (ctx->rp_pbo) {
        ctx->driver_ps->DeleteBuffers(ctx->driver_ps, 1, &ctx->rp_pbo);
        ctx->rp_pbo = 0;
        ctx->rp_pbo_width = 0;
        ctx->rp_pbo_height = 0;
        ctx->rp_pbo_bpp = 0;
    }

    assert(ctx->ts != NULL);
    ctx->ts = NULL;
}

void yagl_gles_context_cleanup(struct yagl_gles_context *ctx)
{
    int i;

    if (ctx->fbo) {
        yagl_sharegroup_reap_object(ctx->base.sg, &ctx->fbo->base);
    }

    if (ctx->ebo) {
        yagl_sharegroup_reap_object(ctx->base.sg, &ctx->ebo->base);
    }

    if (ctx->vbo) {
        yagl_sharegroup_reap_object(ctx->base.sg, &ctx->vbo->base);
    }

    for (i = 0; i < ctx->num_texture_units; ++i) {
        yagl_gles_texture_unit_cleanup(&ctx->texture_units[i]);
    }

    for (i = 0; i < ctx->num_arrays; ++i) {
        yagl_gles_array_cleanup(&ctx->arrays[i]);
    }

    g_free(ctx->texture_units);
    ctx->texture_units = NULL;

    g_free(ctx->arrays);
    ctx->arrays = NULL;
}

void yagl_gles_context_set_error(struct yagl_gles_context *ctx, GLenum error)
{
    if (ctx->error == GL_NO_ERROR) {
        ctx->error = error;
    }
}

GLenum yagl_gles_context_get_error(struct yagl_gles_context *ctx)
{
    GLenum error = ctx->error;

    ctx->error = GL_NO_ERROR;

    return error;
}

struct yagl_gles_array
    *yagl_gles_context_get_array(struct yagl_gles_context *ctx, GLuint index)
{
    int i;

    for (i = 0; i < ctx->num_arrays; ++i) {
        if (ctx->arrays[i].index == index) {
            return &ctx->arrays[i];
        }
    }

    return NULL;
}

bool yagl_gles_context_transfer_arrays(struct yagl_gles_context *ctx,
                                       uint32_t first,
                                       uint32_t count)
{
    int i;

    for (i = 0; i < ctx->num_arrays; ++i) {
        if (!yagl_gles_array_transfer(&ctx->arrays[i], first, count)) {
            return false;
        }
    }

    return true;
}

bool yagl_gles_context_set_active_texture(struct yagl_gles_context *ctx,
                                          GLenum texture)
{
    if ((texture < GL_TEXTURE0) ||
        (texture >= (GL_TEXTURE0 + ctx->num_texture_units))) {
        return false;
    }

    ctx->active_texture_unit = texture - GL_TEXTURE0;

    return true;
}

struct yagl_gles_texture_unit
    *yagl_gles_context_get_active_texture_unit(struct yagl_gles_context *ctx)
{
    return &ctx->texture_units[ctx->active_texture_unit];
}

struct yagl_gles_texture_target_state
    *yagl_gles_context_get_active_texture_target_state(struct yagl_gles_context *ctx,
                                                       yagl_gles_texture_target texture_target)
{
    return &yagl_gles_context_get_active_texture_unit(ctx)->target_states[texture_target];
}

void yagl_gles_context_bind_texture(struct yagl_gles_context *ctx,
                                    yagl_gles_texture_target texture_target,
                                    yagl_object_name texture_local_name)
{
    struct yagl_gles_texture_target_state *texture_target_state =
        yagl_gles_context_get_active_texture_target_state(ctx, texture_target);

    texture_target_state->texture_local_name = texture_local_name;
}

void yagl_gles_context_unbind_texture(struct yagl_gles_context *ctx,
                                      yagl_object_name texture_local_name)
{
    int i;
    struct yagl_gles_texture_unit *texture_unit =
        yagl_gles_context_get_active_texture_unit(ctx);

    for (i = 0; i < YAGL_NUM_GLES_TEXTURE_TARGETS; ++i) {
        if (texture_unit->target_states[i].texture_local_name == texture_local_name) {
            texture_unit->target_states[i].texture_local_name = 0;
        }
    }
}

bool yagl_gles_context_bind_buffer(struct yagl_gles_context *ctx,
                                   GLenum target,
                                   struct yagl_gles_buffer *buffer,
                                   yagl_object_name buffer_local_name)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
        yagl_gles_buffer_acquire(buffer);
        yagl_gles_buffer_release(ctx->vbo);
        ctx->vbo = buffer;
        ctx->vbo_local_name = buffer_local_name;
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        yagl_gles_buffer_acquire(buffer);
        yagl_gles_buffer_release(ctx->ebo);
        ctx->ebo = buffer;
        ctx->ebo_local_name = buffer_local_name;
        break;
    default:
        return false;
    }

    return true;
}

void yagl_gles_context_unbind_buffer(struct yagl_gles_context *ctx,
                                     struct yagl_gles_buffer *buffer)
{
    if (buffer == ctx->vbo) {
        yagl_gles_buffer_release(ctx->vbo);
        ctx->vbo = NULL;
        ctx->vbo_local_name = 0;
    } else if (buffer == ctx->ebo) {
        yagl_gles_buffer_release(ctx->ebo);
        ctx->ebo = NULL;
        ctx->ebo_local_name = 0;
    }
}

bool yagl_gles_context_bind_framebuffer(struct yagl_gles_context *ctx,
                                        GLenum target,
                                        struct yagl_gles_framebuffer *fbo,
                                        yagl_object_name fbo_local_name)
{
    switch (target) {
    case GL_FRAMEBUFFER:
        yagl_gles_framebuffer_acquire(fbo);
        yagl_gles_framebuffer_release(ctx->fbo);
        ctx->fbo = fbo;
        ctx->fbo_local_name = fbo_local_name;
        break;
    default:
        return false;
    }

    return true;
}

void yagl_gles_context_unbind_framebuffer(struct yagl_gles_context *ctx,
                                          yagl_object_name fbo_local_name)
{
    if (fbo_local_name == ctx->fbo_local_name) {
        yagl_gles_framebuffer_release(ctx->fbo);
        ctx->fbo = NULL;
        ctx->fbo_local_name = 0;
    }
}

bool yagl_gles_context_bind_renderbuffer(struct yagl_gles_context *ctx,
                                         GLenum target,
                                         yagl_object_name rbo_local_name)
{
    switch (target) {
    case GL_RENDERBUFFER:
        ctx->rbo_local_name = rbo_local_name;
        break;
    default:
        return false;
    }

    return true;
}

void yagl_gles_context_unbind_renderbuffer(struct yagl_gles_context *ctx,
                                           yagl_object_name rbo_local_name)
{
    if (rbo_local_name == ctx->rbo_local_name) {
        ctx->rbo_local_name = 0;
    }
}

struct yagl_gles_buffer
    *yagl_gles_context_acquire_binded_buffer(struct yagl_gles_context *ctx,
                                             GLenum target)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
        yagl_gles_buffer_acquire(ctx->vbo);
        return ctx->vbo;
    case GL_ELEMENT_ARRAY_BUFFER:
        yagl_gles_buffer_acquire(ctx->ebo);
        return ctx->ebo;
    default:
        return NULL;
    }
}

struct yagl_gles_framebuffer
    *yagl_gles_context_acquire_binded_framebuffer(struct yagl_gles_context *ctx,
                                                  GLenum target)
{
    switch (target) {
    case GL_FRAMEBUFFER:
        yagl_gles_framebuffer_acquire(ctx->fbo);
        return ctx->fbo;
    default:
        return NULL;
    }
}
