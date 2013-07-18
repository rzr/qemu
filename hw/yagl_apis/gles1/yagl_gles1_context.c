#include <GLES/gl.h>
#include <GLES/glext.h>
#include "yagl_gles1_context.h"
#include "yagl_apis/gles/yagl_gles_array.h"
#include "yagl_apis/gles/yagl_gles_buffer.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_sharegroup.h"
#include "yagl_gles1_driver.h"
#include "exec/user/abitypes.h"

#define YAGL_GLES1_NUM_COMP_TEX_FORMATS    10
#define YAGL_TARGET_INT_MAX                \
    ((1ll << ((sizeof(abi_int) * 8) - 1)) - 1)

static void yagl_gles1_vertex_array_apply(struct yagl_gles_array *array)
{
    struct yagl_gles1_driver *driver = ((YaglGles1Context *)array->ctx)->driver;
    GLenum type = array->type;
    GLsizei stride = array->stride;

    if (array->vbo) {
        yagl_object_name old_buffer_name = 0;
        GLvoid *pointer = (GLvoid *)(uintptr_t)array->offset;

        yagl_gles_buffer_bind(array->vbo,
                              array->type,
                              array->need_convert,
                              GL_ARRAY_BUFFER,
                              &old_buffer_name);

        if (array->type == GL_BYTE) {
            type = GL_SHORT;
            pointer = (GLvoid *)(uintptr_t)(array->offset * sizeof(GLshort));
            stride = array->stride * sizeof(GLshort);
        } else if (array->type == GL_FIXED) {
            type = GL_FLOAT;
        }

        driver->VertexPointer(array->size, type, stride, pointer);

        driver->base.BindBuffer(GL_ARRAY_BUFFER, old_buffer_name);
    } else {
        assert(array->host_data);

        if (array->type == GL_BYTE) {
            type = GL_SHORT;
            stride = 0;
        } else if (array->type == GL_FIXED) {
            type = GL_FLOAT;
        }

        driver->VertexPointer(array->size, type, stride, array->host_data);
    }
}

static void yagl_gles1_normal_array_apply(struct yagl_gles_array *array)
{
    struct yagl_gles1_driver *driver = ((YaglGles1Context *)array->ctx)->driver;
    GLenum type = array->type == GL_FIXED ? GL_FLOAT : array->type;

    if (array->vbo) {
        yagl_object_name old_buffer_name = 0;

        yagl_gles_buffer_bind(array->vbo,
                              array->type,
                              array->need_convert,
                              GL_ARRAY_BUFFER,
                              &old_buffer_name);

        driver->NormalPointer(type,
                              array->stride,
                              (GLvoid *)(uintptr_t)array->offset);

        driver->base.BindBuffer(GL_ARRAY_BUFFER, old_buffer_name);
    } else {
        assert(array->host_data);

        driver->NormalPointer(type,
                              array->stride,
                              array->host_data);
    }
}

static void yagl_gles1_color_array_apply(struct yagl_gles_array *array)
{
    struct yagl_gles1_driver *driver = ((YaglGles1Context *)array->ctx)->driver;
    GLenum type = array->type == GL_FIXED ? GL_FLOAT : array->type;

    if (array->vbo) {
        yagl_object_name old_buffer_name = 0;

        yagl_gles_buffer_bind(array->vbo,
                              array->type,
                              array->need_convert,
                              GL_ARRAY_BUFFER,
                              &old_buffer_name);

        driver->ColorPointer(array->size,
                             type,
                             array->stride,
                             (GLvoid *)(uintptr_t)array->offset);

        driver->base.BindBuffer(GL_ARRAY_BUFFER, old_buffer_name);
    } else {
        assert(array->host_data);

        driver->ColorPointer(array->size,
                             type,
                             array->stride,
                             array->host_data);
    }
}

static void yagl_gles1_tex_coord_array_apply(struct yagl_gles_array *array)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)array->ctx;
    struct yagl_gles1_driver *driver = gles1_ctx->driver;
    GLenum type = array->type;
    GLsizei stride = array->stride;
    int tex_id = array->index - YAGL_GLES1_ARRAY_TEX_COORD;

    if (tex_id != gles1_ctx->client_active_texture) {
        driver->ClientActiveTexture(tex_id + GL_TEXTURE0);
    }

    if (array->vbo) {
        yagl_object_name old_buffer_name = 0;
        GLvoid *pointer = (GLvoid *)(uintptr_t)array->offset;

        yagl_gles_buffer_bind(array->vbo,
                              array->type,
                              array->need_convert,
                              GL_ARRAY_BUFFER,
                              &old_buffer_name);

        if (array->type == GL_BYTE) {
            type = GL_SHORT;
            pointer = (GLvoid *)(uintptr_t)(array->offset * sizeof(GLshort));
            stride = array->stride * sizeof(GLshort);
        } else if (array->type == GL_FIXED) {
            type = GL_FLOAT;
        }

        driver->TexCoordPointer(array->size, type, stride, pointer);

        driver->base.BindBuffer(GL_ARRAY_BUFFER, old_buffer_name);
    } else {
        assert(array->host_data);

        if (array->type == GL_BYTE) {
            type = GL_SHORT;
            stride = 0;
        } else if (array->type == GL_FIXED) {
            type = GL_FLOAT;
        }

        driver->TexCoordPointer(array->size, type, stride, array->host_data);
    }

    if (tex_id != gles1_ctx->client_active_texture) {
        driver->ClientActiveTexture(gles1_ctx->client_active_texture +
                                    GL_TEXTURE0);
    }
}

