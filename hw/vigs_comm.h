#ifndef _QEMU_VIGS_COMM_H
#define _QEMU_VIGS_COMM_H

#include "vigs_types.h"

struct vigs_comm_ops
{
    bool (*init)(void */*user_data*/);

    bool (*reset)(void */*user_data*/);

    bool (*exit)(void */*user_data*/);

    bool (*create_surface)(void */*user_data*/,
                           uint32_t /*width*/,
                           uint32_t /*height*/,
                           uint32_t /*stride*/,
                           vigsp_surface_format /*format*/,
                           vigsp_offset /*vram_offset*/,
                           vigsp_surface_id */*id*/);

    bool (*destroy_surface)(void */*user_data*/,
                            vigsp_surface_id /*id*/);

    bool (*set_root_surface)(void */*user_data*/,
                             vigsp_surface_id /*id*/);

    bool (*copy)(void */*user_data*/,
                 const struct vigsp_surface */*src*/,
                 const struct vigsp_surface */*dst*/,
                 const struct vigsp_copy */*entries*/,
                 uint32_t /*num_entries*/);

    bool (*solid_fill)(void */*user_data*/,
                       const struct vigsp_surface */*sfc*/,
                       vigsp_color /*color*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    bool (*update_vram)(void */*user_data*/,
                        const struct vigsp_surface */*sfc*/);

    bool (*put_image)(void */*user_data*/,
                      const struct vigsp_surface */*sfc*/,
                      target_ulong /*src_va*/,
                      uint32_t /*src_stride*/,
                      const struct vigsp_rect */*rect*/,
                      bool */*is_pf*/);

    bool (*get_image)(void */*user_data*/,
                      vigsp_surface_id /*sfc_id*/,
                      target_ulong /*dst_va*/,
                      uint32_t /*dst_stride*/,
                      const struct vigsp_rect */*rect*/,
                      bool */*is_pf*/);

    bool (*assign_resource)(void */*user_data*/,
                            vigsp_resource_id /*res_id*/,
                            vigsp_resource_type /*res_type*/,
                            vigsp_surface_id /*sfc_id*/);

    bool (*destroy_resource)(void */*user_data*/,
                             vigsp_resource_id /*res_id*/);
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
