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

#include <GL/gl.h>
#include "yagl_gles_driver.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_egl_interface.h"

void yagl_gles_driver_init(struct yagl_gles_driver *driver,
                           yagl_gl_version gl_version)
{
    driver->gl_version = gl_version;
}

void yagl_gles_driver_cleanup(struct yagl_gles_driver *driver)
{
}

uint32_t yagl_get_ctx_id(void)
{
    assert(cur_ts);
    return cur_ts->ps->egl_iface->get_ctx_id(cur_ts->ps->egl_iface);
}

void yagl_ensure_ctx(uint32_t ctx_id)
{
    assert(cur_ts);
    cur_ts->ps->egl_iface->ensure_ctx(cur_ts->ps->egl_iface, ctx_id);
}

void yagl_unensure_ctx(uint32_t ctx_id)
{
    assert(cur_ts);
    cur_ts->ps->egl_iface->unensure_ctx(cur_ts->ps->egl_iface, ctx_id);
}