static void yagl_gles1_point_size_array_apply(struct yagl_gles_array *array)
{

}

static void yagl_gles1_context_prepare(YaglGles1Context *gles1_ctx)
{
    struct yagl_gles_driver *gles_driver = &gles1_ctx->driver->base;
    GLint i, num_texture_units = 0;
    struct yagl_gles_array *arrays;
    const gchar *extns = NULL;
    int num_arrays;

    YAGL_LOG_FUNC_ENTER(yagl_gles2_context_prepare, "%p", gles1_ctx);

    gles_driver->GetIntegerv(GL_MAX_TEXTURE_UNITS, &num_texture_units);

    /*
     * We limit this by 32 for conformance.
     */
    if (num_texture_units > 32) {
        num_texture_units = 32;
    }

    /* Each texture unit has its own client-side array state */
    num_arrays = YAGL_GLES1_ARRAY_TEX_COORD + num_texture_units;

    arrays = g_new(struct yagl_gles_array, num_arrays);

    yagl_gles_array_init(&arrays[YAGL_GLES1_ARRAY_VERTEX],
                         YAGL_GLES1_ARRAY_VERTEX,
                         &gles1_ctx->base,
                         &yagl_gles1_vertex_array_apply);

    yagl_gles_array_init(&arrays[YAGL_GLES1_ARRAY_COLOR],
                         YAGL_GLES1_ARRAY_COLOR,
                         &gles1_ctx->base,
                         &yagl_gles1_color_array_apply);

    yagl_gles_array_init(&arrays[YAGL_GLES1_ARRAY_NORMAL],
                         YAGL_GLES1_ARRAY_NORMAL,
                         &gles1_ctx->base,
                         &yagl_gles1_normal_array_apply);

    yagl_gles_array_init(&arrays[YAGL_GLES1_ARRAY_POINTSIZE],
                         YAGL_GLES1_ARRAY_POINTSIZE,
                         &gles1_ctx->base,
                         &yagl_gles1_point_size_array_apply);

    for (i = YAGL_GLES1_ARRAY_TEX_COORD; i < num_arrays; ++i) {
        yagl_gles_array_init(&arrays[i],
                             i,
                             &gles1_ctx->base,
                             &yagl_gles1_tex_coord_array_apply);
    }

    yagl_gles_context_prepare(&gles1_ctx->base, arrays,
                              num_arrays, num_texture_units);

    gles_driver->GetIntegerv(GL_MAX_CLIP_PLANES, &gles1_ctx->max_clip_planes);

    if (gles1_ctx->max_clip_planes < 6) {
        YAGL_LOG_WARN("host GL_MAX_CLIP_PLANES=%d is less then required 6",
            gles1_ctx->max_clip_planes);
    } else {
        /* According to OpenGLES 1.1 docs on khrnos website we only need
         * to support 6 planes. This will protect us from bogus
         * GL_MAX_CLIP_PLANES value reported by some drivers */
        gles1_ctx->max_clip_planes = 6;
    }

    gles_driver->GetIntegerv(GL_MAX_LIGHTS, &gles1_ctx->max_lights);

    gles_driver->GetIntegerv(GL_MAX_TEXTURE_SIZE, &gles1_ctx->max_tex_size);

    extns = (const gchar *)gles_driver->GetString(GL_EXTENSIONS);

    gles1_ctx->framebuffer_object =
        (g_strstr_len(extns, -1, "GL_EXT_framebuffer_object ") != NULL) ||
        (g_strstr_len(extns, -1, "GL_ARB_framebuffer_object ") != NULL);

    gles1_ctx->matrix_palette =
        (g_strstr_len(extns, -1, "GL_ARB_vertex_blend ") != NULL) &&
        (g_strstr_len(extns, -1, "GL_ARB_matrix_palette ") != NULL);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles1_context_activate(struct yagl_client_context *ctx)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)ctx;

    if (!gles1_ctx->prepared) {
        yagl_gles1_context_prepare(gles1_ctx);
        gles1_ctx->prepared = true;
    }

    yagl_gles_context_activate(&gles1_ctx->base);
}

static void yagl_gles1_context_deactivate(struct yagl_client_context *ctx)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)ctx;

    yagl_gles_context_deactivate(&gles1_ctx->base);
}

