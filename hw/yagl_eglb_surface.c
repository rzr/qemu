#include "yagl_eglb_surface.h"

void yagl_eglb_surface_init(struct yagl_eglb_surface *sfc,
                            struct yagl_eglb_display *dpy,
                            EGLenum type,
                            const void *attribs)
{
    sfc->dpy = dpy;
    sfc->type = type;

    switch (sfc->type) {
    case EGL_PBUFFER_BIT:
        memcpy(&sfc->attribs.pbuffer, attribs, sizeof(sfc->attribs.pbuffer));
        break;
    case EGL_PIXMAP_BIT:
        memcpy(&sfc->attribs.pixmap, attribs, sizeof(sfc->attribs.pixmap));
        break;
    case EGL_WINDOW_BIT:
        memcpy(&sfc->attribs.window, attribs, sizeof(sfc->attribs.window));
        break;
    default:
        assert(0);
        memset(&sfc->attribs, 0, sizeof(sfc->attribs));
    }
}

void yagl_eglb_surface_cleanup(struct yagl_eglb_surface *sfc)
{
}
