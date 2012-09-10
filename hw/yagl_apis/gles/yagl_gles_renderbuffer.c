#include <GL/gl.h>
#include "yagl_gles_renderbuffer.h"
#include "yagl_gles_driver.h"

static void yagl_gles_renderbuffer_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_renderbuffer *rb = (struct yagl_gles_renderbuffer*)ref;

    if (!rb->base.nodelete) {
        rb->driver_ps->DeleteRenderbuffers(rb->driver_ps, 1, &rb->global_name);
    }

    yagl_object_cleanup(&rb->base);

    g_free(rb);
}

struct yagl_gles_renderbuffer
    *yagl_gles_renderbuffer_create(struct yagl_gles_driver_ps *driver_ps)
{
    GLuint global_name = 0;
    struct yagl_gles_renderbuffer *rb;

    driver_ps->GenRenderbuffers(driver_ps, 1, &global_name);

    rb = g_malloc0(sizeof(*rb));

    yagl_object_init(&rb->base, &yagl_gles_renderbuffer_destroy);

    rb->driver_ps = driver_ps;
    rb->global_name = global_name;

    return rb;
}

void yagl_gles_renderbuffer_acquire(struct yagl_gles_renderbuffer *rb)
{
    if (rb) {
        yagl_object_acquire(&rb->base);
    }
}

void yagl_gles_renderbuffer_release(struct yagl_gles_renderbuffer *rb)
{
    if (rb) {
        yagl_object_release(&rb->base);
    }
}
