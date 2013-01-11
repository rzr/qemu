#ifndef _QEMU_WINSYS_H
#define _QEMU_WINSYS_H

#include "qemu-common.h"

typedef uint32_t winsys_id;

typedef enum
{
    winsys_res_type_window = 0,
    winsys_res_type_pixmap = 1,
} winsys_res_type;

struct winsys_surface
{
    uint32_t width;
    uint32_t height;

    void (*acquire)(struct winsys_surface */*sfc*/);
    void (*release)(struct winsys_surface */*sfc*/);
};

struct winsys_resource;

typedef void (*winsys_resource_cb)(struct winsys_resource */*res*/,
                                   void */*user_data*/);

struct winsys_resource
{
    winsys_id id;

    winsys_res_type type;

    void (*acquire)(struct winsys_resource */*res*/);
    void (*release)(struct winsys_resource */*res*/);

    void *(*add_callback)(struct winsys_resource */*res*/,
                          winsys_resource_cb /*cb*/,
                          void */*user_data*/);

    void (*remove_callback)(struct winsys_resource */*res*/,
                            void */*cookie*/);

    uint32_t (*get_width)(struct winsys_resource */*res*/);
    uint32_t (*get_height)(struct winsys_resource */*res*/);

    struct winsys_surface *(*acquire_surface)(struct winsys_resource */*res*/);
};

struct winsys_info
{
};

struct winsys_interface
{
    struct winsys_info *ws_info;

    /*
     * Acquires resource corresponding to winsys id. NULL if no such resource.
     */
    struct winsys_resource *(*acquire_resource)(struct winsys_interface */*wsi*/,
                                                winsys_id /*id*/);
};

#endif
