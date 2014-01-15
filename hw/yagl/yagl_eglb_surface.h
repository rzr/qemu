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

#ifndef _QEMU_YAGL_EGLB_SURFACE_H
#define _QEMU_YAGL_EGLB_SURFACE_H

#include "yagl_types.h"
#include "yagl_egl_surface_attribs.h"
#include <EGL/egl.h>

struct yagl_eglb_display;

struct yagl_eglb_surface
{
    struct yagl_eglb_display *dpy;

    EGLenum type;

    union
    {
        struct yagl_egl_window_attribs window;
        struct yagl_egl_pixmap_attribs pixmap;
        struct yagl_egl_pbuffer_attribs pbuffer;
    } attribs;

    /*
     * Surface has been invalidated on target, update it
     * from 'id'.
     */
    void (*invalidate)(struct yagl_eglb_surface */*sfc*/,
                       yagl_winsys_id /*id*/);

    /*
     * Replaces 'sfc' with 'with', destroying 'with' afterwards.
     * 'sfc' and 'with' must be of same type, but can have
     * different formats.
     */
    void (*replace)(struct yagl_eglb_surface */*sfc*/,
                    struct yagl_eglb_surface */*with*/);

    /*
     * Can be called for surfaces that were reset.
     */
    bool (*query)(struct yagl_eglb_surface */*sfc*/,
                  EGLint /*attribute*/,
                  EGLint */*value*/);

    void (*swap_buffers)(struct yagl_eglb_surface */*sfc*/);

    void (*copy_buffers)(struct yagl_eglb_surface */*sfc*/);

    void (*wait_gl)(struct yagl_eglb_surface */*sfc*/);

    void (*destroy)(struct yagl_eglb_surface */*sfc*/);
};

void yagl_eglb_surface_init(struct yagl_eglb_surface *sfc,
                            struct yagl_eglb_display *dpy,
                            EGLenum type,
                            const void *attribs);
void yagl_eglb_surface_cleanup(struct yagl_eglb_surface *sfc);

#endif
