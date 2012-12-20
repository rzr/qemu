#include <GL/gl.h>
#include "yagl_gles1_api_ps.h"
#include "yagl_gles1_driver.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_client_interface.h"


void yagl_gles1_api_ps_init(YaglGles1ApiPs *gles1_api_ps,
                            struct yagl_gles1_driver *driver,
                            struct yagl_client_interface *client_iface)
{
    gles1_api_ps->driver = driver;
    gles1_api_ps->client_iface = client_iface;

    yagl_process_register_client_interface(cur_ts->ps,
                                           yagl_client_api_gles1,
                                           gles1_api_ps->client_iface);
}

void yagl_gles1_api_ps_fini(YaglGles1ApiPs *gles1_api_ps)
{
}

void yagl_gles1_api_ps_cleanup(YaglGles1ApiPs *gles1_api_ps)
{
    yagl_process_unregister_client_interface(cur_ts->ps,
                                             yagl_client_api_gles1);

    yagl_client_interface_cleanup(gles1_api_ps->client_iface);
    g_free(gles1_api_ps->client_iface);
    gles1_api_ps->client_iface = NULL;
}
