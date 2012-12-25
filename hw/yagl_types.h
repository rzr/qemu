#ifndef _QEMU_YAGL_TYPES_H
#define _QEMU_YAGL_TYPES_H

#include "qemu-common.h"

typedef uint32_t yagl_pid;
typedef uint32_t yagl_tid;
typedef uint32_t yagl_func_id;
typedef uint32_t yagl_host_handle;
typedef uint32_t yagl_object_name;
typedef uint32_t yagl_winsys_id;

/*
 * YaGL supported render types.
 */
typedef enum
{
    yagl_render_type_offscreen = 1,
    yagl_render_type_onscreen = 2,
} yagl_render_type;

/*
 * YaGL supported API ids.
 */
typedef enum
{
    yagl_api_id_egl = 1,
    yagl_api_id_gles1 = 2,
    yagl_api_id_gles2 = 3,
} yagl_api_id;

#define YAGL_NUM_APIS 3

/*
 * EGL client APIs.
 */
typedef enum
{
    yagl_client_api_ogl = 0,
    yagl_client_api_gles1 = 1,
    yagl_client_api_gles2 = 2,
    yagl_client_api_ovg = 3
} yagl_client_api;

#define YAGL_NUM_CLIENT_APIS 4

typedef bool (*yagl_api_func)(uint8_t **/*out_buff*/,
                              uint8_t */*in_buff*/);

static inline float yagl_fixed_to_float(int32_t x)
{
    return (float)x * (1.0f / 65536.0f);
}

static inline double yagl_fixed_to_double(int32_t x)
{
    return (double)x * (1.0f / 65536.0f);
}

static inline int32_t yagl_fixed_to_int(int32_t x)
{
    return (x + 0x0800) >> 16;
}

static inline int32_t yagl_float_to_fixed(float f)
{
    return (int32_t)(f * 65536.0f + 0.5f);
}

static inline int32_t yagl_double_to_fixed(double d)
{
    return (int32_t)(d * 65536.0f + 0.5f);
}

static inline int32_t yagl_int_to_fixed(int32_t i)
{
    return i << 16;
}

#endif
