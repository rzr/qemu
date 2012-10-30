#include <GLES/gl.h>
#include "yagl_gles_texture_unit.h"
#include "yagl_gles_context.h"

void yagl_gles_texture_unit_init(struct yagl_gles_texture_unit *texture_unit,
                                 struct yagl_gles_context *ctx)
{
    memset(texture_unit, 0, sizeof(*texture_unit));

    texture_unit->ctx = ctx;
}

void yagl_gles_texture_unit_cleanup(struct yagl_gles_texture_unit *texture_unit)
{
}
