#include "yagl_host_gles1_calls.h"
#include "yagl_apis/gles/yagl_gles_array.h"
#include "yagl_apis/gles/yagl_gles_framebuffer.h"
#include "yagl_apis/gles/yagl_gles_renderbuffer.h"
#include "yagl_apis/gles/yagl_gles_texture.h"
#include "yagl_apis/gles/yagl_gles_texture_unit.h"
#include "yagl_apis/gles/yagl_gles_image.h"
#include "yagl_gles1_calls.h"
#include "yagl_gles1_api.h"
#include "yagl_gles1_driver.h"
#include "yagl_gles1_api_ps.h"
#include "yagl_gles1_api_ts.h"
#include "yagl_gles1_context.h"
#include "yagl_egl_interface.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_client_interface.h"
#include "yagl_sharegroup.h"
#include "yagl_mem_gl.h"

#define YAGL_TEX_ENV_PARAM_MAX_LEN        4
#define YAGL_POINT_PARAM_MAX_LEN          3
#define YAGL_FOG_PARAM_MAX_LEN            4
#define YAGL_LIGHT_PARAM_MAX_LEN          4
#define YAGL_LIGHT_MODEL_PARAM_MAX_LEN    4
#define YAGL_MATERIAL_PARAM_MAX_LEN       4

#define YAGL_SET_ERR(err) \
    yagl_gles_context_set_error(&ctx->base, err); \
    YAGL_LOG_ERROR("error = 0x%X", err)

static YAGL_DEFINE_TLS(YaglGles1ApiTs *, gles1_api_ts);

#define YAGL_GET_CTX_IMPL(func, ret_expr) \
    YaglGles1Context *ctx = \
        (YaglGles1Context *)cur_ts->ps->egl_iface->get_ctx(cur_ts->ps->egl_iface); \
    YAGL_LOG_FUNC_SET(func); \
    if (!ctx || \
        (ctx->base.base.client_api != yagl_client_api_gles1)) { \
        YAGL_LOG_WARN("no current context"); \
        ret_expr; \
        return true; \
    }

#define YAGL_GET_CTX_RET(func, ret) YAGL_GET_CTX_IMPL(func, *retval = ret)

#define YAGL_GET_CTX(func) YAGL_GET_CTX_IMPL(func,)

static bool yagl_gles1_get_float(struct yagl_gles_context *ctx,
                                 GLenum pname,
                                 GLfloat *param)
{
    switch (pname) {
    case GL_ACTIVE_TEXTURE:
        *param = GL_TEXTURE0 + ctx->active_texture_unit;
        break;
    case GL_TEXTURE_BINDING_2D:
        *param = yagl_gles_context_get_active_texture_target_state(ctx,
            yagl_gles_texture_target_2d)->texture_local_name;
        break;
    case GL_ARRAY_BUFFER_BINDING:
        *param = ctx->vbo_local_name;
        break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        *param = ctx->ebo_local_name;
        break;
    default:
        return false;
    }

    return true;
}

static struct yagl_client_context
    *yagl_host_gles1_create_ctx(struct yagl_client_interface *iface,
                                struct yagl_sharegroup *sg)
{
    YaglGles1Context *ctx =
        yagl_gles1_context_create(sg, gles1_api_ts->driver);

    if (!ctx) {
        return NULL;
    }

    return &ctx->base.base;
}

