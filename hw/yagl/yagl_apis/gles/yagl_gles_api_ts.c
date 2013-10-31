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

#include "yagl_gles_api_ts.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_vector.h"

void yagl_gles_api_ts_init(struct yagl_gles_api_ts *gles_api_ts,
                           struct yagl_gles_driver *driver,
                           struct yagl_gles_api_ps *ps)
{
    gles_api_ts->driver = driver;
    gles_api_ts->ps = ps;
}

void yagl_gles_api_ts_cleanup(struct yagl_gles_api_ts *gles_api_ts)
{
    uint32_t i;

    for (i = 0; i < gles_api_ts->num_arrays; ++i) {
        yagl_vector_cleanup(&gles_api_ts->arrays[i]);
    }
    g_free(gles_api_ts->arrays);
}
