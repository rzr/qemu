#include <GL/gl.h>
#include "yagl_gles_framebuffer.h"
#include "yagl_gles_renderbuffer.h"
#include "yagl_gles_texture.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_validate.h"

static void yagl_gles_framebuffer_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_framebuffer *fb = (struct yagl_gles_framebuffer*)ref;

    if (!fb->base.nodelete) {
        fb->driver_ps->DeleteFramebuffers(fb->driver_ps, 1, &fb->global_name);
    }

    qemu_mutex_destroy(&fb->mutex);

    yagl_object_cleanup(&fb->base);

    g_free(fb);
}

struct yagl_gles_framebuffer
    *yagl_gles_framebuffer_create(struct yagl_gles_driver_ps *driver_ps)
{
    GLuint global_name = 0;
    struct yagl_gles_framebuffer *fb;
    int i;

    driver_ps->GenFramebuffers(driver_ps, 1, &global_name);

    fb = g_malloc0(sizeof(*fb));

    yagl_object_init(&fb->base, &yagl_gles_framebuffer_destroy);

    fb->driver_ps = driver_ps;
    fb->global_name = global_name;

    qemu_mutex_init(&fb->mutex);

    for (i = 0; i < YAGL_NUM_GLES_FRAMEBUFFER_ATTACHMENTS; ++i) {
        fb->attachment_states[i].type = GL_NONE;
    }

    return fb;
}

void yagl_gles_framebuffer_acquire(struct yagl_gles_framebuffer *fb)
{
    if (fb) {
        yagl_object_acquire(&fb->base);
    }
}

void yagl_gles_framebuffer_release(struct yagl_gles_framebuffer *fb)
{
    if (fb) {
        yagl_object_release(&fb->base);
    }
}

bool yagl_gles_framebuffer_renderbuffer(struct yagl_gles_framebuffer *fb,
                                        GLenum target,
                                        GLenum attachment,
                                        GLenum renderbuffer_target,
                                        struct yagl_gles_renderbuffer *rb,
                                        yagl_object_name rb_local_name)
{
    yagl_gles_framebuffer_attachment framebuffer_attachment;

    if (!yagl_gles_validate_framebuffer_attachment(attachment,
                                                   &framebuffer_attachment)) {
        return false;
    }

    if (rb && (renderbuffer_target != GL_RENDERBUFFER)) {
        return false;
    }

    qemu_mutex_lock(&fb->mutex);

    if (rb) {
        fb->attachment_states[framebuffer_attachment].type = GL_RENDERBUFFER;
        fb->attachment_states[framebuffer_attachment].local_name = rb_local_name;
    } else {
        fb->attachment_states[framebuffer_attachment].type = GL_NONE;
        fb->attachment_states[framebuffer_attachment].local_name = 0;
    }

    /*
     * Standalone stencil attachments are not supported on modern
     * desktop graphics cards, so we just drop it here. Currently this
     * doesn't do us any harm, but apps in target system that use
     * framebuffers with stencil buffers might not be displayed correctly.
     * If this ever happens we'll have to work around here by create some
     * dummy texture with both color and stencil buffers and copying stencil
     * data from that texture to texture/renderbuffer being attached here.
     */
    if (attachment != GL_STENCIL_ATTACHMENT) {
        fb->driver_ps->FramebufferRenderbuffer(fb->driver_ps,
                                               target,
                                               attachment,
                                               renderbuffer_target,
                                               (rb ? rb->global_name : 0));
    }

    qemu_mutex_unlock(&fb->mutex);

    return true;
}

bool yagl_gles_framebuffer_texture2d(struct yagl_gles_framebuffer *fb,
                                     GLenum target,
                                     GLenum attachment,
                                     GLenum textarget,
                                     GLint level,
                                     struct yagl_gles_texture *texture,
                                     yagl_object_name texture_local_name)
{
    yagl_gles_framebuffer_attachment framebuffer_attachment;
    GLenum squashed_textarget;

    if (!yagl_gles_validate_framebuffer_attachment(attachment,
                                                   &framebuffer_attachment)) {
        return false;
    }

    if (texture && (level != 0)) {
        return false;
    }

    if (!yagl_gles_validate_texture_target_squash(textarget,
                                                  &squashed_textarget)) {
        return false;
    }

    if (texture && (yagl_gles_texture_get_target(texture) != squashed_textarget)) {
        return false;
    }

    qemu_mutex_lock(&fb->mutex);

    if (texture) {
        fb->attachment_states[framebuffer_attachment].type = GL_TEXTURE;
        fb->attachment_states[framebuffer_attachment].local_name = texture_local_name;
    } else {
        fb->attachment_states[framebuffer_attachment].type = GL_NONE;
        fb->attachment_states[framebuffer_attachment].local_name = 0;
    }

    if (attachment != GL_STENCIL_ATTACHMENT) {
        fb->driver_ps->FramebufferTexture2D(fb->driver_ps,
                                            target,
                                            attachment,
                                            textarget,
                                            (texture ? texture->global_name : 0),
                                            level);
    }

    qemu_mutex_unlock(&fb->mutex);

    return true;
}

bool yagl_gles_framebuffer_get_attachment_parameter(struct yagl_gles_framebuffer *fb,
                                                    GLenum attachment,
                                                    GLenum pname,
                                                    GLint *value)
{
    yagl_gles_framebuffer_attachment framebuffer_attachment;

    if (!yagl_gles_validate_framebuffer_attachment(attachment,
                                                   &framebuffer_attachment)) {
        return false;
    }

    qemu_mutex_lock(&fb->mutex);

    switch (pname) {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
        *value = fb->attachment_states[framebuffer_attachment].type;
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
        *value = fb->attachment_states[framebuffer_attachment].local_name;
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        fb->driver_ps->GetFramebufferAttachmentParameteriv(fb->driver_ps,
                                                           fb->global_name,
                                                           attachment,
                                                           pname,
                                                           value);
        break;
    default:
        qemu_mutex_unlock(&fb->mutex);
        return false;
    }

    qemu_mutex_unlock(&fb->mutex);

    return true;
}

void yagl_gles_framebuffer_set_bound(struct yagl_gles_framebuffer *fb)
{
    qemu_mutex_lock(&fb->mutex);

    fb->was_bound = true;

    qemu_mutex_unlock(&fb->mutex);
}

bool yagl_gles_framebuffer_was_bound(struct yagl_gles_framebuffer *fb)
{
    bool ret = false;

    qemu_mutex_lock(&fb->mutex);

    ret = fb->was_bound;

    qemu_mutex_unlock(&fb->mutex);

    return ret;
}
