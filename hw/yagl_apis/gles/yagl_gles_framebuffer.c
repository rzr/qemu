#include <GL/gl.h>
#include "yagl_gles_framebuffer.h"
#include "yagl_gles_renderbuffer.h"
#include "yagl_gles_texture.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_validate.h"

static void yagl_gles_framebuffer_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_framebuffer *fb = (struct yagl_gles_framebuffer*)ref;

    yagl_ensure_ctx();
    fb->driver->DeleteFramebuffers(1, &fb->global_name);
    yagl_unensure_ctx();

    yagl_object_cleanup(&fb->base);

    g_free(fb);
}

struct yagl_gles_framebuffer
    *yagl_gles_framebuffer_create(struct yagl_gles_driver *driver)
{
    GLuint global_name = 0;
    struct yagl_gles_framebuffer *fb;
    int i;

    driver->GenFramebuffers(1, &global_name);

    fb = g_malloc0(sizeof(*fb));

    yagl_object_init(&fb->base, &yagl_gles_framebuffer_destroy);

    fb->driver = driver;
    fb->global_name = global_name;

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

    if (rb) {
        fb->attachment_states[framebuffer_attachment].type = GL_RENDERBUFFER;
        fb->attachment_states[framebuffer_attachment].local_name = rb_local_name;
    } else {
        fb->attachment_states[framebuffer_attachment].type = GL_NONE;
        fb->attachment_states[framebuffer_attachment].local_name = 0;
    }

    fb->driver->FramebufferRenderbuffer(target,
                                        attachment,
                                        renderbuffer_target,
                                        (rb ? rb->global_name : 0));

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

    if (texture) {
        fb->attachment_states[framebuffer_attachment].type = GL_TEXTURE;
        fb->attachment_states[framebuffer_attachment].local_name = texture_local_name;
    } else {
        fb->attachment_states[framebuffer_attachment].type = GL_NONE;
        fb->attachment_states[framebuffer_attachment].local_name = 0;
    }

    fb->driver->FramebufferTexture2D(target,
                                     attachment,
                                     textarget,
                                     (texture ? texture->global_name : 0),
                                     level);

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

    switch (pname) {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
        *value = fb->attachment_states[framebuffer_attachment].type;
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
        *value = fb->attachment_states[framebuffer_attachment].local_name;
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        fb->driver->GetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                                        attachment,
                                                        pname,
                                                        value);
        break;
    default:
        return false;
    }

    return true;
}

void yagl_gles_framebuffer_set_bound(struct yagl_gles_framebuffer *fb)
{
    fb->was_bound = true;
}

bool yagl_gles_framebuffer_was_bound(struct yagl_gles_framebuffer *fb)
{
    return fb->was_bound;
}
