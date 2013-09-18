#include "yagl_host_gles2_calls.h"
#include "yagl_apis/gles/yagl_gles_array.h"
#include "yagl_apis/gles/yagl_gles_texture.h"
#include "yagl_apis/gles/yagl_gles_image.h"
#include "yagl_gles2_calls.h"
#include "yagl_gles2_api.h"
#include "yagl_gles2_driver.h"
#include "yagl_gles2_api_ps.h"
#include "yagl_gles2_api_ts.h"
#include "yagl_gles2_context.h"
#include "yagl_gles2_shader.h"
#include "yagl_gles2_program.h"
#include "yagl_gles2_validate.h"
#include "yagl_egl_interface.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_client_interface.h"
#include "yagl_sharegroup.h"

#define YAGL_SET_ERR(err) \
    yagl_gles_context_set_error(&ctx->base, err); \
    YAGL_LOG_ERROR("error = 0x%X", err)

static YAGL_DEFINE_TLS(struct yagl_gles2_api_ts*, gles2_api_ts);

#define YAGL_GET_CTX_IMPL(func, ret_expr) \
    struct yagl_gles2_context *ctx = \
        (struct yagl_gles2_context*)cur_ts->ps->egl_iface->get_ctx(cur_ts->ps->egl_iface); \
    YAGL_LOG_FUNC_SET(func); \
    if (!ctx || \
        (ctx->base.base.client_api != yagl_client_api_gles2)) { \
        YAGL_LOG_WARN("no current context"); \
        ret_expr; \
    }

#define YAGL_GET_CTX_RET(func, ret) YAGL_GET_CTX_IMPL(func, return ret)

#define YAGL_GET_CTX(func) YAGL_GET_CTX_IMPL(func, return)

#define YAGL_UNIMPLEMENTED_RET(func, ret) \
    YAGL_GET_CTX_RET(func, ret); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    *retval = ret; \
    return true

#define YAGL_UNIMPLEMENTED(func) \
    YAGL_GET_CTX(func); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    return

static bool yagl_get_array_param(struct yagl_gles_array *array,
                                 GLenum pname,
                                 GLint *param)
{
    switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        *param = array->vbo_local_name;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        *param = array->enabled;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
        *param = array->size;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
        *param = array->stride;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
        *param = array->type;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        *param = array->normalized;
        break;
    default:
        return false;
    }

    return true;
}

static struct yagl_client_context
    *yagl_host_gles2_create_ctx(struct yagl_client_interface *iface,
                                struct yagl_sharegroup *sg)
{
    struct yagl_gles2_context *ctx =
        yagl_gles2_context_create(sg, gles2_api_ts->driver);

    if (!ctx) {
        return NULL;
    }

    return &ctx->base.base;
}

static struct yagl_client_image
    *yagl_host_gles2_create_image(struct yagl_client_interface *iface,
                                  yagl_object_name tex_global_name,
                                  struct yagl_ref *tex_data)
{
    struct yagl_gles_image *image =
        yagl_gles_image_create_from_texture(&gles2_api_ts->driver->base,
                                            tex_global_name,
                                            tex_data);

    return image ? &image->base : NULL;
}

static yagl_api_func yagl_host_gles2_get_func(struct yagl_api_ps *api_ps,
                                              uint32_t func_id)
{
    if ((func_id <= 0) || (func_id > yagl_gles2_api_num_funcs)) {
        return NULL;
    } else {
        return yagl_gles2_api_funcs[func_id - 1];
    }
}

