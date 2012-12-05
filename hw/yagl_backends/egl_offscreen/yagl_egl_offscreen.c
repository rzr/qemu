#include "yagl_egl_offscreen.h"
#include "yagl_egl_offscreen_ps.h"
#include "yagl_egl_driver.h"
#include "yagl_log.h"

static struct yagl_egl_backend_ps *yagl_egl_offscreen_process_init(
    struct yagl_egl_backend *backend,
    struct yagl_process_state *ps)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps =
        yagl_egl_offscreen_ps_create((struct yagl_egl_offscreen*)backend, ps);

    return egl_offscreen_ps ? &egl_offscreen_ps->base : NULL;
}

static void yagl_egl_offscreen_destroy(struct yagl_egl_backend *backend)
{
    struct yagl_egl_offscreen *egl_offscreen = (struct yagl_egl_offscreen*)backend;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_egl_offscreen_destroy, NULL);

    egl_offscreen->driver->destroy(egl_offscreen->driver);
    egl_offscreen->driver = NULL;

    yagl_egl_backend_cleanup(&egl_offscreen->base);

    g_free(egl_offscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_backend *yagl_egl_offscreen_create(struct yagl_egl_driver *driver)
{
    struct yagl_egl_offscreen *egl_offscreen = g_malloc0(sizeof(struct yagl_egl_offscreen));

    YAGL_LOG_FUNC_ENTER_NPT(yagl_egl_offscreen_create, NULL);

    yagl_egl_backend_init(&egl_offscreen->base);

    egl_offscreen->base.process_init = &yagl_egl_offscreen_process_init;
    egl_offscreen->base.destroy = &yagl_egl_offscreen_destroy;

    egl_offscreen->driver = driver;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_offscreen->base;
}
