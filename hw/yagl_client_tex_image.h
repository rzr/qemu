#ifndef _QEMU_YAGL_CLIENT_TEX_IMAGE_H
#define _QEMU_YAGL_CLIENT_TEX_IMAGE_H

#include "yagl_types.h"
#include "yagl_object.h"

struct yagl_client_tex_image
{
    struct yagl_object base;

    struct yagl_ref *data;

    void (*unbind)(struct yagl_client_tex_image */*tex_image*/);
};

void yagl_client_tex_image_init(struct yagl_client_tex_image *tex_image,
                                yagl_ref_destroy_func destroy_func,
                                struct yagl_ref *data);

void yagl_client_tex_image_cleanup(struct yagl_client_tex_image *tex_image);

/*
 * Helper functions that simply acquire/release yagl_client_tex_image::ref
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_client_tex_image_acquire(struct yagl_client_tex_image *tex_image);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_client_tex_image_release(struct yagl_client_tex_image *tex_image);

/*
 * @}
 */

#endif
