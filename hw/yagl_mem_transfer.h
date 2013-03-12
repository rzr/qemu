#ifndef _QEMU_YAGL_MEM_TRANSFER_H
#define _QEMU_YAGL_MEM_TRANSFER_H

#include "yagl_types.h"

struct yagl_mem_transfer
{
    struct yagl_thread_state *ts;

    target_phys_addr_t *pages;
    int max_pages;

    target_ulong va;
    int offset;
    int len;
    int num_pages;
};

/*
 * Guaranteed to succeed.
 */
struct yagl_mem_transfer
    *yagl_mem_transfer_create(struct yagl_thread_state *ts);

void yagl_mem_transfer_destroy(struct yagl_mem_transfer *mt);

bool yagl_mem_prepare(struct yagl_mem_transfer *mt,
                      target_ulong va,
                      int len);

static __inline bool yagl_mem_prepare_uint8(struct yagl_mem_transfer *mt,
                                            target_ulong va)
{
    return yagl_mem_prepare(mt, va, sizeof(uint8_t));
}

static __inline bool yagl_mem_prepare_uint16(struct yagl_mem_transfer *mt,
                                             target_ulong va)
{
    return yagl_mem_prepare(mt, va, sizeof(uint16_t));
}

static __inline bool yagl_mem_prepare_uint32(struct yagl_mem_transfer *mt,
                                             target_ulong va)
{
    return yagl_mem_prepare(mt, va, sizeof(uint32_t));
}

static __inline bool yagl_mem_prepare_float(struct yagl_mem_transfer *mt,
                                            target_ulong va)
{
    return yagl_mem_prepare(mt, va, sizeof(float));
}

void yagl_mem_put(struct yagl_mem_transfer *mt, const void *data);

void yagl_mem_put_uint8(struct yagl_mem_transfer *mt, uint8_t value);

void yagl_mem_put_uint16(struct yagl_mem_transfer *mt, uint16_t value);

void yagl_mem_put_uint32(struct yagl_mem_transfer *mt, uint32_t value);

void yagl_mem_put_float(struct yagl_mem_transfer *mt, float value);

#endif
