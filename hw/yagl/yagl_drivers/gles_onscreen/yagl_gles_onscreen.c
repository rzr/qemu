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
#include "yagl_gles_onscreen.h"
#include "yagl_gles_driver.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_backends/egl_onscreen/yagl_egl_onscreen_ts.h"
#include "yagl_backends/egl_onscreen/yagl_egl_onscreen_context.h"

struct yagl_gles_onscreen
{
    struct yagl_gles_driver base;

    struct yagl_gles_driver *orig_driver;
};

YAGL_DECLARE_TLS(struct yagl_egl_onscreen_ts*, egl_onscreen_ts);

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_BindFramebuffer(GLenum target,
    GLuint framebuffer)
{
    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    assert(framebuffer != egl_onscreen_ts->ctx->fb);

    if (framebuffer == 0) {
        framebuffer = egl_onscreen_ts->ctx->fb;
    }

    egl_onscreen_ts->gles_driver->BindFramebuffer(target, framebuffer);
}

static void yagl_gles_onscreen_destroy(struct yagl_gles_driver *driver)
{
    struct yagl_gles_onscreen *gles_onscreen = (struct yagl_gles_onscreen*)driver;

    YAGL_LOG_FUNC_ENTER(yagl_gles_onscreen_destroy, NULL);

    gles_onscreen->orig_driver->destroy(gles_onscreen->orig_driver);
    gles_onscreen->orig_driver = NULL;

    yagl_gles_driver_cleanup(driver);
    g_free(gles_onscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles_driver
    *yagl_gles_onscreen_create(struct yagl_gles_driver *orig_driver)
{
    struct yagl_gles_onscreen *driver = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_gles_onscreen_create, NULL);

    driver = g_malloc0(sizeof(*driver));

    /*
     * Just copy over everything, then init our fields.
     */
    memcpy(&driver->base, orig_driver, sizeof(*orig_driver));
    driver->orig_driver = orig_driver;

    yagl_gles_driver_init(&driver->base, orig_driver->gl_version);

    driver->base.BindFramebuffer = &yagl_gles_onscreen_BindFramebuffer;

    driver->base.destroy = &yagl_gles_onscreen_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &driver->base;
}
