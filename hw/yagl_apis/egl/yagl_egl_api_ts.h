#ifndef _QEMU_YAGL_EGL_API_TS_H
#define _QEMU_YAGL_EGL_API_TS_H

#include "yagl_api.h"
#include <EGL/egl.h>

struct yagl_egl_backend;
struct yagl_egl_context;

struct yagl_egl_api_ts
{
    struct yagl_egl_api_ps *api_ps;

    /*
     * From 'api_ps->backend' for speed.
     */
    struct yagl_egl_backend *backend;

    EGLenum api;

    /*
     * This is GLES or OpenGL context, only one GLES or OpenGL context
     * can be current to a thread at the same time.
     */
    struct yagl_egl_context *context;
};

void yagl_egl_api_ts_init(struct yagl_egl_api_ts *egl_api_ts,
                          struct yagl_egl_api_ps *api_ps);

void yagl_egl_api_ts_cleanup(struct yagl_egl_api_ts *egl_api_ts);

void yagl_egl_api_ts_update_context(struct yagl_egl_api_ts *egl_api_ts,
                                    struct yagl_egl_context *ctx);

void yagl_egl_api_ts_reset(struct yagl_egl_api_ts *egl_api_ts);

#endif
