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

#include "yagl_egl_onscreen.h"
#include <GL/gl.h>
#include "yagl_egl_onscreen_ts.h"
#include "yagl_egl_onscreen_display.h"
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_surface.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_gles_driver.h"
#include "winsys_gl.h"

YAGL_DEFINE_TLS(struct yagl_egl_onscreen_ts*, egl_onscreen_ts);

static void yagl_egl_onscreen_setup_framebuffer_zero(struct yagl_egl_onscreen *egl_onscreen)
{
    GLuint cur_fb = 0;

    assert(egl_onscreen_ts->dpy);

    yagl_egl_onscreen_context_setup(egl_onscreen_ts->ctx);

    if (!egl_onscreen_ts->sfc_draw) {
        return;
    }

    yagl_egl_onscreen_surface_setup(egl_onscreen_ts->sfc_draw);

    egl_onscreen->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                           (GLint*)&cur_fb);

    egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER,
                                               egl_onscreen_ts->ctx->fb);

    yagl_egl_onscreen_surface_attach_to_framebuffer(egl_onscreen_ts->sfc_draw);

    egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER,
                                               cur_fb);
}

static void yagl_egl_onscreen_thread_init(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_thread_init, NULL);

    egl_onscreen_ts = yagl_egl_onscreen_ts_create(egl_onscreen->gles_driver);

    cur_ts->egl_onscreen_ts = egl_onscreen_ts;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_onscreen_batch_start(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;

    egl_onscreen_ts = cur_ts->egl_onscreen_ts;

    if (!egl_onscreen_ts->dpy) {
        return;
    }

    if (egl_onscreen_ts->sfc_draw && egl_onscreen_ts->sfc_read) {
        egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                               egl_onscreen_ts->dpy->native_dpy,
                                               egl_onscreen_ts->sfc_draw->dummy_native_sfc,
                                               egl_onscreen_ts->sfc_read->dummy_native_sfc,
                                               egl_onscreen_ts->ctx->native_ctx);
    } else {
        egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                               egl_onscreen_ts->dpy->native_dpy,
                                               egl_onscreen_ts->ctx->null_sfc,
                                               egl_onscreen_ts->ctx->null_sfc,
                                               egl_onscreen_ts->ctx->native_ctx);
    }
}

static void yagl_egl_onscreen_batch_end(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;

    if (!egl_onscreen_ts->dpy) {
        return;
    }

    egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                           egl_onscreen_ts->dpy->native_dpy,
                                           EGL_NO_SURFACE,
                                           EGL_NO_SURFACE,
                                           EGL_NO_CONTEXT);
}

static struct yagl_eglb_display *yagl_egl_onscreen_create_display(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;
    struct yagl_egl_onscreen_display *dpy =
        yagl_egl_onscreen_display_create(egl_onscreen);

    return dpy ? &dpy->base : NULL;
}

