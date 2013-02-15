#include <GLES2/gl2.h>
#include "yagl_gles2_api_ps.h"
#include "yagl_gles2_driver.h"
#include "yagl_process.h"
#include "yagl_client_interface.h"

void yagl_gles2_api_ps_init(struct yagl_gles2_api_ps *gles2_api_ps,
                            struct yagl_gles2_driver_ps *driver_ps,
                            struct yagl_client_interface *client_iface)
{
    gles2_api_ps->driver_ps = driver_ps;
    gles2_api_ps->client_iface = client_iface;

    yagl_process_register_client_interface(gles2_api_ps->base.ps,
                                           yagl_client_api_gles2,
                                           gles2_api_ps->client_iface);
}

void yagl_gles2_api_ps_fini(struct yagl_gles2_api_ps *gles2_api_ps)
{
}

void yagl_gles2_api_ps_cleanup(struct yagl_gles2_api_ps *gles2_api_ps)
{
    yagl_process_unregister_client_interface(gles2_api_ps->base.ps,
                                             yagl_client_api_gles2);

    yagl_client_interface_cleanup(gles2_api_ps->client_iface);
    g_free(gles2_api_ps->client_iface);
    gles2_api_ps->client_iface = NULL;

    gles2_api_ps->driver_ps->destroy(gles2_api_ps->driver_ps);
    gles2_api_ps->driver_ps = NULL;
}
