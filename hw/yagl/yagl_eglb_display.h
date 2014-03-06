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

#ifndef _QEMU_YAGL_EGLB_DISPLAY_H
#define _QEMU_YAGL_EGLB_DISPLAY_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_egl_backend;
struct yagl_eglb_context;
struct yagl_eglb_surface;
struct yagl_egl_native_config;
struct yagl_egl_window_attribs;
struct yagl_egl_pixmap_attribs;
struct yagl_egl_pbuffer_attribs;
struct yagl_object;

struct yagl_eglb_display
{
    struct yagl_egl_backend *backend;

    struct yagl_egl_native_config *(*config_enum)(struct yagl_eglb_display */*dpy*/,
                                                  int */*num_configs*/);

    void (*config_cleanup)(struct yagl_eglb_display */*dpy*/,
                           const struct yagl_egl_native_config */*cfg*/);

    struct yagl_eglb_context *(*create_context)(struct yagl_eglb_display */*dpy*/,
                                                const struct yagl_egl_native_config */*cfg*/,
                                                struct yagl_eglb_context */*share_context*/,
                                                int /*version*/);

    /*
     * 'pixels' are locked in target's memory, no page fault possible.
     */
    struct yagl_eglb_surface *(*create_offscreen_surface)(struct yagl_eglb_display */*dpy*/,
                                                          const struct yagl_egl_native_config */*cfg*/,
                                                          EGLenum /*type*/,
                                                          const void */*attribs*/,
                                                          uint32_t /*width*/,
                                                          uint32_t /*height*/,
                                                          uint32_t /*bpp*/,
                                                          target_ulong /*pixels*/);

    struct yagl_eglb_surface *(*create_onscreen_window_surface)(struct yagl_eglb_display */*dpy*/,
                                                                const struct yagl_egl_native_config */*cfg*/,
                                                                const struct yagl_egl_window_attribs */*attribs*/,
                                                                yagl_winsys_id /*id*/);

    struct yagl_eglb_surface *(*create_onscreen_pixmap_surface)(struct yagl_eglb_display */*dpy*/,
                                                                const struct yagl_egl_native_config */*cfg*/,
                                                                const struct yagl_egl_pixmap_attribs */*attribs*/,
                                                                yagl_winsys_id /*id*/);

    struct yagl_eglb_surface *(*create_onscreen_pbuffer_surface)(struct yagl_eglb_display */*dpy*/,
                                                                 const struct yagl_egl_native_config */*cfg*/,
                                                                 const struct yagl_egl_pbuffer_attribs */*attribs*/,
                                                                 yagl_winsys_id /*id*/);

    struct yagl_object *(*create_image)(struct yagl_eglb_display */*dpy*/,
                                        yagl_winsys_id /*buffer*/);

    void (*destroy)(struct yagl_eglb_display */*dpy*/);
};

void yagl_eglb_display_init(struct yagl_eglb_display *dpy,
                            struct yagl_egl_backend *backend);
void yagl_eglb_display_cleanup(struct yagl_eglb_display *dpy);

#endif
