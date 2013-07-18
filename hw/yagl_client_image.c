#include "yagl_client_image.h"

void yagl_client_image_init(struct yagl_client_image *image,
                            yagl_ref_destroy_func destroy_func,
                            struct yagl_ref *data)
{
    yagl_object_init(&image->base, destroy_func);
    if (data) {
        yagl_ref_acquire(data);
    }
    image->data = data;
}

void yagl_client_image_cleanup(struct yagl_client_image *image)
{
    if (image->data) {
        yagl_ref_release(image->data);
    }
    yagl_object_cleanup(&image->base);
}

void yagl_client_image_acquire(struct yagl_client_image *image)
{
    if (image) {
        yagl_object_acquire(&image->base);
    }
}

void yagl_client_image_release(struct yagl_client_image *image)
{
    if (image) {
        yagl_object_release(&image->base);
    }
}
