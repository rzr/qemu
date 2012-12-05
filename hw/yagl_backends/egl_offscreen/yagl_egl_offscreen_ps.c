#include "yagl_egl_offscreen_ps.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_driver.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

YAGL_DEFINE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static void yagl_egl_offscreen_ps_thread_init(struct yagl_egl_backend_ps *backend_ps,
                                              struct yagl_thread_state *ts)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_egl_offscreen_ps_thread_init, NULL);

    egl_offscreen_ps->driver_ps->thread_init(egl_offscreen_ps->driver_ps, ts);

    egl_offscreen_ts = yagl_egl_offscreen_ts_create(ts);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_display *yagl_egl_offscreen_ps_create_display(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;
    struct yagl_egl_offscreen_display *dpy =
        yagl_egl_offscreen_display_create(egl_offscreen_ps);

    return dpy ? &dpy->base : NULL;
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

    YAGL_LOG_FUNC_ENTER_TS(egl_offscreen_ts->ts, yagl_egl_offscreen_ps_make_current, NULL);

    res = egl_offscreen_ps->driver_ps->make_current(egl_offscreen_ps->driver_ps,
                                                    egl_offscreen_dpy->native_dpy,
                                                    egl_offscreen_draw->native_sfc,
                                                    egl_offscreen_read->native_sfc,
                                                    egl_offscreen_ctx->native_ctx);

    if (res) {
        egl_offscreen_ts->dpy = egl_offscreen_dpy;
        egl_offscreen_ts->ctx = egl_offscreen_ctx;
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res;
}

static bool yagl_egl_offscreen_ps_release_current(struct yagl_egl_backend_ps *backend_ps, bool force)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;
    bool res;

    YAGL_LOG_FUNC_ENTER_TS(egl_offscreen_ts->ts, yagl_egl_offscreen_ps_release_current, NULL);

    assert(egl_offscreen_ts->dpy);

    if (!egl_offscreen_ts->dpy) {
        return false;
    }

    res = egl_offscreen_ps->driver_ps->make_current(egl_offscreen_ps->driver_ps,
                                                    egl_offscreen_ts->dpy->native_dpy,
                                                    EGL_NO_SURFACE,
                                                    EGL_NO_SURFACE,
                                                    EGL_NO_CONTEXT);

    if (res || force) {
        egl_offscreen_ts->dpy = NULL;
        egl_offscreen_ts->ctx = NULL;
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res || force;
}

static void yagl_egl_offscreen_ps_wait_native(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    YAGL_LOG_FUNC_ENTER_TS(egl_offscreen_ts->ts, yagl_egl_offscreen_ps_wait_native, NULL);

    egl_offscreen_ps->driver_ps->wait_native(egl_offscreen_ps->driver_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_offscreen_ps_thread_fini(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    YAGL_LOG_FUNC_ENTER_TS(egl_offscreen_ts->ts, yagl_egl_offscreen_ps_thread_fini, NULL);

    yagl_egl_offscreen_ts_destroy(egl_offscreen_ts);
    egl_offscreen_ts = NULL;

    egl_offscreen_ps->driver_ps->thread_fini(egl_offscreen_ps->driver_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_offscreen_ps_destroy(struct yagl_egl_backend_ps *backend_ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps = (struct yagl_egl_offscreen_ps*)backend_ps;

    YAGL_LOG_FUNC_ENTER(backend_ps->ps->id, 0, yagl_egl_offscreen_ps_destroy, NULL);

    egl_offscreen_ps->driver_ps->destroy(egl_offscreen_ps->driver_ps);
    egl_offscreen_ps->driver_ps = NULL;

    yagl_egl_backend_ps_cleanup(&egl_offscreen_ps->base);

    g_free(egl_offscreen_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_ps
    *yagl_egl_offscreen_ps_create(struct yagl_egl_offscreen *egl_offscreen,
                                  struct yagl_process_state *ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps;
    struct yagl_egl_driver_ps *driver_ps;

    YAGL_LOG_FUNC_ENTER(ps->id, 0, yagl_egl_offscreen_ps_create, NULL);

    driver_ps = egl_offscreen->driver->process_init(egl_offscreen->driver, ps);

    if (!driver_ps) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    egl_offscreen_ps = g_malloc0(sizeof(*egl_offscreen_ps));

    yagl_egl_backend_ps_init(&egl_offscreen_ps->base, &egl_offscreen->base, ps);

    egl_offscreen_ps->base.thread_init = &yagl_egl_offscreen_ps_thread_init;
    egl_offscreen_ps->base.create_display = &yagl_egl_offscreen_ps_create_display;
    egl_offscreen_ps->base.make_current = &yagl_egl_offscreen_ps_make_current;
    egl_offscreen_ps->base.release_current = &yagl_egl_offscreen_ps_release_current;
    egl_offscreen_ps->base.wait_native = &yagl_egl_offscreen_ps_wait_native;
    egl_offscreen_ps->base.thread_fini = &yagl_egl_offscreen_ps_thread_fini;
    egl_offscreen_ps->base.destroy = &yagl_egl_offscreen_ps_destroy;

    egl_offscreen_ps->driver_ps = driver_ps;

    YAGL_LOG_FUNC_EXIT(NULL);

    return egl_offscreen_ps;
}