static void yagl_gles1_context_destroy(struct yagl_client_context *ctx)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)ctx;

    YAGL_LOG_FUNC_ENTER(gles1_ctx->driver_ps->common->ps->id,
                        0,
                        yagl_gles1_context_destroy,
                        "%p",
                        gles1_ctx);

    yagl_gles_context_cleanup(&gles1_ctx->base);
    yagl_client_context_cleanup(&gles1_ctx->base.base);

    g_free(gles1_ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static bool yagl_gles1_context_get_param_count(struct yagl_gles_context *ctx,
                                               GLenum pname,
                                               int *count)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)ctx;

    switch (pname) {
    case GL_COMPRESSED_TEXTURE_FORMATS:
        *count = YAGL_GLES1_NUM_COMP_TEX_FORMATS;
        break;
    case GL_ACTIVE_TEXTURE:
    case GL_ALPHA_BITS:
    case GL_ALPHA_TEST:
    case GL_ALPHA_TEST_FUNC:
    case GL_ALPHA_TEST_REF:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_BLEND:
    case GL_BLEND_DST:
    case GL_BLEND_SRC:
    case GL_BLUE_BITS:
    case GL_CLIENT_ACTIVE_TEXTURE:
    case GL_COLOR_ARRAY:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_SIZE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_TYPE:
    case GL_COLOR_LOGIC_OP:
    case GL_COLOR_MATERIAL:
    case GL_CULL_FACE:
    case GL_CULL_FACE_MODE:
    case GL_DEPTH_BITS:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_DEPTH_FUNC:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_FOG:
    case GL_FOG_DENSITY:
    case GL_FOG_END:
    case GL_FOG_HINT:
    case GL_FOG_MODE:
    case GL_FOG_START:
    case GL_FRONT_FACE:
    case GL_GENERATE_MIPMAP_HINT:
    case GL_GREEN_BITS:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_LIGHTING:
    case GL_LINE_SMOOTH:
    case GL_LINE_SMOOTH_HINT:
    case GL_LINE_WIDTH:
    case GL_LOGIC_OP_MODE:
    case GL_MATRIX_MODE:
    case GL_MAX_CLIP_PLANES:
    case GL_MAX_LIGHTS:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MAX_TEXTURE_UNITS:
    case GL_MODELVIEW_STACK_DEPTH:
    case GL_MULTISAMPLE:
    case GL_NORMAL_ARRAY:
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_STRIDE:
    case GL_NORMAL_ARRAY_TYPE:
    case GL_NORMALIZE:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_PACK_ALIGNMENT:
    case GL_PERSPECTIVE_CORRECTION_HINT:
    case GL_POINT_FADE_THRESHOLD_SIZE:
    case GL_POINT_SIZE:
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
    case GL_POINT_SIZE_ARRAY_OES:
    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
    case GL_POINT_SIZE_ARRAY_TYPE_OES:
    case GL_POINT_SIZE_MAX:
    case GL_POINT_SIZE_MIN:
    case GL_POINT_SMOOTH:
    case GL_POINT_SMOOTH_HINT:
    case GL_POINT_SPRITE_OES:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_FILL:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_PROJECTION_STACK_DEPTH:
    case GL_RED_BITS:
    case GL_RESCALE_NORMAL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_ALPHA_TO_ONE:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLE_COVERAGE:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_SAMPLES:
    case GL_SCISSOR_TEST:
    case GL_SHADE_MODEL:
    case GL_STENCIL_BITS:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_TEST:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK:
    case GL_SUBPIXEL_BITS:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_TEXTURE_STACK_DEPTH:
    case GL_UNPACK_ALIGNMENT:
    case GL_VERTEX_ARRAY:
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ARRAY_SIZE:
    case GL_VERTEX_ARRAY_STRIDE:
    case GL_VERTEX_ARRAY_TYPE:
    /* GL_OES_blend_equation_separate */
    case GL_BLEND_EQUATION_RGB_OES:
    case GL_BLEND_EQUATION_ALPHA_OES:
    /* OES_blend_func_separate */
    case GL_BLEND_DST_RGB_OES:
    case GL_BLEND_SRC_RGB_OES:
    case GL_BLEND_DST_ALPHA_OES:
    case GL_BLEND_SRC_ALPHA_OES:
        *count = 1;
        break;
    case GL_DEPTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
        *count = 2;
        break;
    case GL_CURRENT_NORMAL:
    case GL_POINT_DISTANCE_ATTENUATION:
        *count = 3;
        break;
    case GL_COLOR_WRITEMASK:
    case GL_CURRENT_COLOR:
    case GL_FOG_COLOR:
    case GL_COLOR_CLEAR_VALUE:
    case GL_LIGHT_MODEL_AMBIENT:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT:
        *count = 4;
        break;
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
        *count = 16;
        break;
    /* GL_OES_framebuffer_object */
    case GL_FRAMEBUFFER_BINDING_OES:
    case GL_RENDERBUFFER_BINDING_OES:
    case GL_MAX_RENDERBUFFER_SIZE_OES:
        if (!gles1_ctx->framebuffer_object) {
            return false;
        }
        *count = 1;
        break;
    /* GL_OES_matrix_palette */
    case GL_MAX_PALETTE_MATRICES_OES:
    case GL_MAX_VERTEX_UNITS_OES:
    case GL_CURRENT_PALETTE_MATRIX_OES:
    case GL_MATRIX_INDEX_ARRAY_SIZE_OES:
    case GL_MATRIX_INDEX_ARRAY_TYPE_OES:
    case GL_MATRIX_INDEX_ARRAY_STRIDE_OES:
    case GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES:
    case GL_WEIGHT_ARRAY_SIZE_OES:
    case GL_WEIGHT_ARRAY_TYPE_OES:
    case GL_WEIGHT_ARRAY_STRIDE_OES:
    case GL_WEIGHT_ARRAY_BUFFER_BINDING_OES:
        if (!gles1_ctx->matrix_palette) {
            return false;
        }
        *count = 1;
        break;
    default:
        if ((pname >= GL_CLIP_PLANE0 &&
             pname < (GL_CLIP_PLANE0 + gles1_ctx->max_clip_planes - 1)) ||
            (pname >= GL_LIGHT0 &&
             pname < (GL_LIGHT0 + gles1_ctx->max_lights - 1))) {
            *count = 1;
            break;
        }
        return false;
    }

    return true;
}

