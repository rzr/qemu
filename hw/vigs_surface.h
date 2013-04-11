#ifndef _QEMU_VIGS_SURFACE_H
#define _QEMU_VIGS_SURFACE_H

#include "vigs_types.h"
#include <glib.h>

struct winsys_surface;
struct vigs_backend;

struct vigs_surface
{
    struct winsys_surface *ws_sfc;

    struct vigs_backend *backend;

    vigsp_surface_id id;

    GHashTable *resource_ids;

    uint32_t width;
    uint32_t height;
    uint32_t stride;
    vigsp_surface_format format;

    /*
     * From protocol.
     */
    vigsp_offset vram_offset;

    uint8_t *data;

    bool is_dirty;

    void (*update)(struct vigs_surface */*sfc*/,
                   vigsp_offset /*vram_offset*/,
                   uint8_t */*data*/);

    void (*set_data)(struct vigs_surface */*sfc*/,
                     vigsp_offset /*vram_offset*/,
                     uint8_t */*data*/);

    void (*read_pixels)(struct vigs_surface */*sfc*/,
                        uint32_t /*x*/,
                        uint32_t /*y*/,
                        uint32_t /*width*/,
                        uint32_t /*height*/,
                        uint32_t /*stride*/,
                        uint8_t */*pixels*/);

    void (*copy)(struct vigs_surface */*dst*/,
                 struct vigs_surface */*src*/,
                 const struct vigsp_copy */*entries*/,
                 uint32_t /*num_entries*/);

    void (*solid_fill)(struct vigs_surface */*sfc*/,
                       vigsp_color /*color*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*put_image)(struct vigs_surface */*sfc*/,
                      const void */*src*/,
                      uint32_t /*src_stride*/,
                      const struct vigsp_rect */*rect*/);

    void (*destroy)(struct vigs_surface */*sfc*/);
};

void vigs_surface_init(struct vigs_surface *sfc,
                       struct winsys_surface *ws_sfc,
                       struct vigs_backend *backend,
                       vigsp_surface_id id,
                       uint32_t stride,
                       vigsp_surface_format format,
                       vigsp_offset vram_offset,
                       uint8_t *data);

void vigs_surface_cleanup(struct vigs_surface *sfc);

void vigs_surface_update_vram(struct vigs_surface *sfc);

#endif
