#include "yagl_gles2_context.h"
#include "yagl_apis/gles/yagl_gles_array.h"
#include "yagl_apis/gles/yagl_gles_buffer.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_sharegroup.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "yagl_gles2_driver.h"

/*
 * We can't include GL/glext.h here
 */
#define GL_POINT_SPRITE                   0x8861
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642

static void yagl_gles2_array_apply(struct yagl_gles_array *array)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)array->ctx;

    if (array->vbo) {
        yagl_object_name old_buffer_name = 0;

        yagl_gles_buffer_bind(array->vbo,
                              array->type,
                              GL_ARRAY_BUFFER,
                              &old_buffer_name);

        gles2_ctx->driver_ps->VertexAttribPointer(gles2_ctx->driver_ps,
                                                  array->index,
                                                  array->size,
                                                  array->type,
                                                  array->normalized,
                                                  array->stride,
                                                  (GLvoid*)array->offset);

        gles2_ctx->driver_ps->common->BindBuffer(gles2_ctx->driver_ps->common,
                                                 GL_ARRAY_BUFFER,
                                                 old_buffer_name);
    } else {
        assert(array->host_data);

        gles2_ctx->driver_ps->VertexAttribPointer(gles2_ctx->driver_ps,
                                                  array->index,
                                                  array->size,
                                                  array->type,
                                                  array->normalized,
                                                  array->stride,
                                                  array->host_data);
    }
}

