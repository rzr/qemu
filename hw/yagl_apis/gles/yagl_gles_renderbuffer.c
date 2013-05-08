#include <GL/gl.h>
#include "yagl_gles_renderbuffer.h"
#include "yagl_gles_driver.h"

static void yagl_gles_renderbuffer_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_renderbuffer *rb = (struct yagl_gles_renderbuffer*)ref;

    yagl_ensure_ctx();
    rb->driver->DeleteRenderbuffers(1, &rb->global_name);
    yagl_unensure_ctx();

    yagl_object_cleanup(&rb->base);

    g_free(rb);
}

struct yagl_gles_renderbuffer
    *yagl_gles_renderbuffer_create(struct yagl_gles_driver *driver)
{
    GLuint global_name = 0;
    struct yagl_gles_renderbuffer *rb;

    driver->GenRenderbuffers(1, &global_name);

    rb = g_malloc0(sizeof(*rb));

    yagl_object_init(&rb->base, &yagl_gles_renderbuffer_destroy);

    rb->driver = driver;
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

void yagl_gles_renderbuffer_set_bound(struct yagl_gles_renderbuffer *rb)
{
    rb->was_bound = true;
}

bool yagl_gles_renderbuffer_was_bound(struct yagl_gles_renderbuffer *rb)
{
    return rb->was_bound;
}
