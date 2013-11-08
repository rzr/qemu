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

#ifndef _QEMU_YAGL_TYPES_H
#define _QEMU_YAGL_TYPES_H

#include "qemu-common.h"

typedef uint32_t yagl_pid;
typedef uint32_t yagl_tid;
typedef uint32_t yagl_func_id;
typedef uint32_t yagl_host_handle;
typedef uint32_t yagl_object_name;
typedef uint32_t yagl_winsys_id;

struct yagl_transport;

/*
 * YaGL supported render types.
 */
typedef enum
{
    yagl_render_type_offscreen = 1,
    yagl_render_type_onscreen = 2,
} yagl_render_type;

/*
 * YaGL supported API ids.
 */
typedef enum
{
    yagl_api_id_egl = 1,
    yagl_api_id_gles = 2
} yagl_api_id;

#define YAGL_NUM_APIS 2

/*
 * EGL client APIs.
 */
typedef enum
{
    yagl_client_api_ogl = 0,
    yagl_client_api_gles1 = 1,
    yagl_client_api_gles2 = 2,
    yagl_client_api_ovg = 3
} yagl_client_api;

#define YAGL_NUM_CLIENT_APIS 4

typedef bool (*yagl_api_func)(struct yagl_transport */*t*/);

#endif
