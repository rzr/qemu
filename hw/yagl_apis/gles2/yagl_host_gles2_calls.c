#include "yagl_host_gles2_calls.h"
#include "yagl_apis/gles/yagl_gles_array.h"
#include "yagl_apis/gles/yagl_gles_framebuffer.h"
#include "yagl_apis/gles/yagl_gles_renderbuffer.h"
#include "yagl_apis/gles/yagl_gles_texture.h"
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
#include "yagl_mem_gl.h"

#define YAGL_SET_ERR(err) \
    yagl_gles_context_set_error(&ctx->base, err); \
    YAGL_LOG_ERROR("error = 0x%X", err)

YAGL_DEFINE_TLS(struct yagl_gles2_api_ts*, gles2_api_ts);

#define YAGL_GET_CTX_RET(func, ret) \
    struct yagl_gles2_context *ctx = \
        (struct yagl_gles2_context*)gles2_api_ts->egl_iface->get_ctx(gles2_api_ts->egl_iface); \
    YAGL_LOG_FUNC_SET_TS(gles2_api_ts->ts, func); \
    if (!ctx || \
        (ctx->base.base.client_api != yagl_client_api_gles2)) { \
        YAGL_LOG_WARN("no current context"); \
        return ret; \
    }

#define YAGL_GET_CTX(func) YAGL_GET_CTX_RET(func,)

#define YAGL_UNIMPLEMENTED_RET(func, ret) \
    YAGL_GET_CTX_RET(func, ret); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    return ret;

#define YAGL_UNIMPLEMENTED(func) \
    YAGL_GET_CTX(func); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!");

static struct yagl_client_context
    *yagl_host_gles2_create_ctx(struct yagl_client_interface *iface,
                                struct yagl_sharegroup *sg)
{
    struct yagl_gles2_context *ctx =
        yagl_gles2_context_create(sg, gles2_api_ts->driver_ps);

    if (!ctx) {
        return NULL;
    }

    return &ctx->base.base;
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

static void yagl_host_gles2_thread_init(struct yagl_api_ps *api_ps,
                                        struct yagl_thread_state *ts)
{
    struct yagl_gles2_api_ps *gles2_api_ps = (struct yagl_gles2_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_host_gles2_thread_init, NULL);

    gles2_api_ps->driver_ps->thread_init(gles2_api_ps->driver_ps, ts);

    gles2_api_ts = g_malloc0(sizeof(*gles2_api_ts));

