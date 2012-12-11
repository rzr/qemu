#ifndef _QEMU_YAGL_GLES2_API_TS_H
#define _QEMU_YAGL_GLES2_API_TS_H

#include "yagl_types.h"

struct yagl_gles2_driver;
struct yagl_egl_interface;

struct yagl_gles2_api_ts
{
    struct yagl_gles2_driver *driver;
};

/*
 * Does NOT take ownership of anything.
 */
void yagl_gles2_api_ts_init(struct yagl_gles2_api_ts *gles2_api_ts,
                            struct yagl_gles2_driver *driver);

void yagl_gles2_api_ts_cleanup(struct yagl_gles2_api_ts *gles2_api_ts);

#endif
