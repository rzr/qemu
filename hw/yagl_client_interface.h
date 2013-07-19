#ifndef _QEMU_YAGL_CLIENT_INTERFACE_H
#define _QEMU_YAGL_CLIENT_INTERFACE_H

#include "yagl_types.h"

struct yagl_client_context;
struct yagl_client_image;
struct yagl_sharegroup;
struct yagl_ref;

struct yagl_client_interface
{
    struct yagl_client_context *(*create_ctx)(struct yagl_client_interface */*iface*/,
                                              struct yagl_sharegroup */*sg*/);

    struct yagl_client_image
        *(*create_image)(struct yagl_client_interface */*iface*/,
                         yagl_object_name /*tex_global_name*/,
                         struct yagl_ref */*tex_data*/);
};

void yagl_client_interface_init(struct yagl_client_interface *iface);

void yagl_client_interface_cleanup(struct yagl_client_interface *iface);

#endif
