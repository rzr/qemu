#ifndef _QEMU_YAGL_GLES_TEXTURE_H
#define _QEMU_YAGL_GLES_TEXTURE_H

#include "yagl_types.h"
#include "yagl_object.h"

#define YAGL_NS_TEXTURE 1

struct yagl_gles_driver_ps;

struct yagl_gles_texture
{
    struct yagl_object base;

    struct yagl_gles_driver_ps *driver_ps;

    yagl_object_name global_name;

    QemuMutex mutex;

    GLenum target;
};

struct yagl_gles_texture
    *yagl_gles_texture_create(struct yagl_gles_driver_ps *driver_ps);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_texture_acquire(struct yagl_gles_texture *texture);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_texture_release(struct yagl_gles_texture *texture);

bool yagl_gles_texture_bind(struct yagl_gles_texture *texture,
                            GLenum target);

GLenum yagl_gles_texture_get_target(struct yagl_gles_texture *texture);

#endif
