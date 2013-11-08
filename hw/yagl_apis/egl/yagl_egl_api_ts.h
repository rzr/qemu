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

#ifndef _QEMU_YAGL_EGL_API_TS_H
#define _QEMU_YAGL_EGL_API_TS_H

#include "yagl_api.h"
#include <EGL/egl.h>

struct yagl_egl_backend;
struct yagl_egl_context;

struct yagl_egl_api_ts
{
    struct yagl_egl_api_ps *api_ps;

    /*
     * From 'api_ps->backend' for speed.
     */
    struct yagl_egl_backend *backend;

    EGLenum api;

    /*
     * This is GLES or OpenGL context, only one GLES or OpenGL context
     * can be current to a thread at the same time.
     */
    struct yagl_egl_context *context;
};

void yagl_egl_api_ts_init(struct yagl_egl_api_ts *egl_api_ts,
                          struct yagl_egl_api_ps *api_ps);

void yagl_egl_api_ts_cleanup(struct yagl_egl_api_ts *egl_api_ts);

void yagl_egl_api_ts_update_context(struct yagl_egl_api_ts *egl_api_ts,
                                    struct yagl_egl_context *ctx);

void yagl_egl_api_ts_reset(struct yagl_egl_api_ts *egl_api_ts);

#endif
