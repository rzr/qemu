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

#ifndef _QEMU_YAGL_RESOURCE_LIST_H
#define _QEMU_YAGL_RESOURCE_LIST_H

#include "yagl_types.h"
#include "qemu/queue.h"

/*
 * reference counted resource management.
 */

struct yagl_resource;

struct yagl_resource_list
{
    QTAILQ_HEAD(, yagl_resource) resources;
    int count;
};


/*
 * Initialize the list.
 */
void yagl_resource_list_init(struct yagl_resource_list *list);

/*
 * Release all resources in the list and empty the list. The
 * list should be initialized again in order to be used after this.
 */
void yagl_resource_list_cleanup(struct yagl_resource_list *list);

int yagl_resource_list_get_count(struct yagl_resource_list *list);

/*
 * Add a resource to the end of the list, this acquires 'res', so the
 * caller should
 * release 'res' if he doesn't want to use it and wants it to belong to this
 * list alone.
 */
void yagl_resource_list_add(struct yagl_resource_list *list,
                            struct yagl_resource *res);

/*
 * Moves all resources from 'from_list' to 'to_list'. The resources will be
 * added to the tail of the 'to_list'. 'from_list' will be empty. Resource
 * reference count will remain the same.
 */
void yagl_resource_list_move(struct yagl_resource_list *from_list,
                             struct yagl_resource_list *to_list);

/*
 * Acquires a resource by handle. Be sure to release the resource when
 * you're done.
 */
struct yagl_resource
    *yagl_resource_list_acquire(struct yagl_resource_list *list,
                                yagl_host_handle handle);

/*
 * Removes a resource from the end of the list, this also releases
 * the resource.
 */
bool yagl_resource_list_remove(struct yagl_resource_list *list,
                               yagl_host_handle handle);

#endif
