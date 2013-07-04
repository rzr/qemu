#include "yagl_client_tex_image.h"

void yagl_client_tex_image_init(struct yagl_client_tex_image *tex_image,
                                yagl_ref_destroy_func destroy_func,
                                struct yagl_ref *data)
{
    yagl_object_init(&tex_image->base, destroy_func);
    yagl_ref_acquire(data);
    tex_image->data = data;
}

void yagl_client_tex_image_cleanup(struct yagl_client_tex_image *tex_image)
{
    if (tex_image->data) {
        yagl_ref_release(tex_image->data);
    }
    yagl_object_cleanup(&tex_image->base);
}

void yagl_client_tex_image_acquire(struct yagl_client_tex_image *tex_image)
{
    if (tex_image) {
        yagl_object_acquire(&tex_image->base);
    }
}

void yagl_client_tex_image_release(struct yagl_client_tex_image *tex_image)
{
    if (tex_image) {
        yagl_object_release(&tex_image->base);
    }
}
