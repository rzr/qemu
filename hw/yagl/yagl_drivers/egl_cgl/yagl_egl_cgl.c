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
#include <OpenGL/OpenGL.h>
#include "yagl_egl_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_egl_native_config.h"
#include "yagl_egl_surface_attribs.h"
#include "yagl_process.h"
#include "yagl_tls.h"
#include "yagl_thread.h"

#define LIBGL_IMAGE_NAME \
"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#ifndef kCGLPFAOpenGLProfile
#   define kCGLPFAOpenGLProfile 99
#   define kCGLOGLPVersion_3_2_Core 0x3200
#   define kCGLOGLPVersion_Legacy 0x1000
#endif

static const CGLPixelFormatAttribute pixel_format_legacy_attrs[] =
{
    kCGLPFAAccelerated,
    kCGLPFAMinimumPolicy,
    kCGLPFAColorSize, 32,
    kCGLPFAAlphaSize, 8,
    kCGLPFADepthSize, 24,
    kCGLPFAStencilSize, 8,
    kCGLPFANoRecovery,
    kCGLPFAPBuffer,
    0
};

static const CGLPixelFormatAttribute pixel_format_3_2_core_attrs[] =
{
    kCGLPFAAccelerated,
    kCGLPFAMinimumPolicy,
    kCGLPFAColorSize, 32,
    kCGLPFAAlphaSize, 8,
    kCGLPFADepthSize, 24,
    kCGLPFAStencilSize, 8,
    kCGLPFANoRecovery,
    kCGLPFAOpenGLProfile, kCGLOGLPVersion_3_2_Core,
    0
};

struct yagl_egl_cgl
{
    struct yagl_egl_driver base;

    CGLPixelFormatObj pixel_format_legacy;
    CGLPixelFormatObj pixel_format_3_2_core;
};

struct yagl_egl_cgl_context
{
    CGLContextObj base;

    bool is_3_2_core;
};

static bool yagl_egl_cgl_get_gl_version(struct yagl_egl_cgl *egl_cgl,
                                        yagl_gl_version *version)
{
    CGLError error;
    bool res = false;
    const char *tmp;
    CGLPixelFormatObj pixel_format;
    int n;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_get_gl_version, NULL);

    tmp = getenv("GL_VERSION");

    if (tmp) {
        if (strcmp(tmp, "2") == 0) {
            YAGL_LOG_INFO("GL_VERSION forces OpenGL version to 2.1");
            *version = yagl_gl_2;
            res = true;
        } else if (strcmp(tmp, "3_2") == 0) {
            YAGL_LOG_INFO("GL_VERSION forces OpenGL version to 3.2");
            *version = yagl_gl_3_2;
            res = true;
        } else {
            YAGL_LOG_CRITICAL("Bad GL_VERSION value = %s", tmp);
        }

        goto out;
    }

    error = CGLChoosePixelFormat(pixel_format_3_2_core_attrs,
                                 &pixel_format,
                                 &n);

    if (error) {
        YAGL_LOG_INFO("CGLChoosePixelFormat failed for 3_2_core attrs: %s, using OpenGL 2.1", CGLErrorString(error));
        *version = yagl_gl_2;
        res = true;
        goto out;
    }

    if (!pixel_format) {
        YAGL_LOG_INFO("CGLChoosePixelFormat failed to find formats for 3_2_core attrs, using OpenGL 2.1");
        *version = yagl_gl_2;
        res = true;
        goto out;
    }

    CGLDestroyPixelFormat(pixel_format);

    YAGL_LOG_INFO("Using OpenGL 3.2");
    *version = yagl_gl_3_2;
    res = true;

out:
    if (res) {
        YAGL_LOG_FUNC_EXIT("%d, version = %u", res, *version);
    } else {
        YAGL_LOG_FUNC_EXIT("%d", res);
    }

    return res;
}

static EGLNativeDisplayType yagl_egl_cgl_display_open(struct yagl_egl_driver *driver)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_display_open, NULL);

    YAGL_LOG_FUNC_EXIT("%p", (void*)1);

    return (EGLNativeDisplayType)1;
}

static void yagl_egl_cgl_display_close(struct yagl_egl_driver *driver,
                                       EGLNativeDisplayType dpy)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_display_close, "%p", dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_native_config
    *yagl_egl_cgl_config_enum(struct yagl_egl_driver *driver,
                              EGLNativeDisplayType dpy,
                              int *num_configs)
{
    struct yagl_egl_native_config *cfg = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_config_enum, "%p", dpy);

    YAGL_LOG_TRACE("got 1 config");

    cfg = g_malloc0(sizeof(*cfg));

    yagl_egl_native_config_init(cfg);

    cfg->red_size = 8;
    cfg->green_size = 8;
    cfg->blue_size = 8;
    cfg->alpha_size = 8;
    cfg->buffer_size = 32;
    cfg->caveat = EGL_NONE;
    cfg->config_id = 1;
    cfg->frame_buffer_level = 0;
    cfg->depth_size = 24;
    cfg->max_pbuffer_width = 4096;
    cfg->max_pbuffer_height = 4096;
    cfg->max_pbuffer_size = 4096 * 4096;
    cfg->max_swap_interval = 1000;
    cfg->min_swap_interval = 0;
    cfg->native_visual_id = 0;
    cfg->native_visual_type = EGL_NONE;
    cfg->samples_per_pixel = 0;
    cfg->stencil_size = 8;
    cfg->transparent_type = EGL_NONE;
    cfg->trans_red_val = 0;
    cfg->trans_green_val = 0;
    cfg->trans_blue_val = 0;
    cfg->driver_data = NULL;

    *num_configs = 1;

    YAGL_LOG_FUNC_EXIT(NULL);

    return cfg;
}

