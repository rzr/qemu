#ifndef _QEMU_YAGL_GLES_TEXTURE_UNIT_H
#define _QEMU_YAGL_GLES_TEXTURE_UNIT_H

#include "yagl_gles_types.h"

struct yagl_gles_context;

struct yagl_gles_texture_target_state
{
    yagl_object_name texture_local_name;

    /*
     * For GLESv1 only. In GLESv2 2D texture and cubemap textures cannot be
     * enabled/disabled.
     */
    bool enabled;
};

struct yagl_gles_texture_unit
{
    /*
     * Owning context.
     */
    struct yagl_gles_context *ctx;

    struct yagl_gles_texture_target_state target_states[YAGL_NUM_GLES_TEXTURE_TARGETS];
};

void yagl_gles_texture_unit_init(struct yagl_gles_texture_unit *texture_unit,
                                 struct yagl_gles_context *ctx);

void yagl_gles_texture_unit_cleanup(struct yagl_gles_texture_unit *texture_unit);

#endif
