#include "yagl_transport.h"
#include "yagl_mem.h"

typedef enum
{
    yagl_call_result_ok = 0xA,    /* Call is ok. */
    yagl_call_result_retry = 0xB, /* Page fault on host, retry is required. */
} yagl_call_result;

static void yagl_transport_copy_from(struct yagl_transport *t,
                                     uint8_t *data,
                                     uint32_t size)
{
    while (size > 0) {
        uint32_t rem = t->next_page - t->ptr;
        rem = MIN(rem, size);

        memcpy(data, t->ptr, rem);

        size -= rem;
        data += rem;

        yagl_transport_advance(t, rem);
    }
}

static void yagl_transport_copy_to(struct yagl_transport *t,
                                   uint32_t page_index,
                                   uint8_t *ptr,
                                   uint8_t *data,
                                   uint32_t size)
{
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

    for (i = 0; i < YAGL_TRANSPORT_MAX_OUT; ++i) {
        yagl_vector_init(&t->out_arrays[i], 1, 0);
    }

    for (i = 0; i < YAGL_TRANSPORT_MAX_IN; ++i) {
        yagl_vector_init(&t->in_arrays[i].v, 1, 0);
        t->in_arrays[i].mt = yagl_mem_transfer_create();
    }

    return t;
}

void yagl_transport_destroy(struct yagl_transport *t)
{
    uint32_t i;

    for (i = 0; i < YAGL_TRANSPORT_MAX_OUT; ++i) {
        yagl_vector_cleanup(&t->out_arrays[i]);
    }

    for (i = 0; i < YAGL_TRANSPORT_MAX_IN; ++i) {
        yagl_vector_cleanup(&t->in_arrays[i].v);
        yagl_mem_transfer_destroy(t->in_arrays[i].mt);
    }

    g_free(t);
}

void yagl_transport_begin(struct yagl_transport *t,
                          uint8_t **pages,
                          uint32_t offset)
{
    t->pages = pages;
    t->offset = offset;
    t->page_index = offset / TARGET_PAGE_SIZE;
    t->ptr = t->pages[t->page_index] + (offset & ~TARGET_PAGE_MASK);
    t->next_page = t->pages[t->page_index] + TARGET_PAGE_SIZE;
}

void yagl_transport_begin_call(struct yagl_transport *t,
                               yagl_api_id *api_id,
                               yagl_func_id *func_id)
{
    *api_id = yagl_transport_get_out_uint32_t(t);

    if (!*api_id) {
        return;
    }

    *func_id = yagl_transport_get_out_uint32_t(t);

    t->res = (uint32_t*)t->ptr;
    t->direct = yagl_transport_get_out_uint32_t(t);
    t->num_out_arrays = t->num_in_arrays = 0;
}

void yagl_transport_end_call(struct yagl_transport *t)
{
    uint32_t i;

    for (i = 0; i < t->num_in_arrays; ++i) {
        struct yagl_transport_in_array *in_array = &t->in_arrays[i];

        if (*in_array->count > 0) {
            if (t->direct) {
                yagl_mem_put(in_array->mt,
                             yagl_vector_data(&in_array->v),
                             *in_array->count * in_array->el_size);
            } else {
                yagl_transport_copy_to(t,
                                       in_array->page_index,
                                       in_array->ptr,
                                       yagl_vector_data(&in_array->v),
                                       *in_array->count * in_array->el_size);
            }
        }
    }

    *t->res = yagl_call_result_ok;
}

uint32_t yagl_transport_bytes_processed(struct yagl_transport *t)
{
    uint32_t start_page_index = t->offset / TARGET_PAGE_SIZE;
    uint32_t start_page_offset = t->offset & ~TARGET_PAGE_MASK;
    uint32_t end_page_offset = t->ptr - t->pages[t->page_index];

    return (t->page_index - start_page_index) * TARGET_PAGE_SIZE +
           end_page_offset - start_page_offset;
}

bool yagl_transport_get_out_array(struct yagl_transport *t,
                                  int32_t el_size,
                                  const void **data,
                                  int32_t *count)
{
    target_ulong va = (target_ulong)yagl_transport_get_out_uint32_t(t);
    uint32_t size;
    struct yagl_vector *v;

    *count = yagl_transport_get_out_uint32_t(t);

    if (!va) {
        *data = NULL;
        return true;
    }

    size = (*count > 0) ? (*count * el_size) : 0;

    if (t->direct) {
        v = &t->out_arrays[t->num_out_arrays];

        yagl_vector_resize(v, size);

        if (!yagl_mem_get(va, size, yagl_vector_data(v))) {
            *t->res = yagl_call_result_retry;
            return false;
        }

        ++t->num_out_arrays;

        *data = yagl_vector_data(v);

        return true;
    }

    if ((t->ptr + size) <= t->next_page) {
        *data = t->ptr;

        yagl_transport_advance(t, (size + 7U) & ~7U);

        return true;
    }

    v = &t->out_arrays[t->num_out_arrays];

    yagl_vector_resize(v, size);

    yagl_transport_copy_from(t, yagl_vector_data(v), size);
    yagl_transport_advance(t, 7U - ((size + 7U) & 7U));

    ++t->num_out_arrays;

    *data = yagl_vector_data(v);

    return true;
}

bool yagl_transport_get_in_array(struct yagl_transport *t,
                                 int32_t el_size,
                                 void **data,
                                 int32_t *maxcount,
                                 int32_t **count)
{
    target_ulong va = (target_ulong)yagl_transport_get_out_uint32_t(t);
    uint32_t size;
    struct yagl_transport_in_array *in_array;

    *count = (int32_t*)t->ptr;
    *maxcount = yagl_transport_get_out_uint32_t(t);

    if (!va) {
        *data = NULL;
        return true;
    }

    size = (*maxcount > 0) ? (*maxcount * el_size) : 0;

    if (t->direct) {
        in_array = &t->in_arrays[t->num_in_arrays];

        if (!yagl_mem_prepare(in_array->mt, va, size)) {
            *t->res = yagl_call_result_retry;
            return false;
        }

        yagl_vector_resize(&in_array->v, size);

        in_array->el_size = el_size;
        in_array->count = *count;

        ++t->num_in_arrays;

        *data = yagl_vector_data(&in_array->v);

        return true;
    }

    if ((t->ptr + size) <= t->next_page) {
        *data = t->ptr;

        yagl_transport_advance(t, (size + 7U) & ~7U);

        return true;
    }

    in_array = &t->in_arrays[t->num_in_arrays];

    in_array->page_index = t->page_index;
    in_array->ptr = t->ptr;

    yagl_vector_resize(&in_array->v, size);

    in_array->el_size = el_size;
    in_array->count = *count;

    yagl_transport_advance(t, (size + 7U) & ~7U);

    ++t->num_in_arrays;

    *data = yagl_vector_data(&in_array->v);

    return true;
}
