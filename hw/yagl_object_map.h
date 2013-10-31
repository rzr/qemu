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

#ifndef _QEMU_YAGL_OBJECT_MAP_H
#define _QEMU_YAGL_OBJECT_MAP_H

#include "yagl_types.h"

struct yagl_avl_table;

struct yagl_object
{
    yagl_object_name global_name;

    void (*destroy)(struct yagl_object */*obj*/);
};

struct yagl_object_map
{
    struct yagl_avl_table *entries;
};

struct yagl_object_map *yagl_object_map_create(void);

void yagl_object_map_destroy(struct yagl_object_map *object_map);

void yagl_object_map_add(struct yagl_object_map *object_map,
                         yagl_object_name local_name,
                         struct yagl_object *obj);

void yagl_object_map_remove(struct yagl_object_map *object_map,
                            yagl_object_name local_name);

void yagl_object_map_remove_all(struct yagl_object_map *object_map);

yagl_object_name yagl_object_map_get(struct yagl_object_map *object_map,
                                     yagl_object_name local_name);

#endif
