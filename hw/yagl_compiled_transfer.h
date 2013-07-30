#ifndef _QEMU_YAGL_COMPILED_TRANSFER_H
#define _QEMU_YAGL_COMPILED_TRANSFER_H

#include "yagl_types.h"

struct yagl_compiled_transfer_section
{
    /*
     * For 'cpy_physical_memory_unmap'.
     */
    void *map_base;
    target_phys_addr_t map_len;

    /*
     * For actual I/O.
     */
    void *base;
    uint32_t len;
};

struct yagl_compiled_transfer
{
    struct yagl_compiled_transfer_section *sections;
    int num_sections;
    bool is_write;
};

struct yagl_compiled_transfer
    *yagl_compiled_transfer_create(struct yagl_thread_state *ts,
                                   target_ulong va,
                                   uint32_t len,
                                   bool is_write);

void yagl_compiled_transfer_destroy(struct yagl_compiled_transfer *ct);

void yagl_compiled_transfer_exec(struct yagl_compiled_transfer *ct, void* data);

#endif
