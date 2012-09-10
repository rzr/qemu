#ifndef _QEMU_YAGL_TYPES_H
#define _QEMU_YAGL_TYPES_H

#include "qemu-common.h"

typedef uint32_t yagl_pid;
typedef uint32_t yagl_tid;
typedef uint32_t yagl_func_id;
typedef uint32_t yagl_host_handle;
typedef uint32_t yagl_object_name;

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

struct yagl_thread_state;

typedef void (*yagl_api_func)(struct yagl_thread_state */*ts*/,
                              uint8_t */*out_buff*/,
                              uint8_t */*in_buff*/);

#endif