    yagl_gles2_api_ts_init(gles2_api_ts, ts, gles2_api_ps->driver_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_gles2_thread_fini(struct yagl_api_ps *api_ps)
{
    struct yagl_gles2_api_ps *gles2_api_ps = (struct yagl_gles2_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER_TS(gles2_api_ts->ts, yagl_host_gles2_thread_fini, NULL);

    yagl_gles2_api_ts_cleanup(gles2_api_ts);

    g_free(gles2_api_ts);

    gles2_api_ts = NULL;

    gles2_api_ps->driver_ps->thread_fini(gles2_api_ps->driver_ps);

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

    YAGL_LOG_FUNC_ENTER(api_ps->ps->id, 0, yagl_host_gles2_process_destroy, NULL);

    yagl_gles2_api_ps_cleanup(gles2_api_ps);
    yagl_api_ps_cleanup(&gles2_api_ps->base);

    g_free(gles2_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_api_ps
    *yagl_host_gles2_process_init(struct yagl_api *api,
                                  struct yagl_process_state *ps)
{
    struct yagl_gles2_api *gles2_api = (struct yagl_gles2_api*)api;
    struct yagl_gles2_driver_ps *driver_ps;
    struct yagl_gles2_api_ps *gles2_api_ps;
    struct yagl_client_interface *client_iface;

    YAGL_LOG_FUNC_ENTER(ps->id, 0, yagl_host_gles2_process_init, NULL);

    /*
     * Create driver ps.
     */

    driver_ps = gles2_api->driver->process_init(gles2_api->driver, ps);
    assert(driver_ps);

    /*
     * Create GLES2 interface.
     */

    client_iface = g_malloc0(sizeof(*client_iface));

    yagl_client_interface_init(client_iface);

    client_iface->create_ctx = &yagl_host_gles2_create_ctx;

    /*
     * Finally, create API ps.
     */

    gles2_api_ps = g_malloc0(sizeof(*gles2_api_ps));

    yagl_api_ps_init(&gles2_api_ps->base, api, ps);

    gles2_api_ps->base.thread_init = &yagl_host_gles2_thread_init;
    gles2_api_ps->base.get_func = &yagl_host_gles2_get_func;
    gles2_api_ps->base.thread_fini = &yagl_host_gles2_thread_fini;
    gles2_api_ps->base.fini = &yagl_host_gles2_process_fini;
    gles2_api_ps->base.destroy = &yagl_host_gles2_process_destroy;

    yagl_gles2_api_ps_init(gles2_api_ps, driver_ps, client_iface);

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
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
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
    target_ulong /* const GLchar* */ name_)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLchar *name = NULL;

    YAGL_GET_CTX(glBindAttribLocation);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (name_) {
        name = yagl_mem_get_string(gles2_api_ts->ts, name_);
        if (!name) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
        YAGL_LOG_TRACE("binding attrib %s location to %d", name, index);
    }

    yagl_gles2_program_bind_attrib_location(program_obj, index, name);

out:
    g_free(name);
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glBindFramebuffer(GLenum target,
    GLuint framebuffer)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;

    YAGL_GET_CTX(glBindFramebuffer);

    if (framebuffer != 0) {
        framebuffer_obj = (struct yagl_gles_framebuffer*)yagl_sharegroup_acquire_object(ctx->sg,
            YAGL_NS_FRAMEBUFFER, framebuffer);

        if (!framebuffer_obj) {
            framebuffer_obj = yagl_gles_framebuffer_create(ctx->driver_ps->common);

            if (!framebuffer_obj) {
                goto out;
            }

            framebuffer_obj = (struct yagl_gles_framebuffer*)yagl_sharegroup_add_named(ctx->sg,
               YAGL_NS_FRAMEBUFFER, framebuffer, &framebuffer_obj->base);
        }
    }

    if (!yagl_gles_context_bind_framebuffer(&ctx->base, target, framebuffer_obj, framebuffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    ctx->driver_ps->common->BindFramebuffer(ctx->driver_ps->common,
                                            target,
                                            (framebuffer_obj ? framebuffer_obj->global_name : 0));

out:
    yagl_gles_framebuffer_release(framebuffer_obj);
}

void yagl_host_glBindRenderbuffer(GLenum target,
    GLuint renderbuffer)
{
    struct yagl_gles_renderbuffer *renderbuffer_obj = NULL;

    YAGL_GET_CTX(glBindRenderbuffer);

    if (renderbuffer != 0) {
        renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_acquire_object(ctx->sg,
            YAGL_NS_RENDERBUFFER, renderbuffer);

        if (!renderbuffer_obj) {
            renderbuffer_obj = yagl_gles_renderbuffer_create(ctx->driver_ps->common);

            if (!renderbuffer_obj) {
                goto out;
            }

            renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_add_named(ctx->sg,
               YAGL_NS_RENDERBUFFER, renderbuffer, &renderbuffer_obj->base);
        }
    }

    if (!yagl_gles_context_bind_renderbuffer(&ctx->base, target, renderbuffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    ctx->driver_ps->common->BindRenderbuffer(ctx->driver_ps->common,
                                            target,
                                            (renderbuffer_obj ? renderbuffer_obj->global_name : 0));

out:
    yagl_gles_renderbuffer_release(renderbuffer_obj);
}

void yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    YAGL_GET_CTX(glBlendColor);

    ctx->driver_ps->BlendColor(ctx->driver_ps, red, green, blue, alpha);
}

void yagl_host_glBlendEquation(GLenum mode)
{
    YAGL_GET_CTX(glBlendEquation);

    ctx->driver_ps->BlendEquation(ctx->driver_ps, mode);
}

void yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha)
{
    YAGL_GET_CTX(glBlendEquationSeparate);

    ctx->driver_ps->BlendEquationSeparate(ctx->driver_ps, modeRGB, modeAlpha);
}

void yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha)
{
    YAGL_GET_CTX(glBlendFuncSeparate);

    ctx->driver_ps->BlendFuncSeparate(ctx->driver_ps,
                                      srcRGB,
                                      dstRGB,
                                      srcAlpha,
                                      dstAlpha);
}

GLenum yagl_host_glCheckFramebufferStatus(GLenum target)
{
    YAGL_GET_CTX_RET(glCheckFramebufferStatus, 0);
    return ctx->driver_ps->common->CheckFramebufferStatus(ctx->driver_ps->common,
                                                          target);
}

void yagl_host_glCompileShader(GLuint shader)
{
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glCompileShader);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    yagl_gles2_shader_compile(shader_obj);

out:
    yagl_gles2_shader_release(shader_obj);
}

GLuint yagl_host_glCreateProgram(void)
{
    GLuint ret = 0;
    struct yagl_gles2_program *program = NULL;

    YAGL_GET_CTX_RET(glCreateProgram, 0);

    program = yagl_gles2_program_create(ctx->driver_ps);

    if (!program) {
        goto out;
    }

    ret = yagl_sharegroup_add(ctx->sg, YAGL_NS_PROGRAM, &program->base);

out:
    yagl_gles2_program_release(program);

    return ret;
}

GLuint yagl_host_glCreateShader(GLenum type)
{
    GLuint ret = 0;
    struct yagl_gles2_shader *shader = NULL;

    YAGL_GET_CTX_RET(glCreateShader, 0);

    shader = yagl_gles2_shader_create(ctx->driver_ps, type);

    if (!shader) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    ret = yagl_sharegroup_add(ctx->sg, YAGL_NS_SHADER, &shader->base);

out:
    yagl_gles2_shader_release(shader);

    return ret;
}

void yagl_host_glDeleteFramebuffers(GLsizei n,
    target_ulong /* const GLuint* */ framebuffers_)
{
    GLuint *framebuffer_names = NULL;
    GLsizei i;

    YAGL_GET_CTX(glDeleteFramebuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    framebuffer_names = g_malloc0(n * sizeof(*framebuffer_names));

    if (framebuffers_) {
        if (!yagl_mem_get(gles2_api_ts->ts,
                          framebuffers_,
                          n * sizeof(*framebuffer_names),
                          framebuffer_names)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    if (framebuffer_names) {
        for (i = 0; i < n; ++i) {
            yagl_gles_context_unbind_framebuffer(&ctx->base, framebuffer_names[i]);

            yagl_sharegroup_remove(ctx->sg,
                                   YAGL_NS_FRAMEBUFFER,
                                   framebuffer_names[i]);
        }
    }

out:
    g_free(framebuffer_names);
}

void yagl_host_glDeleteProgram(GLuint program)
{
    YAGL_GET_CTX(glDeleteProgram);

    yagl_gles2_context_unuse_program(ctx, program);

    yagl_sharegroup_remove(ctx->sg,
                           YAGL_NS_PROGRAM,
                           program);
}

void yagl_host_glDeleteRenderbuffers(GLsizei n,
    target_ulong /* const GLuint* */ renderbuffers_)
{
    GLuint *renderbuffer_names = NULL;
    GLsizei i;

    YAGL_GET_CTX(glDeleteRenderbuffers);

    if (n < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    renderbuffer_names = g_malloc0(n * sizeof(*renderbuffer_names));

    if (renderbuffers_) {
        if (!yagl_mem_get(gles2_api_ts->ts,
                          renderbuffers_,
                          n * sizeof(*renderbuffer_names),
                          renderbuffer_names)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    if (renderbuffer_names) {
        for (i = 0; i < n; ++i) {
            yagl_gles_context_unbind_renderbuffer(&ctx->base, renderbuffer_names[i]);

            yagl_sharegroup_remove(ctx->sg,
                                   YAGL_NS_RENDERBUFFER,
                                   renderbuffer_names[i]);
        }
    }

out:
    g_free(renderbuffer_names);
}

void yagl_host_glDeleteShader(GLuint shader)
{
    YAGL_GET_CTX(glDeleteShader);

    yagl_sharegroup_remove(ctx->sg,
                           YAGL_NS_SHADER,
                           shader);
}

void yagl_host_glDetachShader(GLuint program,
    GLuint shader)
{
    struct yagl_gles2_program *program_obj = NULL;
    struct yagl_gles2_shader *shader_obj = NULL;

    YAGL_GET_CTX(glDetachShader);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
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

    ctx->driver_ps->DisableVertexAttribArray(ctx->driver_ps, index);
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

    ctx->driver_ps->EnableVertexAttribArray(ctx->driver_ps, index);
}

void yagl_host_glFramebufferRenderbuffer(GLenum target,
    GLenum attachment,
    GLenum renderbuffertarget,
    GLuint renderbuffer)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;
    struct yagl_gles_renderbuffer *renderbuffer_obj = NULL;

    YAGL_GET_CTX(glFramebufferRenderbuffer);

    framebuffer_obj = yagl_gles_context_acquire_binded_framebuffer(&ctx->base, target);

    if (!framebuffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (renderbuffer) {
        renderbuffer_obj = (struct yagl_gles_renderbuffer*)yagl_sharegroup_acquire_object(ctx->sg,
            YAGL_NS_RENDERBUFFER, renderbuffer);

        if (!renderbuffer_obj) {
            YAGL_SET_ERR(GL_INVALID_OPERATION);
            goto out;
        }
    }

    if (!yagl_gles_framebuffer_renderbuffer(framebuffer_obj,
                                            target,
                                            attachment,
                                            renderbuffertarget,
                                            renderbuffer_obj,
                                            renderbuffer)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

out:
    yagl_gles_renderbuffer_release(renderbuffer_obj);
    yagl_gles_framebuffer_release(framebuffer_obj);
}

void yagl_host_glFramebufferTexture2D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level)
{
    struct yagl_gles_framebuffer *framebuffer_obj = NULL;
    struct yagl_gles_texture *texture_obj = NULL;

    YAGL_GET_CTX(glFramebufferTexture2D);

    framebuffer_obj = yagl_gles_context_acquire_binded_framebuffer(&ctx->base, target);

    if (!framebuffer_obj) {
        YAGL_SET_ERR(GL_INVALID_OPERATION);
        goto out;
    }

    if (texture) {
        texture_obj = (struct yagl_gles_texture*)yagl_sharegroup_acquire_object(ctx->sg,
            YAGL_NS_TEXTURE, texture);

        if (!texture_obj) {
            YAGL_SET_ERR(GL_INVALID_OPERATION);
            goto out;
        }
    }

    if (!yagl_gles_framebuffer_texture2d(framebuffer_obj,
                                         target,
                                         attachment,
                                         textarget,
                                         level,
                                         texture_obj,
                                         texture)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

out:
    yagl_gles_texture_release(texture_obj);
    yagl_gles_framebuffer_release(framebuffer_obj);
}

void yagl_host_glGenerateMipmap(GLenum target)
{
    YAGL_GET_CTX(glGenerateMipmap);

    ctx->driver_ps->common->GenerateMipmap(ctx->driver_ps->common, target);
}

void yagl_host_glGenFramebuffers(GLsizei n,
    target_ulong /* GLuint* */ framebuffers_)
{
    struct yagl_gles_framebuffer **framebuffers = NULL;
    GLsizei i;
    GLuint *framebuffer_names = NULL;

    YAGL_GET_CTX(glGenFramebuffers);

    framebuffers = g_malloc0(n * sizeof(*framebuffers));

    for (i = 0; i < n; ++i) {
        framebuffers[i] = yagl_gles_framebuffer_create(ctx->driver_ps->common);

        if (!framebuffers[i]) {
            goto out;
        }
    }

    framebuffer_names = g_malloc(n * sizeof(*framebuffer_names));

    for (i = 0; i < n; ++i) {
        framebuffer_names[i] = yagl_sharegroup_add(ctx->sg,
                                                   YAGL_NS_FRAMEBUFFER,
                                                   &framebuffers[i]->base);
    }

    if (framebuffers_) {
        for (i = 0; i < n; ++i) {
            yagl_mem_put_GLuint(gles2_api_ts->ts,
                                framebuffers_ + (i * sizeof(GLuint)),
                                framebuffer_names[i]);
        }
    }

out:
    g_free(framebuffer_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_framebuffer_release(framebuffers[i]);
    }
    g_free(framebuffers);
}

void yagl_host_glGenRenderbuffers(GLsizei n,
    target_ulong /* GLuint* */ renderbuffers_)
{
    struct yagl_gles_renderbuffer **renderbuffers = NULL;
    GLsizei i;
    GLuint *renderbuffer_names = NULL;

    YAGL_GET_CTX(glGenRenderbuffers);

    renderbuffers = g_malloc0(n * sizeof(*renderbuffers));

    for (i = 0; i < n; ++i) {
        renderbuffers[i] = yagl_gles_renderbuffer_create(ctx->driver_ps->common);

        if (!renderbuffers[i]) {
            goto out;
        }
    }

    renderbuffer_names = g_malloc(n * sizeof(*renderbuffer_names));

    for (i = 0; i < n; ++i) {
        renderbuffer_names[i] = yagl_sharegroup_add(ctx->sg,
                                                    YAGL_NS_RENDERBUFFER,
                                                    &renderbuffers[i]->base);
    }

    if (renderbuffers_) {
        for (i = 0; i < n; ++i) {
            yagl_mem_put_GLuint(gles2_api_ts->ts,
                                renderbuffers_ + (i * sizeof(GLuint)),
                                renderbuffer_names[i]);
        }
    }

out:
    g_free(renderbuffer_names);
    for (i = 0; i < n; ++i) {
        yagl_gles_renderbuffer_release(renderbuffers[i]);
    }
    g_free(renderbuffers);
}

void yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLint* */ size_,
    target_ulong /* GLenum* */ type_,
    target_ulong /* GLchar* */ name_)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLchar *name = NULL;
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;

    YAGL_GET_CTX(glGetActiveAttrib);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (bufsize > 0) {
        name = g_malloc(bufsize);
    }

    yagl_gles2_program_get_active_attrib(program_obj,
                                         index,
                                         bufsize,
                                         &length,
                                         &size,
                                         &type,
                                         name);

    YAGL_LOG_TRACE("got active attrib: size = %d, type = %u, name = %s",
                   size, type, name);

    if (length_) {
        yagl_mem_put_GLsizei(gles2_api_ts->ts, length_, length);
    }

    if (size_) {
        yagl_mem_put_GLint(gles2_api_ts->ts, size_, size);
    }

    if (type_) {
        yagl_mem_put_GLenum(gles2_api_ts->ts, type_, type);
    }

    if (name_ && name) {
        yagl_mem_put(gles2_api_ts->ts, name_, length + 1, name);
    }

out:
    g_free(name);
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLint* */ size_,
    target_ulong /* GLenum* */ type_,
    target_ulong /* GLchar* */ name_)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLchar *name = NULL;
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;

    YAGL_GET_CTX(glGetActiveUniform);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (bufsize > 0) {
        name = g_malloc(bufsize);
    }

    yagl_gles2_program_get_active_uniform(program_obj,
                                          index,
                                          bufsize,
                                          &length,
                                          &size,
                                          &type,
                                          name);

    YAGL_LOG_TRACE("got active uniform: size = %d, type = %u, name = %s",
                   size, type, name);

    if (length_) {
        yagl_mem_put_GLsizei(gles2_api_ts->ts, length_, length);
    }

    if (size_) {
        yagl_mem_put_GLint(gles2_api_ts->ts, size_, size);
    }

    if (type_) {
        yagl_mem_put_GLenum(gles2_api_ts->ts, type_, type);
    }

    if (name_ && name) {
        yagl_mem_put(gles2_api_ts->ts, name_, length + 1, name);
    }

out:
    g_free(name);
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetAttachedShaders(GLuint program,
    GLsizei maxcount,
    target_ulong /* GLsizei* */ count,
    target_ulong /* GLuint* */ shaders)
{
    YAGL_UNIMPLEMENTED(glGetAttachedShaders);
}

int yagl_host_glGetAttribLocation(GLuint program,
    target_ulong /* const GLchar* */ name_)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLchar *name = NULL;
    int ret = 0;

    YAGL_GET_CTX_RET(glGetAttribLocation, 0);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (name_) {
        name = yagl_mem_get_string(gles2_api_ts->ts, name_);
        if (!name) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
        YAGL_LOG_TRACE("getting attrib %s location", name);
    }

    ret = yagl_gles2_program_get_attrib_location(program_obj, name);

out:
    g_free(name);
    yagl_gles2_program_release(program_obj);

    return ret;
}

void yagl_host_glGetFramebufferAttachmentParameteriv(GLenum target,
    GLenum attachment,
    GLenum pname,
    target_ulong /* GLint* */ params)
{
    YAGL_UNIMPLEMENTED(glGetFramebufferAttachmentParameteriv);
}

void yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLint params = 0;

    YAGL_GET_CTX(glGetProgramiv);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (params_) {
        if (!yagl_mem_get_GLint(gles2_api_ts->ts, params_, &params)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    yagl_gles2_program_get_param(program_obj, pname, &params);

    if (params_) {
        yagl_mem_put_GLint(gles2_api_ts->ts, params_, params);
    }

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glGetProgramInfoLog(GLuint program,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length,
    target_ulong /* GLchar* */ infolog)
{
    YAGL_UNIMPLEMENTED(glGetProgramInfoLog);
}

void yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params)
{
    YAGL_UNIMPLEMENTED(glGetRenderbufferParameteriv);
}

void yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    struct yagl_gles2_shader *shader_obj = NULL;
    GLint params = 0;

    YAGL_GET_CTX(glGetShaderiv);

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (params_) {
        if (!yagl_mem_get_GLint(gles2_api_ts->ts, params_, &params)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    yagl_gles2_shader_get_param(shader_obj, pname, &params);

    if (params_) {
        yagl_mem_put_GLint(gles2_api_ts->ts, params_, params);
    }

out:
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glGetShaderInfoLog(GLuint shader,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length,
    target_ulong /* GLchar* */ infolog)
{
    YAGL_UNIMPLEMENTED(glGetShaderInfoLog);
}

void yagl_host_glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype,
    target_ulong /* GLint* */ range,
    target_ulong /* GLint* */ precision)
{
    YAGL_UNIMPLEMENTED(glGetShaderPrecisionFormat);
}

void yagl_host_glGetShaderSource(GLuint shader,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length,
    target_ulong /* GLchar* */ source)
{
    YAGL_UNIMPLEMENTED(glGetShaderSource);
}

void yagl_host_glGetUniformfv(GLuint program,
    GLint location,
    target_ulong /* GLfloat* */ params)
{
    YAGL_UNIMPLEMENTED(glGetUniformfv);
}

void yagl_host_glGetUniformiv(GLuint program,
    GLint location,
    target_ulong /* GLint* */ params)
{
    YAGL_UNIMPLEMENTED(glGetUniformiv);
}

int yagl_host_glGetUniformLocation(GLuint program,
    target_ulong /* const GLchar* */ name_)
{
    struct yagl_gles2_program *program_obj = NULL;
    GLchar *name = NULL;
    int ret = 0;

    YAGL_GET_CTX_RET(glGetUniformLocation, 0);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (name_) {
        name = yagl_mem_get_string(gles2_api_ts->ts, name_);
        if (!name) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
        YAGL_LOG_TRACE("getting uniform %s location", name);
    }

    ret = yagl_gles2_program_get_uniform_location(program_obj, name);

out:
    g_free(name);
    yagl_gles2_program_release(program_obj);

    return ret;
}

void yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    target_ulong /* GLfloat* */ params)
{
    YAGL_UNIMPLEMENTED(glGetVertexAttribfv);
}

void yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    struct yagl_gles_array *array = NULL;
    int i, count = 0;
    GLint *params = NULL;

    YAGL_GET_CTX(glGetVertexAttribiv);

    array = yagl_gles_context_get_array(&ctx->base, index);

    if (!array) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (!yagl_gles2_get_array_param_count(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    params = g_malloc0(count * sizeof(*params));

    switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        params[0] = array->vbo_local_name;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        params[0] = array->enabled;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
        params[0] = array->size;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
        params[0] = array->stride;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
        params[0] = array->type;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        params[0] = array->normalized;
        break;
    default:
        ctx->driver_ps->GetVertexAttribiv(ctx->driver_ps, index, pname, params);
        break;
    }

    if (params_) {
        for (i = 0; i < count; ++i) {
            yagl_mem_put_GLint(gles2_api_ts->ts,
                               params_ + (i * sizeof(*params)),
                               params[i]);
        }
    }

out:
    g_free(params);
}

void yagl_host_glGetVertexAttribPointerv(GLuint index,
    GLenum pname,
    target_ulong /* GLvoid** */ pointer)
{
    YAGL_UNIMPLEMENTED(glGetVertexAttribPointerv);
}

GLboolean yagl_host_glIsFramebuffer(GLuint framebuffer)
{
    YAGL_UNIMPLEMENTED_RET(glIsFramebuffer, GL_FALSE);
}

GLboolean yagl_host_glIsProgram(GLuint program)
{
    YAGL_UNIMPLEMENTED_RET(glIsProgram, GL_FALSE);
}

GLboolean yagl_host_glIsRenderbuffer(GLuint renderbuffer)
{
    YAGL_UNIMPLEMENTED_RET(glIsRenderbuffer, GL_FALSE);
}

GLboolean yagl_host_glIsShader(GLuint shader)
{
    YAGL_UNIMPLEMENTED_RET(glIsShader, GL_FALSE);
}

void yagl_host_glLinkProgram(GLuint program)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glLinkProgram);

    program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_PROGRAM, program);

    if (!program_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    yagl_gles2_program_link(program_obj);

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glReleaseShaderCompiler(void)
{
    YAGL_GET_CTX(glReleaseShaderCompiler);

    ctx->driver_ps->ReleaseShaderCompiler(ctx->driver_ps);
}

void yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height)
{
    YAGL_GET_CTX(glRenderbufferStorage);

    ctx->driver_ps->common->RenderbufferStorage(ctx->driver_ps->common,
                                                target,
                                                internalformat,
                                                width,
                                                height);
}

void yagl_host_glShaderBinary(GLsizei n,
    target_ulong /* const GLuint* */ shaders,
    GLenum binaryformat,
    target_ulong /* const GLvoid* */ binary,
    GLsizei length)
{
    YAGL_UNIMPLEMENTED(glShaderBinary);
}

void yagl_host_glShaderSource(GLuint shader,
    GLsizei count,
    target_ulong /* const GLchar** */ string_,
    target_ulong /* const GLint* */ length_)
{
    struct yagl_gles2_shader *shader_obj = NULL;
    target_ulong *string_ptrs = NULL;
    GLint *lengths = NULL;
    GLchar** strings = NULL;
    int i;

    YAGL_GET_CTX(glShaderSource);

    if (count < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    shader_obj = (struct yagl_gles2_shader*)yagl_sharegroup_acquire_object(ctx->sg,
        YAGL_NS_SHADER, shader);

    if (!shader_obj) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    string_ptrs = g_malloc(count * sizeof(*string_ptrs));
    if (!yagl_mem_get(gles2_api_ts->ts, string_, count * sizeof(*string_ptrs), string_ptrs)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        goto out;
    }

    if (length_) {
        lengths = g_malloc(count * sizeof(*lengths));
        if (!yagl_mem_get(gles2_api_ts->ts, length_, count * sizeof(*lengths), lengths)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    strings = g_malloc0(count * sizeof(*strings));

    for (i = 0; i < count; ++i) {
        char *tmp;

        if (lengths && (lengths[i] >= 0)) {
            tmp = g_malloc(lengths[i] + 1);
            if (!yagl_mem_get(gles2_api_ts->ts, string_ptrs[i], lengths[i], tmp)) {
                g_free(tmp);
                tmp = NULL;
            } else {
                tmp[lengths[i]] = '\0';
            }
        } else {
            tmp = yagl_mem_get_string(gles2_api_ts->ts, string_ptrs[i]);
        }

        if (!tmp) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }

        strings[i] = tmp;

        YAGL_LOG_TRACE("string %d = %s", i, tmp);
    }

    yagl_gles2_shader_source(shader_obj,
                             strings,
                             count,
                             ctx->shader_strip_precision);

out:
    if (strings) {
        for (i = 0; i < count; ++i) {
            g_free(strings[i]);
        }
        g_free(strings);
    }
    g_free(string_ptrs);
    g_free(lengths);
    yagl_gles2_shader_release(shader_obj);
}

void yagl_host_glStencilFuncSeparate(GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilFuncSeparate);

    ctx->driver_ps->StencilFuncSeparate(ctx->driver_ps, face, func, ref, mask);
}

void yagl_host_glStencilMaskSeparate(GLenum face,
    GLuint mask)
{
    YAGL_GET_CTX(glStencilMaskSeparate);

    ctx->driver_ps->StencilMaskSeparate(ctx->driver_ps, face, mask);
}

void yagl_host_glStencilOpSeparate(GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    YAGL_GET_CTX(glStencilOpSeparate);

    ctx->driver_ps->StencilOpSeparate(ctx->driver_ps, face, fail, zfail, zpass);
}

void yagl_host_glUniform1f(GLint location,
    GLfloat x)
{
    YAGL_GET_CTX(glUniform1f);

    ctx->driver_ps->Uniform1f(ctx->driver_ps, location, x);
}

void yagl_host_glUniform1fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_)
{
    GLfloat *v = NULL;

    YAGL_GET_CTX(glUniform1fv);

    if (v_) {
        v = g_malloc(count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform1fv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform1i(GLint location,
    GLint x)
{
    YAGL_GET_CTX(glUniform1i);

    ctx->driver_ps->Uniform1i(ctx->driver_ps, location, x);
}

void yagl_host_glUniform1iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_)
{
    GLint *v = NULL;

    YAGL_GET_CTX(glUniform1iv);

    if (v_) {
        v = g_malloc(count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform1iv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform2f(GLint location,
    GLfloat x,
    GLfloat y)
{
    YAGL_GET_CTX(glUniform2f);

    ctx->driver_ps->Uniform2f(ctx->driver_ps, location, x, y);
}

void yagl_host_glUniform2fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_)
{
    GLfloat *v = NULL;

    YAGL_GET_CTX(glUniform2fv);

    if (v_) {
        v = g_malloc(2 * count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          2 * count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform2fv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform2i(GLint location,
    GLint x,
    GLint y)
{
    YAGL_GET_CTX(glUniform2i);

    ctx->driver_ps->Uniform2i(ctx->driver_ps, location, x, y);
}

void yagl_host_glUniform2iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_)
{
    GLint *v = NULL;

    YAGL_GET_CTX(glUniform2iv);

    if (v_) {
        v = g_malloc(2 * count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          2 * count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform2iv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform3f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glUniform3f);

    ctx->driver_ps->Uniform3f(ctx->driver_ps, location, x, y, z);
}

void yagl_host_glUniform3fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_)
{
    GLfloat *v = NULL;

    YAGL_GET_CTX(glUniform3fv);

    if (v_) {
        v = g_malloc(3 * count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          3 * count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform3fv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform3i(GLint location,
    GLint x,
    GLint y,
    GLint z)
{
    YAGL_GET_CTX(glUniform3i);

    ctx->driver_ps->Uniform3i(ctx->driver_ps, location, x, y, z);
}

void yagl_host_glUniform3iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_)
{
    GLint *v = NULL;

    YAGL_GET_CTX(glUniform3iv);

    if (v_) {
        v = g_malloc(3 * count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          3 * count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform3iv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform4f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w)
{
    YAGL_GET_CTX(glUniform4f);

    ctx->driver_ps->Uniform4f(ctx->driver_ps, location, x, y, z, w);
}

void yagl_host_glUniform4fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_)
{
    GLfloat *v = NULL;

    YAGL_GET_CTX(glUniform4fv);

    if (v_) {
        v = g_malloc(4 * count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          4 * count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform4fv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniform4i(GLint location,
    GLint x,
    GLint y,
    GLint z,
    GLint w)
{
    YAGL_GET_CTX(glUniform4i);

    ctx->driver_ps->Uniform4i(ctx->driver_ps, location, x, y, z, w);
}

void yagl_host_glUniform4iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_)
{
    GLint *v = NULL;

    YAGL_GET_CTX(glUniform4iv);

    if (v_) {
        v = g_malloc(4 * count * sizeof(*v));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          v_,
                          4 * count * sizeof(*v),
                          v)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->Uniform4iv(ctx->driver_ps, location, count, v);

out:
    g_free(v);
}

void yagl_host_glUniformMatrix2fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_)
{
    GLfloat *value = NULL;

    YAGL_GET_CTX(glUniformMatrix2fv);

    if (value_) {
        value = g_malloc(2 * 2 * count * sizeof(*value));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          value_,
                          2 * 2 * count * sizeof(*value),
                          value)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->UniformMatrix2fv(ctx->driver_ps, location, count, transpose, value);

out:
    g_free(value);
}

void yagl_host_glUniformMatrix3fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_)
{
    GLfloat *value = NULL;

    YAGL_GET_CTX(glUniformMatrix3fv);

    if (value_) {
        value = g_malloc(3 * 3 * count * sizeof(*value));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          value_,
                          3 * 3 * count * sizeof(*value),
                          value)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->UniformMatrix3fv(ctx->driver_ps, location, count, transpose, value);

out:
    g_free(value);
}

void yagl_host_glUniformMatrix4fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_)
{
    GLfloat *value = NULL;

    YAGL_GET_CTX(glUniformMatrix4fv);

    if (value_) {
        value = g_malloc(4 * 4 * count * sizeof(*value));
        if (!yagl_mem_get(gles2_api_ts->ts,
                          value_,
                          4 * 4 * count * sizeof(*value),
                          value)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    ctx->driver_ps->UniformMatrix4fv(ctx->driver_ps, location, count, transpose, value);

out:
    g_free(value);
}

void yagl_host_glUseProgram(GLuint program)
{
    struct yagl_gles2_program *program_obj = NULL;

    YAGL_GET_CTX(glUseProgram);

    if (program != 0) {
        program_obj = (struct yagl_gles2_program*)yagl_sharegroup_acquire_object(ctx->sg,
            YAGL_NS_PROGRAM, program);

        if (!program_obj) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            goto out;
        }
    }

    yagl_gles2_context_use_program(ctx, program);

    ctx->driver_ps->UseProgram(ctx->driver_ps,
                               (program_obj ? program_obj->global_name : 0));

out:
    yagl_gles2_program_release(program_obj);
}

void yagl_host_glValidateProgram(GLuint program)
{
    YAGL_UNIMPLEMENTED(glValidateProgram);
}

void yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x)
{
    YAGL_GET_CTX(glVertexAttrib1f);

    ctx->driver_ps->VertexAttrib1f(ctx->driver_ps, indx, x);
}

void yagl_host_glVertexAttrib1fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_)
{
    GLfloat values[1];

    YAGL_GET_CTX(glVertexAttrib1fv);

    if (values_) {
        if (!yagl_mem_get(gles2_api_ts->ts,
                          values_,
                          sizeof(values),
                          &values[0])) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            return;
        }
    }

    ctx->driver_ps->VertexAttrib1fv(ctx->driver_ps,
                                    indx,
                                    (values_ ? &values[0] : NULL));
}

void yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y)
{
    YAGL_GET_CTX(glVertexAttrib2f);

    ctx->driver_ps->VertexAttrib2f(ctx->driver_ps, indx, x, y);
}

void yagl_host_glVertexAttrib2fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_)
{
    GLfloat values[2];

    YAGL_GET_CTX(glVertexAttrib2fv);

    if (values_) {
        if (!yagl_mem_get(gles2_api_ts->ts,
                          values_,
                          sizeof(values),
                          &values[0])) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            return;
        }
    }

    ctx->driver_ps->VertexAttrib2fv(ctx->driver_ps,
                                    indx,
                                    (values_ ? &values[0] : NULL));
}

void yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glVertexAttrib3f);

    ctx->driver_ps->VertexAttrib3f(ctx->driver_ps, indx, x, y, z);
}

void yagl_host_glVertexAttrib3fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_)
{
    GLfloat values[3];

    YAGL_GET_CTX(glVertexAttrib3fv);

    if (values_) {
        if (!yagl_mem_get(gles2_api_ts->ts,
                          values_,
                          sizeof(values),
                          &values[0])) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            return;
        }
    }

    ctx->driver_ps->VertexAttrib3fv(ctx->driver_ps,
                                    indx,
                                    (values_ ? &values[0] : NULL));
}

void yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w)
{
    YAGL_GET_CTX(glVertexAttrib4f);

    ctx->driver_ps->VertexAttrib4f(ctx->driver_ps, indx, x, y, z, w);
}

void yagl_host_glVertexAttrib4fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_)
{
    GLfloat values[4];

    YAGL_GET_CTX(glVertexAttrib4fv);

    if (values_) {
        if (!yagl_mem_get(gles2_api_ts->ts,
                          values_,
                          sizeof(values),
                          &values[0])) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
            return;
        }
    }

    ctx->driver_ps->VertexAttrib4fv(ctx->driver_ps,
                                    indx,
                                    (values_ ? &values[0] : NULL));
}

void yagl_host_glVertexAttribPointer(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    target_ulong /* const GLvoid* */ ptr)
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
                                    normalized,
                                    stride,
                                    ptr)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }
}
