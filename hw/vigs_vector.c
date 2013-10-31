/*
 * vigs
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

#include "vigs_vector.h"

#define VIGS_VECTOR_DEFAULT_CAPACITY 100

void vigs_vector_init(struct vigs_vector *v,
                      uint32_t initial_capacity)
{
    v->capacity = (initial_capacity < VIGS_VECTOR_DEFAULT_CAPACITY) ?
                      VIGS_VECTOR_DEFAULT_CAPACITY
                    : initial_capacity;
    v->data = g_malloc0(v->capacity);
    v->size = 0;
}

void vigs_vector_cleanup(struct vigs_vector *v)
{
    g_free(v->data);
    memset(v, 0, sizeof(*v));
}

uint32_t vigs_vector_size(struct vigs_vector *v)
{
    return v->size;
}

uint32_t vigs_vector_capacity(struct vigs_vector *v)
{
    return v->capacity;
}

void vigs_vector_resize(struct vigs_vector *v, uint32_t new_size)
{
    if (new_size <= v->capacity) {
        v->size = new_size;
    } else {
        void *tmp;

        v->capacity = (new_size * 3) / 2;

        tmp = g_malloc(v->capacity);
        memcpy(tmp, v->data, v->size);

        g_free(v->data);

        v->data = tmp;
        v->size = new_size;
    }
}

void *vigs_vector_append(struct vigs_vector *v, uint32_t append_size)
{
    uint32_t old_size = v->size;

    vigs_vector_resize(v, old_size + append_size);

    return v->data + old_size;
}

void *vigs_vector_data(struct vigs_vector *v)
{
    return v->data;
}