static void yagl_host_gles1_thread_init(struct yagl_api_ps *api_ps)
{
    YaglGles1ApiPs *gles1_api_ps = (YaglGles1ApiPs *)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles1_thread_init, NULL);

    gles1_api_ts = g_malloc0(sizeof(*gles1_api_ts));

    yagl_gles1_api_ts_init(gles1_api_ts, gles1_api_ps->driver);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_gles1_thread_fini(struct yagl_api_ps *api_ps)
{
    YAGL_LOG_FUNC_ENTER(yagl_host_gles1_thread_fini, NULL);

    yagl_gles1_api_ts_cleanup(gles1_api_ts);

    g_free(gles1_api_ts);

    gles1_api_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static yagl_api_func yagl_host_gles1_get_func(struct yagl_api_ps *api_ps,
                                              uint32_t func_id)
{
    if ((func_id <= 0) || (func_id > yagl_gles1_api_num_funcs)) {
        return NULL;
    } else {
        return yagl_gles1_api_funcs[func_id - 1];
    }
}

static void yagl_host_gles1_process_fini(struct yagl_api_ps *api_ps)
{
    YaglGles1ApiPs *gles1_api_ps = (YaglGles1ApiPs *)api_ps;

    yagl_gles1_api_ps_fini(gles1_api_ps);
}

static void yagl_host_gles1_process_destroy(struct yagl_api_ps *api_ps)
{
    YaglGles1ApiPs *gles1_api_ps = (YaglGles1ApiPs *)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles1_process_destroy, NULL);

    yagl_gles1_api_ps_cleanup(gles1_api_ps);
    yagl_api_ps_cleanup(&gles1_api_ps->base);

    g_free(gles1_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_client_image
    *yagl_host_gles1_create_image(struct yagl_client_interface *iface,
                                  yagl_object_name tex_global_name,
                                  struct yagl_ref *tex_data)
{
    struct yagl_gles_image *image =
        yagl_gles_image_create_from_texture(&gles1_api_ts->driver->base,
                                            tex_global_name,
                                            tex_data);

    return image ? &image->base : NULL;
}

struct yagl_api_ps *yagl_host_gles1_process_init(struct yagl_api *api)
{
    YaglGles1API *gles1_api = (YaglGles1API *)api;
    YaglGles1ApiPs *gles1_api_ps;
    struct yagl_client_interface *client_iface;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles1_process_init, NULL);

    client_iface = g_new0(struct yagl_client_interface, 1);

    yagl_client_interface_init(client_iface);

    client_iface->create_ctx = &yagl_host_gles1_create_ctx;
    client_iface->create_image = &yagl_host_gles1_create_image;

    gles1_api_ps = g_new0(YaglGles1ApiPs, 1);

    yagl_api_ps_init(&gles1_api_ps->base, api);

    gles1_api_ps->base.thread_init = &yagl_host_gles1_thread_init;
    gles1_api_ps->base.get_func = &yagl_host_gles1_get_func;
    gles1_api_ps->base.thread_fini = &yagl_host_gles1_thread_fini;
    gles1_api_ps->base.fini = &yagl_host_gles1_process_fini;
    gles1_api_ps->base.destroy = &yagl_host_gles1_process_destroy;

    yagl_gles1_api_ps_init(gles1_api_ps, gles1_api->driver, client_iface);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles1_api_ps->base;
}

static inline bool yagl_gles1_array_idx_get(YaglGles1Context *ctx,
                                            GLenum array,
                                            unsigned *arr_idx_p)
{
    switch (array) {
    case GL_VERTEX_ARRAY:
    case GL_VERTEX_ARRAY_POINTER:
        *arr_idx_p = YAGL_GLES1_ARRAY_VERTEX;
        break;
    case GL_COLOR_ARRAY:
    case GL_COLOR_ARRAY_POINTER:
        *arr_idx_p = YAGL_GLES1_ARRAY_COLOR;
        break;
    case GL_NORMAL_ARRAY:
    case GL_NORMAL_ARRAY_POINTER:
        *arr_idx_p = YAGL_GLES1_ARRAY_NORMAL;
        break;
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_POINTER:
        *arr_idx_p = YAGL_GLES1_ARRAY_TEX_COORD + ctx->client_active_texture;
        break;
    case GL_POINT_SIZE_ARRAY_OES:
    case GL_POINT_SIZE_ARRAY_POINTER_OES:
        *arr_idx_p = YAGL_GLES1_ARRAY_POINTSIZE;
        break;
    default:
        return false;
    }

    return true;
}

static inline bool yagl_gles1_light_param_len(GLenum pname, unsigned *len)
{
    switch (pname) {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_POSITION:
        *len = 4;
        break;
    case GL_SPOT_DIRECTION:
        *len = 3;
        break;
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
        *len = 1;
        break;
    default:
        return false;
    }

    return true;
}

static inline bool yagl_gles1_material_param_len(GLenum pname, unsigned *len)
{
    switch (pname) {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_AMBIENT_AND_DIFFUSE:
        *len = 4;
        break;
    case GL_SHININESS:
        *len = 1;
        break;
    default:
        return false;
    }

    return true;
}

bool yagl_host_glAlphaFunc(GLenum func,
    GLclampf ref)
{
    YAGL_GET_CTX(glAlphaFunc);

    ctx->driver->AlphaFunc(func, ref);

    return true;
}

bool yagl_host_glClipPlanef(GLenum plane,
    target_ulong /* const GLfloat* */ equation_)
{
    GLfloat equationf[4];
    yagl_GLdouble equationd[4];
    unsigned i;

    YAGL_GET_CTX(glClipPlanef);

    if (plane < GL_CLIP_PLANE0 || plane >= (GL_CLIP_PLANE0 + ctx->max_clip_planes)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (!yagl_mem_get(equation_, 4 * sizeof(GLfloat), equationf)) {
        return false;
    }

    for (i = 0; i < 4; ++i) {
        equationd[i] = (yagl_GLdouble)equationf[i];
    }

    ctx->driver->ClipPlane(plane, equationd);

    return true;
}

bool yagl_host_glColor4f(GLfloat red,
    GLfloat green,
    GLfloat blue,
    GLfloat alpha)
{
    YAGL_GET_CTX(glColor4f);

    ctx->driver->Color4f(red, green, blue, alpha);

    return true;
}

bool yagl_host_glFogf(GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glFogf);

    if (pname != GL_FOG_MODE && pname != GL_FOG_DENSITY &&
        pname != GL_FOG_START && pname != GL_FOG_END) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->Fogf(pname, param);
    }

    return true;
}

bool yagl_host_glFogfv(GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    GLfloat params[YAGL_FOG_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glFogfv);

    if (pname != GL_FOG_MODE && pname != GL_FOG_DENSITY &&
        pname != GL_FOG_START && pname != GL_FOG_END &&
        pname != GL_FOG_COLOR) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_FOG_COLOR) {
        count = YAGL_FOG_PARAM_MAX_LEN;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLfloat), params)) {
        return false;
    }

    ctx->driver->Fogfv(pname,  params);

    return true;
}

bool yagl_host_glFrustumf(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar)
{
    YAGL_GET_CTX(glFrustumf);

    if (zNear <= 0 || zFar <= 0 || left == right ||
        bottom == top || zNear == zFar) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
    } else {
        ctx->driver->Frustum(left, right, bottom, top, zNear, zFar);
    }

    return true;
}

bool yagl_host_glGetClipPlanef(GLenum plane,
    target_ulong /* GLfloat* */ eqn)
{
    yagl_GLdouble equationd[4];
    GLfloat equationf[4];
    unsigned i;

    YAGL_GET_CTX(glGetClipPlanef);

    if (plane < GL_CLIP_PLANE0 || plane >= (GL_CLIP_PLANE0 + ctx->max_clip_planes)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, eqn, 4 * sizeof(GLfloat))) {
        return false;
    }

    ctx->driver->GetClipPlane(plane, equationd);

    for (i = 0; i < 4; ++i) {
        equationf[i] = (GLfloat)equationd[i];
    }

    if (eqn) {
        yagl_mem_put(cur_ts->mt1, equationf);
    }

    return true;
}

bool yagl_host_glGetLightfv(GLenum light,
    GLenum pname,
    target_ulong /* GLfloat* */ params_)
{
    GLfloat params[YAGL_LIGHT_PARAM_MAX_LEN];
    unsigned count;

    YAGL_GET_CTX(glGetLightfv);

    if (light < GL_LIGHT0 || light >= (GL_LIGHT0 + ctx->max_lights) ||
        !yagl_gles1_light_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_LIGHT_PARAM_MAX_LEN);

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfloat))) {
        return false;
    }

    ctx->driver->GetLightfv(light, pname, params);

    if (params_) {
        yagl_mem_put(cur_ts->mt1, params);
    }

    return true;
}

bool yagl_host_glGetMaterialfv(GLenum face,
    GLenum pname,
    target_ulong /* GLfloat* */ params_)
{
    GLfloat params[YAGL_MATERIAL_PARAM_MAX_LEN];
    unsigned count;

    YAGL_GET_CTX(glGetMaterialfv);

    if (!yagl_gles1_material_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_MATERIAL_PARAM_MAX_LEN);

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfloat))) {
        return false;
    }

    ctx->driver->GetMaterialfv(face, pname, params);

    if (params_) {
        yagl_mem_put(cur_ts->mt1, params);
    }

    return true;
}

bool yagl_host_glLightModelf(GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glLightModelf);

    if (pname != GL_LIGHT_MODEL_TWO_SIDE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->LightModelf(pname, param);
    }

    return true;
}

