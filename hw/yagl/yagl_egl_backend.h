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

#ifndef _QEMU_YAGL_EGL_BACKEND_H
#define _QEMU_YAGL_EGL_BACKEND_H

#include "yagl_types.h"

struct yagl_eglb_display;
struct yagl_eglb_context;
struct yagl_eglb_surface;

/*
 * YaGL EGL backend.
 * @{
 */

struct yagl_egl_backend
{
    yagl_render_type render_type;

    void (*thread_init)(struct yagl_egl_backend */*backend*/);

    void (*batch_start)(struct yagl_egl_backend */*backend*/);

    struct yagl_eglb_display *(*create_display)(struct yagl_egl_backend */*backend*/);

    bool (*make_current)(struct yagl_egl_backend */*backend*/,
                         struct yagl_eglb_display */*dpy*/,
                         struct yagl_eglb_context */*ctx*/,
                         struct yagl_eglb_surface */*draw*/,
                         struct yagl_eglb_surface */*read*/);

    bool (*release_current)(struct yagl_egl_backend */*backend*/,
                            bool /*force*/);

    void (*batch_end)(struct yagl_egl_backend */*backend*/);

    void (*thread_fini)(struct yagl_egl_backend */*backend*/);

    /*
     * Make sure that some GL context is currently active. Can
     * be called before/after thread_init/thread_fini.
     * @{
     */

    void (*ensure_current)(struct yagl_egl_backend */*backend*/);

    void (*unensure_current)(struct yagl_egl_backend */*backend*/);

    /*
     * @}
     */

    void (*destroy)(struct yagl_egl_backend */*backend*/);
};

void yagl_egl_backend_init(struct yagl_egl_backend *backend,
                           yagl_render_type render_type);
void yagl_egl_backend_cleanup(struct yagl_egl_backend *backend);

/*
 * @}
 */

#endif
