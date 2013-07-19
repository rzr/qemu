#ifndef _QEMU_VIGS_BACKEND_H
#define _QEMU_VIGS_BACKEND_H

#include "vigs_types.h"

struct winsys_info;
struct vigs_surface;

struct vigs_backend
{
    struct winsys_info *ws_info;

    void (*batch_start)(struct vigs_backend */*backend*/);

    struct vigs_surface *(*create_surface)(struct vigs_backend */*backend*/,
                                           uint32_t /*width*/,
                                           uint32_t /*height*/,
                                           uint32_t /*stride*/,
                                           vigsp_surface_format /*format*/,
                                           vigsp_surface_id /*id*/);

    void (*batch_end)(struct vigs_backend */*backend*/);

    void (*destroy)(struct vigs_backend */*backend*/);
};

void vigs_backend_init(struct vigs_backend *backend,
                       struct winsys_info *ws_info);

void vigs_backend_cleanup(struct vigs_backend *backend);

struct vigs_backend *vigs_gl_backend_create(void *display);
struct vigs_backend *vigs_sw_backend_create(void);

#endif
