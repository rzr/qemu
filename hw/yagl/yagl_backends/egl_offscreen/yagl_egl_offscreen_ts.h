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

#ifndef _QEMU_YAGL_EGL_OFFSCREEN_TS_H
#define _QEMU_YAGL_EGL_OFFSCREEN_TS_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_egl_offscreen_display;
struct yagl_egl_offscreen_context;
struct yagl_egl_offscreen_surface;

struct yagl_egl_offscreen_ts
{
    struct yagl_egl_offscreen_display *dpy;
    struct yagl_egl_offscreen_context *ctx;
    EGLSurface sfc_draw;
    EGLSurface sfc_read;
};

struct yagl_egl_offscreen_ts *yagl_egl_offscreen_ts_create(void);

void yagl_egl_offscreen_ts_destroy(struct yagl_egl_offscreen_ts *egl_offscreen_ts);

#endif
