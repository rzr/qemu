#ifndef YAGL_GLES1_API_PS_H_
#define YAGL_GLES1_API_PS_H_

#include "yagl_api.h"

struct yagl_client_interface;
struct yagl_gles1_driver;

typedef struct YaglGles1ApiPs
{
    struct yagl_api_ps base;

    struct yagl_gles1_driver *driver;

    struct yagl_client_interface *client_iface;
} YaglGles1ApiPs;

/*
 * Takes ownership of 'driver_ps' and 'client_iface'.
 */
void yagl_gles1_api_ps_init(YaglGles1ApiPs *gles1_api_ps,
                            struct yagl_gles1_driver *driver,
                            struct yagl_client_interface *client_iface);

/*
 * This MUST be called before cleanup in order to purge all resources.
 * This cannot be done in 'xxx_cleanup' since by that time
 * egl interface will be destroyed and it might be required in order to
 * destroy some internal state.
 */
void yagl_gles1_api_ps_fini(YaglGles1ApiPs *gles1_api_ps);

void yagl_gles1_api_ps_cleanup(YaglGles1ApiPs *gles1_api_ps);

#endif /* YAGL_GLES1_API_PS_H_ */
