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

#include "yagl_transport.h"
#include "yagl_compiled_transfer.h"
#include "yagl_mem.h"
#include "yagl_log.h"
#include "yagl_thread.h"
#include "yagl_process.h"

typedef enum
{
    yagl_call_result_ok = 0xA,    /* Call is ok. */
    yagl_call_result_retry = 0xB, /* Page fault on host, retry is required. */
} yagl_call_result;

#define YAGL_TRANSPORT_BATCH_HEADER_SIZE (4 * 8)

static __inline uint32_t yagl_transport_uint32_t_at(struct yagl_transport *t,
                                                    uint32_t offset)
{
    return *(uint32_t*)(t->pages[offset / TARGET_PAGE_SIZE] +
                        (offset & ~TARGET_PAGE_MASK));
}

static __inline target_ulong yagl_transport_va_at(struct yagl_transport *t,
                                                  uint32_t offset)
{
    return *(target_ulong*)(t->pages[offset / TARGET_PAGE_SIZE] +
                            (offset & ~TARGET_PAGE_MASK));
}

static __inline void yagl_transport_uint32_t_to(struct yagl_transport *t,
                                                uint32_t offset,
                                                uint32_t value)
{
    *(uint32_t*)(t->pages[offset / TARGET_PAGE_SIZE] +
                 (offset & ~TARGET_PAGE_MASK)) = value;
}

static void yagl_transport_copy_from(struct yagl_transport *t,
                                     uint32_t offset,
                                     uint8_t *data,
                                     uint32_t size)
{
    uint32_t page_index = offset / TARGET_PAGE_SIZE;
    uint8_t *ptr = t->pages[page_index] + (offset & ~TARGET_PAGE_MASK);

    while (size > 0) {
        uint32_t rem = t->pages[page_index] + TARGET_PAGE_SIZE - ptr;
        rem = MIN(rem, size);

        memcpy(data, ptr, rem);

        size -= rem;
        data += rem;

        ++page_index;
        ptr = t->pages[page_index];
    }
}

static void yagl_transport_copy_to(struct yagl_transport *t,
                                   uint32_t offset,
                                   const uint8_t *data,
                                   uint32_t size)
{
    uint32_t page_index = offset / TARGET_PAGE_SIZE;
    uint8_t *ptr = t->pages[page_index] + (offset & ~TARGET_PAGE_MASK);

    while (size > 0) {
        uint32_t rem = t->pages[page_index] + TARGET_PAGE_SIZE - ptr;
        rem = MIN(rem, size);

        memcpy(ptr, data, rem);

        size -= rem;
        data += rem;

        ++page_index;
        ptr = t->pages[page_index];
    }
}

struct yagl_transport *yagl_transport_create(void)
{
    struct yagl_transport *t;
    uint32_t i;

    t = g_malloc0(sizeof(*t));

    for (i = 0; i < YAGL_TRANSPORT_MAX_IN; ++i) {
        yagl_vector_init(&t->in_arrays[i].v, 1, 0);
    }

    QLIST_INIT(&t->compiled_transfers);

    return t;
}

void yagl_transport_destroy(struct yagl_transport *t)
{
    uint32_t i;

    for (i = 0; i < YAGL_TRANSPORT_MAX_IN; ++i) {
        yagl_vector_cleanup(&t->in_arrays[i].v);
    }

    g_free(t);
}

void yagl_transport_set_buffer(struct yagl_transport *t, uint8_t **pages)
{
    t->pages = pages;
}

uint8_t *yagl_transport_begin(struct yagl_transport *t,
                              uint32_t header_size,
                              uint32_t *batch_size,
                              uint32_t *out_arrays_size,
                              uint32_t *fence_seq)
{
    uint32_t num_out_da, i;
    uint8_t *batch_data, *tmp;

    *fence_seq = yagl_transport_uint32_t_at(t, 1 * 8);
    *batch_size = yagl_transport_uint32_t_at(t, 2 * 8);
    num_out_da = yagl_transport_uint32_t_at(t, 3 * 8);

    *out_arrays_size = 0;

    for (i = 0; i < num_out_da; ++i) {
        *out_arrays_size += yagl_transport_uint32_t_at(t,
            YAGL_TRANSPORT_BATCH_HEADER_SIZE + *batch_size + ((2 * i + 1) * 8));
    }

    batch_data = tmp = g_malloc(
        header_size + YAGL_TRANSPORT_BATCH_HEADER_SIZE +
        *batch_size + *out_arrays_size);

    tmp += header_size + YAGL_TRANSPORT_BATCH_HEADER_SIZE + *batch_size;

    for (i = 0; i < num_out_da; ++i) {
        target_ulong va = yagl_transport_va_at(t,
            YAGL_TRANSPORT_BATCH_HEADER_SIZE + *batch_size + ((2 * i + 0) * 8));
        uint32_t size = yagl_transport_uint32_t_at(t,
            YAGL_TRANSPORT_BATCH_HEADER_SIZE + *batch_size + ((2 * i + 1) * 8));

        if (!yagl_mem_get(va, size, tmp)) {
            yagl_transport_uint32_t_to(t, 0, yagl_call_result_retry);
            g_free(batch_data);
            return NULL;
        }

        tmp += size;
    }

    yagl_transport_copy_from(t,
        0,
        batch_data + header_size,
        YAGL_TRANSPORT_BATCH_HEADER_SIZE + *batch_size);

    yagl_transport_uint32_t_to(t, 0, yagl_call_result_ok);

    return batch_data;
}

