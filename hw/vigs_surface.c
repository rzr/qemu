#include "vigs_surface.h"
#include "winsys.h"

void vigs_surface_init(struct vigs_surface *sfc,
                       struct winsys_surface *ws_sfc,
                       struct vigs_backend *backend,
                       vigsp_surface_id id,
                       uint32_t stride,
                       vigsp_surface_format format,
                       vigsp_offset vram_offset,
                       uint8_t *data)
{
    ws_sfc->acquire(ws_sfc);
    sfc->ws_sfc = ws_sfc;
    sfc->backend = backend;
    sfc->id = id;
    sfc->width = ws_sfc->width;
    sfc->height = ws_sfc->height;
    sfc->stride = stride;
    sfc->format = format;
    sfc->vram_offset = vram_offset;
    sfc->data = data;
    sfc->resource_ids = g_hash_table_new(g_direct_hash, g_direct_equal);
}

void vigs_surface_cleanup(struct vigs_surface *sfc)
{
    sfc->ws_sfc->release(sfc->ws_sfc);
    g_hash_table_destroy(sfc->resource_ids);
}

void vigs_surface_update_vram(struct vigs_surface *sfc)
{
    assert(sfc->data);
    if (sfc->is_dirty) {
        sfc->read_pixels(sfc, 0, 0,
                         sfc->width, sfc->height,
                         sfc->stride, sfc->data);
        sfc->is_dirty = false;
    }
}
