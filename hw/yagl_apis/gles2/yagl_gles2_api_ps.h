#ifndef _QEMU_YAGL_GLES2_API_PS_H
#define _QEMU_YAGL_GLES2_API_PS_H

#include "yagl_api.h"

struct yagl_client_interface;
struct yagl_gles2_driver_ps;

struct yagl_gles2_api_ps
{
    struct yagl_api_ps base;

    struct yagl_gles2_driver_ps *driver_ps;

    struct yagl_client_interface *client_iface;
};

/*
 * Takes ownership of 'driver_ps' and 'client_iface'.
 */
void yagl_gles2_api_ps_init(struct yagl_gles2_api_ps *gles2_api_ps,
                            struct yagl_gles2_driver_ps *driver_ps,
                            struct yagl_client_interface *client_iface);

/*
 * This MUST be called before cleanup in order to purge all resources.
 * This cannot be done in 'xxx_cleanup' since by that time
 * egl interface will be destroyed and it might be required in order to
 * destroy some internal state.
 */
void yagl_gles2_api_ps_fini(struct yagl_gles2_api_ps *gles2_api_ps);

void yagl_gles2_api_ps_cleanup(struct yagl_gles2_api_ps *gles2_api_ps);

#endif