static unsigned yagl_gles1_array_idx_from_pname(YaglGles1Context *ctx,
                                                GLenum pname)
{
    switch (pname) {
    case GL_VERTEX_ARRAY:
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ARRAY_SIZE:
    case GL_VERTEX_ARRAY_STRIDE:
    case GL_VERTEX_ARRAY_TYPE:
        return YAGL_GLES1_ARRAY_VERTEX;
    case GL_COLOR_ARRAY:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_SIZE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_TYPE:
        return YAGL_GLES1_ARRAY_COLOR;
    case GL_NORMAL_ARRAY:
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_STRIDE:
    case GL_NORMAL_ARRAY_TYPE:
        return YAGL_GLES1_ARRAY_NORMAL;
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
        return YAGL_GLES1_ARRAY_TEX_COORD + ctx->client_active_texture;
    case GL_POINT_SIZE_ARRAY_TYPE_OES:
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
    case GL_POINT_SIZE_ARRAY_OES:
    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
        return YAGL_GLES1_ARRAY_POINTSIZE;
    default:
        exit(1);
    }
}

static void yagl_gles1_compressed_texture_formats_fill(GLint *params)
{
    params[0] = GL_PALETTE4_RGB8_OES;
    params[1] = GL_PALETTE4_RGBA8_OES;
    params[2] = GL_PALETTE4_R5_G6_B5_OES;
    params[3] = GL_PALETTE4_RGBA4_OES;
    params[4] = GL_PALETTE4_RGB5_A1_OES;
    params[5] = GL_PALETTE8_RGB8_OES;
    params[6] = GL_PALETTE8_RGBA8_OES;
    params[7] = GL_PALETTE8_R5_G6_B5_OES;
    params[8] = GL_PALETTE8_RGBA4_OES;
    params[9] = GL_PALETTE8_RGB5_A1_OES;
}

static bool yagl_gles1_context_get_integerv(struct yagl_gles_context *ctx,
                                            GLenum pname,
                                            GLint *params)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)ctx;

    switch (pname) {
    case GL_MAX_CLIP_PLANES:
        params[0] = gles1_ctx->max_clip_planes;
        break;
    case GL_MAX_LIGHTS:
        params[0] = gles1_ctx->max_lights;
        break;
    case GL_MAX_TEXTURE_SIZE:
        params[0] = gles1_ctx->max_tex_size;
        break;
    case GL_VERTEX_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_POINT_SIZE_ARRAY_OES:
        params[0] = ctx->arrays[yagl_gles1_array_idx_from_pname(gles1_ctx, pname)].enabled;
        break;
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
        params[0] = ctx->arrays[yagl_gles1_array_idx_from_pname(gles1_ctx, pname)].vbo_local_name;
        break;
    case GL_VERTEX_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_NORMAL_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
        params[0] = ctx->arrays[yagl_gles1_array_idx_from_pname(gles1_ctx, pname)].stride;
        break;
    case GL_VERTEX_ARRAY_TYPE:
    case GL_COLOR_ARRAY_TYPE:
    case GL_NORMAL_ARRAY_TYPE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_POINT_SIZE_ARRAY_TYPE_OES:
        params[0] = ctx->arrays[yagl_gles1_array_idx_from_pname(gles1_ctx, pname)].type;
        break;
    case GL_VERTEX_ARRAY_SIZE:
    case GL_COLOR_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
        params[0] = ctx->arrays[yagl_gles1_array_idx_from_pname(gles1_ctx, pname)].size;
        break;
    case GL_FRAMEBUFFER_BINDING_OES:
        params[0] = ctx->fbo_local_name;
        break;
    case GL_RENDERBUFFER_BINDING_OES:
        params[0] = ctx->rbo_local_name;
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        params[0] = YAGL_GLES1_NUM_COMP_TEX_FORMATS;
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        yagl_gles1_compressed_texture_formats_fill(params);
        break;
    case GL_ALPHA_TEST_REF:
    {
        /* According to spec, GL_ALPHA_TEST_REF must be scaled to
         * -INT_MAX..+INT_MAX range, but driver might not do it, we do
         * it manually here */
        GLfloat tmp;

        ctx->driver->GetFloatv(GL_ALPHA_TEST_REF, &tmp);
        params[0] = (GLint)(tmp * (GLfloat)YAGL_TARGET_INT_MAX) - 1;
        break;
    }
    default:
        return false;
    }

    return true;
}

