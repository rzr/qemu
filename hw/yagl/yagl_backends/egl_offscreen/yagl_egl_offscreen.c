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
#include "yagl_egl_offscreen.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_gles_driver.h"

YAGL_DEFINE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static void yagl_egl_offscreen_thread_init(struct yagl_egl_backend *backend)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_thread_init, NULL);

    egl_offscreen_ts = yagl_egl_offscreen_ts_create();

    cur_ts->egl_offscreen_ts = egl_offscreen_ts;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_offscreen_batch_start(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    egl_offscreen_ts = cur_ts->egl_offscreen_ts;

    if (!egl_offscreen_ts->dpy) {
        return;
    }

    egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                            egl_offscreen_ts->dpy->native_dpy,
                                            egl_offscreen_ts->sfc_draw,
                                            egl_offscreen_ts->sfc_read,
                                            egl_offscreen_ts->ctx->native_ctx);
}

static void yagl_egl_offscreen_batch_end(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    if (!egl_offscreen_ts->dpy) {
        return;
    }

    egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                            egl_offscreen_ts->dpy->native_dpy,
                                            EGL_NO_SURFACE,
                                            EGL_NO_SURFACE,
                                            EGL_NO_CONTEXT);
}

static struct yagl_eglb_display *yagl_egl_offscreen_create_display(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;
    struct yagl_egl_offscreen_display *dpy =
        yagl_egl_offscreen_display_create(egl_offscreen);

    return dpy ? &dpy->base : NULL;
}

static bool yagl_egl_offscreen_make_current(struct yagl_egl_backend *backend,
                                            struct yagl_eglb_display *dpy,
                                            struct yagl_eglb_context *ctx,
                                            struct yagl_eglb_surface *draw,
                                            struct yagl_eglb_surface *read)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;
    struct yagl_egl_offscreen_display *egl_offscreen_dpy = (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_context *egl_offscreen_ctx = (struct yagl_egl_offscreen_context*)ctx;
    struct yagl_egl_offscreen_surface *egl_offscreen_draw = (struct yagl_egl_offscreen_surface*)draw;
    struct yagl_egl_offscreen_surface *egl_offscreen_read = (struct yagl_egl_offscreen_surface*)read;
    bool res = false;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_make_current, NULL);

    if (egl_offscreen_ts->dpy) {
        egl_offscreen->gles_driver->Flush();
    }

    if (draw && read) {
        res = egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                                      egl_offscreen_dpy->native_dpy,
                                                      egl_offscreen_draw->native_sfc,
                                                      egl_offscreen_read->native_sfc,
                                                      egl_offscreen_ctx->native_ctx);

        if (res) {
            egl_offscreen_ts->dpy = egl_offscreen_dpy;
            egl_offscreen_ts->ctx = egl_offscreen_ctx;
            egl_offscreen_ts->sfc_draw = egl_offscreen_draw->native_sfc;
            egl_offscreen_ts->sfc_read = egl_offscreen_read->native_sfc;
        }
    } else {
        /*
         * EGL_KHR_surfaceless_context not supported for offscreen yet.
         */

        YAGL_LOG_ERROR("EGL_KHR_surfaceless_context not supported");
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res;
}

static bool yagl_egl_offscreen_release_current(struct yagl_egl_backend *backend, bool force)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;
    bool res;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_release_current, NULL);

    assert(egl_offscreen_ts->dpy);

    if (!egl_offscreen_ts->dpy) {
        return false;
    }

    egl_offscreen->gles_driver->Flush();

    res = egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                                  egl_offscreen_ts->dpy->native_dpy,
                                                  EGL_NO_SURFACE,
                                                  EGL_NO_SURFACE,
                                                  EGL_NO_CONTEXT);

    if (res || force) {
        egl_offscreen_ts->dpy = NULL;
        egl_offscreen_ts->ctx = NULL;
        egl_offscreen_ts->sfc_draw = NULL;
        egl_offscreen_ts->sfc_read = NULL;
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res || force;
}

static void yagl_egl_offscreen_thread_fini(struct yagl_egl_backend *backend)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_thread_fini, NULL);

    yagl_egl_offscreen_ts_destroy(egl_offscreen_ts);
    egl_offscreen_ts = cur_ts->egl_offscreen_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_offscreen_ensure_current(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                            egl_offscreen->ensure_dpy,
                                            egl_offscreen->ensure_sfc,
                                            egl_offscreen->ensure_sfc,
                                            egl_offscreen->ensure_ctx);
}

