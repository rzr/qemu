#include "yagl_egl_backend.h"

void yagl_egl_backend_ps_init(struct yagl_egl_backend_ps *backend_ps,
                              struct yagl_egl_backend *backend,
                              struct yagl_process_state *ps)
{
    backend_ps->backend = backend;
    backend_ps->ps = ps;
}

void yagl_egl_backend_ps_cleanup(struct yagl_egl_backend_ps *backend_ps)
{
}

void yagl_egl_backend_init(struct yagl_egl_backend *backend,
                           yagl_render_type render_type)
{
    backend->render_type = render_type;
}

void yagl_egl_backend_cleanup(struct yagl_egl_backend *backend)
{
}
