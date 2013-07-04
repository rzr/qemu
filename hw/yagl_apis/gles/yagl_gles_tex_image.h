#ifndef _QEMU_YAGL_GLES_TEX_IMAGE_H
#define _QEMU_YAGL_GLES_TEX_IMAGE_H

#include "yagl_types.h"
#include "yagl_client_tex_image.h"

struct yagl_gles_driver;
struct yagl_gles_texture;

struct yagl_gles_tex_image
{
    struct yagl_client_tex_image base;

    yagl_object_name tex_global_name;

    /*
     * Weak pointer, no ref.
     */
    struct yagl_gles_texture *bound_texture;
};

/*
 * Does NOT take ownership of 'bound_texture'.
 */
void yagl_gles_tex_image_create(yagl_object_name tex_global_name,
                                struct yagl_ref *tex_data,
                                struct yagl_gles_texture *bound_texture);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_tex_image_acquire(struct yagl_gles_tex_image *tex_image);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_tex_image_release(struct yagl_gles_tex_image *tex_image);

#endif
