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

#ifndef _QEMU_YAGL_TRANSPORT_H
#define _QEMU_YAGL_TRANSPORT_H

#include "yagl_types.h"
#include "yagl_vector.h"
#include "qemu/queue.h"

#define YAGL_TRANSPORT_MAX_IN 8

struct yagl_compiled_transfer;

struct yagl_transport_in_array
{
    struct yagl_vector v;

    union
    {
        target_ulong va;
        uint32_t offset;
    };

    int32_t el_size;
    int32_t *count;
};

struct yagl_transport
{
    /*
     * per-transport.
     * @{
     */

    uint8_t **pages;

    /*
     * @}
     */

    /*
     * per-batch.
     * @{
     */

    uint8_t *batch_data;
    uint32_t batch_size;
    uint32_t out_arrays_size;

    uint8_t *ptr;
    uint8_t *out_array_ptr;

    /*
     * @}
     */

    /*
     * per-call.
     * @{
     */

    bool direct;

    uint32_t num_in_arrays;

    struct yagl_transport_in_array in_arrays[YAGL_TRANSPORT_MAX_IN];

    QLIST_HEAD(, yagl_compiled_transfer) compiled_transfers;

    /*
     * @}
     */
};

struct yagl_transport *yagl_transport_create(void);

void yagl_transport_destroy(struct yagl_transport *t);

void yagl_transport_set_buffer(struct yagl_transport *t, uint8_t **pages);

uint8_t *yagl_transport_begin(struct yagl_transport *t,
                              uint32_t header_size,
                              uint32_t *batch_size,
                              uint32_t *out_arrays_size,
                              uint32_t *fence_seq);

void yagl_transport_end(struct yagl_transport *t);

void yagl_transport_reset(struct yagl_transport *t,
                          uint8_t *batch_data,
                          uint32_t batch_size,
                          uint32_t out_arrays_size);

bool yagl_transport_begin_call(struct yagl_transport *t,
                               yagl_api_id *api_id,
                               yagl_func_id *func_id);

void yagl_transport_end_call(struct yagl_transport *t);

void yagl_transport_get_out_array(struct yagl_transport *t,
                                  int32_t el_size,
                                  const void **data,
                                  int32_t *count);

void yagl_transport_get_in_array(struct yagl_transport *t,
                                 int32_t el_size,
                                 void **data,
                                 int32_t *maxcount,
                                 int32_t **count);

static __inline uint8_t yagl_transport_get_out_uint8_t(struct yagl_transport *t)
{
    uint8_t tmp = *t->ptr;
    t->ptr += 8;
    return tmp;
}

static __inline uint32_t yagl_transport_get_out_uint32_t(struct yagl_transport *t)
{
    uint32_t tmp = *(uint32_t*)t->ptr;
    t->ptr += 8;
    return tmp;
}

static __inline float yagl_transport_get_out_float(struct yagl_transport *t)
{
    float tmp = *(float*)t->ptr;
    t->ptr += 8;
    return tmp;
}

static __inline void yagl_transport_get_in_arg(struct yagl_transport *t,
                                               void **value)
{
    target_ulong va = (target_ulong)yagl_transport_get_out_uint32_t(t);

    if (va) {
        uint32_t offset = t->ptr - t->batch_data;
        *value = t->pages[offset / TARGET_PAGE_SIZE] +
                 (offset & ~TARGET_PAGE_MASK);
        t->ptr += 8;
    } else {
        *value = NULL;
    }
}

static __inline yagl_host_handle yagl_transport_get_out_yagl_host_handle(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline yagl_winsys_id yagl_transport_get_out_yagl_winsys_id(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline target_ulong yagl_transport_get_out_va(struct yagl_transport *t)
{
    return (target_ulong)yagl_transport_get_out_uint32_t(t);
}

#endif
