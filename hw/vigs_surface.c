#include "vigs_surface.h"
#include "winsys.h"

void vigs_surface_init(struct vigs_surface *sfc,
                       struct winsys_surface *ws_sfc,
                       struct vigs_backend *backend,
                       uint32_t stride,
                       vigsp_surface_format format,
                       vigsp_surface_id id)
{
    ws_sfc->acquire(ws_sfc);
    sfc->ws_sfc = ws_sfc;
    sfc->backend = backend;
    sfc->stride = stride;
    sfc->format = format;
    sfc->id = id;
}

void vigs_surface_cleanup(struct vigs_surface *sfc)
{
    sfc->ws_sfc->release(sfc->ws_sfc);
}
