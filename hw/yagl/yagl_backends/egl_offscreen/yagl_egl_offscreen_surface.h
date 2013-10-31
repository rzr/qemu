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

#ifndef _QEMU_YAGL_EGL_OFFSCREEN_SURFACE_H
#define _QEMU_YAGL_EGL_OFFSCREEN_SURFACE_H

#include "yagl_eglb_surface.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_surface_attribs.h"

struct yagl_egl_offscreen_display;
struct yagl_compiled_transfer;
struct yagl_egl_native_config;

struct yagl_egl_offscreen_surface
{
    struct yagl_eglb_surface base;

    /*
     * Compiled transfer for backing image. It'll become NULL
     * after eglDestroySurface is called.
     */
    struct yagl_compiled_transfer *bimage_ct;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;

    /*
     * Host pre-allocated storage for backing image.
     * This will go to compiled transfer above.
     */
    void *host_pixels;

    /*
     * Native pbuffer surface.
     */
    EGLSurface native_sfc;

    /*
     * Attribs with which this 'native_sfc' was created with.
     */
    struct yagl_egl_pbuffer_attribs native_sfc_attribs;
};

struct yagl_egl_offscreen_surface
    *yagl_egl_offscreen_surface_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       EGLenum type,
                                       const void *attribs,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t bpp,
                                       target_ulong pixels);

#endif
