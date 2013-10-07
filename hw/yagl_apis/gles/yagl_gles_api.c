#include "yagl_gles_api.h"
#include "yagl_host_gles_calls.h"
#include "yagl_gles_driver.h"

static void yagl_gles_api_destroy(struct yagl_api *api)
{
    struct yagl_gles_api *gles_api = (struct yagl_gles_api*)api;

    gles_api->driver->destroy(gles_api->driver);
    gles_api->driver = NULL;

    yagl_api_cleanup(&gles_api->base);

    g_free(gles_api);
}

struct yagl_api *yagl_gles_api_create(struct yagl_gles_driver *driver)
{
    struct yagl_gles_api *gles_api = g_malloc0(sizeof(struct yagl_gles_api));

    yagl_api_init(&gles_api->base);

    gles_api->base.process_init = &yagl_host_gles_process_init;
    gles_api->base.destroy = &yagl_gles_api_destroy;

    gles_api->driver = driver;

    return &gles_api->base;
}
