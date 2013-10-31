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

#ifndef _QEMU_YAGL_RESOURCE_H
#define _QEMU_YAGL_RESOURCE_H

#include "yagl_types.h"
#include "yagl_ref.h"
#include "qemu/queue.h"

struct yagl_resource
{
    struct yagl_ref ref;

    QTAILQ_ENTRY(yagl_resource) entry;

    yagl_host_handle handle;
};

/*
 * For implementations.
 * @{
 */

void yagl_resource_init(struct yagl_resource *res,
                        yagl_ref_destroy_func destroy);
void yagl_resource_cleanup(struct yagl_resource *res);

/*
 * @}
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_resource_acquire(struct yagl_resource *res);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_resource_release(struct yagl_resource *res);

#endif
