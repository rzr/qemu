#ifndef _QEMU_YAGL_GLES_IMAGE_H
#define _QEMU_YAGL_GLES_IMAGE_H

#include "yagl_types.h"
#include "yagl_client_image.h"

struct yagl_gles_driver;

struct yagl_gles_image
{
    struct yagl_client_image base;

    struct yagl_gles_driver *driver;

    yagl_object_name tex_global_name;

    bool is_owner;
};

struct yagl_gles_image
    *yagl_gles_image_create(struct yagl_gles_driver *driver);

struct yagl_gles_image
    *yagl_gles_image_create_from_texture(struct yagl_gles_driver *driver,
                                         yagl_object_name tex_global_name,
                                         struct yagl_ref *tex_data);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_image_acquire(struct yagl_gles_image *image);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_image_release(struct yagl_gles_image *image);

#endif
