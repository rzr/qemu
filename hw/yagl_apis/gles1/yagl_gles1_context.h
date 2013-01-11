#ifndef YAGL_GLES1_CONTEXT_H_
#define YAGL_GLES1_CONTEXT_H_

#include <GLES/gl.h>
#include "yagl_apis/gles/yagl_gles_context.h"

struct yagl_gles1_driver;
struct yagl_sharegroup;

/* GLES1 has arrays of vertices, normals, colors, texture coordinates and
 * point sizes. Every texture unit has its own texture coordinates array */
typedef enum {
    YAGL_GLES1_ARRAY_VERTEX = 0,
    YAGL_GLES1_ARRAY_COLOR,
    YAGL_GLES1_ARRAY_NORMAL,
    YAGL_GLES1_ARRAY_POINTSIZE,
    YAGL_GLES1_ARRAY_TEX_COORD,
} YaglGles1ArrayType;

typedef struct YaglGles1Context
{
    struct yagl_gles_context base;

    struct yagl_gles1_driver *driver;

    bool prepared;

    int client_active_texture;

    /*
     * From 'base.base.sg' for speed.
     */
    struct yagl_sharegroup *sg;

    /* GL_OES_framebuffer_object */
    bool framebuffer_object;

    /* GL_OES_matrix_palette */
    bool matrix_palette;

    int max_clip_planes;

    int max_lights;

    int max_tex_size;

} YaglGles1Context;

YaglGles1Context *yagl_gles1_context_create(struct yagl_sharegroup *sg,
                                            struct yagl_gles1_driver *driver);


#endif /* YAGL_GLES1_CONTEXT_H_ */
