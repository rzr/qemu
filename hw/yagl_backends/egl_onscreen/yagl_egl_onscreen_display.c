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

#include "yagl_egl_onscreen_display.h"
#include <GL/gl.h>
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_surface.h"
#include "yagl_egl_onscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_object_map.h"
#include "winsys_gl.h"

struct yagl_egl_onscreen_image
{
    struct yagl_object base;

    struct winsys_gl_surface *ws_sfc;
};

static void yagl_egl_onscreen_image_destroy(struct yagl_object *obj)
{
    struct yagl_egl_onscreen_image *image = (struct yagl_egl_onscreen_image*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_image_destroy, "%u", obj->global_name);

    image->ws_sfc->base.release(&image->ws_sfc->base);

    g_free(image);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_native_config
    *yagl_egl_onscreen_display_config_enum(struct yagl_eglb_display *dpy,
                                            int *num_configs)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;
    struct yagl_egl_native_config *native_configs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_config_enum,
                        "dpy = %p", dpy);

    native_configs =
        egl_onscreen->egl_driver->config_enum(egl_onscreen->egl_driver,
                                              egl_onscreen_dpy->native_dpy,
                                              num_configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return native_configs;
}

static void yagl_egl_onscreen_display_config_cleanup(struct yagl_eglb_display *dpy,
    const struct yagl_egl_native_config *cfg)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_config_cleanup,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    egl_onscreen->egl_driver->config_cleanup(egl_onscreen->egl_driver,
                                             egl_onscreen_dpy->native_dpy,
                                             cfg);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_context
    *yagl_egl_onscreen_display_create_context(struct yagl_eglb_display *dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              struct yagl_eglb_context *share_context)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_context *ctx =
        yagl_egl_onscreen_context_create(egl_onscreen_dpy,
                                         cfg,
                                         (struct yagl_egl_onscreen_context*)share_context);

    return ctx ? &ctx->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_onscreen_display_create_window_surface(struct yagl_eglb_display *dpy,
                                                     const struct yagl_egl_native_config *cfg,
                                                     const struct yagl_egl_window_attribs *attribs,
                                                     yagl_winsys_id id)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_surface *sfc =
        yagl_egl_onscreen_surface_create_window(egl_onscreen_dpy,
                                                cfg,
                                                attribs,
                                                id);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_onscreen_display_create_pixmap_surface(struct yagl_eglb_display *dpy,
                                                     const struct yagl_egl_native_config *cfg,
                                                     const struct yagl_egl_pixmap_attribs *attribs,
                                                     yagl_winsys_id id)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_surface *sfc =
        yagl_egl_onscreen_surface_create_pixmap(egl_onscreen_dpy,
                                                cfg,
                                                attribs,
                                                id);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_onscreen_display_create_pbuffer_surface(struct yagl_eglb_display *dpy,
                                                      const struct yagl_egl_native_config *cfg,
                                                      const struct yagl_egl_pbuffer_attribs *attribs,
                                                      yagl_winsys_id id)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_surface *sfc =
        yagl_egl_onscreen_surface_create_pbuffer(egl_onscreen_dpy,
                                                 cfg,
                                                 attribs,
                                                 id);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_object *yagl_egl_onscreen_display_create_image(struct yagl_eglb_display *dpy,
                                                                  yagl_winsys_id buffer)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_egl_onscreen_image *image;

    ws_sfc = (struct winsys_gl_surface*)egl_onscreen->wsi->acquire_surface(egl_onscreen->wsi, buffer);

    if (!ws_sfc) {
        return NULL;
    }

    image = g_malloc(sizeof(*image));

    image->base.global_name = ws_sfc->get_texture(ws_sfc);
    image->base.destroy = &yagl_egl_onscreen_image_destroy;
    image->ws_sfc = ws_sfc;

    return &image->base;
}

static void yagl_egl_onscreen_display_destroy(struct yagl_eglb_display *dpy)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_destroy,
                        "dpy = %p", dpy);

    egl_onscreen->egl_driver->display_close(egl_onscreen->egl_driver,
                                            egl_onscreen_dpy->native_dpy);

    yagl_eglb_display_cleanup(dpy);

    g_free(egl_onscreen_dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_onscreen_display
    *yagl_egl_onscreen_display_create(struct yagl_egl_onscreen *egl_onscreen)
{
    struct yagl_egl_onscreen_display *dpy;
    EGLNativeDisplayType native_dpy;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_create, NULL);

    native_dpy = egl_onscreen->egl_driver->display_open(egl_onscreen->egl_driver);

    if (!native_dpy) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    dpy = g_malloc0(sizeof(*dpy));

    yagl_eglb_display_init(&dpy->base, &egl_onscreen->base);

    dpy->base.config_enum = &yagl_egl_onscreen_display_config_enum;
    dpy->base.config_cleanup = &yagl_egl_onscreen_display_config_cleanup;
    dpy->base.create_context = &yagl_egl_onscreen_display_create_context;
    dpy->base.create_onscreen_window_surface = &yagl_egl_onscreen_display_create_window_surface;
    dpy->base.create_onscreen_pixmap_surface = &yagl_egl_onscreen_display_create_pixmap_surface;
    dpy->base.create_onscreen_pbuffer_surface = &yagl_egl_onscreen_display_create_pbuffer_surface;
    dpy->base.create_image = &yagl_egl_onscreen_display_create_image;
    dpy->base.destroy = &yagl_egl_onscreen_display_destroy;

    dpy->native_dpy = native_dpy;

    YAGL_LOG_FUNC_EXIT("%p", dpy);

    return dpy;
}
