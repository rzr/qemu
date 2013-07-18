#include "yagl_gles1_api.h"
#include "yagl_host_gles1_calls.h"
#include "yagl_gles1_driver.h"

static void yagl_gles1_api_destroy(struct yagl_api *api)
{
    YaglGles1API *gles1_api = (YaglGles1API *)api;

    gles1_api->driver->destroy(gles1_api->driver);
    gles1_api->driver = NULL;

    yagl_api_cleanup(&gles1_api->base);

    g_free(gles1_api);
}

struct yagl_api *yagl_gles1_api_create(struct yagl_gles1_driver *driver)
{
    YaglGles1API *gles1_api = g_new0(YaglGles1API, 1);

    yagl_api_init(&gles1_api->base);

    gles1_api->base.process_init = &yagl_host_gles1_process_init;
    gles1_api->base.destroy = &yagl_gles1_api_destroy;

    gles1_api->driver = driver;

    return &gles1_api->base;
}