static void yagl_host_gles2_thread_init(struct yagl_api_ps *api_ps)
{
    struct yagl_gles2_api_ps *gles2_api_ps = (struct yagl_gles2_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles2_thread_init, NULL);

    gles2_api_ts = g_malloc0(sizeof(*gles2_api_ts));

    yagl_gles2_api_ts_init(gles2_api_ts, gles2_api_ps->driver);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_gles2_thread_fini(struct yagl_api_ps *api_ps)
{
    YAGL_LOG_FUNC_ENTER(yagl_host_gles2_thread_fini, NULL);

    yagl_gles2_api_ts_cleanup(gles2_api_ts);

    g_free(gles2_api_ts);

    gles2_api_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_gles2_process_fini(struct yagl_api_ps *api_ps)
{
    struct yagl_gles2_api_ps *gles2_api_ps = (struct yagl_gles2_api_ps*)api_ps;

    yagl_gles2_api_ps_fini(gles2_api_ps);
}

static void yagl_host_gles2_process_destroy(struct yagl_api_ps *api_ps)
{
    struct yagl_gles2_api_ps *gles2_api_ps = (struct yagl_gles2_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles2_process_destroy, NULL);

    yagl_gles2_api_ps_cleanup(gles2_api_ps);
    yagl_api_ps_cleanup(&gles2_api_ps->base);

    g_free(gles2_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_api_ps *yagl_host_gles2_process_init(struct yagl_api *api)
{
    struct yagl_gles2_api *gles2_api = (struct yagl_gles2_api*)api;
    struct yagl_gles2_api_ps *gles2_api_ps;
    struct yagl_client_interface *client_iface;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles2_process_init, NULL);

    /*
     * Create GLES2 interface.
     */

    client_iface = g_malloc0(sizeof(*client_iface));

    yagl_client_interface_init(client_iface);

    client_iface->create_ctx = &yagl_host_gles2_create_ctx;
    client_iface->create_image = &yagl_host_gles2_create_image;

    /*
     * Finally, create API ps.
     */

    gles2_api_ps = g_malloc0(sizeof(*gles2_api_ps));

    yagl_api_ps_init(&gles2_api_ps->base, api);

    gles2_api_ps->base.thread_init = &yagl_host_gles2_thread_init;
    gles2_api_ps->base.get_func = &yagl_host_gles2_get_func;
    gles2_api_ps->base.thread_fini = &yagl_host_gles2_thread_fini;
    gles2_api_ps->base.fini = &yagl_host_gles2_process_fini;
    gles2_api_ps->base.destroy = &yagl_host_gles2_process_destroy;

    yagl_gles2_api_ps_init(gles2_api_ps, gles2_api->driver, client_iface);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles2_api_ps->base;
}

void yagl_host_glAttachShader(GLuint program,
    GLuint shader)
{
    struct yagl_gles2_program *program_obj = NULL;
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glAttachShader);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles2_program_attach_shader(program_obj, shader_obj, shader)) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

out:
    yagl_gles2_shader_release(shader_obj);
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glBindAttribLocation(GLuint program,
    GLuint index,
    const GLchar *name, int32_t name_count)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glBindAttribLocation);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (name) {
        YAGL_LOG_TRACE("binding attrib %s location to %d", name, index);
    }

    yagl_gles2_program_bind_attrib_location(program_obj, index, name);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    YAGL_GET_CTX(glBlendColor);

    ctx->driver->BlendColor(red, green, blue, alpha);
}

void yagl_host_glCompileShader(GLuint shader)
{
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glCompileShader);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_shader_compile(shader_obj);

out:
    yagl_gles2_shader_release(shader_obj);
}

GLuint yagl_host_glCreateProgram(void)
{
    GLuint res = 0;
    struct yagl_gles2_program *program = NULL;

    YAGL_GET_CTX_RET(glCreateProgram, 0);

    program = yagl_gles2_program_create(ctx->driver);

    if (!program) {
        goto out;
    }

    res = yagl_sharegroup_add(ctx->sg, YAGL_NS_SHADER_PROGRAM, &program->base);

out:
    yagl_gles2_program_release(program);

    return res;
}

GLuint yagl_host_glCreateShader(GLenum type)
{
    GLuint res = 0;
    struct yagl_gles2_shader *shader = NULL;

    YAGL_GET_CTX_RET(glCreateShader, 0);

    shader = yagl_gles2_shader_create(ctx->driver, type);

    if (!shader) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    res = yagl_sharegroup_add(ctx->sg, YAGL_NS_SHADER_PROGRAM, &shader->base);

out:
    yagl_gles2_shader_release(shader);

    return res;
}

