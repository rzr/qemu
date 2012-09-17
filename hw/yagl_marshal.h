#ifndef _QEMU_YAGL_MARSHAL_H
#define _QEMU_YAGL_MARSHAL_H

#include "yagl_types.h"
#include "exec-memory.h"

/*
 * All marshalling/unmarshalling must be done with 8-byte alignment,
 * since this is the maximum alignment possible. This way we can
 * just do assignments without "memcpy" calls and can be sure that
 * the code won't fail on architectures that don't support unaligned
 * memory access.
 */

/*
 * Each marshalled value is aligned
 * at 8-byte boundary and there may be maximum
 * 2 values returned (status and return value)
 */
#define YAGL_MARSHAL_MAX_RESPONSE (8 * 2)

/*
 * Max marshal buffer size.
 */
#define YAGL_MARSHAL_SIZE 0x8000

static __inline int yagl_marshal_skip(uint8_t** buff)
{
    *buff += 8;
    return 0;
}

static __inline void yagl_marshal_put_uint8(uint8_t** buff, uint8_t value)
{
    **buff = value;
    *buff += 8;
}

static __inline uint8_t yagl_marshal_get_uint8(uint8_t** buff)
{
    uint8_t tmp = **buff;
    *buff += 8;
    return tmp;
}

static __inline void yagl_marshal_put_uint32(uint8_t** buff, uint32_t value)
{
    *(uint32_t*)(*buff) = cpu_to_le32(value);
    *buff += 8;
}

static __inline uint32_t yagl_marshal_get_uint32(uint8_t** buff)
{
    uint32_t tmp = le32_to_cpu(*(uint32_t*)*buff);
    *buff += 8;
    return tmp;
}

static __inline void yagl_marshal_put_float(uint8_t** buff, float value)
{
    *(float*)(*buff) = value;
    *buff += 8;
}

static __inline float yagl_marshal_get_float(uint8_t** buff)
{
    float tmp = *(float*)*buff;
    *buff += 8;
    return tmp;
}

static __inline target_ulong yagl_marshal_get_ptr(uint8_t** buff)
{
#if TARGET_LONG_SIZE == 4
    target_ulong tmp = le32_to_cpu(*(uint32_t*)*buff);
#else
    target_ulong tmp = le64_to_cpu(*(uint64_t*)*buff);
#endif
    *buff += 8;
    return tmp;
}

static __inline void yagl_marshal_put_host_handle(uint8_t** buff, yagl_host_handle value)
{
    *(uint32_t*)(*buff) = cpu_to_le32(value);
    *buff += 8;
}

static __inline yagl_host_handle yagl_marshal_get_host_handle(uint8_t** buff)
{
    yagl_host_handle tmp = le32_to_cpu(*(uint32_t*)*buff);
    *buff += 8;
    return tmp;
}

#define yagl_marshal_put_int8(buff, value) yagl_marshal_put_uint8(buff, (uint8_t)(value))
#define yagl_marshal_get_int8(buff) ((int8_t)yagl_marshal_get_uint8(buff))
#define yagl_marshal_put_int32(buff, value) yagl_marshal_put_uint32(buff, (uint32_t)(value))
#define yagl_marshal_get_int32(buff) ((int32_t)yagl_marshal_get_uint32(buff))
#define yagl_marshal_put_uint32_t(buff, value) yagl_marshal_put_uint32(buff, value)
#define yagl_marshal_get_uint32_t(buff) yagl_marshal_get_uint32(buff)
#define yagl_marshal_put_int(buff, value) yagl_marshal_put_int32(buff, (value))
#define yagl_marshal_get_int(buff) yagl_marshal_get_int32(buff)
#define yagl_marshal_get_pid(buff) yagl_marshal_get_uint32(buff)
#define yagl_marshal_get_tid(buff) yagl_marshal_get_uint32(buff)
#define yagl_marshal_get_api_id(buff) yagl_marshal_get_uint32(buff)
#define yagl_marshal_get_func_id(buff) yagl_marshal_get_uint32(buff)

#endif
