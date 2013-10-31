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

#ifndef _QEMU_YAGL_EGL_CONFIG_H
#define _QEMU_YAGL_EGL_CONFIG_H

#include "yagl_types.h"
#include "yagl_resource.h"
#include "yagl_egl_native_config.h"

struct yagl_egl_display;

struct yagl_egl_config
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_egl_native_config native;
};

/*
 * Returns an array of EGL configs. The array itself must
 * be freed by 'g_free' as usual. EGL configs must be
 * released using YaGL resource release mechanism.
 *
 * Note that config list is unsorted, use 'yagl_egl_config_sort'
 * to sort.
 */
struct yagl_egl_config
    **yagl_egl_config_enum(struct yagl_egl_display *dpy,
                           int* num_configs );

/*
 * Sort according to 'EGL 1.4: 3.4.1. Sorting of EGLConfigs'
 */
void yagl_egl_config_sort(struct yagl_egl_config **cfgs, int num_configs);

/*
 * Choose according to 'EGL 1.4: 3.4.1. Selection of EGLConfigs'
 */
bool yagl_egl_config_is_chosen_by(const struct yagl_egl_config *cfg,
                                  const struct yagl_egl_native_config *dummy);

bool yagl_egl_config_get_attrib(const struct yagl_egl_config *cfg,
                                EGLint attrib,
                                EGLint *value);

/*
 * Helper functions that simply acquire/release yagl_egl_config::res
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_config_acquire(struct yagl_egl_config *cfg);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_config_release(struct yagl_egl_config *cfg);

/*
 * @}
 */

#endif
