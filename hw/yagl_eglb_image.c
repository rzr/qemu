#include "yagl_eglb_image.h"
#include "yagl_client_image.h"

void yagl_eglb_image_init(struct yagl_eglb_image *image,
                          struct yagl_eglb_display *dpy)
{
    image->dpy = dpy;
}

void yagl_eglb_image_cleanup(struct yagl_eglb_image *image)
{
    yagl_client_image_release(image->glegl_image);
}
