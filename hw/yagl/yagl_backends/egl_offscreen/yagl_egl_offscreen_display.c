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
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_object_map.h"
#include "yagl_gles_driver.h"

struct yagl_egl_offscreen_image
{
    struct yagl_object base;

    struct yagl_gles_driver *driver;
};

static void yagl_egl_offscreen_image_destroy(struct yagl_object *obj)
{
    struct yagl_egl_offscreen_image *image = (struct yagl_egl_offscreen_image*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_image_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    image->driver->DeleteTextures(1, &obj->global_name);
    yagl_unensure_ctx(0);

    g_free(image);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_native_config
    *yagl_egl_offscreen_display_config_enum(struct yagl_eglb_display *dpy,
                                            int *num_configs)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;
    struct yagl_egl_native_config *native_configs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_config_enum,
                        "dpy = %p", dpy);

    native_configs =
        egl_offscreen->egl_driver->config_enum(egl_offscreen->egl_driver,
                                               egl_offscreen_dpy->native_dpy,
                                               num_configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return native_configs;
}

static void yagl_egl_offscreen_display_config_cleanup(struct yagl_eglb_display *dpy,
    const struct yagl_egl_native_config *cfg)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_config_cleanup,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    egl_offscreen->egl_driver->config_cleanup(egl_offscreen->egl_driver,
                                              egl_offscreen_dpy->native_dpy,
                                              cfg);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_context
    *yagl_egl_offscreen_display_create_context(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               struct yagl_eglb_context *share_context,
                                               int version)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_context *ctx =
        yagl_egl_offscreen_context_create(egl_offscreen_dpy,
                                          cfg,
                                          (struct yagl_egl_offscreen_context*)share_context,
                                          version);

    return ctx ? &ctx->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_offscreen_display_create_surface(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               EGLenum type,
                                               const void *attribs,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t bpp,
                                               target_ulong pixels)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_surface *sfc =
        yagl_egl_offscreen_surface_create(egl_offscreen_dpy,
                                          cfg,
                                          type,
                                          attribs,
                                          width,
                                          height,
                                          bpp,
                                          pixels);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_object *yagl_egl_offscreen_display_create_image(struct yagl_eglb_display *dpy,
                                                                   yagl_winsys_id buffer)
{
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;
    struct yagl_egl_offscreen_image *image;

    image = g_malloc(sizeof(*image));

    yagl_ensure_ctx(0);
    egl_offscreen->gles_driver->GenTextures(1, &image->base.global_name);
    yagl_unensure_ctx(0);
    image->base.destroy = &yagl_egl_offscreen_image_destroy;
    image->driver = egl_offscreen->gles_driver;

    return &image->base;
}

static void yagl_egl_offscreen_display_destroy(struct yagl_eglb_display *dpy)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_destroy,
                        "dpy = %p", dpy);

    egl_offscreen->egl_driver->display_close(egl_offscreen->egl_driver,
                                             egl_offscreen_dpy->native_dpy);

    yagl_eglb_display_cleanup(dpy);

    g_free(egl_offscreen_dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_display
    *yagl_egl_offscreen_display_create(struct yagl_egl_offscreen *egl_offscreen)
{
    struct yagl_egl_offscreen_display *dpy;
    EGLNativeDisplayType native_dpy;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_create, NULL);

    native_dpy = egl_offscreen->egl_driver->display_open(egl_offscreen->egl_driver);

    if (!native_dpy) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    dpy = g_malloc0(sizeof(*dpy));

    yagl_eglb_display_init(&dpy->base, &egl_offscreen->base);

    dpy->base.config_enum = &yagl_egl_offscreen_display_config_enum;
    dpy->base.config_cleanup = &yagl_egl_offscreen_display_config_cleanup;
    dpy->base.create_context = &yagl_egl_offscreen_display_create_context;
    dpy->base.create_offscreen_surface = &yagl_egl_offscreen_display_create_surface;
    dpy->base.create_image = &yagl_egl_offscreen_display_create_image;
    dpy->base.destroy = &yagl_egl_offscreen_display_destroy;

    dpy->native_dpy = native_dpy;

    YAGL_LOG_FUNC_EXIT("%p", dpy);

    return dpy;
}
