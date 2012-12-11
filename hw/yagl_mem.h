#ifndef _QEMU_YAGL_MEM_H
#define _QEMU_YAGL_MEM_H

#include "yagl_types.h"
#include "yagl_mem_transfer.h"

bool yagl_mem_get_uint8(target_ulong va, uint8_t* value);

bool yagl_mem_get_uint16(target_ulong va, uint16_t* value);

bool yagl_mem_get_uint32(target_ulong va, uint32_t* value);

bool yagl_mem_get_float(target_ulong va, float* value);

bool yagl_mem_get(target_ulong va, uint32_t len, void* data);

typedef bool (*yagl_mem_array_cb)(const void */*el*/, void */*user_data*/);

/*
 * Efficiently reads an array from target memory. It'll continue reading until
 * 'cb' returns false. Note that 'el_size' must be <= TARGET_PAGE_SIZE
 * in order for this function to work.
 */
bool yagl_mem_get_array(target_ulong va,
                        target_ulong el_size,
                        yagl_mem_array_cb cb,
                        void *user_data);

char *yagl_mem_get_string(target_ulong va);

#define yagl_mem_prepare_char(mt, va) yagl_mem_prepare_uint8((mt), (va))
#define yagl_mem_put_char(mt, value) yagl_mem_put_uint8((mt), (uint8_t)(value))
#define yagl_mem_get_char(va, value) yagl_mem_get_uint8((va), (uint8_t*)(value))
#define yagl_mem_prepare_int8(mt, va) yagl_mem_prepare_uint8((mt), (va))
#define yagl_mem_put_int8(mt, value) yagl_mem_put_uint8((mt), (uint8_t)(value))
#define yagl_mem_get_int8(va, value) yagl_mem_get_uint8((va), (uint8_t*)(value))
#define yagl_mem_prepare_int16(mt, va) yagl_mem_prepare_uint16((mt), (va))
#define yagl_mem_put_int16(mt, value) yagl_mem_put_uint16((mt), (uint16_t)(value))
#define yagl_mem_get_int16(va, value) yagl_mem_get_uint16((va), (uint16_t*)(value))
#define yagl_mem_prepare_int32(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_int32(mt, value) yagl_mem_put_uint32((mt), (uint32_t)(value))
#define yagl_mem_get_int32(va, value) yagl_mem_get_uint32((va), (uint32_t*)(value))
#define yagl_mem_prepare_host_handle(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_host_handle(mt, value) yagl_mem_put_uint32((mt), (value))
#define yagl_mem_get_host_handle(va, value) yagl_mem_get_uint32((va), (value))

#if TARGET_LONG_SIZE == 4
#define yagl_mem_prepare_ptr(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_ptr(mt, value) yagl_mem_put_uint32((mt), (value))
#else
#error 64-bit ptr not supported
#endif

#endif
