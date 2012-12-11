#include "yagl_eglb_display.h"

void yagl_eglb_display_init(struct yagl_eglb_display *dpy,
                            struct yagl_egl_backend *backend)
{
    dpy->backend = backend;
}

void yagl_eglb_display_cleanup(struct yagl_eglb_display *dpy)
{
}
