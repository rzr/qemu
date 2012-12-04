#include "yagl_egl_offscreen_ps.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_driver.h"
#include "yagl_tls.h"

static YAGL_DEFINE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static void yagl_egl_offscreen_ps_thread_init(struct yagl_egl_backend_ps *backend_ps,
                                              struct yagl_thread_state *ts)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    egl_offscreen_ps->driver_ps->thread_init(egl_offscreen_ps->driver_ps, ts);

    egl_offscreen_ts = yagl_egl_offscreen_ts_create(ts);
}

static struct yagl_eglb_display *yagl_egl_offscreen_ps_create_display(struct yagl_egl_backend_ps *backend_ps)
{
    return NULL;
}

static bool yagl_egl_offscreen_ps_make_current(struct yagl_egl_backend_ps *backend_ps,
                                               struct yagl_eglb_display *dpy,
                                               struct yagl_eglb_context *ctx,
                                               struct yagl_eglb_surface *draw,
                                               struct yagl_eglb_surface *read)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;
    struct yagl_egl_offscreen_display *egl_offscreen_dpy = (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_context *egl_offscreen_ctx = (struct yagl_egl_offscreen_context*)ctx;
    struct yagl_egl_offscreen_surface *egl_offscreen_draw = (struct yagl_egl_offscreen_surface*)draw;
    struct yagl_egl_offscreen_surface *egl_offscreen_read = (struct yagl_egl_offscreen_surface*)read;
    bool res;

    res = egl_offscreen_ps->driver_ps->make_current(egl_offscreen_ps->driver_ps,
                                                    egl_offscreen_dpy->native_dpy,
                                                    egl_offscreen_draw->native_sfc,
                                                    egl_offscreen_read->native_sfc,
                                                    egl_offscreen_ctx->native_ctx);

    if (res) {
        egl_offscreen_ts->dpy = egl_offscreen_dpy;
    }

    return res;
}

static bool yagl_egl_offscreen_ps_release_current(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;
    bool res;

    assert(egl_offscreen_ts->dpy);

    if (!egl_offscreen_ts->dpy) {
        return false;
    }

    res = egl_offscreen_ps->driver_ps->make_current(egl_offscreen_ps->driver_ps,
                                                    egl_offscreen_ts->dpy->native_dpy,
                                                    EGL_NO_SURFACE,
                                                    EGL_NO_SURFACE,
                                                    EGL_NO_CONTEXT);

    if (res) {
        egl_offscreen_ts->dpy = NULL;
    }

    return res;
}

static void yagl_egl_offscreen_ps_thread_fini(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    yagl_egl_offscreen_ts_destroy(egl_offscreen_ts);
    egl_offscreen_ts = NULL;

    egl_offscreen_ps->driver_ps->thread_fini(egl_offscreen_ps->driver_ps);
}

static void yagl_egl_offscreen_ps_destroy(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    egl_offscreen_ps->driver_ps->destroy(egl_offscreen_ps->driver_ps);
    egl_offscreen_ps->driver_ps = NULL;

    yagl_egl_backend_ps_cleanup(&egl_offscreen_ps->base);

    g_free(egl_offscreen_ps);
}

struct yagl_egl_offscreen_ps
    *yagl_egl_offscreen_ps_create(struct yagl_egl_offscreen *egl_offscreen,
                                  struct yagl_process_state *ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps;
    struct yagl_egl_driver_ps *driver_ps;

    driver_ps = egl_offscreen->driver->process_init(egl_offscreen->driver, ps);

    if (!driver_ps) {
        return NULL;
    }

    egl_offscreen_ps = g_malloc0(sizeof(*egl_offscreen_ps));

    yagl_egl_backend_ps_init(&egl_offscreen_ps->base, &egl_offscreen->base, ps);

    egl_offscreen_ps->base.thread_init = &yagl_egl_offscreen_ps_thread_init;
    egl_offscreen_ps->base.create_display = &yagl_egl_offscreen_ps_create_display;
    egl_offscreen_ps->base.make_current = &yagl_egl_offscreen_ps_make_current;
    egl_offscreen_ps->base.release_current = &yagl_egl_offscreen_ps_release_current;
    egl_offscreen_ps->base.thread_fini = &yagl_egl_offscreen_ps_thread_fini;
    egl_offscreen_ps->base.destroy = &yagl_egl_offscreen_ps_destroy;

    egl_offscreen_ps->driver_ps = driver_ps;

    return egl_offscreen_ps;
}
