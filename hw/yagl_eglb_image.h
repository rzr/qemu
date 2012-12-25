#ifndef _QEMU_YAGL_EGLB_IMAGE_H
#define _QEMU_YAGL_EGLB_IMAGE_H

#include "yagl_types.h"

struct yagl_eglb_display;
struct yagl_client_image;

struct yagl_eglb_image
{
    yagl_winsys_id buffer;

    struct yagl_eglb_display *dpy;

    /*
     * This is GLES part of EGLImageKHR - GLeglImageOES.
     */
    struct yagl_client_image *glegl_image;

    /*
     * Returns 'false' on page fault while reading 'pixels'.
     */
    bool (*update_offscreen)(struct yagl_eglb_image */*image*/,
                             uint32_t /*width*/,
                             uint32_t /*height*/,
                             uint32_t /*bpp*/,
                             target_ulong /*pixels*/);

    void (*destroy)(struct yagl_eglb_image */*image*/);
};

void yagl_eglb_image_init(struct yagl_eglb_image *image,
                          yagl_winsys_id buffer,
                          struct yagl_eglb_display *dpy);
void yagl_eglb_image_cleanup(struct yagl_eglb_image *image);

#endif
