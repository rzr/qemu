#include "yagl_egl_backend.h"

void yagl_egl_backend_init(struct yagl_egl_backend *backend,
                           yagl_render_type render_type,
                           yagl_gl_version gl_version)
{
    backend->render_type = render_type;
    backend->gl_version = gl_version;
}

void yagl_egl_backend_cleanup(struct yagl_egl_backend *backend)
{
}
