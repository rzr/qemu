#include "yagl_egl_image.h"
#include "yagl_egl_display.h"
#include "yagl_eglb_display.h"
#include "yagl_eglb_image.h"

static void yagl_egl_image_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_image *image = (struct yagl_egl_image*)ref;

    image->backend_image->destroy(image->backend_image);

    yagl_resource_cleanup(&image->res);

    g_free(image);
}

struct yagl_egl_image *yagl_egl_image_create(struct yagl_egl_display *dpy,
                                             yagl_winsys_id buffer)
{
    struct yagl_eglb_image *backend_image;
    struct yagl_egl_image *image;

    backend_image = dpy->backend_dpy->create_image(dpy->backend_dpy, buffer);

    if (!backend_image) {
        return NULL;
    }

    image = g_malloc0(sizeof(*image));

    yagl_resource_init(&image->res, &yagl_egl_image_destroy);

    image->dpy = dpy;
    image->backend_image = backend_image;

    return image;
}

void yagl_egl_image_acquire(struct yagl_egl_image *image)
{
    if (image) {
        yagl_resource_acquire(&image->res);
    }
}

void yagl_egl_image_release(struct yagl_egl_image *image)
{
    if (image) {
        yagl_resource_release(&image->res);
    }
}