void yagl_host_glDeleteProgram(GLuint program)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glDeleteProgram);

    if (program == 0) {
        goto out;
    }

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        goto out;
    }

    yagl_gles2_context_unuse_program(ctx, program);

    yagl_sharegroup_remove_check(ctx->sg,
                                 YAGL_NS_SHADER_PROGRAM,
                                 program,
                                 &program_obj->base);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glDeleteShader(GLuint shader)
{
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glDeleteShader);

    if (shader == 0) {
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        goto out;
    }

    yagl_sharegroup_remove_check(ctx->sg,
                                 YAGL_NS_SHADER_PROGRAM,
                                 shader,
                                 &shader_obj->base);

out:
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glDetachShader(GLuint program,
    GLuint shader)
{
    struct yagl_gles2_program *program_obj = NULL;
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glDetachShader);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles2_program_detach_shader(program_obj, shader_obj, shader)) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

out:
    yagl_gles2_shader_release(shader_obj);
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glDisableVertexAttribArray(GLuint index)
{
    struct yagl_gles_array *array = NULL;

    YAGL_GET_CTX(glDisableVertexAttribArray);

    array = yagl_gles_context_get_array(&ctx->base, index);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    yagl_gles_array_enable(array, false);

    ctx->driver->DisableVertexAttribArray(index);
}

void yagl_host_glEnableVertexAttribArray(GLuint index)
{
    struct yagl_gles_array *array = NULL;

    YAGL_GET_CTX(glEnableVertexAttribArray);

    array = yagl_gles_context_get_array(&ctx->base, index);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    yagl_gles_array_enable(array, true);

    ctx->driver->EnableVertexAttribArray(index);
}

void yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count)
{
    GLsizei tmp = -1;
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glGetActiveAttrib);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_get_active_attrib(program_obj,
                                         index,
                                         name_maxcount,
                                         &tmp,
                                         size,
                                         type,
                                         name);

    if (tmp >= 0) {
        YAGL_LOG_TRACE("got active attrib: size = %d, type = %u, name = %s",
                       (size ? *size : -1), (type ? *type : -1), name);
        *name_count = tmp + 1;
    }

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count)
{
    GLsizei tmp = -1;
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glGetActiveUniform);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_get_active_uniform(program_obj,
                                          index,
                                          name_maxcount,
                                          &tmp,
                                          size,
                                          type,
                                          name);

    if (tmp >= 0) {
        YAGL_LOG_TRACE("got active uniform: size = %d, type = %u, name = %s",
                       (size ? *size : -1), (type ? *type : -1), name);
        *name_count = tmp + 1;
    }

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetAttachedShaders(GLuint program,
    GLuint *shaders, int32_t shaders_maxcount, int32_t *shaders_count)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glGetAttachedShaders);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (shaders_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->vertex_shader_local_name != 0) {
        if (*shaders_count < shaders_maxcount) {
            if (shaders) {
                shaders[*shaders_count] = program_obj->vertex_shader_local_name;
            }
            ++*shaders_count;
        }
    }

    if (program_obj->fragment_shader_local_name != 0) {
        if (*shaders_count < shaders_maxcount) {
            if (shaders) {
                shaders[*shaders_count] = program_obj->fragment_shader_local_name;
            }
            ++*shaders_count;
        }
    }

out:
    yagl_gles2_program_release(program_obj);
}

int yagl_host_glGetAttribLocation(GLuint program,
    const GLchar *name, int32_t name_count)
{
    int res = 0;
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX_RET(glGetAttribLocation, 0);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    res = yagl_gles2_program_get_attrib_location(program_obj, name);

out:
    yagl_gles2_program_release(program_obj);

    return res;
}

void yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    GLint *param)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glGetProgramiv);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_get_param(program_obj, pname, param);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetProgramInfoLog(GLuint program,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count)
{
    GLsizei tmp = -1;
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glGetProgramInfoLog);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (infolog_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    yagl_gles2_program_get_info_log(program_obj, infolog_maxcount, &tmp, infolog);

    if (tmp >= 0) {
        *infolog_count = tmp + 1;
    }

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    GLint *param)
{
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glGetShaderiv);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_shader_get_param(shader_obj, pname, param);

