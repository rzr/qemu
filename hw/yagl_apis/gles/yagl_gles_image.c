#include <GL/gl.h>
#include "yagl_gles_image.h"
#include "yagl_gles_driver.h"
#include "yagl_host_gles_calls.h"

static void yagl_gles_image_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_image *image = (struct yagl_gles_image*)ref;

    if (image->is_owner) {
        yagl_ensure_ctx();
        image->driver->DeleteTextures(1, &image->tex_global_name);
        yagl_unensure_ctx();
    }

    yagl_client_image_cleanup(&image->base);

    g_free(image);
}

struct yagl_gles_image
    *yagl_gles_image_create(struct yagl_gles_driver *driver)
{
    GLuint tex_global_name = 0;
    struct yagl_gles_image *image;

    driver->GenTextures(1, &tex_global_name);

    image = g_malloc0(sizeof(*image));

    yagl_client_image_init(&image->base, &yagl_gles_image_destroy, NULL);

    image->base.update = &yagl_host_glEGLUpdateOffscreenImageYAGL;

    image->driver = driver;
    image->tex_global_name = tex_global_name;
    image->is_owner = true;

    return image;
}

struct yagl_gles_image
    *yagl_gles_image_create_from_texture(struct yagl_gles_driver *driver,
                                         yagl_object_name tex_global_name,
                                         struct yagl_ref *tex_data)
{
    struct yagl_gles_image *image;

    image = g_malloc0(sizeof(*image));

    yagl_client_image_init(&image->base, &yagl_gles_image_destroy, tex_data);

    image->base.update = &yagl_host_glEGLUpdateOffscreenImageYAGL;

    image->driver = driver;
    image->tex_global_name = tex_global_name;
    image->is_owner = false;

    return image;
}

void yagl_gles_image_acquire(struct yagl_gles_image *image)
{
    if (image) {
        yagl_client_image_acquire(&image->base);
    }
}

void yagl_gles_image_release(struct yagl_gles_image *image)
{
    if (image) {
        yagl_client_image_release(&image->base);
    }
}
