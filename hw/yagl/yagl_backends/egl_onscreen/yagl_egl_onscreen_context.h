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

#ifndef _QEMU_YAGL_EGL_ONSCREEN_CONTEXT_H
#define _QEMU_YAGL_EGL_ONSCREEN_CONTEXT_H

#include "yagl_eglb_context.h"
#include "yagl_egl_native_config.h"
#include <EGL/egl.h>

struct yagl_egl_onscreen_display;
struct yagl_egl_onscreen_surface;

struct yagl_egl_onscreen_context
{
    struct yagl_eglb_context base;

    EGLContext native_ctx;

    /*
     * Framebuffer that is used to render to window/pixmap/pbuffer texture.
     * Allocated when this context is made current for the first time.
     *
     * Onscreen GLES API redirects framebuffer zero to this framebuffer.
     */
    GLuint fb;
    uint32_t fb_ctx_id;

    /*
     * Config with which this context was created, used
     * for 'null_sfc' creation.
     *
     * TODO: It's not very nice to copy 'cfg' into this. It works
     * only because all configs are display resources and context
     * is also a display resource and contexts always outlive configs.
     * But it could change someday...
     */
    struct yagl_egl_native_config cfg;

    /*
     * 1x1 native pbuffer surface, used to implement
     * EGL_KHR_surfaceless_context. Allocated on first
     * surfaceless eglMakeCurrent.
     */
    EGLSurface null_sfc;
};

struct yagl_egl_onscreen_context
    *yagl_egl_onscreen_context_create(struct yagl_egl_onscreen_display *dpy,
                                      const struct yagl_egl_native_config *cfg,
                                      struct yagl_egl_onscreen_context *share_context,
                                      int version);

void yagl_egl_onscreen_context_setup(struct yagl_egl_onscreen_context *ctx);

bool yagl_egl_onscreen_context_setup_surfaceless(struct yagl_egl_onscreen_context *ctx);

#endif
