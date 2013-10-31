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

#ifndef _QEMU_YAGL_EGL_DISPLAY_H
#define _QEMU_YAGL_EGL_DISPLAY_H

#include "yagl_types.h"
#include "yagl_resource_list.h"
#include "qemu/queue.h"
#include <EGL/egl.h>

struct yagl_egl_backend;
struct yagl_egl_config;
struct yagl_egl_native_config;
struct yagl_egl_surface;
struct yagl_egl_context;
struct yagl_eglb_display;

struct yagl_egl_display
{
    QLIST_ENTRY(yagl_egl_display) entry;

    struct yagl_egl_backend *backend;

    uint32_t display_id;

    yagl_host_handle handle;

    struct yagl_eglb_display *backend_dpy;

    bool initialized;

    struct yagl_resource_list configs;

    struct yagl_resource_list contexts;

    struct yagl_resource_list surfaces;
};

struct yagl_egl_display
    *yagl_egl_display_create(struct yagl_egl_backend *backend,
                             uint32_t display_id);

void yagl_egl_display_destroy(struct yagl_egl_display *dpy);

void yagl_egl_display_initialize(struct yagl_egl_display *dpy);

bool yagl_egl_display_is_initialized(struct yagl_egl_display *dpy);

void yagl_egl_display_terminate(struct yagl_egl_display *dpy);

/*
 * Configs.
 * @{
 */

int32_t yagl_egl_display_get_config_count(struct yagl_egl_display *dpy);

/*
 * Gets at most '*num_configs' config handles and updates
 * 'num_configs' with number of configs actually returned.
 * The configs themselves are not acquired.
 */
void yagl_egl_display_get_config_handles(struct yagl_egl_display *dpy,
                                         yagl_host_handle *handles,
                                         int32_t *num_configs);

/*
 * Gets at most '*num_configs' config handles and updates
 * 'num_configs' with number of configs actually returned.
 * The configs themselves are not acquired.
 *
 * 'dummy' is used for matching, only handles of configs that matched
 * 'dummy' will be returned. if 'handles' is NULL then no handles are
 * returned, but 'num_configs' is still updated.
 */
void yagl_egl_display_choose_configs(struct yagl_egl_display *dpy,
                                     const struct yagl_egl_native_config *dummy,
                                     yagl_host_handle *handles,
                                     int32_t *num_configs);

struct yagl_egl_config
    *yagl_egl_display_acquire_config(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle);

struct yagl_egl_config
    *yagl_egl_display_acquire_config_by_id(struct yagl_egl_display *dpy,
                                           EGLint config_id);

/*
 * @}
 */

/*
 * Contexts.
 * @{
 */

/*
 * This acquires 'ctx', so the caller should
 * release 'ctx' if he doesn't want to use it and wants it to belong to the
 * display alone.
 */
void yagl_egl_display_add_context(struct yagl_egl_display *dpy,
                                  struct yagl_egl_context *ctx);

struct yagl_egl_context
    *yagl_egl_display_acquire_context(struct yagl_egl_display *dpy,
                                      yagl_host_handle handle);

bool yagl_egl_display_remove_context(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle);

/*
 * @}
 */

/*
 * Surfaces.
 * @{
 */

/*
 * This acquires 'sfc', so the caller should
 * release 'sfc' if he doesn't want to use it and wants it to belong to the
 * display alone.
 */
void yagl_egl_display_add_surface(struct yagl_egl_display *dpy,
                                  struct yagl_egl_surface *sfc);

struct yagl_egl_surface
    *yagl_egl_display_acquire_surface(struct yagl_egl_display *dpy,
                                      yagl_host_handle handle);

bool yagl_egl_display_remove_surface(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle);

/*
 * @}
 */

#endif