static bool yagl_gles1_context_get_booleanv(struct yagl_gles_context *ctx,
                                            GLenum pname,
                                            GLboolean *params)
{
    if (pname == GL_COMPRESSED_TEXTURE_FORMATS) {
        GLint tmp[YAGL_GLES1_NUM_COMP_TEX_FORMATS];
        int i;

        if (yagl_gles1_context_get_integerv(ctx, pname, &tmp[0])) {
            for (i = 0; i < YAGL_GLES1_NUM_COMP_TEX_FORMATS; ++i) {
                params[i] = (tmp[i] != 0) ? GL_TRUE : GL_FALSE;
            }
            return true;
        }
    } else {
        GLint tmp;

        if (yagl_gles1_context_get_integerv(ctx, pname, &tmp)) {
            params[0] = ((tmp != 0) ? GL_TRUE : GL_FALSE);
            return true;
        }
    }

    return false;
}

static bool yagl_gles1_context_get_floatv(struct yagl_gles_context *ctx,
                                          GLenum pname,
                                          GLfloat *params)
{
    if (pname == GL_COMPRESSED_TEXTURE_FORMATS) {
        GLint tmp[YAGL_GLES1_NUM_COMP_TEX_FORMATS];
        int i;

        if (yagl_gles1_context_get_integerv(ctx, pname, &tmp[0])) {
            for (i = 0; i < YAGL_GLES1_NUM_COMP_TEX_FORMATS; ++i) {
                params[i] = (GLfloat)tmp[i];
            }
            return true;
        }
    } else {
        GLint tmp;

        if (yagl_gles1_context_get_integerv(ctx, pname, &tmp)) {
            params[0] = (GLfloat)tmp;
            return true;
        }
    }

    return false;
}

static bool yagl_gles1_context_is_enabled(struct yagl_gles_context *ctx,
                                          GLboolean* retval,
                                          GLenum cap)
{
    if (cap == GL_POINT_SIZE_ARRAY_OES) {
        *retval = ctx->arrays[YAGL_GLES1_ARRAY_POINTSIZE].enabled;
        return true;
    }

    return false;
}

static GLchar *yagl_gles1_context_get_extensions(struct yagl_gles_context *ctx)
{
    YaglGles1Context *gles1_ctx = (YaglGles1Context *)ctx;

    const GLchar *default_ext =
        "GL_OES_blend_subtract GL_OES_blend_equation_separate "
        "GL_OES_blend_func_separate GL_OES_element_index_uint "
        "GL_OES_texture_mirrored_repeat "
        "GL_EXT_texture_format_BGRA8888 GL_OES_point_sprite "
        "GL_OES_point_size_array GL_OES_stencil_wrap "
        "GL_OES_compressed_paletted_texture "
        "GL_OES_depth_texture ";
    const GLchar *framebuffer_object_ext =
        "GL_OES_framebuffer_object GL_OES_depth24 GL_OES_depth32 "
        "GL_OES_rgb8_rgba8 GL_OES_stencil1 GL_OES_stencil4 "
        "GL_OES_stencil8 GL_OES_EGL_image ";
    const GLchar *pack_depth_stencil = "GL_OES_packed_depth_stencil ";
    const GLchar *texture_npot = "GL_OES_texture_npot ";
    const GLchar *texture_filter_anisotropic = "GL_EXT_texture_filter_anisotropic ";
    const GLchar *matrix_palette = "GL_OES_matrix_palette ";

    size_t len = strlen(default_ext);
    GLchar *str;

    if (gles1_ctx->base.texture_npot) {
        len += strlen(texture_npot);
    }

    if (gles1_ctx->base.texture_filter_anisotropic) {
        len += strlen(texture_filter_anisotropic);
    }

    if (gles1_ctx->framebuffer_object) {
        len += strlen(framebuffer_object_ext);

        if (gles1_ctx->base.pack_depth_stencil) {
            len += strlen(pack_depth_stencil);
        }
    }

    if (gles1_ctx->matrix_palette) {
        len += strlen(matrix_palette);
    }

    str = g_malloc0(len + 1);

    g_strlcpy(str, default_ext, len + 1);

    if (gles1_ctx->base.texture_npot) {
        g_strlcat(str, texture_npot, len + 1);
    }

    if (gles1_ctx->base.texture_filter_anisotropic) {
        g_strlcat(str, texture_filter_anisotropic, len + 1);
    }

    if (gles1_ctx->framebuffer_object) {
        g_strlcat(str, framebuffer_object_ext, len + 1);

        if (gles1_ctx->base.pack_depth_stencil) {
            g_strlcat(str, pack_depth_stencil, len + 1);
        }
    }

    if (gles1_ctx->matrix_palette) {
        g_strlcat(str, matrix_palette, len + 1);
    }

    return str;
}

