#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_ps.h"

static struct yagl_egl_native_config
    *yagl_egl_offscreen_display_config_enum(struct yagl_eglb_display *dpy,
                                            int *num_configs)
{
    return NULL;
}

static void yagl_egl_offscreen_display_config_cleanup(struct yagl_eglb_display *dpy,
    const struct yagl_egl_native_config *cfg)
{
}

static struct yagl_eglb_context
    *yagl_egl_offscreen_display_create_context(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               struct yagl_client_context *client_ctx,
                                               struct yagl_eglb_context *share_context)
{
    return NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_offscreen_display_create_surface(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               EGLenum type,
                                               const void *attribs,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t bpp,
                                               target_ulong pixels)
{
    return NULL;
}

static void yagl_egl_offscreen_display_destroy(struct yagl_eglb_display *dpy)
{
}

struct yagl_egl_offscreen_display
    *yagl_egl_offscreen_display_create(struct yagl_egl_offscreen_ps *egl_offscreen_ps)
{
    struct yagl_egl_offscreen_display *dpy;
    EGLNativeDisplayType native_dpy;

    native_dpy = egl_offscreen_ps->driver_ps->display_create(egl_offscreen_ps->driver_ps);

    if (!native_dpy) {
        return NULL;
    }

    dpy = g_malloc0(sizeof(*dpy));

    yagl_eglb_display_init(&dpy->base, &egl_offscreen_ps->base);

    dpy->base.config_enum = &yagl_egl_offscreen_display_config_enum;
    dpy->base.config_cleanup = &yagl_egl_offscreen_display_config_cleanup;
    dpy->base.create_context = &yagl_egl_offscreen_display_create_context;
    dpy->base.create_offscreen_surface = &yagl_egl_offscreen_display_create_surface;
    dpy->base.destroy = &yagl_egl_offscreen_display_destroy;

    dpy->native_dpy = native_dpy;

    return dpy;
}
