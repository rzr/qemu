#include "vigs_backend.h"

void vigs_backend_init(struct vigs_backend *backend,
                       struct winsys_info *ws_info)
{
    backend->ws_info = ws_info;
}

void vigs_backend_cleanup(struct vigs_backend *backend)
{
}
