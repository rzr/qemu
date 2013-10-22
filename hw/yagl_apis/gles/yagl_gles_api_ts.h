#ifndef _QEMU_YAGL_GLES_API_TS_H
#define _QEMU_YAGL_GLES_API_TS_H

#include "yagl_types.h"

struct yagl_gles_driver;
struct yagl_gles_api_ps;

struct yagl_gles_api_ts
{
    struct yagl_gles_driver *driver;

    struct yagl_gles_api_ps *ps;

    GLuint *arrays;
    uint32_t num_arrays;
};

void yagl_gles_api_ts_init(struct yagl_gles_api_ts *gles_api_ts,
                           struct yagl_gles_driver *driver,
                           struct yagl_gles_api_ps *ps);

void yagl_gles_api_ts_cleanup(struct yagl_gles_api_ts *gles_api_ts);

#endif