out:
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glGetShaderInfoLog(GLuint shader,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count)
{
    GLsizei tmp = -1;
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glGetShaderInfoLog);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (infolog_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    yagl_gles2_shader_get_info_log(shader_obj, infolog_maxcount, &tmp, infolog);

    if (tmp >= 0) {
        *infolog_count = tmp + 1;
    }

out:
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype,
    GLint *range, int32_t range_maxcount, int32_t *range_count,
    GLint *precision)
{
    YAGL_GET_CTX(glGetShaderPrecisionFormat);

    switch (precisiontype) {
    case GL_LOW_INT:
    case GL_MEDIUM_INT:
    case GL_HIGH_INT:
        if (range) {
            range[0] = range[1] = 16;
            *range_count = 2;
        }
        if (precision) {
            *precision = 0;
        }
        break;
    case GL_LOW_FLOAT:
    case GL_MEDIUM_FLOAT:
    case GL_HIGH_FLOAT:
        if (range) {
            range[0] = range[1] = 127;
            *range_count = 2;
        }
        if (precision) {
            *precision = 24;
        }
        break;
    default:
        YAGL_SET_ERR(GL_INVALID_ENUM);
        break;
    }
}

void yagl_host_glGetShaderSource(GLuint shader,
    GLchar *source, int32_t source_maxcount, int32_t *source_count)
{
    GLsizei tmp = -1;
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glGetShaderSource);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (source_maxcount < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    yagl_gles2_shader_get_source(shader_obj, source_maxcount, &tmp, source);

    if (tmp >= 0) {
        *source_count = tmp + 1;
    }

out:
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glGetUniformfv(GLuint program,
    GLint location,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLenum type;

    YAGL_GET_CTX(glGetUniformfv);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles2_program_get_uniform_type(program_obj,
                                             location,
                                             &type)) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles2_get_uniform_type_count(type, params_count)) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_get_uniform_float(program_obj, location, params);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetUniformiv(GLuint program,
    GLint location,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLenum type;

    YAGL_GET_CTX(glGetUniformiv);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles2_program_get_uniform_type(program_obj,
                                             location,
                                             &type)) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!yagl_gles2_get_uniform_type_count(type, params_count)) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_get_uniform_int(program_obj, location, params);

out:
    yagl_gles2_program_release(program_obj);
}

int yagl_host_glGetUniformLocation(GLuint program,
    const GLchar *name, int32_t name_count)
{
    int res = 0;
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX_RET(glGetUniformLocation, 0);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (name) {
        YAGL_LOG_TRACE("getting uniform %s location", name);
    }

    res = yagl_gles2_program_get_uniform_location(program_obj, name);

out:
    yagl_gles2_program_release(program_obj);

    return res;
}

void yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    struct yagl_gles_array *array = NULL;
    GLint param = 0;

    YAGL_GET_CTX(glGetVertexAttribfv);

    array = yagl_gles_context_get_array(&ctx->base, index);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (!yagl_gles2_get_array_param_count(pname, params_count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    if (yagl_get_array_param(array, pname, &param)) {
        params[0] = param;
    } else {
        ctx->driver->GetVertexAttribfv(index, pname, params);
    }
}

void yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    struct yagl_gles_array *array = NULL;

    YAGL_GET_CTX(glGetVertexAttribiv);

    array = yagl_gles_context_get_array(&ctx->base, index);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (!yagl_gles2_get_array_param_count(pname, params_count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return;
    }

    if (!yagl_get_array_param(array, pname, params)) {
        ctx->driver->GetVertexAttribiv(index, pname, params);
    }
}

void yagl_host_glGetVertexAttribPointerv(GLuint index,
    GLenum pname,
    target_ulong *pointer)
{
    struct yagl_gles_array *array = NULL;
    target_ulong tmp = 0;

    YAGL_GET_CTX(glGetVertexAttribPointerv);

    array = yagl_gles_context_get_array(&ctx->base, index);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (array->vbo) {
        tmp = array->offset;
    } else {
        tmp = array->target_data;
    }

    if (pointer) {
        *pointer = tmp;
    }
}

GLboolean yagl_host_glIsProgram(GLuint program)
{
    GLboolean res = GL_FALSE;
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX_RET(glIsProgram, GL_FALSE);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (program_obj) {
        res = program_obj->is_shader ? GL_FALSE : GL_TRUE;
    }

    yagl_gles2_program_release(program_obj);

    return res;
}

GLboolean yagl_host_glIsShader(GLuint shader)
{
    GLboolean res = GL_FALSE;
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX_RET(glIsShader, GL_FALSE);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (shader_obj) {
        res = shader_obj->is_shader ? GL_TRUE : GL_FALSE;
    }

    yagl_gles2_shader_release(shader_obj);

    return res;
}

void yagl_host_glLinkProgram(GLuint program)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glLinkProgram);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_link(program_obj);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glReleaseShaderCompiler(void)
{
    YAGL_GET_CTX(glReleaseShaderCompiler);

    /*
     * No-op.
     */
}

