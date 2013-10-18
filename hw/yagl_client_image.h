#ifndef _QEMU_YAGL_CLIENT_IMAGE_H
#define _QEMU_YAGL_CLIENT_IMAGE_H

#include "yagl_types.h"
#include "yagl_object.h"

struct yagl_client_image
{
    struct yagl_object base;

    struct yagl_ref *data;

    void (*update)(struct yagl_client_image */*image*/,
                   uint32_t /*width*/,
                   uint32_t /*height*/,
                   uint32_t /*bpp*/,
                   const void */*pixels*/);
};

void yagl_client_image_init(struct yagl_client_image *image,
                            yagl_ref_destroy_func destroy_func,
                            struct yagl_ref *data /* = NULL*/);

void yagl_client_image_cleanup(struct yagl_client_image *image);

/*
 * Helper functions that simply acquire/release yagl_client_image::ref
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_client_image_acquire(struct yagl_client_image *image);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_client_image_release(struct yagl_client_image *image);

/*
 * @}
 */

#endif
