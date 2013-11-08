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

#include "yagl_egl_onscreen_ts.h"
#include <GL/gl.h>
#include "yagl_egl_onscreen.h"
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_surface.h"
#include "yagl_gles_driver.h"

struct yagl_egl_onscreen_ts *yagl_egl_onscreen_ts_create(struct yagl_gles_driver *gles_driver)
{
    struct yagl_egl_onscreen_ts *egl_onscreen_ts =
        g_malloc0(sizeof(struct yagl_egl_onscreen_ts));

    egl_onscreen_ts->gles_driver = gles_driver;

    return egl_onscreen_ts;
}

void yagl_egl_onscreen_ts_destroy(struct yagl_egl_onscreen_ts *egl_onscreen_ts)
{
    g_free(egl_onscreen_ts);
}
