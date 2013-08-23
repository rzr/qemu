#ifndef _QEMU_WINSYS_H
#define _QEMU_WINSYS_H

#include "qemu-common.h"

typedef uint32_t winsys_id;

struct winsys_surface
{
    uint32_t width;
    uint32_t height;

    void (*acquire)(struct winsys_surface */*sfc*/);
    void (*release)(struct winsys_surface */*sfc*/);

    void (*set_dirty)(struct winsys_surface */*sfc*/);
};

struct winsys_info
{
};

struct winsys_interface
{
    struct winsys_info *ws_info;

    /*
     * Acquires surface corresponding to winsys id. NULL if no such surface.
     */
    struct winsys_surface *(*acquire_surface)(struct winsys_interface */*wsi*/,
                                              winsys_id /*id*/);
};

#endif
