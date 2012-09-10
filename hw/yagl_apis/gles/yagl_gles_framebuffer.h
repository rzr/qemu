#ifndef _QEMU_YAGL_GLES_FRAMEBUFFER_H
#define _QEMU_YAGL_GLES_FRAMEBUFFER_H

#include "yagl_gles_types.h"
#include "yagl_object.h"

#define YAGL_NS_FRAMEBUFFER 2

struct yagl_gles_driver_ps;
struct yagl_gles_texture;
struct yagl_gles_renderbuffer;

struct yagl_gles_framebuffer_attachment_state
{
    GLenum type;

    yagl_object_name local_name;
};

struct yagl_gles_framebuffer
{
    struct yagl_object base;

    struct yagl_gles_driver_ps *driver_ps;

    yagl_object_name global_name;

    QemuMutex mutex;

    struct yagl_gles_framebuffer_attachment_state attachment_states[YAGL_NUM_GLES_FRAMEBUFFER_ATTACHMENTS];
};

struct yagl_gles_framebuffer
    *yagl_gles_framebuffer_create(struct yagl_gles_driver_ps *driver_ps);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_framebuffer_acquire(struct yagl_gles_framebuffer *fb);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_framebuffer_release(struct yagl_gles_framebuffer *fb);

bool yagl_gles_framebuffer_renderbuffer(struct yagl_gles_framebuffer *fb,
                                        GLenum target,
                                        GLenum attachment,
                                        GLenum renderbuffer_target,
                                        struct yagl_gles_renderbuffer *rb,
                                        yagl_object_name rb_local_name);

bool yagl_gles_framebuffer_texture2d(struct yagl_gles_framebuffer *fb,
                                     GLenum target,
                                     GLenum attachment,
                                     GLenum textarget,
                                     GLint level,
                                     struct yagl_gles_texture *texture,
                                     yagl_object_name texture_local_name);

#endif
