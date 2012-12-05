#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen_ps.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_egl_native_config.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

YAGL_DECLARE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static struct yagl_egl_native_config
    *yagl_egl_offscreen_display_config_enum(struct yagl_eglb_display *dpy,
                                            int *num_configs)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_ps *egl_offscreen_ps =
        (struct yagl_egl_offscreen_ps*)dpy->backend_ps;
    struct yagl_egl_native_config *native_configs;

    YAGL_LOG_FUNC_ENTER_TS(egl_offscreen_ts->ts, yagl_egl_offscreen_display_config_enum,
                           "dpy = %p", dpy);

    native_configs =
        egl_offscreen_ps->driver_ps->config_enum(egl_offscreen_ps->driver_ps,
                                                 egl_offscreen_dpy->native_dpy,
                                                 num_configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return native_configs;
}

static void yagl_egl_offscreen_display_config_cleanup(struct yagl_eglb_display *dpy,
    const struct yagl_egl_native_config *cfg)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_ps *egl_offscreen_ps =
        (struct yagl_egl_offscreen_ps*)dpy->backend_ps;

    YAGL_LOG_FUNC_ENTER(dpy->backend_ps->ps->id, 0,
                        yagl_egl_offscreen_display_config_cleanup,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    egl_offscreen_ps->driver_ps->config_cleanup(egl_offscreen_ps->driver_ps,
                                                egl_offscreen_dpy->native_dpy,
                                                cfg);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_context
    *yagl_egl_offscreen_display_create_context(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               struct yagl_client_context *client_ctx,
                                               struct yagl_eglb_context *share_context)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_context *ctx =
        yagl_egl_offscreen_context_create(egl_offscreen_dpy,
                                          cfg,
                                          client_ctx,
                                          (struct yagl_egl_offscreen_context*)share_context);

    return ctx ? &ctx->base : NULL;
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
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_surface *sfc =
        yagl_egl_offscreen_surface_create(egl_offscreen_dpy,
                                          cfg,
                                          type,
                                          attribs,
                                          width,
                                          height,
                                          bpp,
                                          pixels);

    return sfc ? &sfc->base : NULL;
}

static void yagl_egl_offscreen_display_destroy(struct yagl_eglb_display *dpy)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_ps *egl_offscreen_ps =
        (struct yagl_egl_offscreen_ps*)dpy->backend_ps;

    YAGL_LOG_FUNC_ENTER(dpy->backend_ps->ps->id, 0,
                        yagl_egl_offscreen_display_destroy,
                        "dpy = %p", dpy);

    egl_offscreen_ps->driver_ps->display_destroy(egl_offscreen_ps->driver_ps,
                                                 egl_offscreen_dpy->native_dpy);

    yagl_eglb_display_cleanup(dpy);

    g_free(egl_offscreen_dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_display
    *yagl_egl_offscreen_display_create(struct yagl_egl_offscreen_ps *egl_offscreen_ps)
{
    struct yagl_egl_offscreen_display *dpy;
    EGLNativeDisplayType native_dpy;

    YAGL_LOG_FUNC_ENTER_TS(egl_offscreen_ts->ts, yagl_egl_offscreen_display_create, NULL);

    native_dpy = egl_offscreen_ps->driver_ps->display_create(egl_offscreen_ps->driver_ps);

    if (!native_dpy) {
        YAGL_LOG_FUNC_EXIT(NULL);
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

    YAGL_LOG_FUNC_EXIT("%p", dpy);

    return dpy;
}