bool yagl_host_glLightModelfv(GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    GLfloat params[YAGL_LIGHT_MODEL_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glLightModelfv);

    if (pname != GL_LIGHT_MODEL_TWO_SIDE && pname != GL_LIGHT_MODEL_AMBIENT) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_LIGHT_MODEL_AMBIENT) {
        count = 4;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLfloat), params)) {
        return false;
    }

    ctx->driver->LightModelfv(pname, params);

    return true;
}

bool yagl_host_glLightf(GLenum light,
    GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glLightf);

    if (light < GL_LIGHT0 || light >= (GL_LIGHT0 + ctx->max_lights)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->Lightf(light, pname, param);
    }

    return true;
}

bool yagl_host_glLightfv(GLenum light,
    GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    GLfloat params[YAGL_LIGHT_PARAM_MAX_LEN];
    unsigned count;

    YAGL_GET_CTX(glLightfv);

    if (light < GL_LIGHT0 || light >= (GL_LIGHT0 + ctx->max_lights) ||
        !yagl_gles1_light_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_LIGHT_PARAM_MAX_LEN);

    if (!yagl_mem_get(params_, count * sizeof(GLfloat), &params[0])) {
        return false;
    }

    ctx->driver->Lightfv(light, pname, params);

    return true;
}

bool yagl_host_glLoadMatrixf(target_ulong /* const GLfloat* */ m)
{
    GLfloat *matrix = NULL;

    YAGL_GET_CTX(glLoadMatrixf);

    if (m) {
        matrix = yagl_gles_context_malloc(&ctx->base, 16 * sizeof(*matrix));
        if (!yagl_mem_get(m, 16 * sizeof(*matrix), matrix)) {
            return false;
        }
    }

    ctx->driver->LoadMatrixf(matrix);

    return true;
}

bool yagl_host_glMaterialf(GLenum face,
    GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glMaterialf);

    if (face != GL_FRONT_AND_BACK) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->Materialf(face, pname, param);
    }

    return true;
}

bool yagl_host_glMaterialfv(GLenum face,
    GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    GLfloat params[YAGL_MATERIAL_PARAM_MAX_LEN];
    unsigned count;

    YAGL_GET_CTX(glMaterialfv);

    if (face != GL_FRONT_AND_BACK ||
        !yagl_gles1_material_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_MATERIAL_PARAM_MAX_LEN);

    if (!yagl_mem_get(params_, count * sizeof(GLfloat), &params[0])) {
        return false;
    }

    ctx->driver->Materialfv(face, pname, params);

    return true;
}

bool yagl_host_glMultMatrixf(target_ulong /* const GLfloat* */ m)
{
    GLfloat *matrix = NULL;

    YAGL_GET_CTX(glMultMatrixf);

    if (m) {
        matrix = yagl_gles_context_malloc(&ctx->base, 16 * sizeof(*matrix));
        if (!yagl_mem_get(m, 16 * sizeof(*matrix), matrix)) {
            return false;
        }
    }

    ctx->driver->MultMatrixf(matrix);

    return true;
}

bool yagl_host_glMultiTexCoord4f(GLenum target,
    GLfloat s,
    GLfloat t,
    GLfloat r,
    GLfloat q)
{
    YAGL_GET_CTX(glMultiTexCoord4f);

    if (target >= GL_TEXTURE0 &&
        target < (GL_TEXTURE0 + ctx->base.num_texture_units)) {
        ctx->driver->MultiTexCoord4f(target, s, t, r, q);
    }

    return true;
}

bool yagl_host_glNormal3f(GLfloat nx,
    GLfloat ny,
    GLfloat nz)
{
    YAGL_GET_CTX(glNormal3f);

    ctx->driver->Normal3f(nx, ny, nz);

    return true;
}

bool yagl_host_glOrthof(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar)
{
    YAGL_GET_CTX(glOrthof);

    if (left == right || bottom == top || zNear == zFar) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
    } else {
        ctx->driver->Ortho(left, right, bottom, top, zNear, zFar);
    }

    return true;
}

bool yagl_host_glPointParameterf(GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glPointParameterf);

    if (pname != GL_POINT_SIZE_MIN && pname != GL_POINT_SIZE_MIN &&
        pname != GL_POINT_SIZE_MAX && pname != GL_POINT_FADE_THRESHOLD_SIZE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->PointParameterf(pname, param);
    }

    return true;
}

bool yagl_host_glPointParameterfv(GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    GLfloat params[YAGL_POINT_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glPointParameterfv);

    if (pname != GL_POINT_SIZE_MIN && pname != GL_POINT_SIZE_MIN &&
        pname != GL_POINT_SIZE_MAX && pname != GL_POINT_FADE_THRESHOLD_SIZE &&
        pname != GL_POINT_DISTANCE_ATTENUATION) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_POINT_DISTANCE_ATTENUATION) {
        count = YAGL_POINT_PARAM_MAX_LEN;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLfloat),  &params[0])) {
        return false;
    }

    ctx->driver->PointParameterfv(pname, params);

    return true;
}

bool yagl_host_glPointSize(GLfloat size)
{
    YAGL_GET_CTX(glPointSize);

    if (size <= 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
    } else {
        ctx->driver->PointSize(size);
    }

    return true;
}

bool yagl_host_glRotatef(GLfloat angle,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glRotatef);

    ctx->driver->Rotatef(angle, x, y, z);

    return true;
}

bool yagl_host_glScalef(GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glScalef);

    ctx->driver->Scalef(x, y, z);

    return true;
}

bool yagl_host_glTexEnvf(GLenum target,
    GLenum pname,
    GLfloat param)
{
    YAGL_GET_CTX(glTexEnvf);

    ctx->driver->TexEnvf(target, pname, param);

    return true;
}

bool yagl_host_glTexEnvfv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfloat* */ params_)
{
    GLfloat params[YAGL_TEX_ENV_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glTexEnvfv);

    if (pname == GL_TEXTURE_ENV_COLOR) {
        count = YAGL_TEX_ENV_PARAM_MAX_LEN;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLfloat), &params[0])) {
        return false;
    }

    ctx->driver->TexEnvfv(target, pname, params);

    return true;
}

bool yagl_host_glTranslatef(GLfloat x,
    GLfloat y,
    GLfloat z)
{
    YAGL_GET_CTX(glTranslatef);

    ctx->driver->Translatef(x, y, z);

    return true;
}

bool yagl_host_glAlphaFuncx(GLenum func,
    GLclampx ref)
{
    YAGL_GET_CTX(glAlphaFuncx);

    ctx->driver->AlphaFunc(func, yagl_fixed_to_float(ref));

    return true;
}

