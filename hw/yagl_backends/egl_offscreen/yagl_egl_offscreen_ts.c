#include "yagl_egl_offscreen_ts.h"

struct yagl_egl_offscreen_ts *yagl_egl_offscreen_ts_create(struct yagl_egl_driver *driver)
{
    struct yagl_egl_offscreen_ts *egl_offscreen_ts =
        g_malloc0(sizeof(struct yagl_egl_offscreen_ts));

    egl_offscreen_ts->driver = driver;

    return egl_offscreen_ts;
}

void yagl_egl_offscreen_ts_destroy(struct yagl_egl_offscreen_ts *egl_offscreen_ts)
{
    g_free(egl_offscreen_ts);
}
