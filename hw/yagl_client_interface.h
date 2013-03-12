#ifndef _QEMU_YAGL_CLIENT_INTERFACE_H
#define _QEMU_YAGL_CLIENT_INTERFACE_H

#include "yagl_types.h"

struct yagl_client_context;
struct yagl_sharegroup;

struct yagl_client_interface
{
    struct yagl_client_context *(*create_ctx)(struct yagl_client_interface */*iface*/,
                                              struct yagl_sharegroup */*sg*/);
};

void yagl_client_interface_init(struct yagl_client_interface *iface);

void yagl_client_interface_cleanup(struct yagl_client_interface *iface);

#endif