bool yagl_host_glClearColorx(GLclampx red,
    GLclampx green,
    GLclampx blue,
    GLclampx alpha)
{
    YAGL_GET_CTX(glClearColorx);

    ctx->driver->base.ClearColor(yagl_fixed_to_float(red),
                                 yagl_fixed_to_float(green),
                                 yagl_fixed_to_float(blue),
                                 yagl_fixed_to_float(alpha));

    return true;
}

bool yagl_host_glClearDepthx(GLclampx depth)
{
    YAGL_GET_CTX(glClearDepthx);

    ctx->driver->base.ClearDepth(yagl_fixed_to_double(depth));

    return true;
}

bool yagl_host_glClientActiveTexture(GLenum new_texture)
{
    YAGL_GET_CTX(glClientActiveTexture);

    if ((new_texture < GL_TEXTURE0) ||
        (new_texture >= (GL_TEXTURE0 + ctx->base.num_texture_units))) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }
    ctx->client_active_texture = new_texture - GL_TEXTURE0;

    ctx->driver->ClientActiveTexture(new_texture);

    return true;
}

bool yagl_host_glClipPlanex(GLenum plane,
    target_ulong /* const GLfixed* */ equation_)
{
    GLfixed equationx[4];
    yagl_GLdouble equationd[4];
    unsigned i;

    YAGL_GET_CTX(glClipPlanex);

    if (plane < GL_CLIP_PLANE0 ||
        plane >= (GL_CLIP_PLANE0 + ctx->max_clip_planes)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (!yagl_mem_get(equation_, 4 * sizeof(GLfixed), &equationx[0])) {
        return false;
    }

    for (i = 0; i < 4; ++i) {
        equationd[i] = yagl_fixed_to_double(equationx[i]);
    }

    ctx->driver->ClipPlane(plane, equationd);

    return true;
}

bool yagl_host_glColor4ub(GLubyte red,
    GLubyte green,
    GLubyte blue,
    GLubyte alpha)
{
    YAGL_GET_CTX(glColor4ub);

    ctx->driver->Color4ub(red, green, blue, alpha);

    return true;
}

bool yagl_host_glColor4x(GLfixed red,
    GLfixed green,
    GLfixed blue,
    GLfixed alpha)
{
    YAGL_GET_CTX(glColor4x);

    ctx->driver->Color4f(yagl_fixed_to_float(red),
                         yagl_fixed_to_float(green),
                         yagl_fixed_to_float(blue),
                         yagl_fixed_to_float(alpha));

    return true;
}

bool yagl_host_glColorPointer(GLint size,
    GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer)
{
    struct yagl_gles_array *carray;

    YAGL_GET_CTX(glColorPointer);

    if (size != 4 || stride < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    if (type != GL_FLOAT && type != GL_FIXED && type != GL_UNSIGNED_BYTE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    carray = yagl_gles_context_get_array(&ctx->base, YAGL_GLES1_ARRAY_COLOR);

    assert(carray);

    if (ctx->base.vbo) {
        if (!yagl_gles_array_update_vbo(carray,
                                        size,
                                        type,
                                        type == GL_FIXED,
                                        GL_FALSE,
                                        stride,
                                        ctx->base.vbo,
                                        ctx->base.vbo_local_name,
                                        pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    } else {
        if (!yagl_gles_array_update(carray,
                                    size,
                                    type,
                                    type == GL_FIXED,
                                    GL_FALSE,
                                    stride,
                                    pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }

    return true;
}

bool yagl_host_glDepthRangex(GLclampx zNear,
    GLclampx zFar)
{
    YAGL_GET_CTX(glDepthRangex);

    ctx->driver->base.DepthRange(yagl_fixed_to_double(zNear),
                                 yagl_fixed_to_double(zFar));

    return true;
}

bool yagl_host_glDisableClientState(GLenum array_name)
{
    struct yagl_gles_array *array;
    unsigned arr_idx;

    YAGL_GET_CTX(glDisableClientState);

    if (!yagl_gles1_array_idx_get(ctx, array_name, &arr_idx)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    array = yagl_gles_context_get_array(&ctx->base, arr_idx);

    assert(array);

    yagl_gles_array_enable(array, false);

    if (array_name != GL_POINT_SIZE_ARRAY_OES) {
        ctx->driver->DisableClientState(array_name);
    }

    return true;
}

bool yagl_host_glEnableClientState(GLenum array_name)
{
    struct yagl_gles_array *array;
    unsigned arr_idx;

    YAGL_GET_CTX(glEnableClientState);

    if (!yagl_gles1_array_idx_get(ctx, array_name, &arr_idx)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    array = yagl_gles_context_get_array(&ctx->base, arr_idx);

    assert(array);

    yagl_gles_array_enable(array, true);

    if (array_name != GL_POINT_SIZE_ARRAY_OES) {
        ctx->driver->EnableClientState(array_name);
    }

    return true;
}

bool yagl_host_glFogx(GLenum pname,
    GLfixed param)
{
    YAGL_GET_CTX(glFogx);

    if (pname != GL_FOG_MODE && pname != GL_FOG_DENSITY &&
        pname != GL_FOG_START && pname != GL_FOG_END) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        if (pname == GL_FOG_MODE) {
            ctx->driver->Fogf(pname, (GLfloat)param);
        } else {
            ctx->driver->Fogf(pname, yagl_fixed_to_float(param));
        }
    }

    return true;
}

bool yagl_host_glFogxv(GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfixed paramsx[YAGL_FOG_PARAM_MAX_LEN];
    GLfloat paramsf[YAGL_FOG_PARAM_MAX_LEN];
    unsigned count = 1, i;

    YAGL_GET_CTX(glFogxv);

    if (pname != GL_FOG_MODE && pname != GL_FOG_DENSITY &&
        pname != GL_FOG_START && pname != GL_FOG_END &&
        pname != GL_FOG_COLOR) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_FOG_COLOR) {
        count = YAGL_FOG_PARAM_MAX_LEN;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLfixed), &paramsx[0])) {
        return false;
    }

    if (pname == GL_FOG_MODE) {
        paramsf[0] = (GLfloat)paramsx[0];
    } else {
        for (i = 0; i < count; ++i) {
            paramsf[i] = yagl_fixed_to_float(paramsx[i]);
        }
    }

    ctx->driver->Fogfv(pname, paramsf);

    return true;
}

bool yagl_host_glFrustumx(GLfixed left,
    GLfixed right,
    GLfixed bottom,
    GLfixed top,
    GLfixed zNear,
    GLfixed zFar)
{
    YAGL_GET_CTX(glFrustumx);

    if (zNear <= 0 || zFar <= 0 ||left == right ||
        bottom == top || zNear == zFar) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
    } else {
        ctx->driver->Frustum(yagl_fixed_to_double(left),
                             yagl_fixed_to_double(right),
                             yagl_fixed_to_double(bottom),
                             yagl_fixed_to_double(top),
                             yagl_fixed_to_double(zNear),
                             yagl_fixed_to_double(zFar));
    }

    return true;
}

bool yagl_host_glGetClipPlanex(GLenum plane,
    target_ulong /* GLfixed* */ eqn)
{
    yagl_GLdouble equationd[4];
    GLfixed equationx[4];
    unsigned i;

    YAGL_GET_CTX(glGetClipPlanex);

    if (plane < GL_CLIP_PLANE0 ||
        plane >= (GL_CLIP_PLANE0 + ctx->max_clip_planes)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, eqn, 4 * sizeof(GLfixed))) {
        return false;
    }

    ctx->driver->GetClipPlane(plane, equationd);

    for (i = 0; i < 4; ++i) {
        equationx[i] = yagl_double_to_fixed((GLfloat)equationd[i]);
    }

    if (eqn) {
        yagl_mem_put(cur_ts->mt1, equationx);
    }

    return true;
}

bool yagl_host_glGetFixedv(GLenum pname,
    target_ulong /* GLfixed* */ params_)
{
    int count = 0, i;
    void *params = NULL;

    YAGL_GET_CTX(glGetFixedv);

    if (!ctx->base.get_param_count(&ctx->base, pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        goto out;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfixed))) {
        return false;
    }

    params = yagl_gles_context_malloc0(&ctx->base, count * sizeof(GLfloat));

    if (!ctx->base.get_floatv(&ctx->base, pname, params)) {
        if (!yagl_gles1_get_float(&ctx->base, pname, (GLfloat *)params)) {
            ctx->driver->base.GetFloatv(pname, params);
        }
    }

    assert(sizeof(GLfixed) == sizeof(GLfloat));

    for (i = 0; i < count; ++i) {
        ((GLfixed *)params)[i] = yagl_float_to_fixed(((GLfloat *)params)[i]);
    }

    if (params_) {
        yagl_mem_put(cur_ts->mt1, params);
    }

out:
    return true;
}

bool yagl_host_glGetLightxv(GLenum light,
    GLenum pname,
    target_ulong /* GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_LIGHT_PARAM_MAX_LEN];
    GLfixed paramsx[YAGL_LIGHT_PARAM_MAX_LEN];
    unsigned count, i;

    YAGL_GET_CTX(glGetLightxv);

    if (light < GL_LIGHT0 || light >= (GL_LIGHT0 + ctx->max_lights) ||
        !yagl_gles1_light_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_LIGHT_PARAM_MAX_LEN);

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfixed))) {
        return false;
    }

    ctx->driver->GetLightfv(light, pname, paramsf);

    for (i = 0; i < count; ++i) {
        paramsx[i] = yagl_float_to_fixed(paramsf[i]);
    }

    if (params_) {
        yagl_mem_put(cur_ts->mt1, paramsx);
    }

    return true;
}

bool yagl_host_glGetMaterialxv(GLenum face,
    GLenum pname,
    target_ulong /* GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_MATERIAL_PARAM_MAX_LEN];
    GLfixed paramsx[YAGL_MATERIAL_PARAM_MAX_LEN];
    unsigned count, i;

    YAGL_GET_CTX(glGetMaterialxv);

    if (!yagl_gles1_material_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_MATERIAL_PARAM_MAX_LEN);

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfixed))) {
        return false;
    }

    ctx->driver->GetMaterialfv(face, pname, paramsf);

    for (i = 0; i < count; ++i) {
        paramsx[i] = yagl_float_to_fixed(paramsf[i]);
    }

    if (params_) {
        yagl_mem_put(cur_ts->mt1, paramsx);
    }

    return true;
}

bool yagl_host_glGetPointerv(GLenum pname,
    target_ulong /* GLvoid** */ params)
{
    struct yagl_gles_array *array;
    unsigned arr_idx;
    target_ulong pointer = 0;

    YAGL_GET_CTX(glGetPointerv);

    if (!yagl_gles1_array_idx_get(ctx, pname, &arr_idx)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (!yagl_mem_prepare_ptr(cur_ts->mt1, params)) {
        return false;
    }

    array = yagl_gles_context_get_array(&ctx->base, arr_idx);

    assert(array);

    if (array->vbo) {
        pointer = array->offset;
    } else {
        pointer = array->target_data;
    }

    if (params) {
        yagl_mem_put_ptr(cur_ts->mt1, pointer);
    }

    return true;
}

bool yagl_host_glGetTexEnviv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params_)
{
    GLint params[YAGL_TEX_ENV_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glGetTexEnviv);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        count = YAGL_TEX_ENV_PARAM_MAX_LEN;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLint))) {
        return false;
    }

    ctx->driver->GetTexEnviv(target, pname, params);

    yagl_mem_put(cur_ts->mt1, params);

    return true;
}

