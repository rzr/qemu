#include <GL/gl.h>
#include "yagl_gles_texture.h"
#include "yagl_gles_driver.h"

static void yagl_gles_texture_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_texture *texture = (struct yagl_gles_texture*)ref;

    if (!texture->base.nodelete) {
        texture->driver_ps->DeleteTextures(texture->driver_ps, 1, &texture->global_name);
    }

    qemu_mutex_destroy(&texture->mutex);

    yagl_object_cleanup(&texture->base);

    g_free(texture);
}

struct yagl_gles_texture
    *yagl_gles_texture_create(struct yagl_gles_driver_ps *driver_ps)
{
    GLuint global_name = 0;
    struct yagl_gles_texture *texture;

    driver_ps->GenTextures(driver_ps, 1, &global_name);

    texture = g_malloc0(sizeof(*texture));

    yagl_object_init(&texture->base, &yagl_gles_texture_destroy);

    texture->driver_ps = driver_ps;
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

    texture->driver_ps->BindTexture(texture->driver_ps,
                                    target,
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
