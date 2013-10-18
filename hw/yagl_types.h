#ifndef _QEMU_YAGL_TYPES_H
#define _QEMU_YAGL_TYPES_H

#include "qemu-common.h"

typedef uint32_t yagl_pid;
typedef uint32_t yagl_tid;
typedef uint32_t yagl_func_id;
typedef uint32_t yagl_host_handle;
typedef uint32_t yagl_object_name;
typedef uint32_t yagl_winsys_id;

struct yagl_transport;

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
    yagl_api_id_gles = 2
} yagl_api_id;

#define YAGL_NUM_APIS 2

typedef bool (*yagl_api_func)(struct yagl_transport */*t*/);

#endif