static void yagl_egl_cgl_config_cleanup(struct yagl_egl_driver *driver,
                                        EGLNativeDisplayType dpy,
                                        const struct yagl_egl_native_config *cfg)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_config_cleanup,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLSurface yagl_egl_cgl_pbuffer_surface_create(struct yagl_egl_driver *driver,
                                                      EGLNativeDisplayType dpy,
                                                      const struct yagl_egl_native_config *cfg,
                                                      EGLint width,
                                                      EGLint height,
                                                      const struct yagl_egl_pbuffer_attribs *attribs)
{
    CGLPBufferObj pbuffer = NULL;
    CGLError error;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_pbuffer_surface_create,
                        "dpy = %p, width = %d, height = %d",
                        dpy,
                        width,
                        height);

    error = CGLCreatePBuffer(width, height, GL_TEXTURE_2D, GL_RGBA,
                             0, &pbuffer);

    if (error) {
        YAGL_LOG_ERROR("CGLCreatePBuffer failed: %s", CGLErrorString(error));
        pbuffer = NULL;
    }

    YAGL_LOG_FUNC_EXIT("%p", pbuffer);

    return pbuffer;
}

static void yagl_egl_cgl_pbuffer_surface_destroy(struct yagl_egl_driver *driver,
                                                 EGLNativeDisplayType dpy,
                                                 EGLSurface sfc)
{
    CGLError error;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_pbuffer_surface_destroy,
                        "dpy = %p, sfc = %p",
                        dpy,
                        sfc);

    error = CGLDestroyPBuffer((CGLPBufferObj)sfc);

    if (error) {
        YAGL_LOG_ERROR("CGLDestroyPBuffer failed: %s", CGLErrorString(error));
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLContext yagl_egl_cgl_context_create(struct yagl_egl_driver *driver,
                                              EGLNativeDisplayType dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              EGLContext share_context,
                                              int version)
{
    struct yagl_egl_cgl *egl_cgl = (struct yagl_egl_cgl*)driver;
    struct yagl_egl_cgl_context *ctx = NULL;
    CGLContextObj share_ctx;
    CGLError error;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_context_create,
                        "dpy = %p, share_context = %p, version = %d",
                        dpy,
                        share_context,
                        version);

    ctx = g_malloc0(sizeof(*ctx));

    if (share_context != EGL_NO_CONTEXT) {
        share_ctx = ((struct yagl_egl_cgl_context*)share_context)->base;
    } else {
        share_ctx = NULL;
    }

    if ((egl_cgl->base.gl_version > yagl_gl_2) && (version != 1)) {
        error = CGLCreateContext(egl_cgl->pixel_format_3_2_core,
                                 share_ctx,
                                 &ctx->base);
        ctx->is_3_2_core = true;
    } else {
        error = CGLCreateContext(egl_cgl->pixel_format_legacy,
                                 share_ctx,
                                 &ctx->base);
        ctx->is_3_2_core = false;
    }

    if (error) {
        YAGL_LOG_ERROR("CGLCreateContext failed: %s", CGLErrorString(error));
        g_free(ctx);
        ctx = NULL;
    }

    YAGL_LOG_FUNC_EXIT("%p", ctx);

    return (EGLContext)ctx;
}

static void yagl_egl_cgl_context_destroy(struct yagl_egl_driver *driver,
                                         EGLNativeDisplayType dpy,
                                         EGLContext ctx)
{
    CGLError error;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_context_destroy,
                        "dpy = %p, ctx = %p",
                        dpy,
                        ctx);

    error = CGLDestroyContext(((struct yagl_egl_cgl_context*)ctx)->base);

    if (error) {
        YAGL_LOG_ERROR("CGLDestroyContext failed: %s", CGLErrorString(error));
    }

    g_free(ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static bool yagl_egl_cgl_make_current(struct yagl_egl_driver *driver,
                                      EGLNativeDisplayType dpy,
                                      EGLSurface draw,
                                      EGLSurface read,
                                      EGLContext ctx)
{
    struct yagl_egl_cgl_context *cgl_ctx = (struct yagl_egl_cgl_context*)ctx;
    CGLPBufferObj draw_pbuffer = (CGLPBufferObj)draw;
    CGLPBufferObj read_pbuffer = (CGLPBufferObj)read;
    CGLError error;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_make_current,
                        "dpy = %p, draw = %p, read = %p, ctx = %p",
                        dpy,
                        draw,
                        read,
                        ctx);

    if (cgl_ctx && !cgl_ctx->is_3_2_core) {
        if (read_pbuffer) {
            error = CGLSetPBuffer(cgl_ctx->base, read_pbuffer, 0, 0, 0);

            if (error) {
                YAGL_LOG_ERROR("CGLSetPBuffer failed for read: %s", CGLErrorString(error));
                goto fail;
            }
        }

        if (draw_pbuffer) {
            error = CGLSetPBuffer(cgl_ctx->base, draw_pbuffer, 0, 0, 0);

            if (error) {
                YAGL_LOG_ERROR("CGLSetPBuffer failed for draw: %s", CGLErrorString(error));
                goto fail;
            }
        }
    }

    error = CGLSetCurrentContext(cgl_ctx ? cgl_ctx->base : NULL);

    if (error) {
        YAGL_LOG_ERROR("CGLSetCurrentContext failed: %s", CGLErrorString(error));
        goto fail;
    }

    YAGL_LOG_FUNC_EXIT("context %p was made current", cgl_ctx);

    return true;

fail:
    YAGL_LOG_FUNC_EXIT("Failed to make context %p current", cgl_ctx);

    return false;
}