void yagl_transport_end(struct yagl_transport *t)
{
    uint32_t i;
    struct yagl_compiled_transfer *ct, *tmp;

    for (i = 0; i < t->num_in_arrays; ++i) {
        struct yagl_transport_in_array *in_array = &t->in_arrays[i];

        if ((*in_array->count > 0) && t->direct) {
            if (!yagl_mem_put(in_array->va,
                              yagl_vector_data(&in_array->v),
                              *in_array->count * in_array->el_size)) {
                yagl_transport_uint32_t_to(t, 0, yagl_call_result_retry);
                return;
            }
        }
    }

    QLIST_FOREACH_SAFE(ct, &t->compiled_transfers, entry, tmp) {
        yagl_compiled_transfer_prepare(ct);
    }

    t->direct = false;
    t->num_in_arrays = 0;

    yagl_transport_uint32_t_to(t, 0, yagl_call_result_ok);
}

void yagl_transport_reset(struct yagl_transport *t,
                          uint8_t *batch_data,
                          uint32_t batch_size,
                          uint32_t out_arrays_size)
{
    t->batch_data = batch_data;
    t->batch_size = batch_size;
    t->out_arrays_size = out_arrays_size;

    t->ptr = batch_data + YAGL_TRANSPORT_BATCH_HEADER_SIZE;
    t->out_array_ptr = batch_data + YAGL_TRANSPORT_BATCH_HEADER_SIZE + batch_size;
}

bool yagl_transport_begin_call(struct yagl_transport *t,
                               yagl_api_id *api_id,
                               yagl_func_id *func_id)
{
    if (t->ptr >= (t->batch_data + t->batch_size)) {
        return false;
    }

    *api_id = yagl_transport_get_out_uint32_t(t);
    *func_id = yagl_transport_get_out_uint32_t(t);
    t->direct = yagl_transport_get_out_uint32_t(t);
    t->num_in_arrays = 0;

    return true;
}

void yagl_transport_end_call(struct yagl_transport *t)
{
    uint32_t i;

    for (i = 0; i < t->num_in_arrays; ++i) {
        struct yagl_transport_in_array *in_array = &t->in_arrays[i];

        if ((*in_array->count > 0) && !t->direct) {
            yagl_transport_copy_to(t,
                                   in_array->offset,
                                   t->batch_data + in_array->offset,
                                   *in_array->count * in_array->el_size);
        }
    }
}

void yagl_transport_get_out_array(struct yagl_transport *t,
                                  int32_t el_size,
                                  const void **data,
                                  int32_t *count)
{
    target_ulong va = (target_ulong)yagl_transport_get_out_uint32_t(t);
    uint32_t size;

    *count = yagl_transport_get_out_uint32_t(t);

    if (!va) {
        *data = NULL;
        return;
    }

    size = (*count > 0) ? (*count * el_size) : 0;

    if (t->direct) {
        *data = t->out_array_ptr;
        t->out_array_ptr += size;
    } else {
        *data = t->ptr;
        t->ptr += (size + 7U) & ~7U;
    }
}

void yagl_transport_get_in_array(struct yagl_transport *t,
                                 int32_t el_size,
                                 void **data,
                                 int32_t *maxcount,
                                 int32_t **count)
{
    target_ulong va = (target_ulong)yagl_transport_get_out_uint32_t(t);
    uint32_t size;
    struct yagl_transport_in_array *in_array;
    uint32_t offset;

    offset = t->ptr - t->batch_data;

    *count = (int32_t*)(t->pages[offset / TARGET_PAGE_SIZE] +
                        (offset & ~TARGET_PAGE_MASK));
    *maxcount = yagl_transport_get_out_uint32_t(t);

    if (!va) {
        *data = NULL;
        return;
    }

    size = (*maxcount > 0) ? (*maxcount * el_size) : 0;

    in_array = &t->in_arrays[t->num_in_arrays];

    if (t->direct) {
        yagl_vector_resize(&in_array->v, size);

        in_array->va = va;
        in_array->el_size = el_size;
        in_array->count = *count;

        ++t->num_in_arrays;

        *data = yagl_vector_data(&in_array->v);
    } else {
        in_array->offset = t->ptr - t->batch_data;
        in_array->el_size = el_size;
        in_array->count = *count;

        ++t->num_in_arrays;

        *data = t->ptr;

        t->ptr += (size + 7U) & ~7U;
    }
}

const char **yagl_transport_get_out_string_array(const char *data,
                                                 int32_t data_count,
                                                 int32_t *array_count)
{
    struct yagl_vector v;
    char *tmp;

    YAGL_LOG_FUNC_SET(yagl_transport_get_out_string_array);

    if (!data) {
        *array_count = 0;
        return NULL;
    }

    yagl_vector_init(&v, sizeof(char*), 0);

    while (data_count > 0) {
        tmp = memchr(data, '\0', data_count);

        if (!tmp) {
            YAGL_LOG_ERROR("0 not found in string array");
            break;
        }

        yagl_vector_push_back(&v, &data);

        data_count -= tmp - data + 1;

        data = tmp + 1;
    }

    *array_count = yagl_vector_size(&v);

    return (const char**)yagl_vector_detach(&v);
}
