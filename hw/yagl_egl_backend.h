#ifndef _QEMU_YAGL_EGL_BACKEND_H
#define _QEMU_YAGL_EGL_BACKEND_H

#include "yagl_types.h"

struct yagl_eglb_display;
struct yagl_eglb_context;
struct yagl_eglb_surface;

/*
 * YaGL EGL backend.
 * @{
 */

struct yagl_egl_backend
{
    yagl_render_type render_type;

    void (*thread_init)(struct yagl_egl_backend */*backend*/);

    struct yagl_eglb_display *(*create_display)(struct yagl_egl_backend */*backend*/);

    bool (*make_current)(struct yagl_egl_backend */*backend*/,
                         struct yagl_eglb_display */*dpy*/,
                         struct yagl_eglb_context */*ctx*/,
                         struct yagl_eglb_surface */*draw*/,
                         struct yagl_eglb_surface */*read*/);

    bool (*release_current)(struct yagl_egl_backend */*backend*/,
                            bool /*force*/);

    void (*thread_fini)(struct yagl_egl_backend */*backend*/);

    /*
     * Make sure that some GL context is currently active. Can
     * be called before/after thread_init/thread_fini.
     * @{
     */

    void (*ensure_current)(struct yagl_egl_backend */*backend*/);

    void (*unensure_current)(struct yagl_egl_backend */*backend*/);

    /*
     * @}
     */

    void (*destroy)(struct yagl_egl_backend */*backend*/);
};

void yagl_egl_backend_init(struct yagl_egl_backend *backend,
                           yagl_render_type render_type);
void yagl_egl_backend_cleanup(struct yagl_egl_backend *backend);

/*
 * @}
 */

#endif
