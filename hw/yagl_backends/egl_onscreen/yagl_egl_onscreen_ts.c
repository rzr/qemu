#include "yagl_egl_onscreen_ts.h"
#include <GL/gl.h>
#include "yagl_egl_onscreen.h"
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_surface.h"
#include "yagl_gles_driver.h"

struct yagl_egl_onscreen_ts *yagl_egl_onscreen_ts_create(struct yagl_gles_driver *gles_driver)
{
    struct yagl_egl_onscreen_ts *egl_onscreen_ts =
        g_malloc0(sizeof(struct yagl_egl_onscreen_ts));

    egl_onscreen_ts->gles_driver = gles_driver;

    return egl_onscreen_ts;
}

void yagl_egl_onscreen_ts_destroy(struct yagl_egl_onscreen_ts *egl_onscreen_ts)
{
    g_free(egl_onscreen_ts);
}
