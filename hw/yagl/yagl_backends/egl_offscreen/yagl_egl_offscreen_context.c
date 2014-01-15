/*
 * yagl
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include <GL/gl.h>
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_gles_driver.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_egl_offscreen_context_destroy(struct yagl_eglb_context *ctx)
{
    struct yagl_egl_offscreen_context *egl_offscreen_ctx =
        (struct yagl_egl_offscreen_context*)ctx;
    struct yagl_egl_offscreen_display *dpy =
        (struct yagl_egl_offscreen_display*)ctx->dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)ctx->dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_context_destroy, NULL);

    if (egl_offscreen_ctx->rp_pbo) {
        yagl_ensure_ctx(0);
        egl_offscreen->gles_driver->DeleteBuffers(1, &egl_offscreen_ctx->rp_pbo);
        yagl_unensure_ctx(0);
    }

    egl_offscreen->egl_driver->context_destroy(egl_offscreen->egl_driver,
                                               dpy->native_dpy,
                                               egl_offscreen_ctx->native_ctx);

    yagl_eglb_context_cleanup(ctx);

    g_free(egl_offscreen_ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_context
    *yagl_egl_offscreen_context_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       struct yagl_egl_offscreen_context *share_context)
{
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->base.backend;
    struct yagl_egl_offscreen_context *ctx;
    EGLContext native_ctx;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_context_create,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    native_ctx = egl_offscreen->egl_driver->context_create(
        egl_offscreen->egl_driver,
        dpy->native_dpy,
        cfg,
        egl_offscreen->global_ctx);

    if (!native_ctx) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_eglb_context_init(&ctx->base, &dpy->base);

    ctx->base.destroy = &yagl_egl_offscreen_context_destroy;

    ctx->native_ctx = native_ctx;

    YAGL_LOG_FUNC_EXIT(NULL);

    return ctx;
}

bool yagl_egl_offscreen_context_read_pixels(struct yagl_egl_offscreen_context *ctx,
                                            uint32_t width,
                                            uint32_t height,
                                            uint32_t bpp,
                                            void *pixels)
{
    struct yagl_gles_driver *gles_driver =
        ((struct yagl_egl_offscreen*)ctx->base.dpy->backend)->gles_driver;
    bool ret = false;
    GLuint current_fb = 0;
    GLint current_pack_alignment = 0;
    GLuint current_pbo = 0;
    uint32_t rp_line_size = width * bpp;
    uint32_t rp_size = rp_line_size * height;
    GLenum format = 0;
    void *mapped_pixels = NULL;
    uint32_t i;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_context_read_pixels,
                        "%ux%ux%u", width, height, bpp);

    gles_driver->GetIntegerv(GL_READ_FRAMEBUFFER_BINDING,
                             (GLint*)&current_fb);

    gles_driver->GetIntegerv(GL_PACK_ALIGNMENT,
                             &current_pack_alignment);

    gles_driver->BindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    if (!ctx->rp_pbo) {
        /*
         * No buffer yet, create one.
         */

        gles_driver->GenBuffers(1, &ctx->rp_pbo);

        if (!ctx->rp_pbo) {
            YAGL_LOG_ERROR("GenBuffers failed");
            goto out;
        }

        YAGL_LOG_TRACE("Created pbo %u", ctx->rp_pbo);
    }

    gles_driver->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB,
                             (GLint*)&current_pbo);

    if (current_pbo != ctx->rp_pbo) {
        YAGL_LOG_TRACE("Binding pbo");
        gles_driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, ctx->rp_pbo);
    }

    if ((width != ctx->rp_pbo_width) ||
        (height != ctx->rp_pbo_height) ||
        (bpp != ctx->rp_pbo_bpp)) {
        /*
         * The surface was resized/changed, recreate pbo data accordingly.
         */

        ctx->rp_pbo_width = width;
        ctx->rp_pbo_height = height;
        ctx->rp_pbo_bpp = bpp;

        YAGL_LOG_TRACE("Recreating pbo storage");

        gles_driver->BufferData(GL_PIXEL_PACK_BUFFER_ARB,
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

    gles_driver->PixelStorei(GL_PACK_ALIGNMENT, 1);

    gles_driver->ReadPixels(0, 0,
                            width, height, format, GL_UNSIGNED_INT_8_8_8_8_REV,
                            NULL);

    mapped_pixels = gles_driver->MapBuffer(GL_PIXEL_PACK_BUFFER_ARB,
                                           GL_READ_ONLY);

    if (!mapped_pixels) {
        YAGL_LOG_ERROR("MapBuffer failed");
        goto out;
    }

    if (height > 0) {
        pixels += (height - 1) * rp_line_size;

        for (i = 0; i < height; ++i) {
            memcpy(pixels, mapped_pixels, rp_line_size);
            pixels -= rp_line_size;
            mapped_pixels += rp_line_size;
        }
    }

    ret = true;

out:
    if (mapped_pixels) {
        gles_driver->UnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    }
    gles_driver->PixelStorei(GL_PACK_ALIGNMENT, current_pack_alignment);
    if ((current_pbo != 0) &&
        (current_pbo != ctx->rp_pbo)) {
        YAGL_LOG_ERROR("Target binded a pbo ?");
        gles_driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
                                current_pbo);
    }

    gles_driver->BindFramebuffer(GL_READ_FRAMEBUFFER, current_fb);

    YAGL_LOG_FUNC_EXIT(NULL);

    return ret;
}