static bool yagl_gles2_context_get_param_count(struct yagl_gles_context *ctx,
                                               GLenum pname,
                                               int *count)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)ctx;

    switch (pname) {
    case GL_ACTIVE_TEXTURE: *count = 1; break;
    case GL_ALIASED_LINE_WIDTH_RANGE: *count = 2; break;
    case GL_ALIASED_POINT_SIZE_RANGE: *count = 2; break;
    case GL_ALPHA_BITS: *count = 1; break;
    case GL_ARRAY_BUFFER_BINDING: *count = 1; break;
    case GL_BLEND: *count = 1; break;
    case GL_BLEND_COLOR: *count = 4; break;
    case GL_BLEND_DST_ALPHA: *count = 1; break;
    case GL_BLEND_DST_RGB: *count = 1; break;
    case GL_BLEND_EQUATION_ALPHA: *count = 1; break;
    case GL_BLEND_EQUATION_RGB: *count = 1; break;
    case GL_BLEND_SRC_ALPHA: *count = 1; break;
    case GL_BLEND_SRC_RGB: *count = 1; break;
    case GL_BLUE_BITS: *count = 1; break;
    case GL_COLOR_CLEAR_VALUE: *count = 4; break;
    case GL_COLOR_WRITEMASK: *count = 4; break;
    case GL_COMPRESSED_TEXTURE_FORMATS: *count = ctx->num_compressed_texture_formats; break;
    case GL_CULL_FACE: *count = 1; break;
    case GL_CULL_FACE_MODE: *count = 1; break;
    case GL_CURRENT_PROGRAM: *count = 1; break;
    case GL_DEPTH_BITS: *count = 1; break;
    case GL_DEPTH_CLEAR_VALUE: *count = 1; break;
    case GL_DEPTH_FUNC: *count = 1; break;
    case GL_DEPTH_RANGE: *count = 2; break;
    case GL_DEPTH_TEST: *count = 1; break;
    case GL_DEPTH_WRITEMASK: *count = 1; break;
    case GL_DITHER: *count = 1; break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING: *count = 1; break;
    case GL_FRAMEBUFFER_BINDING: *count = 1; break;
    case GL_FRONT_FACE: *count = 1; break;
    case GL_GENERATE_MIPMAP_HINT: *count = 1; break;
    case GL_GREEN_BITS: *count = 1; break;
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT: *count = 1; break;
    case GL_IMPLEMENTATION_COLOR_READ_TYPE: *count = 1; break;
    case GL_LINE_WIDTH: *count = 1; break;
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: *count = 1; break;
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE: *count = 1; break;
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS: *count = 1; break;
    case GL_MAX_RENDERBUFFER_SIZE: *count = 1; break;
    case GL_MAX_TEXTURE_IMAGE_UNITS: *count = 1; break;
    case GL_MAX_TEXTURE_SIZE: *count = 1; break;
    case GL_MAX_VARYING_VECTORS: *count = 1; break;
    case GL_MAX_VERTEX_ATTRIBS: *count = 1; break;
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: *count = 1; break;
    case GL_MAX_VERTEX_UNIFORM_VECTORS: *count = 1; break;
    case GL_MAX_VIEWPORT_DIMS: *count = 2; break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS: *count = 1; break;
    case GL_NUM_SHADER_BINARY_FORMATS: *count = 1; break;
    case GL_PACK_ALIGNMENT: *count = 1; break;
    case GL_POLYGON_OFFSET_FACTOR: *count = 1; break;
    case GL_POLYGON_OFFSET_FILL: *count = 1; break;
    case GL_POLYGON_OFFSET_UNITS: *count = 1; break;
    case GL_RED_BITS: *count = 1; break;
    case GL_RENDERBUFFER_BINDING: *count = 1; break;
    case GL_SAMPLE_ALPHA_TO_COVERAGE: *count = 1; break;
    case GL_SAMPLE_BUFFERS: *count = 1; break;
    case GL_SAMPLE_COVERAGE: *count = 1; break;
    case GL_SAMPLE_COVERAGE_INVERT: *count = 1; break;
    case GL_SAMPLE_COVERAGE_VALUE: *count = 1; break;
    case GL_SAMPLES: *count = 1; break;
    case GL_SCISSOR_BOX: *count = 4; break;
    case GL_SCISSOR_TEST: *count = 1; break;
    case GL_SHADER_BINARY_FORMATS: *count = gles2_ctx->num_shader_binary_formats; break;
    case GL_SHADER_COMPILER: *count = 1; break;
    case GL_STENCIL_BACK_FAIL: *count = 1; break;
    case GL_STENCIL_BACK_FUNC: *count = 1; break;
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL: *count = 1; break;
    case GL_STENCIL_BACK_PASS_DEPTH_PASS: *count = 1; break;
    case GL_STENCIL_BACK_REF: *count = 1; break;
    case GL_STENCIL_BACK_VALUE_MASK: *count = 1; break;
    case GL_STENCIL_BACK_WRITEMASK: *count = 1; break;
    case GL_STENCIL_BITS: *count = 1; break;
    case GL_STENCIL_CLEAR_VALUE: *count = 1; break;
    case GL_STENCIL_FAIL: *count = 1; break;
    case GL_STENCIL_FUNC: *count = 1; break;
    case GL_STENCIL_PASS_DEPTH_FAIL: *count = 1; break;
    case GL_STENCIL_PASS_DEPTH_PASS: *count = 1; break;
    case GL_STENCIL_REF: *count = 1; break;
    case GL_STENCIL_TEST: *count = 1; break;
    case GL_STENCIL_VALUE_MASK: *count = 1; break;
    case GL_STENCIL_WRITEMASK: *count = 1; break;
    case GL_SUBPIXEL_BITS: *count = 1; break;
    case GL_TEXTURE_BINDING_2D: *count = 1; break;
    case GL_TEXTURE_BINDING_CUBE_MAP: *count = 1; break;
    case GL_UNPACK_ALIGNMENT: *count = 1; break;
    case GL_VIEWPORT: *count = 4; break;
    case GL_MAX_SAMPLES_IMG: *count = 1; break;
    case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: *count = 1; break;
    default: return false;
    }
    return true;
}

