#ifndef _QEMU_YAGL_MEM_H
#define _QEMU_YAGL_MEM_H

#include "yagl_types.h"

struct yagl_mem_transfer
{
    hwaddr *pages;
    int max_pages;

    target_ulong va;
    int offset;
    int len;
    int num_pages;
};

/*
 * Guaranteed to succeed.
 */
struct yagl_mem_transfer *yagl_mem_transfer_create(void);

void yagl_mem_transfer_destroy(struct yagl_mem_transfer *mt);

bool yagl_mem_prepare(struct yagl_mem_transfer *mt,
                      target_ulong va,
                      int len);

void yagl_mem_put(struct yagl_mem_transfer *mt, const void *data, int len);

bool yagl_mem_get(target_ulong va, uint32_t len, void *data);

#endif
