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