static void yagl_gles1_draw_arrays_psize(struct yagl_gles_context *ctx,
                                         GLint first,
                                         GLsizei count)
{
    struct yagl_gles1_driver *gles1_driver = ((YaglGles1Context *)ctx)->driver;
    struct yagl_gles_array *parray = &ctx->arrays[YAGL_GLES1_ARRAY_POINTSIZE];
    unsigned i = 0;
    const unsigned stride = parray->stride;
    GLsizei points_cnt;
    GLint arr_offset;
    void *next_psize_p;
    GLfloat cur_psize;

    if (parray->vbo) {
        next_psize_p = parray->vbo->data + parray->offset + first * stride;
    } else {
        next_psize_p = parray->host_data + first * stride;
    }

    while (i < count) {
        points_cnt = 0;
        arr_offset = i;
        cur_psize = *((GLfloat *)next_psize_p);

        do {
            ++points_cnt;
            ++i;
            next_psize_p += stride;
        } while (i < count && cur_psize == *((GLfloat *)next_psize_p));

        gles1_driver->PointSize(cur_psize);

        ctx->driver->DrawArrays(GL_POINTS, first + arr_offset, points_cnt);
    }
}

static inline void *yagl_get_next_psize_p(struct yagl_gles_buffer *ebo,
                                          struct yagl_gles_array *parray,
                                          GLenum type,
                                          unsigned idx,
                                          const GLvoid *indices)
{
    unsigned idx_val;

    if (ebo) {
        if (type == GL_UNSIGNED_SHORT) {
            idx_val = ((uint16_t *)(ebo->data + (uintptr_t)indices))[idx];
        } else {
            idx_val = ((uint8_t *)(ebo->data + (uintptr_t)indices))[idx];
        }
    } else {
        if (type == GL_UNSIGNED_SHORT) {
            idx_val = ((uint16_t *)indices)[idx];
        } else {
            idx_val = ((uint8_t *)indices)[idx];
        }
    }

    if (parray->vbo) {
        return parray->vbo->data + parray->offset + idx_val * parray->stride;
    } else {
        return parray->host_data + idx_val * parray->stride;
    }
}

static void yagl_gles1_draw_elem_psize(struct yagl_gles_context *ctx,
                                       GLsizei count,
                                       GLenum type,
                                       const GLvoid *indices)
{
    struct yagl_gles1_driver *gles1_driver = ((YaglGles1Context *)ctx)->driver;
    struct yagl_gles_array *parray = &ctx->arrays[YAGL_GLES1_ARRAY_POINTSIZE];
    unsigned i = 0, el_size;
    GLsizei points_cnt;
    GLint arr_offset;
    GLfloat cur_psize;
    void *next_psize_p;

    switch (type) {
    case GL_UNSIGNED_BYTE:
        el_size = 1;
        break;
    case GL_UNSIGNED_SHORT:
        el_size = 2;
        break;
    default:
        el_size = 0;
        break;
    }

    assert(el_size > 0);

    next_psize_p = yagl_get_next_psize_p(ctx->ebo, parray, type, i, indices);

    while (i < count) {
        points_cnt = 0;
        arr_offset = i;
        cur_psize = *((GLfloat *)next_psize_p);

        do {
            ++points_cnt;
            ++i;
            next_psize_p = yagl_get_next_psize_p(ctx->ebo,
                                                 parray,
                                                 type,
                                                 i,
                                                 indices);
        } while (i < count && cur_psize == *((GLfloat *)next_psize_p));

        gles1_driver->PointSize(cur_psize);

        ctx->driver->DrawElements(GL_POINTS,
                                  points_cnt,
                                  type,
                                  indices + arr_offset * el_size);
    }
}

static void yagl_gles1_context_draw_arrays(struct yagl_gles_context *ctx,
                                           GLenum mode,
                                           GLint first,
                                           GLsizei count)
{
    if (!ctx->arrays[YAGL_GLES1_ARRAY_VERTEX].enabled) {
        return;
    }

    if (mode == GL_POINTS && ctx->arrays[YAGL_GLES1_ARRAY_POINTSIZE].enabled) {
        yagl_gles1_draw_arrays_psize(ctx, first, count);
    } else {
        ctx->driver->DrawArrays(mode, first, count);
    }
}

static void yagl_gles1_context_draw_elements(struct yagl_gles_context *ctx,
                                             GLenum mode,
                                             GLsizei count,
                                             GLenum type,
                                             const GLvoid *indices)
{
    if (!ctx->arrays[YAGL_GLES1_ARRAY_VERTEX].enabled) {
        return;
    }

    if (mode == GL_POINTS && ctx->arrays[YAGL_GLES1_ARRAY_POINTSIZE].enabled) {
        yagl_gles1_draw_elem_psize(ctx, count, type, indices);
    } else {
        ctx->driver->DrawElements(mode, count, type, indices);
    }
}

typedef struct YaglGles1PalFmtDesc {
    GLenum uncomp_format;
    GLenum pixel_type;
    unsigned pixel_size;
    unsigned bits_per_index;
} YaglGles1PalFmtDesc;

static inline int yagl_log2(int val)
{
   int ret = 0;

   if (val > 0) {
       while (val >>= 1) {
           ret++;
       }
   }

   return ret;
}

static inline bool yagl_gles1_tex_dims_valid(GLsizei width,
                                             GLsizei height,
                                             int max_size)
{
    if (width < 0 || height < 0 || width > max_size || height > max_size ||
        (width & (width - 1)) || (height & (height - 1))) {
        return false;
    }

    return true;
}

