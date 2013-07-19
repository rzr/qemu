#include <GL/gl.h>
#include "yagl_gles_tex_image.h"
#include "yagl_gles_texture.h"

static void yagl_gles_tex_image_unbind(struct yagl_client_tex_image *tex_image)
{
    struct yagl_gles_tex_image *gles_tex_image =
        (struct yagl_gles_tex_image*)tex_image;
    struct yagl_ref *data = tex_image->data;

    assert(gles_tex_image->bound_texture);

    yagl_gles_texture_unset_tex_image(gles_tex_image->bound_texture);

    gles_tex_image->tex_global_name = 0;
    gles_tex_image->bound_texture = NULL;

    tex_image->data = NULL;
    yagl_ref_release(data);
}

static void yagl_gles_tex_image_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_tex_image *tex_image = (struct yagl_gles_tex_image*)ref;

    /*
     * Can't get here if bound to texture.
     */
    assert(!tex_image->bound_texture);

    yagl_client_tex_image_cleanup(&tex_image->base);

    g_free(tex_image);
}

void yagl_gles_tex_image_create(yagl_object_name tex_global_name,
                                struct yagl_ref *tex_data,
                                struct yagl_gles_texture *bound_texture)
{
    struct yagl_gles_tex_image *tex_image;

    tex_image = g_malloc0(sizeof(*tex_image));

    yagl_client_tex_image_init(&tex_image->base,
                               &yagl_gles_tex_image_destroy,
                               tex_data);

    tex_image->base.unbind = &yagl_gles_tex_image_unbind;

    tex_image->tex_global_name = tex_global_name;
    tex_image->bound_texture = bound_texture;

    yagl_gles_texture_set_tex_image(bound_texture, tex_image);
}

void yagl_gles_tex_image_acquire(struct yagl_gles_tex_image *tex_image)
{
    if (tex_image) {
        yagl_client_tex_image_acquire(&tex_image->base);
    }
}

void yagl_gles_tex_image_release(struct yagl_gles_tex_image *tex_image)
{
    if (tex_image) {
        yagl_client_tex_image_release(&tex_image->base);
    }
}