bool yagl_host_glGetTexEnvfv(GLenum target,
    GLenum pname,
    target_ulong /* GLfloat* */ params_)
{
    GLfloat params[YAGL_TEX_ENV_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glGetTexEnvfv);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        count = YAGL_TEX_ENV_PARAM_MAX_LEN;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfloat))) {
        return false;
    }

    ctx->driver->GetTexEnvfv(target, pname, params);

    yagl_mem_put(cur_ts->mt1, params);

    return true;
}

bool yagl_host_glGetTexEnvxv(GLenum target,
    GLenum pname,
    target_ulong /* GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_TEX_ENV_PARAM_MAX_LEN];
    GLfixed paramsx[YAGL_TEX_ENV_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glGetTexEnvxv);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        count = YAGL_TEX_ENV_PARAM_MAX_LEN;
    }

    if (!yagl_mem_prepare(cur_ts->mt1, params_, count * sizeof(GLfixed))) {
        return false;
    }

    ctx->driver->GetTexEnvfv(target, pname, paramsf);

    if (pname == GL_TEXTURE_ENV_COLOR || pname == GL_RGB_SCALE ||
        pname == GL_ALPHA_SCALE) {
        unsigned i;

        for (i = 0; i < count; ++i) {
            paramsx[i] = yagl_float_to_fixed(paramsf[i]);
        }
    } else {
        paramsx[0] = (GLfixed)paramsf[0];
    }

    yagl_mem_put(cur_ts->mt1, paramsx);

    return true;
}

bool yagl_host_glGetTexParameterxv(GLenum target,
    GLenum pname,
    target_ulong /* GLfixed* */ params_)
{
    GLfloat paramf;

    YAGL_GET_CTX(glGetTexParameterxv);

    if (target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (!yagl_mem_prepare_GLfixed(cur_ts->mt1, params_)) {
        return false;
    }

    ctx->driver->base.GetTexParameterfv(target, pname, &paramf);

    if (params_) {
        yagl_mem_put_GLfixed(cur_ts->mt1, (GLfixed)paramf);
    }

    return true;
}

bool yagl_host_glLightModelx(GLenum pname,
    GLfixed param)
{
    YAGL_GET_CTX(glLightModelx);

    if (pname != GL_LIGHT_MODEL_TWO_SIDE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->LightModelf(pname, (GLfloat)param);
    }

    return true;
}

bool yagl_host_glLightModelxv(GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_LIGHT_MODEL_PARAM_MAX_LEN];

    YAGL_GET_CTX(glLightModelxv);

    if (pname == GL_LIGHT_MODEL_TWO_SIDE) {
        GLfixed paramx;

        if (!yagl_mem_get_GLfixed(params_, &paramx)) {
            return false;
        }

        paramsf[0] = (GLfloat)paramx;
    } else if (pname == GL_LIGHT_MODEL_AMBIENT) {
        GLfixed paramsx[YAGL_LIGHT_MODEL_PARAM_MAX_LEN];
        unsigned i;

        if (!yagl_mem_get(params_,
                          YAGL_LIGHT_MODEL_PARAM_MAX_LEN * sizeof(GLfixed),
                          &paramsx[0])) {
            return false;
        }

        for (i = 0; i < YAGL_LIGHT_MODEL_PARAM_MAX_LEN; ++i) {
            paramsf[i] = yagl_fixed_to_float(paramsx[i]);
        }
    } else {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    ctx->driver->LightModelfv(pname, paramsf);

    return true;
}

bool yagl_host_glLightx(GLenum light,
    GLenum pname,
    GLfixed param)
{
    YAGL_GET_CTX(glLightx);

    if (light < GL_LIGHT0 || light >= (GL_LIGHT0 + ctx->max_lights)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->Lightf(light, pname, yagl_fixed_to_float(param));
    }

    return true;
}

bool yagl_host_glLightxv(GLenum light,
    GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_LIGHT_PARAM_MAX_LEN];
    GLfixed paramsx[YAGL_LIGHT_PARAM_MAX_LEN];
    unsigned count, i;

    YAGL_GET_CTX(glLightxv);

    if (light < GL_LIGHT0 || light >= (GL_LIGHT0 + ctx->max_lights) ||
        !yagl_gles1_light_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_LIGHT_PARAM_MAX_LEN);

    if (!yagl_mem_get(params_, count * sizeof(GLfixed), &paramsx[0])) {
        return false;
    }

    for (i = 0; i < count; ++i) {
        paramsf[i] = yagl_fixed_to_float(paramsx[i]);
    }

    ctx->driver->Lightfv(light, pname, paramsf);

    return true;
}

