#ifndef _QEMU_VIGS_COMM_H
#define _QEMU_VIGS_COMM_H

#include "vigs_types.h"

struct vigs_comm_ops
{
    void (*batch_start)(void */*user_data*/);

    bool (*init)(void */*user_data*/);

    void (*reset)(void */*user_data*/);

    void (*exit)(void */*user_data*/);

    void (*create_surface)(void */*user_data*/,
                           uint32_t /*width*/,
                           uint32_t /*height*/,
                           uint32_t /*stride*/,
                           vigsp_surface_format /*format*/,
                           vigsp_surface_id /*id*/);

    void (*destroy_surface)(void */*user_data*/,
                            vigsp_surface_id /*id*/);

    void (*set_root_surface)(void */*user_data*/,
                             vigsp_surface_id /*id*/,
                             vigsp_offset /*offset*/);

    void (*update_vram)(void */*user_data*/,
                        vigsp_surface_id /*sfc_id*/,
                        vigsp_offset /*offset*/);

    void (*update_gpu)(void */*user_data*/,
                       vigsp_surface_id /*sfc_id*/,
                       vigsp_offset /*offset*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*copy)(void */*user_data*/,
                 vigsp_surface_id /*src_id*/,
                 vigsp_surface_id /*dst_id*/,
                 const struct vigsp_copy */*entries*/,
                 uint32_t /*num_entries*/);

    void (*solid_fill)(void */*user_data*/,
                       vigsp_surface_id /*sfc_id*/,
                       vigsp_color /*color*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*batch_end)(void */*user_data*/);
};

struct vigs_comm
{
    uint8_t *ram_ptr;

    struct vigs_comm_ops *comm_ops;

    void *user_data;
};

struct vigs_comm *vigs_comm_create(uint8_t *ram_ptr,
                                   struct vigs_comm_ops *comm_ops,
                                   void *user_data);

void vigs_comm_destroy(struct vigs_comm *comm);

void vigs_comm_dispatch(struct vigs_comm *comm,
                        uint32_t ram_offset);

#endif
