#include "yagl_egl_interface.h"

void yagl_egl_interface_init(struct yagl_egl_interface *iface,
                             yagl_render_type render_type)
{
    iface->render_type = render_type;
}

void yagl_egl_interface_cleanup(struct yagl_egl_interface *iface)
{
}