void yagl_host_glShaderBinary(const GLuint *shaders, int32_t shaders_count,
    GLenum binaryformat,
    const GLvoid *binary, int32_t binary_count)
{
    /*
     * Don't allow to load precompiled shaders.
     */

    YAGL_UNIMPLEMENTED(glShaderBinary);
}

void yagl_host_glShaderSource(GLuint shader,
    const GLchar *string, int32_t string_count)
{
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glShaderSource);

    if (string_count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!shader_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (!string) {
        goto out;
    }

    YAGL_LOG_TRACE("string 0 = %s", string);

    yagl_gles2_shader_source(shader_obj, string);

out:
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glStencilFuncSeparate(GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilFuncSeparate);

    ctx->driver->StencilFuncSeparate(face, func, ref, mask);
}

void yagl_host_glStencilMaskSeparate(GLenum face,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilMaskSeparate);

    ctx->driver->StencilMaskSeparate(face, mask);
}

void yagl_host_glStencilOpSeparate(GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    YAGL_GET_CTX(glStencilOpSeparate);

    ctx->driver->StencilOpSeparate(face, fail, zfail, zpass);
}

void yagl_host_glUniform1f(GLint location,
    GLfloat x)
{
    YAGL_GET_CTX(glUniform1f);

    ctx->driver->Uniform1f(location, x);
}

void yagl_host_glUniform1fv(GLint location,
    const GLfloat *v, int32_t v_count)
{

    YAGL_GET_CTX(glUniform1fv);

    ctx->driver->Uniform1fv(location, v_count, v);
}

void yagl_host_glUniform1i(GLint location,
    GLint x)
{
    YAGL_GET_CTX(glUniform1i);

    ctx->driver->Uniform1i(location, x);
}

void yagl_host_glUniform1iv(GLint location,
    const GLint *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform1iv);

    ctx->driver->Uniform1iv(location, v_count, v);
}

void yagl_host_glUniform2f(GLint location,
    GLfloat x,
    GLfloat y)
{
    YAGL_GET_CTX(glUniform2f);

    ctx->driver->Uniform2f(location, x, y);
}

void yagl_host_glUniform2fv(GLint location,
    const GLfloat *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform2fv);

    ctx->driver->Uniform2fv(location, (v_count / 2), v);
}

void yagl_host_glUniform2i(GLint location,
    GLint x,
    GLint y)
{
    YAGL_GET_CTX(glUniform2i);

    ctx->driver->Uniform2i(location, x, y);
}

void yagl_host_glUniform2iv(GLint location,
    const GLint *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform2iv);

    ctx->driver->Uniform2iv(location, (v_count / 2), v);
}

void yagl_host_glUniform3f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glUniform3f);

    ctx->driver->Uniform3f(location, x, y, z);
}

void yagl_host_glUniform3fv(GLint location,
    const GLfloat *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform3fv);

    ctx->driver->Uniform3fv(location, (v_count / 3), v);
}

void yagl_host_glUniform3i(GLint location,
    GLint x,
    GLint y,
    GLint z)
{
    YAGL_GET_CTX(glUniform3i);

    ctx->driver->Uniform3i(location, x, y, z);
}

void yagl_host_glUniform3iv(GLint location,
    const GLint *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform3iv);

    ctx->driver->Uniform3iv(location, (v_count / 3), v);
}

