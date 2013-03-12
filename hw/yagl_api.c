#include "yagl_api.h"
#include "yagl_log.h"
#include "yagl_thread.h"
#include "yagl_process.h"

void yagl_api_ps_init(struct yagl_api_ps *api_ps,
                      struct yagl_api *api,
                      struct yagl_process_state *ps)
{
    api_ps->api = api;
    api_ps->ps = ps;
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
