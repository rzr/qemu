#include "yagl_egl_offscreen.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

YAGL_DEFINE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static void yagl_egl_offscreen_thread_init(struct yagl_egl_backend *backend)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_thread_init, NULL);

    egl_offscreen_ts = yagl_egl_offscreen_ts_create();

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_display *yagl_egl_offscreen_create_display(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;
    struct yagl_egl_offscreen_display *dpy =
        yagl_egl_offscreen_display_create(egl_offscreen);

    return dpy ? &dpy->base : NULL;
}

static bool yagl_egl_offscreen_make_current(struct yagl_egl_backend *backend,
                                            struct yagl_eglb_display *dpy,
                                            struct yagl_eglb_context *ctx,
                                            struct yagl_eglb_surface *draw,
                                            struct yagl_eglb_surface *read)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;
    struct yagl_egl_offscreen_display *egl_offscreen_dpy = (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_context *egl_offscreen_ctx = (struct yagl_egl_offscreen_context*)ctx;
    struct yagl_egl_offscreen_surface *egl_offscreen_draw = (struct yagl_egl_offscreen_surface*)draw;
    struct yagl_egl_offscreen_surface *egl_offscreen_read = (struct yagl_egl_offscreen_surface*)read;
    bool res = false;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_make_current, NULL);

    if (draw && read) {
        res = egl_offscreen->driver->make_current(egl_offscreen->driver,
                                                  egl_offscreen_dpy->native_dpy,
                                                  egl_offscreen_draw->native_sfc,
                                                  egl_offscreen_read->native_sfc,
                                                  egl_offscreen_ctx->native_ctx);

        if (res) {
            egl_offscreen_ts->dpy = egl_offscreen_dpy;
            egl_offscreen_ts->ctx = egl_offscreen_ctx;
        }
    } else {
        /*
         * EGL_KHR_surfaceless_context not supported for offscreen yet.
         */

        YAGL_LOG_ERROR("EGL_KHR_surfaceless_context not supported");
    }

    YAGL_LOG_FUNC_EXIT("%d", res);

    return res;
}

static bool yagl_egl_offscreen_release_current(struct yagl_egl_backend *backend, bool force)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;
    bool res;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_release_current, NULL);

    assert(egl_offscreen_ts->dpy);

    if (!egl_offscreen_ts->dpy) {
        return false;
    }

    res = egl_offscreen->driver->make_current(egl_offscreen->driver,
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

static void yagl_egl_offscreen_thread_fini(struct yagl_egl_backend *backend)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_thread_fini, NULL);

    yagl_egl_offscreen_ts_destroy(egl_offscreen_ts);
    egl_offscreen_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_offscreen_ensure_current(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    if (egl_offscreen_ts && egl_offscreen_ts->dpy) {
        return;
    }

    egl_offscreen->driver->make_current(egl_offscreen->driver,
                                        egl_offscreen->ensure_dpy,
                                        egl_offscreen->ensure_sfc,
                                        egl_offscreen->ensure_sfc,
                                        egl_offscreen->ensure_ctx);
}

static void yagl_egl_offscreen_unensure_current(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    if (egl_offscreen_ts && egl_offscreen_ts->dpy) {
        return;
    }

    egl_offscreen->driver->make_current(egl_offscreen->driver,
                                        egl_offscreen->ensure_dpy,
                                        EGL_NO_SURFACE,
                                        EGL_NO_SURFACE,
                                        EGL_NO_CONTEXT);
}

