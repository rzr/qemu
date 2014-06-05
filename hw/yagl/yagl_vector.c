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

#include "yagl_vector.h"

#define YAGL_VECTOR_DEFAULT_CAPACITY 10

void yagl_vector_init(struct yagl_vector *v,
                      int elem_size,
                      int initial_capacity)
{
    assert(elem_size > 0);

    v->elem_size = elem_size;
    v->capacity = (initial_capacity < YAGL_VECTOR_DEFAULT_CAPACITY) ?
                      YAGL_VECTOR_DEFAULT_CAPACITY
                    : initial_capacity;
    v->data = g_malloc0(v->capacity * v->elem_size);
    v->size = 0;
}

void yagl_vector_cleanup(struct yagl_vector *v)
{
    g_free(v->data);
    memset(v, 0, sizeof(*v));
}

void *yagl_vector_detach(struct yagl_vector *v)
{
    void *tmp = v->data;
    memset(v, 0, sizeof(*v));
    return tmp;
}

int yagl_vector_size(struct yagl_vector *v)
{
    return v->size;
}

int yagl_vector_capacity(struct yagl_vector *v)
{
    return v->capacity;
}

void yagl_vector_push_back(struct yagl_vector *v, const void *elem)
{
    if (v->size >= v->capacity) {
        void *tmp;

        v->capacity = (v->size * 3) / 2;

        tmp = g_malloc(v->capacity * v->elem_size);
        memcpy(tmp, v->data, (v->size * v->elem_size));

        g_free(v->data);

        v->data = tmp;
    }

    memcpy((char*)v->data + (v->size * v->elem_size), elem, v->elem_size);

    ++v->size;
}

void yagl_vector_resize(struct yagl_vector *v, int new_size)
{
    if (new_size <= v->capacity) {
        v->size = new_size;
    } else {
        void *tmp;

        v->capacity = (new_size * 3) / 2;

        tmp = g_malloc(v->capacity * v->elem_size);
        memcpy(tmp, v->data, (v->size * v->elem_size));

        g_free(v->data);

        v->data = tmp;
        v->size = new_size;
    }
}

void *yagl_vector_data(struct yagl_vector *v)
{
    return v->data;
}
