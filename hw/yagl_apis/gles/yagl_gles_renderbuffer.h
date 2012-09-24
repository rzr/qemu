#ifndef _QEMU_YAGL_GLES_RENDERBUFFER_H
#define _QEMU_YAGL_GLES_RENDERBUFFER_H

#include "yagl_types.h"
#include "yagl_object.h"

#define YAGL_NS_RENDERBUFFER 3

struct yagl_gles_driver_ps;

struct yagl_gles_renderbuffer
{
    struct yagl_object base;

    struct yagl_gles_driver_ps *driver_ps;

    yagl_object_name global_name;

    QemuMutex mutex;

    bool was_bound;
};

struct yagl_gles_renderbuffer
    *yagl_gles_renderbuffer_create(struct yagl_gles_driver_ps *driver_ps);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_renderbuffer_acquire(struct yagl_gles_renderbuffer *rb);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_renderbuffer_release(struct yagl_gles_renderbuffer *rb);

void yagl_gles_renderbuffer_set_bound(struct yagl_gles_renderbuffer *rb);

bool yagl_gles_renderbuffer_was_bound(struct yagl_gles_renderbuffer *rb);

#endif
