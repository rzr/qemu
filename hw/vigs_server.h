#ifndef _QEMU_VIGS_SERVER_H
#define _QEMU_VIGS_SERVER_H

#include "vigs_types.h"
#include "vigs_vector.h"
#include "winsys.h"
#include <glib.h>

struct vigs_comm;
struct vigs_backend;

struct vigs_display_ops
{
    /*
     * These are only called from 'vigs_server_update_display'.
     * @{
     */

    void (*resize)(void */*user_data*/,
                   uint32_t /*width*/,
                   uint32_t /*height*/);

    uint32_t (*get_stride)(void */*user_data*/);

    /*
     * Returns display's bytes-per-pixel.
     */
    uint32_t (*get_bpp)(void */*user_data*/);

    uint8_t *(*get_data)(void */*user_data*/);

    /*
     * @}
     */
};

struct vigs_server
{
    struct winsys_interface wsi;

    uint8_t *vram_ptr;

    struct vigs_display_ops *display_ops;
    void *display_user_data;

    struct vigs_backend *backend;

    struct vigs_comm *comm;

    /*
     * The following can be modified during
     * server operation.
     * @{
     */

    bool initialized;

    GHashTable *surfaces;

    struct vigs_surface *root_sfc;
    uint8_t *root_sfc_data;

    bool in_batch;

    /*
     * General purpose vectors.
     * @{
     */

    struct vigs_vector v1;

    /*
     * @}
     */

    /*
     * @}
     */
};

struct vigs_server *vigs_server_create(uint8_t *vram_ptr,
                                       uint8_t *ram_ptr,
                                       struct vigs_display_ops *display_ops,
                                       void *display_user_data,
                                       struct vigs_backend *backend);

void vigs_server_destroy(struct vigs_server *server);

void vigs_server_reset(struct vigs_server *server);

void vigs_server_dispatch(struct vigs_server *server,
                          uint32_t ram_offset);

void vigs_server_update_display(struct vigs_server *server);

#endif
