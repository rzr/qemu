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

#ifndef _QEMU_YAGL_EGL_OFFSCREEN_H
#define _QEMU_YAGL_EGL_OFFSCREEN_H

#include "yagl_egl_backend.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_native_config.h"

struct yagl_gles_driver;

struct yagl_egl_offscreen
{
    struct yagl_egl_backend base;

    struct yagl_egl_driver *egl_driver;

    struct yagl_gles_driver *gles_driver;

    /*
     * Display, config, context and surface which'll be used
     * when we need to ensure that at least some context
     * is current.
     */
    EGLNativeDisplayType ensure_dpy;
    struct yagl_egl_native_config ensure_config;
    EGLContext ensure_ctx;
    EGLSurface ensure_sfc;

    /*
     * Global context, all created contexts share with it. This
     * context is never current to any thread (And never make it
     * current because on windows it's impossible to share with
     * a context that's current).
     */
    EGLContext global_ctx;
};

/*
 * Takes ownership of 'egl_driver'
 */
struct yagl_egl_backend *yagl_egl_offscreen_create(struct yagl_egl_driver *egl_driver,
                                                   struct yagl_gles_driver *gles_driver);

#endif