static bool yagl_gles2_context_get_integerv(struct yagl_gles_context *ctx,
                                            GLenum pname,
                                            GLint *params)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)ctx;

    switch (pname) {
    case GL_CURRENT_PROGRAM:
        params[0] = gles2_ctx->program_local_name;
        break;
    case GL_FRAMEBUFFER_BINDING:
        params[0] = ctx->fbo_local_name;
        break;
    case GL_RENDERBUFFER_BINDING:
        params[0] = ctx->rbo_local_name;
        break;
    default:
        return false;
    }

    return true;
}

static bool yagl_gles2_context_get_booleanv(struct yagl_gles_context *ctx,
                                            GLenum pname,
                                            GLboolean *params)
{
    GLint tmp;

    switch (pname) {
    case GL_CURRENT_PROGRAM:
    case GL_FRAMEBUFFER_BINDING:
    case GL_RENDERBUFFER_BINDING:
        if (!yagl_gles2_context_get_integerv(ctx, pname, &tmp)) {
            return false;
        }
        params[0] = ((tmp != 0) ? GL_TRUE : GL_FALSE);
        break;
    default:
        return false;
    }

    return true;
}

static bool yagl_gles2_context_get_floatv(struct yagl_gles_context *ctx,
                                          GLenum pname,
                                          GLfloat *params)
{
    GLint tmp;

    switch (pname) {
    case GL_CURRENT_PROGRAM:
    case GL_FRAMEBUFFER_BINDING:
    case GL_RENDERBUFFER_BINDING:
        if (!yagl_gles2_context_get_integerv(ctx, pname, &tmp)) {
            return false;
        }
        params[0] = (GLfloat)tmp;
        break;
    default:
        return false;
    }

    return true;
}

static GLchar *yagl_gles2_context_get_extensions(struct yagl_gles_context *ctx)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)ctx;

    const GLchar *mandatory_extensions =
        "GL_OES_depth24 GL_OES_depth32 "
        "GL_OES_texture_float GL_OES_texture_float_linear "
        "GL_OES_depth_texture ";
    const GLchar *pack_depth_stencil = "GL_OES_packed_depth_stencil ";
    const GLchar *texture_npot = "GL_OES_texture_npot ";
    const GLchar *texture_rectangle = "GL_ARB_texture_rectangle ";
    const GLchar *texture_filter_anisotropic = "GL_EXT_texture_filter_anisotropic ";
    const GLchar *texture_half_float = "GL_OES_texture_half_float GL_OES_texture_half_float_linear ";
    const GLchar *vertex_half_float = "GL_OES_vertex_half_float ";
    const GLchar *standard_derivatives = "GL_OES_standard_derivatives ";

    GLuint len = strlen(mandatory_extensions);
    GLchar *str;

    if (gles2_ctx->base.pack_depth_stencil) {
        len += strlen(pack_depth_stencil);
    }

    if (gles2_ctx->base.texture_npot) {
        len += strlen(texture_npot);
    }

    if (gles2_ctx->base.texture_rectangle) {
        len += strlen(texture_rectangle);
    }

    if (gles2_ctx->base.texture_filter_anisotropic) {
        len += strlen(texture_filter_anisotropic);
    }

    if (gles2_ctx->texture_half_float) {
        len += strlen(texture_half_float);
    }

    if (gles2_ctx->vertex_half_float) {
        len += strlen(vertex_half_float);
    }

    if (gles2_ctx->standard_derivatives) {
        len += strlen(standard_derivatives);
    }

    str = g_malloc0(len + 1);

    strcpy(str, mandatory_extensions);

    if (gles2_ctx->base.pack_depth_stencil) {
        strcat(str, pack_depth_stencil);
    }

    if (gles2_ctx->base.texture_npot) {
        strcat(str, texture_npot);
    }

    if (gles2_ctx->base.texture_rectangle) {
        strcat(str, texture_rectangle);
    }

    if (gles2_ctx->base.texture_filter_anisotropic) {
        strcat(str, texture_filter_anisotropic);
    }

    if (gles2_ctx->texture_half_float) {
        strcat(str, texture_half_float);
    }

    if (gles2_ctx->vertex_half_float) {
        strcat(str, vertex_half_float);
    }

    if (gles2_ctx->standard_derivatives) {
        strcat(str, standard_derivatives);
    }

    return str;
}