static void yagl_egl_cgl_destroy(struct yagl_egl_driver *driver)
{
    struct yagl_egl_cgl *egl_cgl = (struct yagl_egl_cgl*)driver;

    YAGL_LOG_FUNC_ENTER(yagl_egl_cgl_destroy, NULL);

    CGLDestroyPixelFormat(egl_cgl->pixel_format_legacy);
    if (egl_cgl->pixel_format_3_2_core) {
        CGLDestroyPixelFormat(egl_cgl->pixel_format_3_2_core);
    }

    yagl_egl_driver_cleanup(&egl_cgl->base);

    g_free(egl_cgl);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_driver *yagl_egl_driver_create(void *display)
{
    CGLError error;
    struct yagl_egl_cgl *egl_cgl;
    int n;

    YAGL_LOG_FUNC_ENTER(yagl_egl_driver_create, NULL);

    egl_cgl = g_malloc0(sizeof(*egl_cgl));

    yagl_egl_driver_init(&egl_cgl->base);

    egl_cgl->base.dyn_lib = yagl_dyn_lib_create();

    if (!egl_cgl->base.dyn_lib) {
        goto fail;
    }

    if (!yagl_dyn_lib_load(egl_cgl->base.dyn_lib, LIBGL_IMAGE_NAME)) {
        YAGL_LOG_ERROR("Unable to load " LIBGL_IMAGE_NAME ": %s",
                       yagl_dyn_lib_get_error(egl_cgl->base.dyn_lib));
        goto fail;
    }

    if (!yagl_egl_cgl_get_gl_version(egl_cgl, &egl_cgl->base.gl_version)) {
        goto fail;
    }

    error = CGLChoosePixelFormat(pixel_format_legacy_attrs,
                                 &egl_cgl->pixel_format_legacy,
                                 &n);

    if (error) {
        YAGL_LOG_ERROR("CGLChoosePixelFormat failed for legacy attrs: %s", CGLErrorString(error));
        goto fail;
    }

    if (!egl_cgl->pixel_format_legacy) {
        YAGL_LOG_ERROR("CGLChoosePixelFormat failed to find formats for legacy attrs");
        goto fail;
    }

    if (egl_cgl->base.gl_version >= yagl_gl_3_2) {
        error = CGLChoosePixelFormat(pixel_format_3_2_core_attrs,
                                     &egl_cgl->pixel_format_3_2_core,
                                     &n);

        if (error) {
            YAGL_LOG_ERROR("CGLChoosePixelFormat failed for 3_2_core attrs: %s", CGLErrorString(error));
            goto fail;
        }

        if (!egl_cgl->pixel_format_3_2_core) {
            YAGL_LOG_ERROR("CGLChoosePixelFormat failed to find formats for 3_2_core attrs");
            goto fail;
        }
    }

    egl_cgl->base.display_open = &yagl_egl_cgl_display_open;
    egl_cgl->base.display_close = &yagl_egl_cgl_display_close;
    egl_cgl->base.config_enum = &yagl_egl_cgl_config_enum;
    egl_cgl->base.config_cleanup = &yagl_egl_cgl_config_cleanup;
    egl_cgl->base.pbuffer_surface_create = &yagl_egl_cgl_pbuffer_surface_create;
    egl_cgl->base.pbuffer_surface_destroy = &yagl_egl_cgl_pbuffer_surface_destroy;
    egl_cgl->base.context_create = &yagl_egl_cgl_context_create;
    egl_cgl->base.context_destroy = &yagl_egl_cgl_context_destroy;
    egl_cgl->base.make_current = &yagl_egl_cgl_make_current;
    egl_cgl->base.destroy = &yagl_egl_cgl_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_cgl->base;

fail:
    yagl_egl_driver_cleanup(&egl_cgl->base);
    g_free(egl_cgl);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

void *yagl_dyn_lib_get_ogl_procaddr(struct yagl_dyn_lib *dyn_lib,
                                    const char *sym_name)
{
    return yagl_dyn_lib_get_sym(dyn_lib, sym_name);
}
