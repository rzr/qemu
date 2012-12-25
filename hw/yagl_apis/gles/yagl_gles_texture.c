#include <GL/gl.h>
#include "yagl_gles_texture.h"
#include "yagl_gles_image.h"
#include "yagl_gles_driver.h"

static void yagl_gles_texture_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_texture *texture = (struct yagl_gles_texture*)ref;

    yagl_gles_image_release(texture->image);

    if (!texture->image) {
        yagl_ensure_ctx();
        texture->driver->DeleteTextures(1, &texture->global_name);
        yagl_unensure_ctx();
    }

    qemu_mutex_destroy(&texture->mutex);

    yagl_object_cleanup(&texture->base);

    g_free(texture);
}

struct yagl_gles_texture
    *yagl_gles_texture_create(struct yagl_gles_driver *driver)
{
    GLuint global_name = 0;
    struct yagl_gles_texture *texture;

    driver->GenTextures(1, &global_name);

    texture = g_malloc0(sizeof(*texture));

    yagl_object_init(&texture->base, &yagl_gles_texture_destroy);

    texture->driver = driver;
    texture->global_name = global_name;
    texture->target = 0;

    qemu_mutex_init(&texture->mutex);

    return texture;
}

void yagl_gles_texture_acquire(struct yagl_gles_texture *texture)
{
    if (texture) {
        yagl_object_acquire(&texture->base);
    }
}

void yagl_gles_texture_release(struct yagl_gles_texture *texture)
{
    if (texture) {
        yagl_object_release(&texture->base);
    }
}

bool yagl_gles_texture_bind(struct yagl_gles_texture *texture,
                            GLenum target)
{
    qemu_mutex_lock(&texture->mutex);

    if (texture->target && (texture->target != target)) {
        qemu_mutex_unlock(&texture->mutex);
        return false;
    }

    texture->driver->BindTexture(target,
                                 texture->global_name);

    texture->target = target;

    qemu_mutex_unlock(&texture->mutex);

    return true;
}

GLenum yagl_gles_texture_get_target(struct yagl_gles_texture *texture)
{
    GLenum target;

    qemu_mutex_lock(&texture->mutex);

    target = texture->target;

    qemu_mutex_unlock(&texture->mutex);

    return target;
}

void yagl_gles_texture_set_image(struct yagl_gles_texture *texture,
                                 struct yagl_gles_image *image)
{
    assert(texture->target);
    assert(image);

    qemu_mutex_lock(&texture->mutex);

    if (texture->image == image) {
        qemu_mutex_unlock(&texture->mutex);
        return;
    }

    yagl_gles_image_acquire(image);
    yagl_gles_image_release(texture->image);

    if (!texture->image) {
        texture->driver->DeleteTextures(1, &texture->global_name);
    }

    texture->global_name = image->tex_global_name;
    texture->image = image;

    texture->driver->BindTexture(texture->target,
                                 texture->global_name);

    qemu_mutex_unlock(&texture->mutex);
}

void yagl_gles_texture_unset_image(struct yagl_gles_texture *texture)
{
    qemu_mutex_lock(&texture->mutex);

    if (texture->image) {
        GLuint global_name = 0;

        yagl_gles_image_release(texture->image);
        texture->image = NULL;

        texture->driver->GenTextures(1, &global_name);

        texture->global_name = global_name;

        texture->driver->BindTexture(texture->target,
                                     texture->global_name);
    }

    qemu_mutex_unlock(&texture->mutex);
}
