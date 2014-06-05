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

#include "yagl_object_map.h"

static void yagl_object_map_entry_destroy_func(gpointer data)
{
    struct yagl_object *item = data;

    item->destroy(item);
}

struct yagl_object_map *yagl_object_map_create(void)
{
    struct yagl_object_map *object_map = g_malloc0(sizeof(struct yagl_object_map));

    object_map->entries = g_hash_table_new_full(g_direct_hash,
                                                g_direct_equal,
                                                NULL,
                                                &yagl_object_map_entry_destroy_func);

    assert(object_map->entries);

    return object_map;
}

void yagl_object_map_destroy(struct yagl_object_map *object_map)
{
    g_hash_table_destroy(object_map->entries);

    object_map->entries = NULL;

    g_free(object_map);
}

void yagl_object_map_add(struct yagl_object_map *object_map,
                         yagl_object_name local_name,
                         struct yagl_object *obj)
{
    guint size = g_hash_table_size(object_map->entries);

    g_hash_table_insert(object_map->entries,
                        GUINT_TO_POINTER(local_name),
                        obj);

    assert(g_hash_table_size(object_map->entries) > size);
}

void yagl_object_map_remove(struct yagl_object_map *object_map,
                            yagl_object_name local_name)
{
    guint size = g_hash_table_size(object_map->entries);

    g_hash_table_remove(object_map->entries, GUINT_TO_POINTER(local_name));

    assert(g_hash_table_size(object_map->entries) < size);
}

void yagl_object_map_remove_all(struct yagl_object_map *object_map)
{
    g_hash_table_remove_all(object_map->entries);
}

yagl_object_name yagl_object_map_get(struct yagl_object_map *object_map,
                                     yagl_object_name local_name)
{
    struct yagl_object *item = g_hash_table_lookup(object_map->entries,
                                                   GUINT_TO_POINTER(local_name));

    if (item) {
        return item->global_name;
    } else {
        assert(0);
        return 0;
    }
}
