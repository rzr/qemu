#include <GL/gl.h>
#include "yagl_gles_texture.h"
#include "yagl_gles_image.h"
#include "yagl_gles_tex_image.h"
#include "yagl_gles_driver.h"

static bool yagl_gles_texture_unset_internal(struct yagl_gles_texture *texture)
{
    bool ret = false;

    if (texture->image) {
        yagl_gles_image_release(texture->image);
        texture->image = NULL;
        ret = true;
    }

    if (texture->tex_image) {
        texture->tex_image->base.unbind(&texture->tex_image->base);
        assert(!texture->tex_image);
        texture->tex_image = NULL;
        ret = true;
    }

    return ret;
}

static void yagl_gles_texture_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_texture *texture = (struct yagl_gles_texture*)ref;

    if (!yagl_gles_texture_unset_internal(texture)) {
        yagl_ensure_ctx();
        texture->driver->DeleteTextures(1, &texture->global_name);
        yagl_unensure_ctx();
    }

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
    if (texture->target && (texture->target != target)) {
        return false;
    }

    texture->driver->BindTexture(target,
                                 texture->global_name);

    texture->target = target;

    return true;
}

GLenum yagl_gles_texture_get_target(struct yagl_gles_texture *texture)
{
    return texture->target;
}

void yagl_gles_texture_set_image(struct yagl_gles_texture *texture,
                                 struct yagl_gles_image *image)
{
    assert(texture->target);
    assert(image);

    if (texture->image == image) {
        return;
    }

    yagl_gles_image_acquire(image);

    if (!yagl_gles_texture_unset_internal(texture)) {
        texture->driver->DeleteTextures(1, &texture->global_name);
    }

    texture->global_name = image->tex_global_name;
    texture->image = image;

    texture->driver->BindTexture(texture->target,
                                 texture->global_name);
}

void yagl_gles_texture_unset_image(struct yagl_gles_texture *texture)
{
    if (texture->image) {
        GLuint global_name = 0;

        yagl_gles_texture_unset_internal(texture);

        texture->driver->GenTextures(1, &global_name);

        texture->global_name = global_name;

        texture->driver->BindTexture(texture->target,
                                     texture->global_name);
    }
}

void yagl_gles_texture_set_tex_image(struct yagl_gles_texture *texture,
                                     struct yagl_gles_tex_image *tex_image)
{
    assert(texture->target);
    assert(tex_image);

    yagl_gles_tex_image_acquire(tex_image);

    if (!yagl_gles_texture_unset_internal(texture)) {
        texture->driver->DeleteTextures(1, &texture->global_name);
    }

    texture->global_name = tex_image->tex_global_name;
    texture->tex_image = tex_image;

    texture->driver->BindTexture(texture->target,
                                 texture->global_name);
}

void yagl_gles_texture_unset_tex_image(struct yagl_gles_texture *texture)
{
    GLuint global_name = 0;

    assert(texture->tex_image);

    yagl_gles_tex_image_release(texture->tex_image);
    texture->tex_image = NULL;

    /*
     * Should ensure context here since this function
     * can be called when no context is active.
     */
    yagl_ensure_ctx();
    texture->driver->GenTextures(1, &global_name);
    yagl_unensure_ctx();

    texture->global_name = global_name;
}

void yagl_gles_texture_release_tex_image(struct yagl_gles_texture *texture)
{
    if (texture->tex_image) {
        yagl_gles_texture_unset_internal(texture);

        assert(texture->global_name);

        texture->driver->BindTexture(texture->target,
                                     texture->global_name);
    }
}
