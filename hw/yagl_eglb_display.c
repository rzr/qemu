#include "yagl_eglb_display.h"

void yagl_eglb_display_init(struct yagl_eglb_display *dpy,
                            struct yagl_egl_backend_ps *backend_ps)
{
    dpy->backend_ps = backend_ps;
}

void yagl_eglb_display_cleanup(struct yagl_eglb_display *dpy)
{
}
