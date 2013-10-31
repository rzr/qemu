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

#ifndef _QEMU_YAGL_EGL_CONTEXT_H
#define _QEMU_YAGL_EGL_CONTEXT_H

#include "yagl_types.h"
#include "yagl_resource.h"

struct yagl_egl_display;
struct yagl_egl_config;
struct yagl_egl_surface;
struct yagl_eglb_context;

struct yagl_egl_context
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_egl_config *cfg;

    struct yagl_eglb_context *backend_ctx;

    struct yagl_egl_surface *read;

    struct yagl_egl_surface *draw;
};

/*
 * Takes ownership of 'client_ctx'.
 */
struct yagl_egl_context
    *yagl_egl_context_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_eglb_context *backend_share_ctx);

void yagl_egl_context_update_surfaces(struct yagl_egl_context *ctx,
                                      struct yagl_egl_surface *draw,
                                      struct yagl_egl_surface *read);

/*
 * Helper functions that simply acquire/release yagl_egl_context::res
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_context_acquire(struct yagl_egl_context *ctx);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_context_release(struct yagl_egl_context *ctx);

/*
 * @}
 */

#endif