bool yagl_host_glLineWidthx(GLfixed width)
{
    GLfloat widthf;

    YAGL_GET_CTX(glLineWidthx);

    widthf = yagl_fixed_to_float(width);

    if (widthf <= 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    ctx->driver->base.LineWidth(widthf);

    return true;
}

bool yagl_host_glLoadIdentity(void)
{
    YAGL_GET_CTX(glLoadIdentity);

    ctx->driver->LoadIdentity();

    return true;
}

bool yagl_host_glLoadMatrixx(target_ulong /* const GLfixed* */ m)
{
    unsigned i;
    void *matrix = NULL;

    YAGL_GET_CTX(glLoadMatrixx);

    if (m) {
        matrix = yagl_gles_context_malloc(&ctx->base, 16 * sizeof(GLfixed));
        if (!yagl_mem_get(m, 16 * sizeof(GLfixed), matrix)) {
            return false;
        }
    }

    assert(sizeof(GLfloat) == sizeof(GLfixed));

    for (i = 0; i < 16; ++i) {
        ((GLfloat *)matrix)[i] = yagl_fixed_to_float(((GLfixed *)matrix)[i]);
    }

    ctx->driver->LoadMatrixf(matrix);

    return true;
}

bool yagl_host_glLogicOp(GLenum opcode)
{
    YAGL_GET_CTX(glLogicOp);

    ctx->driver->LogicOp(opcode);

    return true;
}

bool yagl_host_glMaterialx(GLenum face,
    GLenum pname,
    GLfixed param)
{
    YAGL_GET_CTX(glMaterialx);

    if (face != GL_FRONT_AND_BACK) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->Materialf(face, pname, yagl_fixed_to_float(param));
    }

    return true;
}

bool yagl_host_glMaterialxv(GLenum face,
    GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_MATERIAL_PARAM_MAX_LEN];
    GLfixed paramsx[YAGL_MATERIAL_PARAM_MAX_LEN];
    unsigned count, i;

    YAGL_GET_CTX(glMaterialxv);

    if (face != GL_FRONT_AND_BACK ||
        !yagl_gles1_material_param_len(pname, &count)) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    assert(count <= YAGL_MATERIAL_PARAM_MAX_LEN);

    if (!yagl_mem_get(params_, count * sizeof(GLfixed), &paramsx[0])) {
        return false;
    }

    for (i = 0; i < count; ++i) {
        paramsf[i] = yagl_fixed_to_float(paramsx[i]);
    }

    ctx->driver->Materialfv(face, pname, paramsf);

    return true;
}

bool yagl_host_glMatrixMode(GLenum mode)
{
    YAGL_GET_CTX(glMatrixMode);

    ctx->driver->MatrixMode(mode);

    return true;
}

bool yagl_host_glMultMatrixx(target_ulong /* const GLfixed* */ m)
{
    unsigned i;
    void *matrix = NULL;

    YAGL_GET_CTX(glMultMatrixf);

    if (m) {
        matrix = yagl_gles_context_malloc(&ctx->base, 16 * sizeof(GLfixed));
        if (!yagl_mem_get(m, 16 * sizeof(GLfixed), matrix)) {
            return false;
        }
    }

    assert(sizeof(GLfloat) == sizeof(GLfixed));

    for (i = 0; i < 16; ++i) {
        ((GLfloat *)matrix)[i] = yagl_fixed_to_float(((GLfixed *)matrix)[i]);
    }

    ctx->driver->MultMatrixf(matrix);

    return true;
}

