#ifndef _QEMU_VIGS_RESOURCE_H
#define _QEMU_VIGS_RESOURCE_H

#include "vigs_types.h"

struct winsys_resource;
struct vigs_surface;

struct vigs_resource
{
    struct winsys_resource *ws_res;

    vigsp_resource_id id;

    vigsp_resource_type type;

    struct vigs_surface *sfc;
};

struct vigs_resource
    *vigs_resource_create(vigsp_resource_id id,
                          vigsp_resource_type type);

void vigs_resource_assign(struct vigs_resource *res,
                          struct vigs_surface *sfc);

void vigs_resource_destroy(struct vigs_resource *res);

#endif