static bool yagl_egl_onscreen_make_current(struct yagl_egl_backend *backend,
                                           struct yagl_eglb_display *dpy,
                                           struct yagl_eglb_context *ctx,
                                           struct yagl_eglb_surface *draw,
                                           struct yagl_eglb_surface *read)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;
    struct yagl_egl_onscreen_display *egl_onscreen_dpy = (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_context *egl_onscreen_ctx = (struct yagl_egl_onscreen_context*)ctx;
    struct yagl_egl_onscreen_surface *egl_onscreen_draw = (struct yagl_egl_onscreen_surface*)draw;
    struct yagl_egl_onscreen_surface *egl_onscreen_read = (struct yagl_egl_onscreen_surface*)read;
    bool res;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_make_current, NULL);

    if (draw && read) {
        res = egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                                     egl_onscreen_dpy->native_dpy,
                                                     egl_onscreen_draw->dummy_native_sfc,
                                                     egl_onscreen_read->dummy_native_sfc,
                                                     egl_onscreen_ctx->native_ctx);
    } else {
        res = yagl_egl_onscreen_context_setup_surfaceless(egl_onscreen_ctx);

        if (res) {
            res = egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                                         egl_onscreen_dpy->native_dpy,
                                                         egl_onscreen_ctx->null_sfc,
                                                         egl_onscreen_ctx->null_sfc,
                                                         egl_onscreen_ctx->native_ctx);
        }
    }

    if (res) {
        GLuint cur_fb = 0;

        egl_onscreen_ts->dpy = egl_onscreen_dpy;
        egl_onscreen_ts->ctx = egl_onscreen_ctx;
        egl_onscreen_ts->sfc_draw = egl_onscreen_draw;
        egl_onscreen_ts->sfc_read = egl_onscreen_read;

        yagl_egl_onscreen_setup_framebuffer_zero(egl_onscreen);

        egl_onscreen->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                               (GLint*)&cur_fb);

        if (cur_fb == 0) {
            /*
             * No framebuffer, then bind our framebuffer zero.
             */

            egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER,
                                                       egl_onscreen_ctx->fb);

            /*
             * Setup default viewport and scissor.
             */
            if (draw) {
                egl_onscreen->gles_driver->Viewport(0,
                    0,
                    yagl_egl_onscreen_surface_width(egl_onscreen_draw),
                    yagl_egl_onscreen_surface_height(egl_onscreen_draw));
                egl_onscreen->gles_driver->Scissor(0,
                    0,
                    yagl_egl_onscreen_surface_width(egl_onscreen_draw),
                    yagl_egl_onscreen_surface_height(egl_onscreen_draw));
            } else {
                /*
                 * "If the first time <ctx> is made current, it is without a default
                 * framebuffer (e.g. both <draw> and <read> are EGL_NO_SURFACE), then
                 * the viewport and scissor regions are set as though
                 * glViewport(0,0,0,0) and glScissor(0,0,0,0) were called."
                 */
                egl_onscreen->gles_driver->Viewport(0, 0, 0, 0);
                egl_onscreen->gles_driver->Scissor(0, 0, 0, 0);
            }
        }
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res;
}

static bool yagl_egl_onscreen_release_current(struct yagl_egl_backend *backend, bool force)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;
    bool res;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_release_current, NULL);

    assert(egl_onscreen_ts->dpy);

    if (!egl_onscreen_ts->dpy) {
        return false;
    }

    res = egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                                 egl_onscreen_ts->dpy->native_dpy,
                                                 EGL_NO_SURFACE,
                                                 EGL_NO_SURFACE,
                                                 EGL_NO_CONTEXT);

    if (res || force) {
        egl_onscreen_ts->dpy = NULL;
        egl_onscreen_ts->ctx = NULL;
        egl_onscreen_ts->sfc_draw = NULL;
        egl_onscreen_ts->sfc_read = NULL;
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res || force;
}

static void yagl_egl_onscreen_thread_fini(struct yagl_egl_backend *backend)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_thread_fini, NULL);

    yagl_egl_onscreen_ts_destroy(egl_onscreen_ts);
    egl_onscreen_ts = cur_ts->egl_onscreen_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_onscreen_ensure_current(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;

    if (egl_onscreen_ts && egl_onscreen_ts->dpy) {
        return;
    }

    egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                           egl_onscreen->ensure_dpy,
                                           egl_onscreen->ensure_sfc,
                                           egl_onscreen->ensure_sfc,
                                           egl_onscreen->ensure_ctx);
}

static void yagl_egl_onscreen_unensure_current(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;

    if (egl_onscreen_ts && egl_onscreen_ts->dpy) {
        return;
    }

    egl_onscreen->egl_driver->make_current(egl_onscreen->egl_driver,
                                           egl_onscreen->ensure_dpy,
                                           EGL_NO_SURFACE,
                                           EGL_NO_SURFACE,
                                           EGL_NO_CONTEXT);
}

