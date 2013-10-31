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

#include "yagl_eglb_surface.h"

void yagl_eglb_surface_init(struct yagl_eglb_surface *sfc,
                            struct yagl_eglb_display *dpy,
                            EGLenum type,
                            const void *attribs)
{
    sfc->dpy = dpy;
    sfc->type = type;

    switch (sfc->type) {
    case EGL_PBUFFER_BIT:
        memcpy(&sfc->attribs.pbuffer, attribs, sizeof(sfc->attribs.pbuffer));
        break;
    case EGL_PIXMAP_BIT:
        memcpy(&sfc->attribs.pixmap, attribs, sizeof(sfc->attribs.pixmap));
        break;
    case EGL_WINDOW_BIT:
        memcpy(&sfc->attribs.window, attribs, sizeof(sfc->attribs.window));
        break;
    default:
        assert(0);
        memset(&sfc->attribs, 0, sizeof(sfc->attribs));
    }
}

void yagl_eglb_surface_cleanup(struct yagl_eglb_surface *sfc)
{
}