static void yagl_egl_offscreen_unensure_current(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    if (egl_offscreen_ts && egl_offscreen_ts->dpy) {
        egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                                egl_offscreen_ts->dpy->native_dpy,
                                                egl_offscreen_ts->sfc_draw,
                                                egl_offscreen_ts->sfc_read,
                                                egl_offscreen_ts->ctx->native_ctx);
    } else {
        egl_offscreen->egl_driver->make_current(egl_offscreen->egl_driver,
                                                egl_offscreen->ensure_dpy,
                                                EGL_NO_SURFACE,
                                                EGL_NO_SURFACE,
                                                EGL_NO_CONTEXT);
    }
}

static void yagl_egl_offscreen_destroy(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_destroy, NULL);

    egl_offscreen->egl_driver->context_destroy(egl_offscreen->egl_driver,
                                               egl_offscreen->ensure_dpy,
                                               egl_offscreen->global_ctx);
    egl_offscreen->egl_driver->context_destroy(egl_offscreen->egl_driver,
                                               egl_offscreen->ensure_dpy,
                                               egl_offscreen->ensure_ctx);
    egl_offscreen->egl_driver->pbuffer_surface_destroy(egl_offscreen->egl_driver,
                                                       egl_offscreen->ensure_dpy,
                                                       egl_offscreen->ensure_sfc);
    egl_offscreen->egl_driver->config_cleanup(egl_offscreen->egl_driver,
                                              egl_offscreen->ensure_dpy,
                                              &egl_offscreen->ensure_config);
    egl_offscreen->egl_driver->display_close(egl_offscreen->egl_driver,
                                             egl_offscreen->ensure_dpy);

    egl_offscreen->egl_driver->destroy(egl_offscreen->egl_driver);
    egl_offscreen->egl_driver = NULL;
    egl_offscreen->gles_driver = NULL;

    yagl_egl_backend_cleanup(&egl_offscreen->base);

    g_free(egl_offscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_backend *yagl_egl_offscreen_create(struct yagl_egl_driver *egl_driver,
                                                   struct yagl_gles_driver *gles_driver)
{
    struct yagl_egl_offscreen *egl_offscreen = g_malloc0(sizeof(struct yagl_egl_offscreen));
    EGLNativeDisplayType dpy = NULL;
    struct yagl_egl_native_config *configs = NULL;
    int i, num_configs = 0;
    struct yagl_egl_pbuffer_attribs attribs;
    EGLSurface sfc = EGL_NO_SURFACE;
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLContext global_ctx = EGL_NO_CONTEXT;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_create, NULL);

    yagl_egl_pbuffer_attribs_init(&attribs);

    yagl_egl_backend_init(&egl_offscreen->base,
                          yagl_render_type_offscreen,
                          egl_driver->gl_version);

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

    ctx = egl_driver->context_create(egl_driver, dpy, &configs[0], NULL, 0);

    if (ctx == EGL_NO_CONTEXT) {
        goto fail;
    }

    global_ctx = egl_driver->context_create(egl_driver, dpy, &configs[0], ctx, 0);

    if (global_ctx == EGL_NO_CONTEXT) {
        goto fail;
    }

    egl_offscreen->base.thread_init = &yagl_egl_offscreen_thread_init;
    egl_offscreen->base.batch_start = &yagl_egl_offscreen_batch_start;
    egl_offscreen->base.create_display = &yagl_egl_offscreen_create_display;
    egl_offscreen->base.make_current = &yagl_egl_offscreen_make_current;
    egl_offscreen->base.release_current = &yagl_egl_offscreen_release_current;
    egl_offscreen->base.batch_end = &yagl_egl_offscreen_batch_end;
    egl_offscreen->base.thread_fini = &yagl_egl_offscreen_thread_fini;
    egl_offscreen->base.ensure_current = &yagl_egl_offscreen_ensure_current;
    egl_offscreen->base.unensure_current = &yagl_egl_offscreen_unensure_current;
    egl_offscreen->base.destroy = &yagl_egl_offscreen_destroy;

    egl_offscreen->egl_driver = egl_driver;
    egl_offscreen->gles_driver = gles_driver;
    egl_offscreen->ensure_dpy = dpy;
    egl_offscreen->ensure_config = configs[0];
    egl_offscreen->ensure_ctx = ctx;
    egl_offscreen->ensure_sfc = sfc;
    egl_offscreen->global_ctx = global_ctx;

    for (i = 1; i < num_configs; ++i) {
        egl_driver->config_cleanup(egl_driver, dpy, &configs[i]);
    }
    g_free(configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_offscreen->base;

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

    yagl_egl_backend_cleanup(&egl_offscreen->base);

    g_free(egl_offscreen);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
