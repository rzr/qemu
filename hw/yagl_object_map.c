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
#include "yagl_avl.h"

struct yagl_object_map_entry
{
    yagl_object_name local_name;

    struct yagl_object *obj;
};

static int yagl_object_map_entry_comparison_func(const void *avl_a,
                                                 const void *avl_b,
                                                 void *avl_param)
{
    const struct yagl_object_map_entry *a = avl_a;
    const struct yagl_object_map_entry *b = avl_b;

    if (a->local_name < b->local_name) {
        return -1;
    } else if (a->local_name > b->local_name) {
        return 1;
    } else {
        return 0;
    }
}

static void yagl_object_map_entry_destroy_func(void *avl_item, void *avl_param)
{
    struct yagl_object_map_entry *item = avl_item;

    item->obj->destroy(item->obj);

    g_free(item);
}

struct yagl_object_map *yagl_object_map_create(void)
{
    struct yagl_object_map *object_map = g_malloc0(sizeof(struct yagl_object_map));

    object_map->entries = yagl_avl_create(&yagl_object_map_entry_comparison_func,
                                          object_map,
                                          NULL);
    assert(object_map->entries);

    return object_map;
}

void yagl_object_map_destroy(struct yagl_object_map *object_map)
{
    yagl_avl_destroy(object_map->entries, &yagl_object_map_entry_destroy_func);
    object_map->entries = NULL;

    g_free(object_map);
}

void yagl_object_map_add(struct yagl_object_map *object_map,
                         yagl_object_name local_name,
                         struct yagl_object *obj)
{
    struct yagl_object_map_entry *item =
        g_malloc0(sizeof(struct yagl_object_map_entry));

    item->local_name = local_name;
    item->obj = obj;

    yagl_avl_assert_insert(object_map->entries, item);
}

void yagl_object_map_remove(struct yagl_object_map *object_map,
                            yagl_object_name local_name)
{
    void *item;
    struct yagl_object_map_entry dummy;

    dummy.local_name = local_name;

    item = yagl_avl_assert_delete(object_map->entries, &dummy);

    yagl_object_map_entry_destroy_func(item, object_map->entries->avl_param);
}

void yagl_object_map_remove_all(struct yagl_object_map *object_map)
{
    yagl_avl_destroy(object_map->entries, &yagl_object_map_entry_destroy_func);

    object_map->entries = yagl_avl_create(&yagl_object_map_entry_comparison_func,
                                          object_map,
                                          NULL);
    assert(object_map->entries);
}

yagl_object_name yagl_object_map_get(struct yagl_object_map *object_map,
                                     yagl_object_name local_name)
{
    struct yagl_object_map_entry *item;
    struct yagl_object_map_entry dummy;

    dummy.local_name = local_name;

    item = yagl_avl_find(object_map->entries, &dummy);

    if (item) {
        return item->obj->global_name;
    } else {
        assert(0);
        return 0;
    }
}
