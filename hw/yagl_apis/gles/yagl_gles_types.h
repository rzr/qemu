#ifndef _QEMU_YAGL_GLES_TYPES_H
#define _QEMU_YAGL_GLES_TYPES_H

#include "yagl_types.h"

typedef enum
{
    yagl_gles_texture_target_2d = 0,
    yagl_gles_texture_target_cubemap = 1
} yagl_gles_texture_target;

#define YAGL_NUM_GLES_TEXTURE_TARGETS 2

typedef enum
{
    yagl_gles_framebuffer_attachment_color0 = 0,
    yagl_gles_framebuffer_attachment_depth = 1,
    yagl_gles_framebuffer_attachment_stencil = 2
} yagl_gles_framebuffer_attachment;

#define YAGL_NUM_GLES_FRAMEBUFFER_ATTACHMENTS 3

#endif
