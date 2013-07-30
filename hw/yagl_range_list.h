#ifndef _QEMU_YAGL_RANGE_LIST_H
#define _QEMU_YAGL_RANGE_LIST_H

#include "yagl_types.h"
#include "yagl_vector.h"

struct yagl_range_list
{
    struct yagl_vector ranges;
};

void yagl_range_list_init(struct yagl_range_list *range_list);

void yagl_range_list_cleanup(struct yagl_range_list *range_list);

void yagl_range_list_add(struct yagl_range_list *range_list,
                         int start,
                         int size);

int yagl_range_list_size(struct yagl_range_list *range_list);

void yagl_range_list_get(struct yagl_range_list *range_list,
                         int i,
                         int *start,
                         int *size);

void yagl_range_list_clear(struct yagl_range_list *range_list);

#endif