bool yagl_host_glMultiTexCoord4x(GLenum target,
    GLfixed s,
    GLfixed t,
    GLfixed r,
    GLfixed q)
{
    YAGL_GET_CTX(glMultiTexCoord4x);

    if (target >= GL_TEXTURE0 &&
        target < (GL_TEXTURE0 + ctx->base.num_texture_units)) {
        ctx->driver->MultiTexCoord4f(target,
                                     yagl_fixed_to_float(s),
                                     yagl_fixed_to_float(t),
                                     yagl_fixed_to_float(r),
                                     yagl_fixed_to_float(q));
    }

    return true;
}

bool yagl_host_glNormal3x(GLfixed nx,
    GLfixed ny,
    GLfixed nz)
{
    YAGL_GET_CTX(glNormal3x);

    ctx->driver->Normal3f(yagl_fixed_to_float(nx),
                          yagl_fixed_to_float(ny),
                          yagl_fixed_to_float(nz));

    return true;
}

bool yagl_host_glNormalPointer(GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer)
{
    struct yagl_gles_array *narray;

    YAGL_GET_CTX(glNormalPointer);

    if (stride < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    if (type != GL_FLOAT && type != GL_FIXED &&
        type != GL_SHORT && type != GL_BYTE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    narray = yagl_gles_context_get_array(&ctx->base, YAGL_GLES1_ARRAY_NORMAL);

    assert(narray);

    if (ctx->base.vbo) {
        if (!yagl_gles_array_update_vbo(narray,
                                        3,
                                        type,
                                        type == GL_FIXED,
                                        GL_FALSE,
                                        stride,
                                        ctx->base.vbo,
                                        ctx->base.vbo_local_name,
                                        pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    } else {
        if (!yagl_gles_array_update(narray,
                                    3,
                                    type,
                                    type == GL_FIXED,
                                    GL_FALSE,
                                    stride,
                                    pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }

    return true;
}

bool yagl_host_glOrthox(GLfixed left,
    GLfixed right,
    GLfixed bottom,
    GLfixed top,
    GLfixed zNear,
    GLfixed zFar)
{
    YAGL_GET_CTX(glOrthox);

    if (left == right || bottom == top || zNear == zFar) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
    } else {
        ctx->driver->Ortho(yagl_fixed_to_double(left),
                           yagl_fixed_to_double(right),
                           yagl_fixed_to_double(bottom),
                           yagl_fixed_to_double(top),
                           yagl_fixed_to_double(zNear),
                           yagl_fixed_to_double(zFar));
    }

    return true;
}

bool yagl_host_glPointParameterx(GLenum pname,
    GLfixed param)
{
    YAGL_GET_CTX(glPointParameterx);

    if (pname != GL_POINT_SIZE_MIN && pname != GL_POINT_SIZE_MIN &&
        pname != GL_POINT_SIZE_MAX && pname != GL_POINT_FADE_THRESHOLD_SIZE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
    } else {
        ctx->driver->PointParameterf(pname, yagl_fixed_to_float(param));
    }

    return true;
}

bool yagl_host_glPointParameterxv(GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfixed paramsx[YAGL_POINT_PARAM_MAX_LEN];
    GLfloat paramsf[YAGL_POINT_PARAM_MAX_LEN];
    unsigned count = 1, i;

    YAGL_GET_CTX(glPointParameterxv);

    if (pname != GL_POINT_SIZE_MIN && pname != GL_POINT_SIZE_MIN &&
        pname != GL_POINT_SIZE_MAX && pname != GL_POINT_FADE_THRESHOLD_SIZE &&
        pname != GL_POINT_DISTANCE_ATTENUATION) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_POINT_DISTANCE_ATTENUATION) {
        count = YAGL_POINT_PARAM_MAX_LEN;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLfixed), &paramsx[0])) {
        return false;
    }

    for (i = 0; i < count; ++i) {
        paramsf[i] = yagl_fixed_to_float(paramsx[i]);
    }

    ctx->driver->PointParameterfv(pname, paramsf);

    return true;
}

bool yagl_host_glPointSizex(GLfixed size)
{
    GLfloat sizef;

    YAGL_GET_CTX(glPointSizex);

    sizef = yagl_fixed_to_float(size);

    if (sizef <= 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
    } else {
        ctx->driver->PointSize(sizef);
    }

    return true;
}

bool yagl_host_glPointSizePointerOES(GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer)
{
    struct yagl_gles_array *parray;

    YAGL_GET_CTX(glPointSizePointerOES);

    if (stride < 0) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    if (type != GL_FLOAT && type != GL_FIXED) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    parray = yagl_gles_context_get_array(&ctx->base, YAGL_GLES1_ARRAY_POINTSIZE);

    assert(parray);

    if (ctx->base.vbo) {
        if (!yagl_gles_array_update_vbo(parray,
                                        1,
                                        type,
                                        type == GL_FIXED,
                                        GL_FALSE,
                                        stride,
                                        ctx->base.vbo,
                                        ctx->base.vbo_local_name,
                                        pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    } else {
        if (!yagl_gles_array_update(parray,
                                    1,
                                    type,
                                    type == GL_FIXED,
                                    GL_FALSE,
                                    stride,
                                    pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }

    return true;
}

bool yagl_host_glPolygonOffsetx(GLfixed factor,
    GLfixed units)
{
    YAGL_GET_CTX(glPolygonOffsetx);

    ctx->driver->base.PolygonOffset(yagl_fixed_to_float(factor),
                                    yagl_fixed_to_float(units));

    return true;
}

bool yagl_host_glPopMatrix(void)
{
    YAGL_GET_CTX(glPopMatrix);

    ctx->driver->PopMatrix();

    return true;
}

bool yagl_host_glPushMatrix(void)
{
    YAGL_GET_CTX(glPushMatrix);

    ctx->driver->PushMatrix();

    return true;
}

bool yagl_host_glRotatex(GLfixed angle,
    GLfixed x,
    GLfixed y,
    GLfixed z)
{
    YAGL_GET_CTX(glRotatex);

    ctx->driver->Rotatef(yagl_fixed_to_float(angle),
                         yagl_fixed_to_float(x),
                         yagl_fixed_to_float(y),
                         yagl_fixed_to_float(z));

    return true;
}

bool yagl_host_glSampleCoveragex(GLclampx value,
    GLboolean invert)
{
    YAGL_GET_CTX(glSampleCoveragex);

    ctx->driver->base.SampleCoverage(yagl_fixed_to_float(value), invert);

    return true;
}

bool yagl_host_glScalex(GLfixed x,
    GLfixed y,
    GLfixed z)
{
    YAGL_GET_CTX(glScalex);

    ctx->driver->Scalef(yagl_fixed_to_float(x),
                        yagl_fixed_to_float(y),
                        yagl_fixed_to_float(z));

    return true;
}

bool yagl_host_glShadeModel(GLenum mode)
{
    YAGL_GET_CTX(glShadeModel);

    ctx->driver->ShadeModel(mode);

    return true;
}

bool yagl_host_glTexCoordPointer(GLint size,
    GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer)
{
    struct yagl_gles_array *texarray;

    YAGL_GET_CTX(glTexCoordPointer);

    if ((size < 2) || (size > 4) || (stride < 0)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    if (type != GL_FLOAT && type != GL_FIXED &&
        type != GL_SHORT && type != GL_BYTE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    texarray = yagl_gles_context_get_array(&ctx->base,
                                           YAGL_GLES1_ARRAY_TEX_COORD +
                                           ctx->client_active_texture);

    assert(texarray);

    if (ctx->base.vbo) {
        if (!yagl_gles_array_update_vbo(texarray,
                                        size,
                                        type,
                                        type == GL_FIXED || type == GL_BYTE,
                                        GL_FALSE,
                                        stride,
                                        ctx->base.vbo,
                                        ctx->base.vbo_local_name,
                                        pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    } else {
        if (!yagl_gles_array_update(texarray,
                                    size,
                                    type,
                                    type == GL_FIXED || type == GL_BYTE,
                                    GL_FALSE,
                                    stride,
                                    pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }

    return true;
}

bool yagl_host_glTexEnvi(GLenum target,
    GLenum pname,
    GLint param)
{
    YAGL_GET_CTX(glTexEnvi);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    ctx->driver->TexEnvi(target, pname, param);

    return true;
}

bool yagl_host_glTexEnvx(GLenum target,
    GLenum pname,
    GLfixed param)
{
    GLfloat paramf;

    YAGL_GET_CTX(glTexEnvx);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_RGB_SCALE || pname == GL_ALPHA_SCALE) {
        paramf = yagl_fixed_to_float(param);
    } else {
        paramf = (GLfloat)param;
    }

    ctx->driver->TexEnvf(target, pname, paramf);

    return true;
}

bool yagl_host_glTexEnviv(GLenum target,
    GLenum pname,
    target_ulong /* const GLint* */ params_)
{
    GLint params[YAGL_TEX_ENV_PARAM_MAX_LEN];
    unsigned count = 1;

    YAGL_GET_CTX(glTexEnviv);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        count = YAGL_TEX_ENV_PARAM_MAX_LEN;
    }

    if (!yagl_mem_get(params_, count * sizeof(GLint), &params[0])) {
        return false;
    }

    ctx->driver->TexEnviv(target, pname, params);

    return true;
}

bool yagl_host_glTexEnvxv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfloat paramsf[YAGL_TEX_ENV_PARAM_MAX_LEN];

    YAGL_GET_CTX(glTexEnvxv);

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        GLfixed paramsx[YAGL_TEX_ENV_PARAM_MAX_LEN];
        unsigned i;

        if (!yagl_mem_get(params_,
                          YAGL_TEX_ENV_PARAM_MAX_LEN * sizeof(GLfixed),
                          &paramsx[0])) {
            return false;
        }

        for (i = 0; i < YAGL_TEX_ENV_PARAM_MAX_LEN; ++i) {
            paramsf[i] = yagl_fixed_to_float(paramsx[i]);
        }
    } else {
        GLfixed paramx;

        if (!yagl_mem_get_GLfixed(params_, &paramx)) {
            return false;
        }

        if (pname == GL_RGB_SCALE || pname == GL_ALPHA_SCALE) {
            paramsf[0] = yagl_fixed_to_float(paramx);
        } else {
            paramsf[0] = (GLfloat)paramx;
        }
    }

    ctx->driver->TexEnvfv(target, pname, paramsf);

    return true;
}

bool yagl_host_glTexParameterx(GLenum target,
    GLenum pname,
    GLfixed param)
{
    YAGL_GET_CTX(glTexParameterx);

    if (target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    ctx->driver->base.TexParameterf(target, pname, (GLfloat)param);

    return true;
}

bool yagl_host_glTexParameterxv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfixed* */ params_)
{
    GLfloat paramf;

    YAGL_GET_CTX(glTexParameterxv);

    if (target != GL_TEXTURE_2D) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    if (params_) {
        GLfixed paramx;

        if (!yagl_mem_get_GLfixed(params_, &paramx)) {
            return false;
        }

        paramf = (GLfloat)paramx;
    }

    ctx->driver->base.TexParameterfv(target, pname, params_ ? &paramf : NULL);

    return true;
}

bool yagl_host_glTranslatex(GLfixed x,
    GLfixed y,
    GLfixed z)
{
    YAGL_GET_CTX(glTranslatex);

    ctx->driver->Translatef(yagl_fixed_to_float(x),
                            yagl_fixed_to_float(y),
                            yagl_fixed_to_float(z));

    return true;
}

bool yagl_host_glVertexPointer(GLint size,
    GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer)
{
    struct yagl_gles_array *varray;

    YAGL_GET_CTX(glVertexPointer);

    if ((size < 2) || (size > 4) || (stride < 0)) {
        YAGL_SET_ERR(GL_INVALID_VALUE);
        return true;
    }

    if (type != GL_FLOAT && type != GL_FIXED &&
        type != GL_SHORT && type != GL_BYTE) {
        YAGL_SET_ERR(GL_INVALID_ENUM);
        return true;
    }

    varray = yagl_gles_context_get_array(&ctx->base, YAGL_GLES1_ARRAY_VERTEX);

    assert(varray);

    if (ctx->base.vbo) {
        if (!yagl_gles_array_update_vbo(varray,
                                        size,
                                        type,
                                        type == GL_FIXED || type == GL_BYTE,
                                        GL_FALSE,
                                        stride,
                                        ctx->base.vbo,
                                        ctx->base.vbo_local_name,
                                        pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    } else {
        if (!yagl_gles_array_update(varray,
                                    size,
                                    type,
                                    type == GL_FIXED || type == GL_BYTE,
                                    GL_FALSE,
                                    stride,
                                    pointer)) {
            YAGL_SET_ERR(GL_INVALID_VALUE);
        }
    }

    return true;
}
