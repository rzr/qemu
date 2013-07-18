#include <GL/gl.h>
#include "yagl_gles2_onscreen.h"
#include "yagl_drivers/gles_onscreen/yagl_gles_onscreen.h"
#include "yagl_gles2_driver.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

struct yagl_gles2_onscreen
{
    struct yagl_gles2_driver base;

    struct yagl_gles2_driver *orig_driver;
};

static void yagl_gles2_onscreen_destroy(struct yagl_gles2_driver *driver)
{
    struct yagl_gles2_onscreen *gles2_onscreen = (struct yagl_gles2_onscreen*)driver;

    YAGL_LOG_FUNC_ENTER(yagl_gles2_onscreen_destroy, NULL);

    gles2_onscreen->orig_driver->destroy(gles2_onscreen->orig_driver);
    gles2_onscreen->orig_driver = NULL;

    yagl_gles2_driver_cleanup(driver);
    yagl_gles_onscreen_cleanup(&driver->base);
    g_free(gles2_onscreen);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles2_driver
    *yagl_gles2_onscreen_create(struct yagl_gles2_driver *orig_driver)
{
    struct yagl_gles2_onscreen *gles2_onscreen = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_gles2_onscreen_create, NULL);

    gles2_onscreen = g_malloc0(sizeof(*gles2_onscreen));

    /*
     * Just copy over everything, then init our fields.
     */
    memcpy(&gles2_onscreen->base, orig_driver, sizeof(*orig_driver));
    gles2_onscreen->orig_driver = orig_driver;

    yagl_gles_onscreen_init(&gles2_onscreen->base.base);

    yagl_gles2_driver_init(&gles2_onscreen->base);

    gles2_onscreen->base.destroy = &yagl_gles2_onscreen_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles2_onscreen->base;
}
