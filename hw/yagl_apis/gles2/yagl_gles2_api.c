#include "yagl_gles2_api.h"
#include "yagl_host_gles2_calls.h"
#include "yagl_gles2_driver.h"

static void yagl_gles2_api_destroy(struct yagl_api *api)
{
    struct yagl_gles2_api *gles2_api = (struct yagl_gles2_api*)api;

    gles2_api->driver->destroy(gles2_api->driver);
    gles2_api->driver = NULL;

    yagl_api_cleanup(&gles2_api->base);

    g_free(gles2_api);
}

struct yagl_api *yagl_gles2_api_create(struct yagl_gles2_driver *driver)
{
    struct yagl_gles2_api *gles2_api = g_malloc0(sizeof(struct yagl_gles2_api));

    yagl_api_init(&gles2_api->base);

    gles2_api->base.process_init = &yagl_host_gles2_process_init;
    gles2_api->base.destroy = &yagl_gles2_api_destroy;

    gles2_api->driver = driver;

    return &gles2_api->base;
}
