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

#include "yagl_egl_surface.h"
#include "yagl_egl_display.h"
#include "yagl_egl_config.h"
#include "yagl_eglb_surface.h"

static void yagl_egl_surface_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_surface *sfc = (struct yagl_egl_surface*)ref;

    sfc->backend_sfc->destroy(sfc->backend_sfc);

    yagl_egl_config_release(sfc->cfg);

    yagl_resource_cleanup(&sfc->res);

    g_free(sfc);
}

struct yagl_egl_surface
    *yagl_egl_surface_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_eglb_surface *backend_sfc)
{
    struct yagl_egl_surface *sfc = g_malloc0(sizeof(struct yagl_egl_surface));

    yagl_resource_init(&sfc->res, &yagl_egl_surface_destroy);

    sfc->dpy = dpy;
    yagl_egl_config_acquire(cfg);
    sfc->cfg = cfg;
    sfc->backend_sfc = backend_sfc;

    return sfc;
}

void yagl_egl_surface_acquire(struct yagl_egl_surface *sfc)
{
    if (sfc) {
        yagl_resource_acquire(&sfc->res);
    }
}

void yagl_egl_surface_release(struct yagl_egl_surface *sfc)
{
    if (sfc) {
        yagl_resource_release(&sfc->res);
    }
}
