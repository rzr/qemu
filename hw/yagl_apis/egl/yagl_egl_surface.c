#include "yagl_egl_surface.h"
#include "yagl_egl_display.h"
#include "yagl_egl_config.h"
#include "yagl_eglb_surface.h"

static void yagl_egl_surface_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_surface *sfc = (struct yagl_egl_surface*)ref;

    sfc->backend_sfc->destroy(sfc->backend_sfc);

    yagl_egl_config_release(sfc->cfg);

    yagl_resource_cleanup(&sfc->res);

    g_free(sfc);
}

struct yagl_egl_surface
    *yagl_egl_surface_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_eglb_surface *backend_sfc)
{
    struct yagl_egl_surface *sfc = g_malloc0(sizeof(struct yagl_egl_surface));

    yagl_resource_init(&sfc->res, &yagl_egl_surface_destroy);

    sfc->dpy = dpy;
    yagl_egl_config_acquire(cfg);
    sfc->cfg = cfg;
    sfc->backend_sfc = backend_sfc;

    return sfc;
}

void yagl_egl_surface_acquire(struct yagl_egl_surface *sfc)
{
    if (sfc) {
        yagl_resource_acquire(&sfc->res);
    }
}

void yagl_egl_surface_release(struct yagl_egl_surface *sfc)
{
    if (sfc) {
        yagl_resource_release(&sfc->res);
    }
}
