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

#ifndef _QEMU_YAGL_EGL_ONSCREEN_SURFACE_H
#define _QEMU_YAGL_EGL_ONSCREEN_SURFACE_H

#include "yagl_eglb_surface.h"
#include "yagl_egl_surface_attribs.h"

struct yagl_egl_onscreen_display;
struct yagl_egl_native_config;

struct winsys_gl_surface;

struct yagl_egl_onscreen_surface
{
    struct yagl_eglb_surface base;

    /*
     * 1x1 native pbuffer surface, used just to be able to make
     * this surface current.
     */
    EGLSurface dummy_native_sfc;

    /*
     * winsys surface.
     */
    struct winsys_gl_surface *ws_sfc;

    /*
     * Depth and stencil renderbuffer for 'ws_sfc'. Allocated
     * when this surface is made current for the first time.
     */
    GLuint rb;
};

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_window(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_window_attribs *attribs,
                                             yagl_winsys_id id);

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pixmap(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pixmap_attribs *attribs,
                                             yagl_winsys_id id);

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pbuffer(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pbuffer_attribs *attribs,
                                             yagl_winsys_id id);

void yagl_egl_onscreen_surface_setup(struct yagl_egl_onscreen_surface *sfc);

void yagl_egl_onscreen_surface_attach_to_framebuffer(struct yagl_egl_onscreen_surface *sfc,
                                                     GLenum target);

uint32_t yagl_egl_onscreen_surface_width(struct yagl_egl_onscreen_surface *sfc);

uint32_t yagl_egl_onscreen_surface_height(struct yagl_egl_onscreen_surface *sfc);

#endif
