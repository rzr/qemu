#include <GL/gl.h>
#include "yagl_gles_validate.h"
#include "yagl_apis/gles2/yagl_gles2_validate.h"

bool yagl_gles_is_buffer_target_valid(GLenum target)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
    case GL_ELEMENT_ARRAY_BUFFER:
        return true;
    default:
        return false;
    }
}

bool yagl_gles_is_buffer_usage_valid(GLenum usage)
{
    switch (usage) {
    case GL_STREAM_DRAW:
    case GL_STATIC_DRAW:
    case GL_DYNAMIC_DRAW:
        return true;
    default:
        return false;
    }
}

bool yagl_gles_get_index_size(GLenum type, int *index_size)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
        *index_size = 1;
        break;
    case GL_UNSIGNED_SHORT:
        *index_size = 2;
        break;
    default:
        return false;
    }
    return true;
}

bool yagl_gles_buffer_target_to_binding(GLenum target, GLenum *binding)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
        *binding = GL_ARRAY_BUFFER_BINDING;
        return true;
    case GL_ELEMENT_ARRAY_BUFFER:
        *binding = GL_ELEMENT_ARRAY_BUFFER_BINDING;
        return true;
    default:
        return false;
    }
}

bool yagl_gles_validate_texture_target(GLenum target,
    yagl_gles_texture_target *texture_target)
{
    switch (target) {
    case GL_TEXTURE_2D:
        *texture_target = yagl_gles_texture_target_2d;
        break;
    case GL_TEXTURE_CUBE_MAP:
        *texture_target = yagl_gles_texture_target_cubemap;
        break;
    default:
        return false;
    }

    return true;
}

bool yagl_gles_validate_framebuffer_attachment(GLenum attachment,
    yagl_gles_framebuffer_attachment *framebuffer_attachment)
{
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        *framebuffer_attachment = yagl_gles_framebuffer_attachment_color0;
        break;
    case GL_DEPTH_ATTACHMENT:
        *framebuffer_attachment = yagl_gles_framebuffer_attachment_depth;
        break;
    case GL_STENCIL_ATTACHMENT:
        *framebuffer_attachment = yagl_gles_framebuffer_attachment_stencil;
        break;
    default:
        return false;
    }

    return true;
}

bool yagl_gles_validate_texture_target_squash(GLenum target,
    GLenum *squashed_target)
{
    switch (target) {
    case GL_TEXTURE_2D:
        *squashed_target = GL_TEXTURE_2D;
        break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        *squashed_target = GL_TEXTURE_CUBE_MAP;
        break;
    default:
        return false;
    }

    return true;
}
