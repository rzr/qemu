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

#include "yagl_gles_api.h"
#include "yagl_host_gles_calls.h"
#include "yagl_gles_driver.h"

static void yagl_gles_api_destroy(struct yagl_api *api)
{
    struct yagl_gles_api *gles_api = (struct yagl_gles_api*)api;

    gles_api->driver->destroy(gles_api->driver);
    gles_api->driver = NULL;

    yagl_api_cleanup(&gles_api->base);

    g_free(gles_api);
}

struct yagl_api *yagl_gles_api_create(struct yagl_gles_driver *driver)
{
    struct yagl_gles_api *gles_api = g_malloc0(sizeof(struct yagl_gles_api));

    yagl_api_init(&gles_api->base);

    gles_api->base.process_init = &yagl_host_gles_process_init;
    gles_api->base.destroy = &yagl_gles_api_destroy;

    gles_api->driver = driver;

    return &gles_api->base;
}