void yagl_host_glUniform4f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w)
{
    YAGL_GET_CTX(glUniform4f);

    ctx->driver->Uniform4f(location, x, y, z, w);
}

void yagl_host_glUniform4fv(GLint location,
    const GLfloat *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform4fv);

    ctx->driver->Uniform4fv(location, (v_count / 4), v);
}

void yagl_host_glUniform4i(GLint location,
    GLint x,
    GLint y,
    GLint z,
    GLint w)
{
    YAGL_GET_CTX(glUniform4i);

    ctx->driver->Uniform4i(location, x, y, z, w);
}

void yagl_host_glUniform4iv(GLint location,
    const GLint *v, int32_t v_count)
{
    YAGL_GET_CTX(glUniform4iv);

    ctx->driver->Uniform4iv(location, (v_count / 4), v);
}

void yagl_host_glUniformMatrix2fv(GLint location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count)
{
    YAGL_GET_CTX(glUniformMatrix2fv);

    ctx->driver->UniformMatrix2fv(location, value_count / (2 * 2), transpose, value);
}

void yagl_host_glUniformMatrix3fv(GLint location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count)
{
    YAGL_GET_CTX(glUniformMatrix3fv);

    ctx->driver->UniformMatrix3fv(location, value_count / (3 * 3), transpose, value);
}

void yagl_host_glUniformMatrix4fv(GLint location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count)
{
    YAGL_GET_CTX(glUniformMatrix4fv);

    ctx->driver->UniformMatrix4fv(location, value_count / (4 * 4), transpose, value);
}

void yagl_host_glUseProgram(GLuint program)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glUseProgram);

    if (program != 0) {
        program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
            YAGL_NS_SHADER_PROGRAM, program);

        if (!program_obj) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }

        if (program_obj->is_shader) {
            YAGL_SET_ERR(GL_INVALID_OPERATION);
            goto out;
        }
    }

    yagl_gles2_context_use_program(ctx, program);

    ctx->driver->UseProgram((program_obj ? program_obj->global_name : 0));

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glValidateProgram(GLuint program)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glValidateProgram);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (program_obj->is_shader) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    yagl_gles2_program_validate(program_obj);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x)
{
    YAGL_GET_CTX(glVertexAttrib1f);

    ctx->driver->VertexAttrib1f(indx, x);
}

void yagl_host_glVertexAttrib1fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    YAGL_GET_CTX(glVertexAttrib1fv);

    ctx->driver->VertexAttrib1fv(indx, values);
}

void yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y)
{
    YAGL_GET_CTX(glVertexAttrib2f);

    ctx->driver->VertexAttrib2f(indx, x, y);
}

void yagl_host_glVertexAttrib2fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    YAGL_GET_CTX(glVertexAttrib2fv);

    ctx->driver->VertexAttrib2fv(indx, values);
}

void yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glVertexAttrib3f);

    ctx->driver->VertexAttrib3f(indx, x, y, z);
}

void yagl_host_glVertexAttrib3fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    YAGL_GET_CTX(glVertexAttrib3fv);

    ctx->driver->VertexAttrib3fv(indx, values);
}

void yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w)
{
    YAGL_GET_CTX(glVertexAttrib4f);

    ctx->driver->VertexAttrib4f(indx, x, y, z, w);
}

void yagl_host_glVertexAttrib4fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    YAGL_GET_CTX(glVertexAttrib4fv);

    ctx->driver->VertexAttrib4fv(indx, values);
}

void yagl_host_glVertexAttribPointer(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    target_ulong ptr)
{
    struct yagl_gles_array *array = NULL;

    YAGL_GET_CTX(glVertexAttribPointer);

    array = yagl_gles_context_get_array(&ctx->base, indx);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return;
    }

    if (ctx->base.vbo) {
        if (!yagl_gles_array_update_vbo(array,
                                        size,
                                        type,
                                        false,
                                        normalized,
                                        stride,
                                        ctx->base.vbo,
                                        ctx->base.vbo_local_name,
                                        ptr)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    } else {
        if (!yagl_gles_array_update(array,
                                    size,
                                    type,
                                    false,
                                    normalized,
                                    stride,
                                    ptr)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }
}
