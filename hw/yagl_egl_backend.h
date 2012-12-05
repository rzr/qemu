#ifndef _QEMU_YAGL_EGL_BACKEND_H
#define _QEMU_YAGL_EGL_BACKEND_H

#include "yagl_types.h"

struct yagl_thread_state;
struct yagl_process_state;
struct yagl_eglb_display;
struct yagl_eglb_context;
struct yagl_eglb_surface;

/*
 * YaGL EGL backend per-process state.
 * @{
 */

struct yagl_egl_backend_ps
{
    struct yagl_egl_backend *backend;

    struct yagl_process_state *ps;

    void (*thread_init)(struct yagl_egl_backend_ps */*backend_ps*/,
                        struct yagl_thread_state */*ts*/);

    struct yagl_eglb_display *(*create_display)(struct yagl_egl_backend_ps */*backend_ps*/);

    bool (*make_current)(struct yagl_egl_backend_ps */*backend_ps*/,
                         struct yagl_eglb_display */*dpy*/,
                         struct yagl_eglb_context */*ctx*/,
                         struct yagl_eglb_surface */*draw*/,
                         struct yagl_eglb_surface */*read*/);

    bool (*release_current)(struct yagl_egl_backend_ps */*backend_ps*/,
                            bool /*force*/);

    void (*wait_native)(struct yagl_egl_backend_ps */*backend_ps*/);

    void (*thread_fini)(struct yagl_egl_backend_ps */*backend_ps*/);

    void (*destroy)(struct yagl_egl_backend_ps */*backend_ps*/);
};

void yagl_egl_backend_ps_init(struct yagl_egl_backend_ps *backend_ps,
                              struct yagl_egl_backend *backend,
                              struct yagl_process_state *ps);
void yagl_egl_backend_ps_cleanup(struct yagl_egl_backend_ps *backend_ps);

/*
 * @}
 */

/*
 * YaGL EGL backend.
 * @{
 */

struct yagl_egl_backend
{
    struct yagl_egl_backend_ps *(*process_init)(struct yagl_egl_backend */*backend*/,
                                                struct yagl_process_state */*ps*/);

    void (*destroy)(struct yagl_egl_backend */*backend*/);
};

void yagl_egl_backend_init(struct yagl_egl_backend *backend);
void yagl_egl_backend_cleanup(struct yagl_egl_backend *backend);

/*
 * @}
 */

#endif