static void yagl_egl_offscreen_destroy(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_destroy, NULL);

    egl_offscreen->driver->context_destroy(egl_offscreen->driver,
                                           egl_offscreen->ensure_dpy,
                                           egl_offscreen->global_ctx);
    egl_offscreen->driver->context_destroy(egl_offscreen->driver,
                                           egl_offscreen->ensure_dpy,
                                           egl_offscreen->ensure_ctx);
    egl_offscreen->driver->pbuffer_surface_destroy(egl_offscreen->driver,
                                                   egl_offscreen->ensure_dpy,
                                                   egl_offscreen->ensure_sfc);
    egl_offscreen->driver->config_cleanup(egl_offscreen->driver,
                                          egl_offscreen->ensure_dpy,
                                          &egl_offscreen->ensure_config);
    egl_offscreen->driver->display_close(egl_offscreen->driver,
                                         egl_offscreen->ensure_dpy);

    egl_offscreen->driver->destroy(egl_offscreen->driver);
    egl_offscreen->driver = NULL;

    yagl_egl_backend_cleanup(&egl_offscreen->base);

    g_free(egl_offscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_backend *yagl_egl_offscreen_create(struct yagl_egl_driver *driver)
{
    struct yagl_egl_offscreen *egl_offscreen = g_malloc0(sizeof(struct yagl_egl_offscreen));
    EGLNativeDisplayType dpy = NULL;
    struct yagl_egl_native_config *configs = NULL;
    int i, num_configs = 0;
    struct yagl_egl_pbuffer_attribs attribs;
    EGLSurface sfc = EGL_NO_SURFACE;
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLContext global_ctx = EGL_NO_CONTEXT;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_create, NULL);

    yagl_egl_pbuffer_attribs_init(&attribs);

    yagl_egl_backend_init(&egl_offscreen->base, yagl_render_type_offscreen);

    dpy = driver->display_open(driver);

    if (!dpy) {
        goto fail;
    }

    configs = driver->config_enum(driver, dpy, &num_configs);

    if (!configs || (num_configs <= 0)) {
        goto fail;
    }

    sfc = driver->pbuffer_surface_create(driver, dpy, &configs[0],
                                         1, 1, &attribs);

    if (sfc == EGL_NO_SURFACE) {
        goto fail;
    }

    ctx = driver->context_create(driver, dpy, &configs[0],
                                 yagl_client_api_gles2, NULL);

    if (ctx == EGL_NO_CONTEXT) {
        goto fail;
    }

    global_ctx = driver->context_create(driver, dpy, &configs[0],
                                        yagl_client_api_gles2, ctx);

    if (global_ctx == EGL_NO_CONTEXT) {
        goto fail;
    }

    egl_offscreen->base.thread_init = &yagl_egl_offscreen_thread_init;
    egl_offscreen->base.create_display = &yagl_egl_offscreen_create_display;
    egl_offscreen->base.make_current = &yagl_egl_offscreen_make_current;
    egl_offscreen->base.release_current = &yagl_egl_offscreen_release_current;
    egl_offscreen->base.thread_fini = &yagl_egl_offscreen_thread_fini;
    egl_offscreen->base.ensure_current = &yagl_egl_offscreen_ensure_current;
    egl_offscreen->base.unensure_current = &yagl_egl_offscreen_unensure_current;
    egl_offscreen->base.destroy = &yagl_egl_offscreen_destroy;

    egl_offscreen->driver = driver;
    egl_offscreen->ensure_dpy = dpy;
    egl_offscreen->ensure_config = configs[0];
    egl_offscreen->ensure_ctx = ctx;
    egl_offscreen->ensure_sfc = sfc;
    egl_offscreen->global_ctx = global_ctx;

    for (i = 1; i < num_configs; ++i) {
        driver->config_cleanup(driver, dpy, &configs[i]);
    }
    g_free(configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_offscreen->base;

fail:
    if (ctx != EGL_NO_CONTEXT) {
        driver->context_destroy(driver, dpy, ctx);
    }

    if (sfc != EGL_NO_SURFACE) {
        driver->pbuffer_surface_destroy(driver, dpy, sfc);
    }

    if (configs) {
        for (i = 0; i < num_configs; ++i) {
            driver->config_cleanup(driver, dpy, &configs[i]);
        }
        g_free(configs);
    }

    if (dpy) {
        driver->display_close(driver, dpy);
    }

    yagl_egl_backend_cleanup(&egl_offscreen->base);

    g_free(egl_offscreen);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