static void yagl_gles1_cpal_format_get_descr(GLenum format,
                                             YaglGles1PalFmtDesc *desc)
{
    assert(format >= GL_PALETTE4_RGB8_OES && format <= GL_PALETTE8_RGB5_A1_OES);

    switch (format) {
    case GL_PALETTE4_RGB8_OES:
        desc->uncomp_format = GL_RGB;
        desc->bits_per_index = 4;
        desc->pixel_type = GL_UNSIGNED_BYTE;
        desc->pixel_size = 3;
        break;
    case GL_PALETTE4_RGBA8_OES:
        desc->uncomp_format = GL_RGBA;
        desc->bits_per_index = 4;
        desc->pixel_type = GL_UNSIGNED_BYTE;
        desc->pixel_size = 4;
        break;
    case GL_PALETTE4_R5_G6_B5_OES:
        desc->uncomp_format = GL_RGB;
        desc->bits_per_index = 4;
        desc->pixel_type = GL_UNSIGNED_SHORT_5_6_5;
        desc->pixel_size = 2;
        break;
    case GL_PALETTE4_RGBA4_OES:
        desc->uncomp_format = GL_RGBA;
        desc->bits_per_index = 4;
        desc->pixel_type = GL_UNSIGNED_SHORT_4_4_4_4;
        desc->pixel_size = 2;
        break;
    case GL_PALETTE4_RGB5_A1_OES:
        desc->uncomp_format = GL_RGBA;
        desc->bits_per_index = 4;
        desc->pixel_type = GL_UNSIGNED_SHORT_5_5_5_1;
        desc->pixel_size = 2;
        break;
    case GL_PALETTE8_RGB8_OES:
        desc->uncomp_format = GL_RGB;
        desc->bits_per_index = 8;
        desc->pixel_type = GL_UNSIGNED_BYTE;
        desc->pixel_size = 3;
        break;
    case GL_PALETTE8_RGBA8_OES:
        desc->uncomp_format = GL_RGBA;
        desc->bits_per_index = 8;
        desc->pixel_type = GL_UNSIGNED_BYTE;
        desc->pixel_size = 4;
        break;
    case GL_PALETTE8_R5_G6_B5_OES:
        desc->uncomp_format = GL_RGB;
        desc->bits_per_index = 8;
        desc->pixel_type = GL_UNSIGNED_SHORT_5_6_5;
        desc->pixel_size = 2;
        break;
    case GL_PALETTE8_RGBA4_OES:
        desc->uncomp_format = GL_RGBA;
        desc->bits_per_index = 8;
        desc->pixel_type = GL_UNSIGNED_SHORT_4_4_4_4;
        desc->pixel_size = 2;
        break;
    case GL_PALETTE8_RGB5_A1_OES:
        desc->uncomp_format = GL_RGBA;
        desc->bits_per_index = 8;
        desc->pixel_type = GL_UNSIGNED_SHORT_5_5_5_1;
        desc->pixel_size = 2;
        break;
    }
}

static GLsizei yagl_gles1_cpal_tex_size(YaglGles1PalFmtDesc *fmt_desc,
                                        unsigned width,
                                        unsigned height,
                                        unsigned max_level)
{
    GLsizei size;

    /* Palette table size */
    size = (1 << fmt_desc->bits_per_index) * fmt_desc->pixel_size;

    /* Texture palette indices array size for each miplevel */
    do {
        if (fmt_desc->bits_per_index == 4) {
            size += (width * height + 1) / 2;
        } else {
            size += width * height;
        }

        width >>= 1;
        if (width == 0) {
            width = 1;
        }

        height >>= 1;
        if (height == 0) {
            height = 1;
        }
    } while (max_level--);

    return size;
}

