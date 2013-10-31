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

#include "yagl_resource.h"
#include "yagl_handle_gen.h"

void yagl_resource_init(struct yagl_resource *res,
                        yagl_ref_destroy_func destroy)
{
    yagl_ref_init(&res->ref, destroy);
    res->handle = yagl_handle_gen();
}

void yagl_resource_cleanup(struct yagl_resource *res)
{
    res->handle = 0;
    yagl_ref_cleanup(&res->ref);
}

void yagl_resource_acquire(struct yagl_resource *res)
{
    if (res) {
        yagl_ref_acquire(&res->ref);
    }
}

void yagl_resource_release(struct yagl_resource *res)
{
    if (res) {
        yagl_ref_release(&res->ref);
    }
}
