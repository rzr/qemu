#include "yagl_egl_api.h"
#include "yagl_host_egl_calls.h"
#include "yagl_egl_backend.h"

static void yagl_egl_api_destroy(struct yagl_api *api)
{
    struct yagl_egl_api *egl_api = (struct yagl_egl_api*)api;

    egl_api->backend->destroy(egl_api->backend);
    egl_api->backend = NULL;

    yagl_api_cleanup(&egl_api->base);

    g_free(egl_api);
}

struct yagl_api *yagl_egl_api_create(struct yagl_egl_backend *backend)
{
    struct yagl_egl_api *egl_api = g_malloc0(sizeof(struct yagl_egl_api));

    yagl_api_init(&egl_api->base);

    egl_api->base.process_init = &yagl_host_egl_process_init;
    egl_api->base.destroy = &yagl_egl_api_destroy;

    egl_api->backend = backend;

    return &egl_api->base;
}
