#include "yagl_egl_backend.h"

void yagl_egl_backend_init(struct yagl_egl_backend *backend,
                           yagl_render_type render_type)
{
    backend->render_type = render_type;
}

void yagl_egl_backend_cleanup(struct yagl_egl_backend *backend)
{
}
