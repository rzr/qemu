#ifndef _QEMU_VIGS_SURFACE_H
#define _QEMU_VIGS_SURFACE_H

#include "vigs_types.h"

struct winsys_surface;
struct vigs_backend;

struct vigs_surface
{
    struct winsys_surface *ws_sfc;

    struct vigs_backend *backend;

    uint32_t stride;
    vigsp_surface_format format;
    vigsp_surface_id id;

    bool is_dirty;

    void (*read_pixels)(struct vigs_surface */*sfc*/,
                        uint32_t /*x*/,
                        uint32_t /*y*/,
                        uint32_t /*width*/,
                        uint32_t /*height*/,
                        uint8_t */*pixels*/);

    void (*draw_pixels)(struct vigs_surface */*sfc*/,
                        uint8_t */*pixels*/,
                        const struct vigsp_rect */*entries*/,
                        uint32_t /*num_entries*/);

    void (*copy)(struct vigs_surface */*dst*/,
                 struct vigs_surface */*src*/,
                 const struct vigsp_copy */*entries*/,
                 uint32_t /*num_entries*/);

    void (*solid_fill)(struct vigs_surface */*sfc*/,
                       vigsp_color /*color*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*destroy)(struct vigs_surface */*sfc*/);
};

void vigs_surface_init(struct vigs_surface *sfc,
                       struct winsys_surface *ws_sfc,
                       struct vigs_backend *backend,
                       uint32_t stride,
                       vigsp_surface_format format,
                       vigsp_surface_id id);

void vigs_surface_cleanup(struct vigs_surface *sfc);

#endif