static void yagl_egl_onscreen_destroy(struct yagl_egl_backend *backend)
{
    struct yagl_egl_onscreen *egl_onscreen = (struct yagl_egl_onscreen*)backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_destroy, NULL);

    egl_onscreen->egl_driver->context_destroy(egl_onscreen->egl_driver,
                                              egl_onscreen->ensure_dpy,
                                              egl_onscreen->global_ctx);
    egl_onscreen->egl_driver->context_destroy(egl_onscreen->egl_driver,
                                              egl_onscreen->ensure_dpy,
                                              egl_onscreen->ensure_ctx);
    egl_onscreen->egl_driver->pbuffer_surface_destroy(egl_onscreen->egl_driver,
                                                      egl_onscreen->ensure_dpy,
                                                      egl_onscreen->ensure_sfc);
    egl_onscreen->egl_driver->config_cleanup(egl_onscreen->egl_driver,
                                             egl_onscreen->ensure_dpy,
                                             &egl_onscreen->ensure_config);
    egl_onscreen->egl_driver->display_close(egl_onscreen->egl_driver,
                                            egl_onscreen->ensure_dpy);

    egl_onscreen->egl_driver->destroy(egl_onscreen->egl_driver);
    egl_onscreen->egl_driver = NULL;
    egl_onscreen->gles_driver = NULL;

    yagl_egl_backend_cleanup(&egl_onscreen->base);

    g_free(egl_onscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_backend *yagl_egl_onscreen_create(struct winsys_interface *wsi,
                                                  struct yagl_egl_driver *egl_driver,
                                                  struct yagl_gles_driver *gles_driver)
{
    struct yagl_egl_onscreen *egl_onscreen = g_malloc0(sizeof(struct yagl_egl_onscreen));
    EGLNativeDisplayType dpy = NULL;
    struct yagl_egl_native_config *configs = NULL;
    int i, num_configs = 0;
    struct yagl_egl_pbuffer_attribs attribs;
    EGLSurface sfc = EGL_NO_SURFACE;
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLContext global_ctx = EGL_NO_CONTEXT;
    struct winsys_gl_info *ws_info = (struct winsys_gl_info*)wsi->ws_info;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_create, NULL);

    yagl_egl_pbuffer_attribs_init(&attribs);

    yagl_egl_backend_init(&egl_onscreen->base, yagl_render_type_onscreen);

    dpy = egl_driver->display_open(egl_driver);

    if (!dpy) {
        goto fail;
    }

    configs = egl_driver->config_enum(egl_driver, dpy, &num_configs);

    if (!configs || (num_configs <= 0)) {
        goto fail;
    }

    sfc = egl_driver->pbuffer_surface_create(egl_driver, dpy, &configs[0],
                                             1, 1, &attribs);

    if (sfc == EGL_NO_SURFACE) {
        goto fail;
    }

    ctx = egl_driver->context_create(egl_driver, dpy, &configs[0],
                                     (EGLContext)ws_info->context);

    if (ctx == EGL_NO_CONTEXT) {
        goto fail;
    }

    global_ctx = egl_driver->context_create(egl_driver, dpy, &configs[0], ctx);

    if (global_ctx == EGL_NO_CONTEXT) {
        goto fail;
    }

    egl_onscreen->base.thread_init = &yagl_egl_onscreen_thread_init;
    egl_onscreen->base.batch_start = &yagl_egl_onscreen_batch_start;
    egl_onscreen->base.create_display = &yagl_egl_onscreen_create_display;
    egl_onscreen->base.make_current = &yagl_egl_onscreen_make_current;
    egl_onscreen->base.release_current = &yagl_egl_onscreen_release_current;
    egl_onscreen->base.batch_end = &yagl_egl_onscreen_batch_end;
    egl_onscreen->base.thread_fini = &yagl_egl_onscreen_thread_fini;
    egl_onscreen->base.ensure_current = &yagl_egl_onscreen_ensure_current;
    egl_onscreen->base.unensure_current = &yagl_egl_onscreen_unensure_current;
    egl_onscreen->base.destroy = &yagl_egl_onscreen_destroy;

    egl_onscreen->wsi = wsi;
    egl_onscreen->egl_driver = egl_driver;
    egl_onscreen->gles_driver = gles_driver;
    egl_onscreen->ensure_dpy = dpy;
    egl_onscreen->ensure_config = configs[0];
    egl_onscreen->ensure_ctx = ctx;
    egl_onscreen->ensure_sfc = sfc;
    egl_onscreen->global_ctx = global_ctx;

    for (i = 1; i < num_configs; ++i) {
        egl_driver->config_cleanup(egl_driver, dpy, &configs[i]);
    }
    g_free(configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_onscreen->base;

fail:
    if (ctx != EGL_NO_CONTEXT) {
        egl_driver->context_destroy(egl_driver, dpy, ctx);
    }

    if (sfc != EGL_NO_SURFACE) {
        egl_driver->pbuffer_surface_destroy(egl_driver, dpy, sfc);
    }

    if (configs) {
        for (i = 0; i < num_configs; ++i) {
            egl_driver->config_cleanup(egl_driver, dpy, &configs[i]);
        }
        g_free(configs);
    }

    if (dpy) {
        egl_driver->display_close(egl_driver, dpy);
    }

    yagl_egl_backend_cleanup(&egl_onscreen->base);

    g_free(egl_onscreen);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
