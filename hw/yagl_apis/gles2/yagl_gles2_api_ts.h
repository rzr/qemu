#ifndef _QEMU_YAGL_GLES2_API_TS_H
#define _QEMU_YAGL_GLES2_API_TS_H

#include "yagl_types.h"

struct yagl_thread_state;
struct yagl_gles2_driver_ps;
struct yagl_egl_interface;

struct yagl_gles2_api_ts
{
    struct yagl_thread_state *ts;

    struct yagl_gles2_driver_ps *driver_ps;

    /*
     * From 'ts->ps->egl_iface' for speed.
     */
    struct yagl_egl_interface *egl_iface;
};

/*
 * Does NOT take ownership of anything.
 */
void yagl_gles2_api_ts_init(struct yagl_gles2_api_ts *gles2_api_ts,
                            struct yagl_thread_state *ts,
                            struct yagl_gles2_driver_ps *driver_ps);

void yagl_gles2_api_ts_cleanup(struct yagl_gles2_api_ts *gles2_api_ts);

#endif
