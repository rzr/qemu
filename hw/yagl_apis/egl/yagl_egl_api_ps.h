#ifndef _QEMU_YAGL_EGL_API_PS_H
#define _QEMU_YAGL_EGL_API_PS_H

#include "yagl_api.h"
#include "qemu/queue.h"

struct yagl_egl_interface;
struct yagl_egl_backend;
struct yagl_egl_display;

struct yagl_egl_api_ps
{
    struct yagl_api_ps base;

    struct yagl_egl_backend *backend;

    struct yagl_egl_interface *egl_iface;

    QLIST_HEAD(, yagl_egl_display) displays;
};

void yagl_egl_api_ps_init(struct yagl_egl_api_ps *egl_api_ps,
                          struct yagl_egl_backend *backend,
                          struct yagl_egl_interface *egl_iface);

/*
 * This MUST be called before cleanup in order to purge all resources.
 * This cannot be done in 'xxx_cleanup' since by that time all
 * client interfaces will be destroyed and they're required in order to
 * destroy some internal state like contexts.
 */
void yagl_egl_api_ps_fini(struct yagl_egl_api_ps *egl_api_ps);

void yagl_egl_api_ps_cleanup(struct yagl_egl_api_ps *egl_api_ps);

struct yagl_egl_display *yagl_egl_api_ps_display_get(struct yagl_egl_api_ps *egl_api_ps,
                                                     yagl_host_handle handle);

struct yagl_egl_display *yagl_egl_api_ps_display_add(struct yagl_egl_api_ps *egl_api_ps,
                                                     uint32_t display_id);

#endif
