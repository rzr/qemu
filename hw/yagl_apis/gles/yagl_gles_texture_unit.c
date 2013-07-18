#include <GLES/gl.h>
#include "yagl_gles_texture_unit.h"
#include "yagl_gles_context.h"
#include "yagl_gles_texture.h"
#include "yagl_sharegroup.h"

void yagl_gles_texture_unit_init(struct yagl_gles_texture_unit *texture_unit,
                                 struct yagl_gles_context *ctx)
{
    memset(texture_unit, 0, sizeof(*texture_unit));

    texture_unit->ctx = ctx;
}

void yagl_gles_texture_unit_cleanup(struct yagl_gles_texture_unit *texture_unit)
{
    int i;

    for (i = 0; i < YAGL_NUM_GLES_TEXTURE_TARGETS; ++i) {
        yagl_gles_texture_release(texture_unit->target_states[i].texture);
        texture_unit->target_states[i].texture = NULL;
        texture_unit->target_states[i].texture_local_name = 0;
    }
}
