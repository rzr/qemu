#ifndef _QEMU_YAGL_TRANSPORT_H
#define _QEMU_YAGL_TRANSPORT_H

#include "yagl_types.h"
#include "yagl_vector.h"

#define YAGL_TRANSPORT_MAX_IN 8
#define YAGL_TRANSPORT_MAX_OUT 8

struct yagl_mem_transfer;

struct yagl_transport_in_array
{
    struct yagl_mem_transfer *mt;
    struct yagl_vector v;

    uint32_t page_index;
    uint8_t *ptr;

    int32_t el_size;
    int32_t *count;
};

struct yagl_transport
{
    /*
     * per-batch.
     * @{
     */

    uint8_t **pages;
    uint32_t offset;

    uint32_t page_index;
    uint8_t *next_page;

    uint8_t *ptr;

    /*
     * @}
     */

    /*
     * per-call.
     * @{
     */

    bool direct;

    uint32_t *res;

    uint32_t num_out_arrays;
    uint32_t num_in_arrays;

    struct yagl_vector out_arrays[YAGL_TRANSPORT_MAX_OUT];
    struct yagl_transport_in_array in_arrays[YAGL_TRANSPORT_MAX_IN];

    /*
     * @}
     */
};

struct yagl_transport *yagl_transport_create(void);

void yagl_transport_destroy(struct yagl_transport *t);

void yagl_transport_begin(struct yagl_transport *t,
                          uint8_t **pages,
                          uint32_t offset);

void yagl_transport_begin_call(struct yagl_transport *t,
                               yagl_api_id *api_id,
                               yagl_func_id *func_id);

void yagl_transport_end_call(struct yagl_transport *t);

uint32_t yagl_transport_bytes_processed(struct yagl_transport *t);

bool yagl_transport_get_out_array(struct yagl_transport *t,
                                  int32_t el_size,
                                  const void **data,
                                  int32_t *count);

bool yagl_transport_get_in_array(struct yagl_transport *t,
                                 int32_t el_size,
                                 void **data,
                                 int32_t *maxcount,
                                 int32_t **count);

const char **yagl_transport_get_out_string_array(const char *data,
                                                 int32_t data_count,
                                                 int32_t *array_count);

static __inline void yagl_transport_advance(struct yagl_transport *t,
                                            uint32_t size)
{
    t->ptr += size;

    if (t->ptr >= t->next_page) {
        uint32_t offset = t->ptr - t->next_page;

        t->page_index += 1 + (offset / TARGET_PAGE_SIZE);
        t->ptr = t->pages[t->page_index] + (offset & ~TARGET_PAGE_MASK);
        t->next_page = t->pages[t->page_index] + TARGET_PAGE_SIZE;
    }
}

static __inline uint8_t yagl_transport_get_out_uint8_t(struct yagl_transport *t)
{
    uint8_t tmp = *t->ptr;
    yagl_transport_advance(t, 8);
    return tmp;
}

static __inline uint32_t yagl_transport_get_out_uint32_t(struct yagl_transport *t)
{
    uint32_t tmp = *(uint32_t*)t->ptr;
    yagl_transport_advance(t, 8);
    return tmp;
}

static __inline float yagl_transport_get_out_float(struct yagl_transport *t)
{
    float tmp = *(float*)t->ptr;
    yagl_transport_advance(t, 8);
    return tmp;
}

static __inline void yagl_transport_get_in_arg(struct yagl_transport *t,
                                               void **value)
{
    target_ulong va = (target_ulong)yagl_transport_get_out_uint32_t(t);

    if (va) {
        *value = t->ptr;
        yagl_transport_advance(t, 8);
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
