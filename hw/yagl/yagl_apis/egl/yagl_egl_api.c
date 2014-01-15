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

#include "yagl_egl_api.h"
#include "yagl_host_egl_calls.h"
#include "yagl_egl_backend.h"

static void yagl_egl_api_destroy(struct yagl_api *api)
{
    struct yagl_egl_api *egl_api = (struct yagl_egl_api*)api;

    egl_api->backend->destroy(egl_api->backend);
    egl_api->backend = NULL;

    yagl_api_cleanup(&egl_api->base);

    g_free(egl_api);
}

struct yagl_api *yagl_egl_api_create(struct yagl_egl_backend *backend)
{
    struct yagl_egl_api *egl_api = g_malloc0(sizeof(struct yagl_egl_api));

    yagl_api_init(&egl_api->base);

    egl_api->base.process_init = &yagl_host_egl_process_init;
    egl_api->base.destroy = &yagl_egl_api_destroy;

    egl_api->backend = backend;

    return &egl_api->base;
}
