#include <GL/gl.h>
#include "yagl_gles1_onscreen.h"
#include "yagl_drivers/gles_onscreen/yagl_gles_onscreen.h"
#include "yagl_gles1_driver.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

struct yagl_gles1_onscreen
{
    struct yagl_gles1_driver base;

    struct yagl_gles1_driver *orig_driver;
};

static void yagl_gles1_onscreen_destroy(struct yagl_gles1_driver *driver)
{
    struct yagl_gles1_onscreen *gles1_onscreen = (struct yagl_gles1_onscreen*)driver;

    YAGL_LOG_FUNC_ENTER(yagl_gles1_onscreen_destroy, NULL);

    gles1_onscreen->orig_driver->destroy(gles1_onscreen->orig_driver);
    gles1_onscreen->orig_driver = NULL;

    yagl_gles1_driver_cleanup(driver);
    yagl_gles_onscreen_cleanup(&driver->base);
    g_free(gles1_onscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles1_driver
    *yagl_gles1_onscreen_create(struct yagl_gles1_driver *orig_driver)
{
    struct yagl_gles1_onscreen *gles1_onscreen = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_gles1_onscreen_create, NULL);

    gles1_onscreen = g_malloc0(sizeof(*gles1_onscreen));

    /*
     * Just copy over everything, then init our fields.
     */
    memcpy(&gles1_onscreen->base, orig_driver, sizeof(*orig_driver));
    gles1_onscreen->orig_driver = orig_driver;

    yagl_gles_onscreen_init(&gles1_onscreen->base.base);

    yagl_gles1_driver_init(&gles1_onscreen->base);

    gles1_onscreen->base.destroy = &yagl_gles1_onscreen_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles1_onscreen->base;
}