static void yagl_gles1_cpal_tex_uncomp_and_apply(struct yagl_gles_context *ctx,
                                                 YaglGles1PalFmtDesc *fmt_desc,
                                                 unsigned max_level,
                                                 unsigned width,
                                                 unsigned height,
                                                 const GLvoid *data)
{
    uint8_t *tex_img_data = NULL;
    uint8_t *img;
    const uint8_t *indices;
    unsigned cur_level, i;
    unsigned num_of_texels = width * height;
    GLint saved_alignment;

    if (!data) {
        for (cur_level = 0; cur_level <= max_level; ++cur_level) {
            ctx->driver->TexImage2D(GL_TEXTURE_2D,
                                    cur_level,
                                    fmt_desc->uncomp_format,
                                    width, height,
                                    0,
                                    fmt_desc->uncomp_format,
                                    fmt_desc->pixel_type,
                                    NULL);
            width >>= 1;
            height >>= 1;

            if (width == 0) {
                width = 1;
            }

            if (height == 0) {
                height = 1;
            }
        }

        return;
    }

    /* Jump over palette data to first image data */
    indices = data + (1 << fmt_desc->bits_per_index) * fmt_desc->pixel_size;

    /* 0 level image is the largest */
    tex_img_data = g_malloc(num_of_texels * fmt_desc->pixel_size);

    /* We will pass tightly packed data to glTexImage2D */
    ctx->driver->GetIntegerv(GL_UNPACK_ALIGNMENT, &saved_alignment);

    if (saved_alignment != 1) {
        ctx->driver->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    for (cur_level = 0; cur_level <= max_level; ++cur_level) {
        img = tex_img_data;

        if (fmt_desc->bits_per_index == 4) {
            unsigned cur_idx;

            for (i = 0; i < num_of_texels; ++i) {
                if ((i % 2) == 0) {
                    cur_idx = indices[i / 2] >> 4;
                } else {
                    cur_idx = indices[i / 2] & 0xf;
                }

                memcpy(img,
                       data + cur_idx * fmt_desc->pixel_size,
                       fmt_desc->pixel_size);

                img += fmt_desc->pixel_size;
            }

            indices += (num_of_texels + 1) / 2;
        } else {
            for (i = 0; i < num_of_texels; ++i) {
                memcpy(img,
                       data + indices[i] * fmt_desc->pixel_size,
                       fmt_desc->pixel_size);
                img += fmt_desc->pixel_size;
            }

            indices += num_of_texels;
        }

        ctx->driver->TexImage2D(GL_TEXTURE_2D,
                                cur_level,
                                fmt_desc->uncomp_format,
                                width, height,
                                0,
                                fmt_desc->uncomp_format,
                                fmt_desc->pixel_type,
                                tex_img_data);

        width >>= 1;
        if (width == 0) {
            width = 1;
        }

        height >>= 1;
        if (height == 0) {
            height = 1;
        }

        num_of_texels = width * height;
    }

    g_free(tex_img_data);

    if (saved_alignment != 1) {
        ctx->driver->PixelStorei(GL_UNPACK_ALIGNMENT, saved_alignment);
    }
}

static GLenum yagl_gles1_compressed_tex_image(struct yagl_gles_context *ctx,
                                              GLenum target,
                                              GLint level,
                                              GLenum internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLint border,
                                              GLsizei imageSize,
                                              const GLvoid *data)
{
    const int max_tex_size = ((YaglGles1Context *)ctx)->max_tex_size;
    YaglGles1PalFmtDesc fmt_desc;

    if (target != GL_TEXTURE_2D) {
        return GL_INVALID_ENUM;
    }

    switch (internalformat) {
    case GL_PALETTE4_RGB8_OES ... GL_PALETTE8_RGB5_A1_OES:
        yagl_gles1_cpal_format_get_descr(internalformat, &fmt_desc);

        if ((level > 0) || (-level > yagl_log2(max_tex_size)) ||
            !yagl_gles1_tex_dims_valid(width, height, max_tex_size) ||
            border != 0 || (imageSize !=
                yagl_gles1_cpal_tex_size(&fmt_desc, width, height, -level))) {
            return GL_INVALID_VALUE;
        }

        yagl_gles1_cpal_tex_uncomp_and_apply(ctx,
                                             &fmt_desc,
                                             -level,
                                             width,
                                             height,
                                             data);
        break;
    default:
        return GL_INVALID_ENUM;
    }

    return GL_NO_ERROR;
}

YaglGles1Context *yagl_gles1_context_create(struct yagl_sharegroup *sg,
                                            struct yagl_gles1_driver *driver)
{
    YaglGles1Context *gles1_ctx;

    YAGL_LOG_FUNC_ENTER(driver_ps->common->ps->id,
                        0,
                        yagl_gles1_context_create,
                        NULL);

    gles1_ctx = g_new0(YaglGles1Context, 1);

    yagl_client_context_init(&gles1_ctx->base.base, yagl_client_api_gles1, sg);

    gles1_ctx->base.base.activate = &yagl_gles1_context_activate;
    gles1_ctx->base.base.deactivate = &yagl_gles1_context_deactivate;
    gles1_ctx->base.base.destroy = &yagl_gles1_context_destroy;

    yagl_gles_context_init(&gles1_ctx->base, &driver->base);

    gles1_ctx->base.get_param_count = &yagl_gles1_context_get_param_count;
    gles1_ctx->base.get_booleanv = &yagl_gles1_context_get_booleanv;
    gles1_ctx->base.get_integerv = &yagl_gles1_context_get_integerv;
    gles1_ctx->base.get_floatv = &yagl_gles1_context_get_floatv;
    gles1_ctx->base.get_extensions = &yagl_gles1_context_get_extensions;
    gles1_ctx->base.draw_arrays = &yagl_gles1_context_draw_arrays;
    gles1_ctx->base.draw_elements = &yagl_gles1_context_draw_elements;
    gles1_ctx->base.compressed_tex_image = &yagl_gles1_compressed_tex_image;
    gles1_ctx->base.is_enabled = &yagl_gles1_context_is_enabled;

    gles1_ctx->driver = driver;
    gles1_ctx->prepared = false;
    gles1_ctx->sg = sg;

    gles1_ctx->client_active_texture = 0;
    gles1_ctx->max_clip_planes = 0;
    gles1_ctx->max_lights = 0;

    YAGL_LOG_FUNC_EXIT("%p", gles1_ctx);

    return gles1_ctx;
}
