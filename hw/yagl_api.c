#include "yagl_api.h"

void yagl_api_ps_init(struct yagl_api_ps *api_ps,
                      struct yagl_api *api)
{
    api_ps->api = api;
}

void yagl_api_ps_cleanup(struct yagl_api_ps *api_ps)
{
}

void yagl_api_init(struct yagl_api *api)
{
}

void yagl_api_cleanup(struct yagl_api *api)
{
}