static void yagl_gles2_context_pre_draw(struct yagl_gles_context *ctx, GLenum mode)
{
    /*
     * Enable texture generation for GL_POINTS and gl_PointSize shader variable.
     * GLESv2 assumes this is enabled by default, we need to set this
     * state for GL.
     */

    if (mode == GL_POINTS) {
        ctx->driver_ps->Enable(ctx->driver_ps, GL_POINT_SPRITE);
        ctx->driver_ps->Enable(ctx->driver_ps, GL_VERTEX_PROGRAM_POINT_SIZE);
    }
}

static void yagl_gles2_context_post_draw(struct yagl_gles_context *ctx, GLenum mode)
{
    if (mode == GL_POINTS) {
        ctx->driver_ps->Disable(ctx->driver_ps, GL_VERTEX_PROGRAM_POINT_SIZE);
        ctx->driver_ps->Disable(ctx->driver_ps, GL_POINT_SPRITE);
    }
}

static void yagl_gles2_context_destroy(struct yagl_client_context *ctx)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)ctx;

    YAGL_LOG_FUNC_ENTER(gles2_ctx->driver_ps->common->ps->id,
                        0,
                        yagl_gles2_context_destroy,
                        "%p",
                        gles2_ctx);

    yagl_gles_context_cleanup(&gles2_ctx->base);
    yagl_client_context_cleanup(&gles2_ctx->base.base);

    g_free(gles2_ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static const char* g_shader_precision_test =
    "varying lowp vec4 c;\n"
    "void main(void) { gl_FragColor=c; }\n";

static bool yagl_gles2_shader_precision_supported(struct yagl_gles2_driver_ps *driver_ps,
                                                  struct yagl_thread_state *ts)
{
    GLuint shader = driver_ps->CreateShader(driver_ps, GL_FRAGMENT_SHADER);
    GLint status = GL_FALSE;

    YAGL_LOG_FUNC_ENTER_TS(ts,
                           yagl_gles2_shader_precision_supported,
                           NULL);

    driver_ps->ShaderSource(driver_ps, shader, 1, &g_shader_precision_test, 0);
    driver_ps->CompileShader(driver_ps, shader);
    driver_ps->GetShaderiv(driver_ps, shader, GL_COMPILE_STATUS, &status);
    driver_ps->DeleteShader(driver_ps, shader);

    if (status == GL_FALSE) {
        YAGL_LOG_WARN("Host OpenGL implementation doesn't understand precision keyword");
    } else {
        YAGL_LOG_DEBUG("Host OpenGL implementation understands precision keyword");
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return (status != GL_FALSE);
}

static void yagl_gles2_context_prepare(struct yagl_gles2_context *gles2_ctx,
                                       struct yagl_thread_state *ts)
{
    struct yagl_gles_driver_ps *gles_driver = gles2_ctx->driver_ps->common;
    GLint i, num_arrays = 0, num_texture_units = 0;
    struct yagl_gles_array *arrays;
    const char *extensions;

    YAGL_LOG_FUNC_ENTER_TS(ts,
                           yagl_gles2_context_prepare,
                           "%p",
                           gles2_ctx);

    gles_driver->GetIntegerv(gles_driver, GL_MAX_VERTEX_ATTRIBS, &num_arrays);

    arrays = g_malloc(num_arrays * sizeof(*arrays));

    for (i = 0; i < num_arrays; ++i) {
        yagl_gles_array_init(&arrays[i],
                             i,
                             &gles2_ctx->base,
                             &yagl_gles2_array_apply);
    }

    gles_driver->GetIntegerv(gles_driver, GL_MAX_TEXTURE_IMAGE_UNITS,
                             &num_texture_units);

    yagl_gles_context_prepare(&gles2_ctx->base, ts, arrays, num_arrays,
                              num_texture_units);

    gles2_ctx->shader_strip_precision =
        !yagl_gles2_shader_precision_supported(gles2_ctx->driver_ps, ts);

    gles_driver->GetIntegerv(gles_driver,
                             GL_NUM_SHADER_BINARY_FORMATS,
                             &gles2_ctx->num_shader_binary_formats);

    extensions = (const char*)gles_driver->GetString(gles_driver, GL_EXTENSIONS);

    gles2_ctx->texture_half_float = (strstr(extensions, "GL_ARB_half_float_pixel ") != NULL) ||
                                    (strstr(extensions, "GL_NV_half_float ") != NULL);

    gles2_ctx->vertex_half_float = (strstr(extensions, "GL_ARB_half_float_vertex ") != NULL);

    gles2_ctx->standard_derivatives = (strstr(extensions, "GL_OES_standard_derivatives ") != NULL);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles2_context_activate(struct yagl_client_context *ctx,
                                        struct yagl_thread_state *ts)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)ctx;

    if (!gles2_ctx->prepared) {
        yagl_gles2_context_prepare(gles2_ctx, ts);
        gles2_ctx->prepared = true;
    }

    yagl_gles_context_activate(&gles2_ctx->base, ts);
}

static void yagl_gles2_context_deactivate(struct yagl_client_context *ctx)
{
    struct yagl_gles2_context *gles2_ctx = (struct yagl_gles2_context*)ctx;

    yagl_gles_context_deactivate(&gles2_ctx->base);
}

struct yagl_gles2_context
    *yagl_gles2_context_create(struct yagl_sharegroup *sg,
                               struct yagl_gles2_driver_ps *driver_ps)
{
    struct yagl_gles2_context *gles2_ctx;

    YAGL_LOG_FUNC_ENTER(driver_ps->common->ps->id,
                        0,
                        yagl_gles2_context_create,
                        NULL);

    gles2_ctx = g_malloc0(sizeof(*gles2_ctx));

    yagl_client_context_init(&gles2_ctx->base.base, yagl_client_api_gles2, sg);

    gles2_ctx->base.base.activate = &yagl_gles2_context_activate;
    gles2_ctx->base.base.deactivate = &yagl_gles2_context_deactivate;
    gles2_ctx->base.base.destroy = &yagl_gles2_context_destroy;

    yagl_gles_context_init(&gles2_ctx->base, driver_ps->common);

    gles2_ctx->base.get_param_count = &yagl_gles2_context_get_param_count;
    gles2_ctx->base.get_booleanv = &yagl_gles2_context_get_booleanv;
    gles2_ctx->base.get_integerv = &yagl_gles2_context_get_integerv;
    gles2_ctx->base.get_floatv = &yagl_gles2_context_get_floatv;
    gles2_ctx->base.get_extensions = &yagl_gles2_context_get_extensions;
    gles2_ctx->base.pre_draw = &yagl_gles2_context_pre_draw;
    gles2_ctx->base.post_draw = &yagl_gles2_context_post_draw;

    gles2_ctx->driver_ps = driver_ps;
    gles2_ctx->prepared = false;
    gles2_ctx->sg = sg;
    gles2_ctx->shader_strip_precision = true;

    gles2_ctx->num_shader_binary_formats = 0;

    gles2_ctx->texture_half_float = false;
    gles2_ctx->vertex_half_float = false;
    gles2_ctx->standard_derivatives = false;

    gles2_ctx->program_local_name = 0;

    YAGL_LOG_FUNC_EXIT("%p", gles2_ctx);

    return gles2_ctx;
}

void yagl_gles2_context_use_program(struct yagl_gles2_context *ctx,
                                    yagl_object_name program_local_name)
{
    ctx->program_local_name = program_local_name;
}

void yagl_gles2_context_unuse_program(struct yagl_gles2_context *ctx,
                                      yagl_object_name program_local_name)
{
    if (program_local_name == ctx->program_local_name) {
        ctx->program_local_name = 0;
    }
}
