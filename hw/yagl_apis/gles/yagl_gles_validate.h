#ifndef _QEMU_YAGL_GLES_VALIDATE_H
#define _QEMU_YAGL_GLES_VALIDATE_H

#include "yagl_gles_types.h"

bool yagl_gles_is_buffer_target_valid(GLenum target);

bool yagl_gles_is_buffer_usage_valid(GLenum usage);

bool yagl_gles_get_index_size(GLenum type, int *index_size);

bool yagl_gles_buffer_target_to_binding(GLenum target, GLenum *binding);

bool yagl_gles_validate_texture_target(GLenum target,
    yagl_gles_texture_target *texture_target);

bool yagl_gles_validate_framebuffer_attachment(GLenum attachment,
    yagl_gles_framebuffer_attachment *framebuffer_attachment);

#endif
