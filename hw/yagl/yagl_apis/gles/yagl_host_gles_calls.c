/*
 * yagl
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "yagl_host_gles_calls.h"
#include "yagl_gles_calls.h"
#include "yagl_gles_api.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_api_ps.h"
#include "yagl_gles_api_ts.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_vector.h"
#include "yagl_object_map.h"
#include "yagl_transport.h"

static YAGL_DEFINE_TLS(struct yagl_gles_api_ts*, gles_api_ts);

struct yagl_gles_object
{
    struct yagl_object base;

    struct yagl_gles_driver *driver;

    uint32_t ctx_id;
};

typedef enum
{
    yagl_gles1_array_vertex = 0,
    yagl_gles1_array_color,
    yagl_gles1_array_normal,
    yagl_gles1_array_pointsize,
    yagl_gles1_array_texcoord,
} yagl_gles1_array_type;

static GLuint yagl_gles_bind_array(uint32_t indx,
                                   GLint first,
                                   GLsizei stride,
                                   const GLvoid *data,
                                   int32_t data_count)
{
    GLuint current_vbo;
    uint32_t size;

    if (indx >= gles_api_ts->num_arrays) {
        struct yagl_gles_array *arrays;
        uint32_t i;

        arrays = g_malloc((indx + 1) * sizeof(arrays[0]));

        memcpy(arrays,
               gles_api_ts->arrays,
               gles_api_ts->num_arrays * sizeof(arrays[0]));

        for (i = gles_api_ts->num_arrays; i <= indx; ++i) {
            gles_api_ts->driver->GenBuffers(1, &arrays[i].vbo);
            arrays[i].size = 0;
        }

        g_free(gles_api_ts->arrays);

        gles_api_ts->arrays = arrays;
        gles_api_ts->num_arrays = indx + 1;
    }

    gles_api_ts->driver->GetIntegerv(GL_ARRAY_BUFFER_BINDING,
                                     (GLint*)&current_vbo);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, gles_api_ts->arrays[indx].vbo);

    size = first * stride + data_count;

    if (size > gles_api_ts->arrays[indx].size) {
        gles_api_ts->driver->BufferData(GL_ARRAY_BUFFER,
                                        size, NULL,
                                        GL_STREAM_DRAW);
        gles_api_ts->arrays[indx].size = size;
    }


    gles_api_ts->driver->BufferSubData(GL_ARRAY_BUFFER,
                                       first * stride, data_count, data);

    return current_vbo;
}

static bool yagl_gles_program_get_uniform_type(GLuint program,
                                               GLint location,
                                               GLenum *type)
{
    GLint link_status = GL_FALSE;
    GLint i = 0, num_active_uniforms = 0;
    GLint uniform_name_max_length = 0;
    GLchar *uniform_name = NULL;
    bool res = false;

    YAGL_LOG_FUNC_SET(yagl_gles_program_get_uniform_type);

    if (location < 0) {
        return false;
    }

    gles_api_ts->driver->GetProgramiv(program,
                                      GL_LINK_STATUS,
                                      &link_status);

    if (link_status == GL_FALSE) {
        return false;
    }

    gles_api_ts->driver->GetProgramiv(program,
                                      GL_ACTIVE_UNIFORMS,
                                      &num_active_uniforms);

    gles_api_ts->driver->GetProgramiv(program,
                                      GL_ACTIVE_UNIFORM_MAX_LENGTH,
                                      &uniform_name_max_length);

    uniform_name = g_malloc(uniform_name_max_length + 1);

    for (i = 0; i < num_active_uniforms; ++i) {
        GLsizei length = 0;
        GLint size = 0;
        GLenum tmp_type = 0;

        gles_api_ts->driver->GetActiveUniform(program,
                                              i,
                                              uniform_name_max_length,
                                              &length,
                                              &size,
                                              &tmp_type,
                                              uniform_name);

        if (length == 0) {
            YAGL_LOG_ERROR("Cannot get active uniform %d for program %d", i, program);
            continue;
        }

        if (gles_api_ts->driver->GetUniformLocation(program,
                                                    uniform_name) == location) {
            *type = tmp_type;
            res = true;
            break;
        }
    }

    g_free(uniform_name);

    return res;
}

static bool yagl_gles_get_uniform_type_count(GLenum uniform_type, int *count)
{
    switch (uniform_type) {
    case GL_FLOAT: *count = 1; break;
    case GL_FLOAT_VEC2: *count = 2; break;
    case GL_FLOAT_VEC3: *count = 3; break;
    case GL_FLOAT_VEC4: *count = 4; break;
    case GL_FLOAT_MAT2: *count = 2*2; break;
    case GL_FLOAT_MAT3: *count = 3*3; break;
    case GL_FLOAT_MAT4: *count = 4*4; break;
    case GL_INT: *count = 1; break;
    case GL_INT_VEC2: *count = 2; break;
    case GL_INT_VEC3: *count = 3; break;
    case GL_INT_VEC4: *count = 4; break;
    case GL_BOOL: *count = 1; break;
    case GL_BOOL_VEC2: *count = 2; break;
    case GL_BOOL_VEC3: *count = 3; break;
    case GL_BOOL_VEC4: *count = 4; break;
    case GL_SAMPLER_2D: *count = 1; break;
    case GL_SAMPLER_CUBE: *count = 1; break;
    case GL_UNSIGNED_INT: *count = 1; break;
    case GL_UNSIGNED_INT_VEC2: *count = 2; break;
    case GL_UNSIGNED_INT_VEC3: *count = 3; break;
    case GL_UNSIGNED_INT_VEC4: *count = 4; break;
    case GL_FLOAT_MAT2x3: *count = 2*3; break;
    case GL_FLOAT_MAT2x4: *count = 2*4; break;
    case GL_FLOAT_MAT3x2: *count = 3*2; break;
    case GL_FLOAT_MAT3x4: *count = 3*4; break;
    case GL_FLOAT_MAT4x2: *count = 4*2; break;
    case GL_FLOAT_MAT4x3: *count = 4*3; break;
    case GL_SAMPLER_3D: *count = 1; break;
    case GL_SAMPLER_2D_SHADOW: *count = 1; break;
    case GL_SAMPLER_2D_ARRAY: *count = 1; break;
    case GL_SAMPLER_2D_ARRAY_SHADOW: *count = 1; break;
    case GL_SAMPLER_CUBE_SHADOW: *count = 1; break;
    case GL_INT_SAMPLER_2D: *count = 1; break;
    case GL_INT_SAMPLER_3D: *count = 1; break;
    case GL_INT_SAMPLER_CUBE: *count = 1; break;
    case GL_INT_SAMPLER_2D_ARRAY: *count = 1; break;
    case GL_UNSIGNED_INT_SAMPLER_2D: *count = 1; break;
    case GL_UNSIGNED_INT_SAMPLER_3D: *count = 1; break;
    case GL_UNSIGNED_INT_SAMPLER_CUBE: *count = 1; break;
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: *count = 1; break;
    default: return false;
    }
    return true;
}

static bool yagl_gles_get_array_param_count(GLenum pname, int *count)
{
    switch (pname) {
    case GL_CURRENT_VERTEX_ATTRIB: *count = 4; break;
    default: return false;
    }
    return true;
}

static void yagl_gles_object_add(GLuint local_name,
                                 GLuint global_name,
                                 uint32_t ctx_id,
                                 void (*destroy_func)(struct yagl_object */*obj*/))
{
    struct yagl_gles_object *obj;

    obj = g_malloc(sizeof(*obj));

    obj->base.global_name = global_name;
    obj->base.destroy = destroy_func;
    obj->driver = gles_api_ts->driver;
    obj->ctx_id = ctx_id;

    yagl_object_map_add(cur_ts->ps->object_map,
                        local_name,
                        &obj->base);
}

static void yagl_gles_buffer_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_buffer_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    gles_obj->driver->DeleteBuffers(1, &obj->global_name);
    yagl_unensure_ctx(0);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_texture_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_texture_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    gles_obj->driver->DeleteTextures(1, &obj->global_name);
    yagl_unensure_ctx(0);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_framebuffer_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_framebuffer_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(gles_obj->ctx_id);
    gles_obj->driver->DeleteFramebuffers(1, &obj->global_name);
    yagl_unensure_ctx(gles_obj->ctx_id);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_renderbuffer_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_renderbuffer_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    gles_obj->driver->DeleteRenderbuffers(1, &obj->global_name);
    yagl_unensure_ctx(0);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_program_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_program_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    gles_obj->driver->DeleteProgram(obj->global_name);
    yagl_unensure_ctx(0);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_shader_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_shader_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    gles_obj->driver->DeleteShader(obj->global_name);
    yagl_unensure_ctx(0);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_vertex_array_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_vertex_array_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(gles_obj->ctx_id);
    gles_obj->driver->DeleteVertexArrays(1, &obj->global_name);
    yagl_unensure_ctx(gles_obj->ctx_id);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_transform_feedback_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_transform_feedback_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(gles_obj->ctx_id);
    gles_obj->driver->DeleteTransformFeedbacks(1, &obj->global_name);
    yagl_unensure_ctx(gles_obj->ctx_id);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_query_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_query_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(gles_obj->ctx_id);
    gles_obj->driver->DeleteQueries(1, &obj->global_name);
    yagl_unensure_ctx(gles_obj->ctx_id);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles_sampler_destroy(struct yagl_object *obj)
{
    struct yagl_gles_object *gles_obj = (struct yagl_gles_object*)obj;

    YAGL_LOG_FUNC_ENTER(yagl_gles_sampler_destroy, "%u", obj->global_name);

    yagl_ensure_ctx(0);
    gles_obj->driver->DeleteSamplers(1, &obj->global_name);
    yagl_unensure_ctx(0);

    g_free(gles_obj);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static __inline GLuint yagl_gles_object_get(GLuint local_name)
{
    return (local_name > 0) ? yagl_object_map_get(cur_ts->ps->object_map, local_name) : 0;
}

static yagl_api_func yagl_host_gles_get_func(struct yagl_api_ps *api_ps,
                                             uint32_t func_id)
{
    if ((func_id <= 0) || (func_id > yagl_gles_api_num_funcs)) {
        return NULL;
    } else {
        return yagl_gles_api_funcs[func_id - 1];
    }
}

static void yagl_host_gles_thread_init(struct yagl_api_ps *api_ps)
{
    struct yagl_gles_api_ps *gles_api_ps = (struct yagl_gles_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles_thread_init, NULL);

    gles_api_ts = g_malloc0(sizeof(*gles_api_ts));

    yagl_gles_api_ts_init(gles_api_ts, gles_api_ps->driver, gles_api_ps);

    cur_ts->gles_api_ts = gles_api_ts;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_gles_batch_start(struct yagl_api_ps *api_ps)
{
    gles_api_ts = cur_ts->gles_api_ts;
}

static void yagl_host_gles_batch_end(struct yagl_api_ps *api_ps)
{
}

static void yagl_host_gles_thread_fini(struct yagl_api_ps *api_ps)
{
    YAGL_LOG_FUNC_ENTER(yagl_host_gles_thread_fini, NULL);

    gles_api_ts = cur_ts->gles_api_ts;

    yagl_gles_api_ts_cleanup(gles_api_ts);

    g_free(gles_api_ts);

    gles_api_ts = cur_ts->gles_api_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_gles_process_destroy(struct yagl_api_ps *api_ps)
{
    struct yagl_gles_api_ps *gles_api_ps = (struct yagl_gles_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles_process_destroy, NULL);

    yagl_gles_api_ps_cleanup(gles_api_ps);
    yagl_api_ps_cleanup(&gles_api_ps->base);

    g_free(gles_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_api_ps *yagl_host_gles_process_init(struct yagl_api *api)
{
    struct yagl_gles_api *gles_api = (struct yagl_gles_api*)api;
    struct yagl_gles_api_ps *gles_api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_gles_process_init, NULL);

    gles_api_ps = g_malloc0(sizeof(*gles_api_ps));

    yagl_api_ps_init(&gles_api_ps->base, api);

    gles_api_ps->base.thread_init = &yagl_host_gles_thread_init;
    gles_api_ps->base.batch_start = &yagl_host_gles_batch_start;
    gles_api_ps->base.get_func = &yagl_host_gles_get_func;
    gles_api_ps->base.batch_end = &yagl_host_gles_batch_end;
    gles_api_ps->base.thread_fini = &yagl_host_gles_thread_fini;
    gles_api_ps->base.destroy = &yagl_host_gles_process_destroy;

    yagl_gles_api_ps_init(gles_api_ps, gles_api->driver);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles_api_ps->base;
}

void yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count)
{
    gles_api_ts->driver->DrawArrays(mode, first, count);
}

void yagl_host_glDrawElements(GLenum mode,
    GLsizei count,
    GLenum type,
    const GLvoid *indices, int32_t indices_count)
{
    if (indices) {
        gles_api_ts->driver->DrawElements(mode, count, type, indices);
    } else {
        gles_api_ts->driver->DrawElements(mode, count, type,
                                          (const GLvoid*)(uintptr_t)indices_count);
    }
}

void yagl_host_glReadPixelsData(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    GLvoid *pixels, int32_t pixels_maxcount, int32_t *pixels_count)
{
    gles_api_ts->driver->ReadPixels(x,
                                    y,
                                    width,
                                    height,
                                    format,
                                    type,
                                    pixels);

    *pixels_count = pixels_maxcount;
}

void yagl_host_glReadPixelsOffset(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    GLsizei pixels)
{
    gles_api_ts->driver->ReadPixels(x,
                                    y,
                                    width,
                                    height,
                                    format,
                                    type,
                                    (GLvoid*)(uintptr_t)pixels);
}

void yagl_host_glDrawArraysInstanced(GLenum mode,
    GLint start,
    GLsizei count,
    GLsizei primcount)
{
    gles_api_ts->driver->DrawArraysInstanced(mode, start, count, primcount);
}

void yagl_host_glDrawElementsInstanced(GLenum mode,
    GLsizei count,
    GLenum type,
    const void *indices, int32_t indices_count,
    GLsizei primcount)
{
    if (indices) {
        gles_api_ts->driver->DrawElementsInstanced(mode, count, type, indices, primcount);
    } else {
        gles_api_ts->driver->DrawElementsInstanced(mode,
                                                   count,
                                                   type,
                                                   (const GLvoid*)(uintptr_t)indices_count,
                                                   primcount);
    }
}

void yagl_host_glGenVertexArrays(const GLuint *arrays, int32_t arrays_count)
{
    int i;

    for (i = 0; i < arrays_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenVertexArrays(1, &global_name);

        yagl_gles_object_add(arrays[i],
                             global_name,
                             yagl_get_ctx_id(),
                             &yagl_gles_vertex_array_destroy);
    }
}

void yagl_host_glBindVertexArray(GLuint array)
{
    gles_api_ts->driver->BindVertexArray(yagl_gles_object_get(array));
}

void yagl_host_glDisableVertexAttribArray(GLuint index)
{
    gles_api_ts->driver->DisableVertexAttribArray(index);
}

void yagl_host_glEnableVertexAttribArray(GLuint index)
{
    gles_api_ts->driver->EnableVertexAttribArray(index);
}

void yagl_host_glVertexAttribPointerData(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    GLuint current_vbo = yagl_gles_bind_array(indx, first, stride,
                                              data, data_count);

    gles_api_ts->driver->VertexAttribPointer(indx, size, type, normalized,
                                             stride,
                                             NULL);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, current_vbo);
}

void yagl_host_glVertexAttribPointerOffset(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    GLsizei offset)
{
    gles_api_ts->driver->VertexAttribPointer(indx, size, type, normalized,
                                             stride,
                                             (const GLvoid*)(uintptr_t)offset);
}

void yagl_host_glVertexPointerData(GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    GLuint current_vbo = yagl_gles_bind_array(yagl_gles1_array_vertex,
                                              first, stride,
                                              data, data_count);

    gles_api_ts->driver->VertexPointer(size, type, stride, NULL);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, current_vbo);
}

void yagl_host_glVertexPointerOffset(GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset)
{
    gles_api_ts->driver->VertexPointer(size, type, stride, (const GLvoid*)(uintptr_t)offset);
}

void yagl_host_glNormalPointerData(GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    GLuint current_vbo = yagl_gles_bind_array(yagl_gles1_array_normal,
                                              first, stride,
                                              data, data_count);

    gles_api_ts->driver->NormalPointer(type, stride, NULL);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, current_vbo);
}

void yagl_host_glNormalPointerOffset(GLenum type,
    GLsizei stride,
    GLsizei offset)
{
    gles_api_ts->driver->NormalPointer(type, stride, (const GLvoid*)(uintptr_t)offset);
}

void yagl_host_glColorPointerData(GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    GLuint current_vbo = yagl_gles_bind_array(yagl_gles1_array_color,
                                              first, stride,
                                              data, data_count);

    gles_api_ts->driver->ColorPointer(size, type, stride, NULL);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, current_vbo);
}

void yagl_host_glColorPointerOffset(GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset)
{
    gles_api_ts->driver->ColorPointer(size, type, stride, (const GLvoid*)(uintptr_t)offset);
}

void yagl_host_glTexCoordPointerData(GLint tex_id,
    GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    GLuint current_vbo = yagl_gles_bind_array(yagl_gles1_array_texcoord + tex_id,
                                              first, stride,
                                              data, data_count);

    gles_api_ts->driver->TexCoordPointer(size, type, stride, NULL);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, current_vbo);
}

void yagl_host_glTexCoordPointerOffset(GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset)
{
    gles_api_ts->driver->TexCoordPointer(size, type, stride, (const GLvoid*)(uintptr_t)offset);
}

void yagl_host_glDisableClientState(GLenum array)
{
    gles_api_ts->driver->DisableClientState(array);
}

void yagl_host_glEnableClientState(GLenum array)
{
    gles_api_ts->driver->EnableClientState(array);
}

void yagl_host_glVertexAttribDivisor(GLuint index,
    GLuint divisor)
{
    gles_api_ts->driver->VertexAttribDivisor(index, divisor);
}

void yagl_host_glVertexAttribIPointerData(GLuint index,
    GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count)
{
    GLuint current_vbo = yagl_gles_bind_array(index, first, stride,
                                              data, data_count);

    gles_api_ts->driver->VertexAttribIPointer(index, size, type, stride, NULL);

    gles_api_ts->driver->BindBuffer(GL_ARRAY_BUFFER, current_vbo);
}

void yagl_host_glVertexAttribIPointerOffset(GLuint index,
    GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset)
{
    gles_api_ts->driver->VertexAttribIPointer(index, size, type,
                                              stride,
                                              (const GLvoid*)(uintptr_t)offset);
}

void yagl_host_glGenBuffers(const GLuint *buffers, int32_t buffers_count)
{
    int i;

    for (i = 0; i < buffers_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenBuffers(1, &global_name);

        yagl_gles_object_add(buffers[i],
                             global_name,
                             0,
                             &yagl_gles_buffer_destroy);
    }
}

void yagl_host_glBindBuffer(GLenum target,
    GLuint buffer)
{
    gles_api_ts->driver->BindBuffer(target, yagl_gles_object_get(buffer));
}

void yagl_host_glBufferData(GLenum target,
    const GLvoid *data, int32_t data_count,
    GLenum usage)
{
    gles_api_ts->driver->BufferData(target, data_count, data, usage);
}

void yagl_host_glBufferSubData(GLenum target,
    GLsizei offset,
    const GLvoid *data, int32_t data_count)
{
    gles_api_ts->driver->BufferSubData(target, offset, data_count, data);
}

void yagl_host_glBindBufferBase(GLenum target,
    GLuint index,
    GLuint buffer)
{
    gles_api_ts->driver->BindBufferBase(target, index,
                                        yagl_gles_object_get(buffer));
}

void yagl_host_glBindBufferRange(GLenum target,
    GLuint index,
    GLuint buffer,
    GLint offset,
    GLsizei size)
{
    gles_api_ts->driver->BindBufferRange(target, index,
                                         yagl_gles_object_get(buffer),
                                         offset, size);
}

void yagl_host_glMapBuffer(GLuint buffer,
    const GLuint *ranges, int32_t ranges_count,
    GLvoid *data, int32_t data_maxcount, int32_t *data_count)
{
    GLuint current_pbo;
    GLvoid *data_ptr = data, *map_ptr;
    GLint i;

    YAGL_LOG_FUNC_SET(glMapBuffer);

    gles_api_ts->driver->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB,
                                     (GLint*)&current_pbo);

    gles_api_ts->driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, yagl_gles_object_get(buffer));

    map_ptr = gles_api_ts->driver->MapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);

    if (!map_ptr) {
        YAGL_LOG_ERROR("glMapBuffer failed");
        goto out1;
    }

    for (i = 0; i < (ranges_count / 2); ++i) {
        GLuint offset = ranges[(i * 2) + 0];
        GLuint size = ranges[(i * 2) + 1];

        map_ptr += offset;

        if (i > 0) {
            data_ptr += offset;
        }

        if ((data_ptr + size) > (data + data_maxcount)) {
            YAGL_LOG_ERROR("read out of range");
            goto out2;
        }

        memcpy(data_ptr, map_ptr, size);

        data_ptr += size;
    }

    *data_count = data_ptr - data;

out2:
    gles_api_ts->driver->UnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
out1:
    gles_api_ts->driver->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, current_pbo);
}

void yagl_host_glCopyBufferSubData(GLenum readTarget,
    GLenum writeTarget,
    GLint readOffset,
    GLint writeOffset,
    GLsizei size)
{
    gles_api_ts->driver->CopyBufferSubData(readTarget, writeTarget,
                                           readOffset, writeOffset, size);
}

void yagl_host_glGenTextures(const GLuint *textures, int32_t textures_count)
{
    int i;

    for (i = 0; i < textures_count; ++i) {
        GLuint global_name;

        /*
         * We need ensure/unensure here for eglReleaseTexImage that
         * might be called without an active context, but
         * which needs to create a texture.
         */
        yagl_ensure_ctx(0);
        gles_api_ts->driver->GenTextures(1, &global_name);
        yagl_unensure_ctx(0);

        yagl_gles_object_add(textures[i],
                             global_name,
                             0,
                             &yagl_gles_texture_destroy);
    }
}

void yagl_host_glBindTexture(GLenum target,
    GLuint texture)
{
    gles_api_ts->driver->BindTexture(target, yagl_gles_object_get(texture));
}

void yagl_host_glActiveTexture(GLenum texture)
{
    gles_api_ts->driver->ActiveTexture(texture);
}

void yagl_host_glCopyTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border)
{
    gles_api_ts->driver->CopyTexImage2D(target,
                                        level,
                                        internalformat,
                                        x,
                                        y,
                                        width,
                                        height,
                                        border);
}

void yagl_host_glCopyTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    gles_api_ts->driver->CopyTexSubImage2D(target,
                                           level,
                                           xoffset,
                                           yoffset,
                                           x,
                                           y,
                                           width,
                                           height);
}

void yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    GLfloat *param)
{
    GLfloat params[10];

    gles_api_ts->driver->GetTexParameterfv(target,
                                           pname,
                                           params);

    if (param) {
        *param = params[0];
    }
}

void yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    GLint *param)
{
    GLint params[10];

    gles_api_ts->driver->GetTexParameteriv(target,
                                           pname,
                                           params);

    if (param) {
        *param = params[0];
    }
}

void yagl_host_glTexImage2DData(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    const GLvoid *pixels, int32_t pixels_count)
{
    gles_api_ts->driver->TexImage2D(target,
                                    level,
                                    internalformat,
                                    width,
                                    height,
                                    border,
                                    format,
                                    type,
                                    pixels);
}

void yagl_host_glTexImage2DOffset(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    GLsizei pixels)
{
    gles_api_ts->driver->TexImage2D(target,
                                    level,
                                    internalformat,
                                    width,
                                    height,
                                    border,
                                    format,
                                    type,
                                    (const GLvoid*)(uintptr_t)pixels);
}

void yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->TexParameterf(target, pname, param);
}

void yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    GLfloat tmp[10];

    memset(tmp, 0, sizeof(tmp));

    if (params) {
        tmp[0] = *params;
    }

    gles_api_ts->driver->TexParameterfv(target,
                                        pname,
                                        (params ? tmp : NULL));
}

void yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param)
{
    gles_api_ts->driver->TexParameteri(target, pname, param);
}

void yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    const GLint *params, int32_t params_count)
{
    GLint tmp[10];

    memset(tmp, 0, sizeof(tmp));

    if (params) {
        tmp[0] = *params;
    }

    gles_api_ts->driver->TexParameteriv(target,
                                        pname,
                                        (params ? tmp : NULL));
}

void yagl_host_glTexSubImage2DData(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    const GLvoid *pixels, int32_t pixels_count)
{
    gles_api_ts->driver->TexSubImage2D(target,
                                       level,
                                       xoffset,
                                       yoffset,
                                       width,
                                       height,
                                       format,
                                       type,
                                       pixels);
}

void yagl_host_glTexSubImage2DOffset(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    GLsizei pixels)
{
    gles_api_ts->driver->TexSubImage2D(target,
                                       level,
                                       xoffset,
                                       yoffset,
                                       width,
                                       height,
                                       format,
                                       type,
                                       (const GLvoid*)(uintptr_t)pixels);
}

void yagl_host_glClientActiveTexture(GLenum texture)
{
    gles_api_ts->driver->ClientActiveTexture(texture);
}

void yagl_host_glTexEnvi(GLenum target,
    GLenum pname,
    GLint param)
{
    gles_api_ts->driver->TexEnvi(target, pname, param);
}

void yagl_host_glTexEnvf(GLenum target,
    GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->TexEnvf(target, pname, param);
}

void yagl_host_glMultiTexCoord4f(GLenum target,
    GLfloat s,
    GLfloat tt,
    GLfloat r,
    GLfloat q)
{
    gles_api_ts->driver->MultiTexCoord4f(target, s, tt, r, q);
}

void yagl_host_glTexEnviv(GLenum target,
    GLenum pname,
    const GLint *params, int32_t params_count)
{
    gles_api_ts->driver->TexEnviv(target, pname, params);
}

void yagl_host_glTexEnvfv(GLenum target,
    GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    gles_api_ts->driver->TexEnvfv(target, pname, params);
}

void yagl_host_glGetTexEnviv(GLenum env,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    gles_api_ts->driver->GetTexEnviv(env, pname, params);
    *params_count = params_maxcount;
}

void yagl_host_glGetTexEnvfv(GLenum env,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    gles_api_ts->driver->GetTexEnvfv(env, pname, params);
    *params_count = params_maxcount;
}

void yagl_host_glTexImage3DData(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLenum format,
    GLenum type,
    const void *pixels, int32_t pixels_count)
{
    gles_api_ts->driver->TexImage3D(target,
                                    level,
                                    internalformat,
                                    width,
                                    height,
                                    depth,
                                    border,
                                    format,
                                    type,
                                    pixels);
}

void yagl_host_glTexImage3DOffset(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLenum format,
    GLenum type,
    GLsizei pixels)
{
    gles_api_ts->driver->TexImage3D(target,
                                    level,
                                    internalformat,
                                    width,
                                    height,
                                    depth,
                                    border,
                                    format,
                                    type,
                                    (const void*)(uintptr_t)pixels);
}

void yagl_host_glTexSubImage3DData(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLenum format,
    GLenum type,
    const void *pixels, int32_t pixels_count)
{
    gles_api_ts->driver->TexSubImage3D(target,
                                       level,
                                       xoffset,
                                       yoffset,
                                       zoffset,
                                       width,
                                       height,
                                       depth,
                                       format,
                                       type,
                                       pixels);
}

void yagl_host_glTexSubImage3DOffset(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLenum format,
    GLenum type,
    GLsizei pixels)
{
    gles_api_ts->driver->TexSubImage3D(target,
                                       level,
                                       xoffset,
                                       yoffset,
                                       zoffset,
                                       width,
                                       height,
                                       depth,
                                       format,
                                       type,
                                       (const void*)(uintptr_t)pixels);
}

void yagl_host_glCopyTexSubImage3D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    gles_api_ts->driver->CopyTexSubImage3D(target,
                                           level,
                                           xoffset,
                                           yoffset,
                                           zoffset,
                                           x,
                                           y,
                                           width,
                                           height);
}

void yagl_host_glGenFramebuffers(const GLuint *framebuffers, int32_t framebuffers_count)
{
    int i;

    for (i = 0; i < framebuffers_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenFramebuffers(1, &global_name);

        yagl_gles_object_add(framebuffers[i],
                             global_name,
                             yagl_get_ctx_id(),
                             &yagl_gles_framebuffer_destroy);
    }
}

void yagl_host_glBindFramebuffer(GLenum target,
    GLuint framebuffer)
{
    gles_api_ts->driver->BindFramebuffer(target,
                                         yagl_gles_object_get(framebuffer));
}

void yagl_host_glFramebufferTexture2D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level)
{
    gles_api_ts->driver->FramebufferTexture2D(target, attachment,
                                              textarget,
                                              yagl_gles_object_get(texture),
                                              level);
}

void yagl_host_glFramebufferRenderbuffer(GLenum target,
    GLenum attachment,
    GLenum renderbuffertarget,
    GLuint renderbuffer)
{
    gles_api_ts->driver->FramebufferRenderbuffer(target,
                                                 attachment,
                                                 renderbuffertarget,
                                                 yagl_gles_object_get(renderbuffer));
}

void yagl_host_glBlitFramebuffer(GLint srcX0,
    GLint srcY0,
    GLint srcX1,
    GLint srcY1,
    GLint dstX0,
    GLint dstY0,
    GLint dstX1,
    GLint dstY1,
    GLbitfield mask,
    GLenum filter)
{
    gles_api_ts->driver->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
                                         dstX0, dstY0, dstX1, dstY1,
                                         mask, filter);
}

void yagl_host_glDrawBuffers(const GLenum *bufs, int32_t bufs_count)
{
    gles_api_ts->driver->DrawBuffers(bufs_count, bufs);
}

void yagl_host_glReadBuffer(GLenum mode)
{
    gles_api_ts->driver->ReadBuffer(mode);
}

void yagl_host_glFramebufferTexture3D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level,
    GLint zoffset)
{
    gles_api_ts->driver->FramebufferTexture3D(target, attachment,
                                              textarget,
                                              yagl_gles_object_get(texture),
                                              level, zoffset);
}

void yagl_host_glFramebufferTextureLayer(GLenum target,
    GLenum attachment,
    GLuint texture,
    GLint level,
    GLint layer)
{
    gles_api_ts->driver->FramebufferTextureLayer(target, attachment,
                                                 yagl_gles_object_get(texture),
                                                 level, layer);
}

void yagl_host_glGenRenderbuffers(const GLuint *renderbuffers, int32_t renderbuffers_count)
{
    int i;

    for (i = 0; i < renderbuffers_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenRenderbuffers(1, &global_name);

        yagl_gles_object_add(renderbuffers[i],
                             global_name,
                             0,
                             &yagl_gles_renderbuffer_destroy);
    }
}

void yagl_host_glBindRenderbuffer(GLenum target,
    GLuint renderbuffer)
{
    gles_api_ts->driver->BindRenderbuffer(target,
                                          yagl_gles_object_get(renderbuffer));
}

void yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height)
{
    gles_api_ts->driver->RenderbufferStorage(target, internalformat, width, height);
}

void yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    GLint *param)
{
    GLint params[10];

    gles_api_ts->driver->GetRenderbufferParameteriv(target,
                                                    pname,
                                                    params);

    if (param) {
        *param = params[0];
    }
}

void yagl_host_glRenderbufferStorageMultisample(GLenum target,
    GLsizei samples,
    GLenum internalformat,
    GLsizei width,
    GLsizei height)
{
    gles_api_ts->driver->RenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

void yagl_host_glCreateProgram(GLuint program)
{
    GLuint global_name = gles_api_ts->driver->CreateProgram();

    yagl_gles_object_add(program,
                         global_name,
                         0,
                         &yagl_gles_program_destroy);
}

void yagl_host_glCreateShader(GLuint shader,
    GLenum type)
{
    GLuint global_name = gles_api_ts->driver->CreateShader(type);

    yagl_gles_object_add(shader,
                         global_name,
                         0,
                         &yagl_gles_shader_destroy);
}

void yagl_host_glShaderSource(GLuint shader,
    const GLchar *string, int32_t string_count)
{
    const GLchar *strings[1];
    GLint lenghts[1];

    strings[0] = string;
    lenghts[0] = string_count - 1;

    gles_api_ts->driver->ShaderSource(yagl_gles_object_get(shader),
                                      1,
                                      strings,
                                      lenghts);
}

void yagl_host_glAttachShader(GLuint program,
    GLuint shader)
{
    gles_api_ts->driver->AttachShader(yagl_gles_object_get(program),
                                      yagl_gles_object_get(shader));
}

void yagl_host_glDetachShader(GLuint program,
    GLuint shader)
{
    gles_api_ts->driver->DetachShader(yagl_gles_object_get(program),
                                      yagl_gles_object_get(shader));
}

void yagl_host_glCompileShader(GLuint shader)
{
    gles_api_ts->driver->CompileShader(yagl_gles_object_get(shader));
}

void yagl_host_glBindAttribLocation(GLuint program,
    GLuint index,
    const GLchar *name, int32_t name_count)
{
    gles_api_ts->driver->BindAttribLocation(yagl_gles_object_get(program),
                                            index,
                                            name);
}

void yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count)
{
    GLsizei tmp = -1;

    gles_api_ts->driver->GetActiveAttrib(yagl_gles_object_get(program),
                                         index,
                                         name_maxcount,
                                         &tmp,
                                         size,
                                         type,
                                         name);

    if (tmp >= 0) {
        *name_count = MIN(tmp + 1, name_maxcount);
    }
}

void yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count)
{
    GLsizei tmp = -1;

    gles_api_ts->driver->GetActiveUniform(yagl_gles_object_get(program),
                                          index,
                                          name_maxcount,
                                          &tmp,
                                          size,
                                          type,
                                          name);

    if (tmp >= 0) {
        *name_count = MIN(tmp + 1, name_maxcount);
    }
}

int yagl_host_glGetAttribLocation(GLuint program,
    const GLchar *name, int32_t name_count)
{
    return gles_api_ts->driver->GetAttribLocation(yagl_gles_object_get(program),
                                                  name);
}

void yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    GLint *param)
{
    gles_api_ts->driver->GetProgramiv(yagl_gles_object_get(program),
                                      pname,
                                      param);
}

GLboolean yagl_host_glGetProgramInfoLog(GLuint program,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count)
{
    GLsizei tmp = -1;

    gles_api_ts->driver->GetProgramInfoLog(yagl_gles_object_get(program),
                                           infolog_maxcount,
                                           &tmp,
                                           infolog);

    if (tmp >= 0) {
        *infolog_count = MIN(tmp + 1, infolog_maxcount);
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

void yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    GLint *param)
{
    gles_api_ts->driver->GetShaderiv(yagl_gles_object_get(shader),
                                     pname,
                                     param);
}

GLboolean yagl_host_glGetShaderInfoLog(GLuint shader,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count)
{
    GLsizei tmp = -1;

    gles_api_ts->driver->GetShaderInfoLog(yagl_gles_object_get(shader),
                                          infolog_maxcount,
                                          &tmp,
                                          infolog);

    if (tmp >= 0) {
        *infolog_count = MIN(tmp + 1, infolog_maxcount);
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

void yagl_host_glGetUniformfv(GLboolean tl,
    GLuint program,
    uint32_t location,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    GLenum type;
    GLuint global_name = yagl_gles_object_get(program);
    GLint actual_location = yagl_gles_api_ps_translate_location(gles_api_ts->ps,
                                                                tl,
                                                                location);

    if (!yagl_gles_program_get_uniform_type(global_name,
                                            actual_location,
                                            &type)) {
        return;
    }

    if (!yagl_gles_get_uniform_type_count(type, params_count)) {
        return;
    }

    gles_api_ts->driver->GetUniformfv(global_name,
                                      actual_location,
                                      params);
}

void yagl_host_glGetUniformiv(GLboolean tl,
    GLuint program,
    uint32_t location,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    GLenum type;
    GLuint global_name = yagl_gles_object_get(program);
    GLint actual_location = yagl_gles_api_ps_translate_location(gles_api_ts->ps,
                                                                tl,
                                                                location);

    if (!yagl_gles_program_get_uniform_type(global_name,
                                            actual_location,
                                            &type)) {
        return;
    }

    if (!yagl_gles_get_uniform_type_count(type, params_count)) {
        return;
    }

    gles_api_ts->driver->GetUniformiv(global_name,
                                      actual_location,
                                      params);
}

int yagl_host_glGetUniformLocation(GLuint program,
    const GLchar *name, int32_t name_count)
{
    return gles_api_ts->driver->GetUniformLocation(yagl_gles_object_get(program),
                                                   name);
}

void yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    if (!yagl_gles_get_array_param_count(pname, params_count)) {
        return;
    }

    gles_api_ts->driver->GetVertexAttribfv(index, pname, params);
}

void yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    if (!yagl_gles_get_array_param_count(pname, params_count)) {
        return;
    }

    gles_api_ts->driver->GetVertexAttribiv(index, pname, params);
}

void yagl_host_glLinkProgram(GLuint program,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    GLuint obj = yagl_gles_object_get(program);

    gles_api_ts->driver->LinkProgram(obj);

    if (!params || (params_maxcount != 8)) {
        return;
    }

    gles_api_ts->driver->GetProgramiv(obj, GL_LINK_STATUS, &params[0]);
    gles_api_ts->driver->GetProgramiv(obj, GL_INFO_LOG_LENGTH, &params[1]);
    gles_api_ts->driver->GetProgramiv(obj, GL_ACTIVE_ATTRIBUTES, &params[2]);
    gles_api_ts->driver->GetProgramiv(obj, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &params[3]);
    gles_api_ts->driver->GetProgramiv(obj, GL_ACTIVE_UNIFORMS, &params[4]);
    gles_api_ts->driver->GetProgramiv(obj, GL_ACTIVE_UNIFORM_MAX_LENGTH, &params[5]);

    *params_count = 6;

    if (gles_api_ts->driver->gl_version > yagl_gl_2) {
        gles_api_ts->driver->GetProgramiv(obj, GL_ACTIVE_UNIFORM_BLOCKS, &params[6]);
        gles_api_ts->driver->GetProgramiv(obj, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &params[7]);

        *params_count = 8;
    }
}

void yagl_host_glUniform1f(GLboolean tl,
    uint32_t location,
    GLfloat x)
{
    gles_api_ts->driver->Uniform1f(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location), x);
}

void yagl_host_glUniform1fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform1fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v_count, v);
}

void yagl_host_glUniform1i(GLboolean tl,
    uint32_t location,
    GLint x)
{
    gles_api_ts->driver->Uniform1i(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x);
}

void yagl_host_glUniform1iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform1iv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v_count, v);
}

void yagl_host_glUniform2f(GLboolean tl,
    uint32_t location,
    GLfloat x,
    GLfloat y)
{
    gles_api_ts->driver->Uniform2f(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x, y);
}

void yagl_host_glUniform2fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform2fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 2), v);
}

void yagl_host_glUniform2i(GLboolean tl,
    uint32_t location,
    GLint x,
    GLint y)
{
    gles_api_ts->driver->Uniform2i(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x, y);
}

void yagl_host_glUniform2iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform2iv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 2), v);
}

void yagl_host_glUniform3f(GLboolean tl,
    uint32_t location,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    gles_api_ts->driver->Uniform3f(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x, y, z);
}

void yagl_host_glUniform3fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform3fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 3), v);
}

void yagl_host_glUniform3i(GLboolean tl,
    uint32_t location,
    GLint x,
    GLint y,
    GLint z)
{
    gles_api_ts->driver->Uniform3i(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x, y, z);
}

void yagl_host_glUniform3iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform3iv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 3), v);
}

void yagl_host_glUniform4f(GLboolean tl,
    uint32_t location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w)
{
    gles_api_ts->driver->Uniform4f(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x, y, z, w);
}

void yagl_host_glUniform4fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform4fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 4), v);
}

void yagl_host_glUniform4i(GLboolean tl,
    uint32_t location,
    GLint x,
    GLint y,
    GLint z,
    GLint w)
{
    gles_api_ts->driver->Uniform4i(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        x, y, z, w);
}

void yagl_host_glUniform4iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform4iv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 4), v);
}

void yagl_host_glUniformMatrix2fv(GLboolean tl,
    uint32_t location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count)
{
    gles_api_ts->driver->UniformMatrix2fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        value_count / (2 * 2), transpose, value);
}

void yagl_host_glUniformMatrix3fv(GLboolean tl,
    uint32_t location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count)
{
    gles_api_ts->driver->UniformMatrix3fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        value_count / (3 * 3), transpose, value);
}

void yagl_host_glUniformMatrix4fv(GLboolean tl,
    uint32_t location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count)
{
    gles_api_ts->driver->UniformMatrix4fv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        value_count / (4 * 4), transpose, value);
}

void yagl_host_glUseProgram(GLuint program)
{
    gles_api_ts->driver->UseProgram(yagl_gles_object_get(program));
}

void yagl_host_glValidateProgram(GLuint program)
{
    gles_api_ts->driver->ValidateProgram(yagl_gles_object_get(program));
}

void yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x)
{
    gles_api_ts->driver->VertexAttrib1f(indx, x);
}

void yagl_host_glVertexAttrib1fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    gles_api_ts->driver->VertexAttrib1fv(indx, values);
}

void yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y)
{
    gles_api_ts->driver->VertexAttrib2f(indx, x, y);
}

void yagl_host_glVertexAttrib2fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    gles_api_ts->driver->VertexAttrib2fv(indx, values);
}

void yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    gles_api_ts->driver->VertexAttrib3f(indx, x, y, z);
}

void yagl_host_glVertexAttrib3fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    gles_api_ts->driver->VertexAttrib3fv(indx, values);
}

void yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w)
{
    gles_api_ts->driver->VertexAttrib4f(indx, x, y, z, w);
}

void yagl_host_glVertexAttrib4fv(GLuint indx,
    const GLfloat *values, int32_t values_count)
{
    gles_api_ts->driver->VertexAttrib4fv(indx, values);
}

void yagl_host_glGetActiveUniformsiv(GLuint program,
    const GLuint *uniformIndices, int32_t uniformIndices_count,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    const GLenum pnames[] =
    {
        GL_UNIFORM_TYPE,
        GL_UNIFORM_SIZE,
        GL_UNIFORM_NAME_LENGTH,
        GL_UNIFORM_BLOCK_INDEX,
        GL_UNIFORM_OFFSET,
        GL_UNIFORM_ARRAY_STRIDE,
        GL_UNIFORM_MATRIX_STRIDE,
        GL_UNIFORM_IS_ROW_MAJOR
    };
    GLuint obj = yagl_gles_object_get(program);
    int i, num_pnames = sizeof(pnames)/sizeof(pnames[0]);

    if (params_maxcount != (uniformIndices_count * num_pnames)) {
        return;
    }

    for (i = 0; i < num_pnames; ++i) {
        gles_api_ts->driver->GetActiveUniformsiv(obj,
                                                 uniformIndices_count,
                                                 uniformIndices,
                                                 pnames[i],
                                                 params);
        params += uniformIndices_count;
    }

    *params_count = uniformIndices_count * num_pnames;
}

void yagl_host_glGetUniformIndices(GLuint program,
    const GLchar *uniformNames, int32_t uniformNames_count,
    GLuint *uniformIndices, int32_t uniformIndices_maxcount, int32_t *uniformIndices_count)
{
    GLuint obj = yagl_gles_object_get(program);
    int max_active_uniform_bufsize = 1, i;
    const GLchar **name_pointers;

    gles_api_ts->driver->GetProgramiv(obj,
                                      GL_ACTIVE_UNIFORM_MAX_LENGTH,
                                      &max_active_uniform_bufsize);

    name_pointers = g_malloc(uniformIndices_maxcount * sizeof(*name_pointers));

    for (i = 0; i < uniformIndices_maxcount; ++i) {
        name_pointers[i] = &uniformNames[max_active_uniform_bufsize * i];
    }

    gles_api_ts->driver->GetUniformIndices(obj,
                                           uniformIndices_maxcount,
                                           name_pointers,
                                           uniformIndices);

    g_free(name_pointers);

    *uniformIndices_count = uniformIndices_maxcount;
}

GLuint yagl_host_glGetUniformBlockIndex(GLuint program,
    const GLchar *uniformBlockName, int32_t uniformBlockName_count)
{
    return gles_api_ts->driver->GetUniformBlockIndex(yagl_gles_object_get(program),
                                                     uniformBlockName);
}

void yagl_host_glUniformBlockBinding(GLuint program,
    GLuint uniformBlockIndex,
    GLuint uniformBlockBinding)
{
    gles_api_ts->driver->UniformBlockBinding(yagl_gles_object_get(program),
                                             uniformBlockIndex,
                                             uniformBlockBinding);
}

void yagl_host_glGetActiveUniformBlockName(GLuint program,
    GLuint uniformBlockIndex,
    GLchar *uniformBlockName, int32_t uniformBlockName_maxcount, int32_t *uniformBlockName_count)
{
    GLsizei tmp = -1;

    gles_api_ts->driver->GetActiveUniformBlockName(yagl_gles_object_get(program),
                                                   uniformBlockIndex,
                                                   uniformBlockName_maxcount,
                                                   &tmp,
                                                   uniformBlockName);

    if (tmp >= 0) {
        *uniformBlockName_count = MIN(tmp + 1, uniformBlockName_maxcount);
    }
}

void yagl_host_glGetActiveUniformBlockiv(GLuint program,
    GLuint uniformBlockIndex,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    GLuint obj;
    const GLenum pnames[] =
    {
        GL_UNIFORM_BLOCK_DATA_SIZE,
        GL_UNIFORM_BLOCK_NAME_LENGTH,
        GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
        GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER,
        GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER
    };
    int i, num_pnames = sizeof(pnames)/sizeof(pnames[0]);

    if (!params) {
        return;
    }

    obj = yagl_gles_object_get(program);

    switch (pname) {
    case 0:
        /*
         * Return everything else.
         */
        if (params_maxcount != num_pnames) {
            return;
        }

        for (i = 0; i < num_pnames; ++i) {
            gles_api_ts->driver->GetActiveUniformBlockiv(obj,
                                                         uniformBlockIndex,
                                                         pnames[i],
                                                         &params[i]);
        }

        *params_count = num_pnames;

        break;
    case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
        /*
         * Return active uniform indices only.
         */

        gles_api_ts->driver->GetActiveUniformBlockiv(obj,
                                                     uniformBlockIndex,
                                                     GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                                     params);

        *params_count = params_maxcount;

        break;
    default:
        break;
    }
}

void yagl_host_glGetVertexAttribIiv(GLuint index,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    if (!yagl_gles_get_array_param_count(pname, params_count)) {
        return;
    }

    gles_api_ts->driver->GetVertexAttribIiv(index, pname, params);
}

void yagl_host_glGetVertexAttribIuiv(GLuint index,
    GLenum pname,
    GLuint *params, int32_t params_maxcount, int32_t *params_count)
{
    if (!yagl_gles_get_array_param_count(pname, params_count)) {
        return;
    }

    gles_api_ts->driver->GetVertexAttribIuiv(index, pname, params);
}

void yagl_host_glVertexAttribI4i(GLuint index,
    GLint x,
    GLint y,
    GLint z,
    GLint w)
{
    gles_api_ts->driver->VertexAttribI4i(index, x, y, z, w);
}

void yagl_host_glVertexAttribI4ui(GLuint index,
    GLuint x,
    GLuint y,
    GLuint z,
    GLuint w)
{
    gles_api_ts->driver->VertexAttribI4ui(index, x, y, z, w);
}

void yagl_host_glVertexAttribI4iv(GLuint index,
    const GLint *v, int32_t v_count)
{
    gles_api_ts->driver->VertexAttribI4iv(index, v);
}

void yagl_host_glVertexAttribI4uiv(GLuint index,
    const GLuint *v, int32_t v_count)
{
    gles_api_ts->driver->VertexAttribI4uiv(index, v);
}

void yagl_host_glGetUniformuiv(GLboolean tl,
    GLuint program,
    uint32_t location,
    GLuint *params, int32_t params_maxcount, int32_t *params_count)
{
    GLenum type;
    GLuint global_name = yagl_gles_object_get(program);
    GLint actual_location = yagl_gles_api_ps_translate_location(gles_api_ts->ps,
                                                                tl,
                                                                location);

    if (!yagl_gles_program_get_uniform_type(global_name,
                                            actual_location,
                                            &type)) {
        return;
    }

    if (!yagl_gles_get_uniform_type_count(type, params_count)) {
        return;
    }

    gles_api_ts->driver->GetUniformuiv(global_name,
                                       actual_location,
                                       params);
}

void yagl_host_glUniform1ui(GLboolean tl,
    uint32_t location,
    GLuint v0)
{
    gles_api_ts->driver->Uniform1ui(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v0);
}

void yagl_host_glUniform2ui(GLboolean tl,
    uint32_t location,
    GLuint v0,
    GLuint v1)
{
    gles_api_ts->driver->Uniform2ui(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v0, v1);
}

void yagl_host_glUniform3ui(GLboolean tl,
    uint32_t location,
    GLuint v0,
    GLuint v1,
    GLuint v2)
{
    gles_api_ts->driver->Uniform3ui(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v0, v1, v2);
}

void yagl_host_glUniform4ui(GLboolean tl,
    uint32_t location,
    GLuint v0,
    GLuint v1,
    GLuint v2,
    GLuint v3)
{
    gles_api_ts->driver->Uniform4ui(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v0, v1, v2, v3);
}

void yagl_host_glUniform1uiv(GLboolean tl,
    uint32_t location,
    const GLuint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform1uiv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        v_count, v);
}

void yagl_host_glUniform2uiv(GLboolean tl,
    uint32_t location,
    const GLuint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform2uiv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 2), v);
}

void yagl_host_glUniform3uiv(GLboolean tl,
    uint32_t location,
    const GLuint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform3uiv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 3), v);
}

void yagl_host_glUniform4uiv(GLboolean tl,
    uint32_t location,
    const GLuint *v, int32_t v_count)
{
    gles_api_ts->driver->Uniform4uiv(
        yagl_gles_api_ps_translate_location(gles_api_ts->ps, tl, location),
        (v_count / 4), v);
}

void yagl_host_glGetIntegerv(GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count)
{
    gles_api_ts->driver->GetIntegerv(pname, params);

    *params_count = params_maxcount;
}

void yagl_host_glGetFloatv(GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    gles_api_ts->driver->GetFloatv(pname, params);

    *params_count = params_maxcount;
}

void yagl_host_glGetString(GLenum name,
    GLchar *str, int32_t str_maxcount, int32_t *str_count)
{
    const char *tmp;

    if ((name == GL_EXTENSIONS) &&
        (gles_api_ts->driver->gl_version > yagl_gl_2)) {
        struct yagl_vector v;
        GLint i, num_extensions = 0;
        char nb = '\0';

        yagl_vector_init(&v, 1, 0);

        gles_api_ts->driver->GetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

        for (i = 0; i < num_extensions; ++i) {
            tmp = (const char*)gles_api_ts->driver->GetStringi(name, i);
            int size = yagl_vector_size(&v);
            int ext_len = strlen(tmp);

            yagl_vector_resize(&v, size + ext_len + 1);

            memcpy(yagl_vector_data(&v) + size, tmp, ext_len);

            *(char*)(yagl_vector_data(&v) + size + ext_len) = ' ';
        }

        yagl_vector_push_back(&v, &nb);

        tmp = yagl_vector_data(&v);

        if (str_count) {
            *str_count = strlen(tmp) + 1;
        }

        if (str && (str_maxcount > 0)) {
            strncpy(str, tmp, str_maxcount);
            str[str_maxcount - 1] = '\0';
        }

        yagl_vector_cleanup(&v);

        return;
    }

    tmp = (const char*)gles_api_ts->driver->GetString(name);

    if (str_count) {
        *str_count = strlen(tmp) + 1;
    }

    if (str && (str_maxcount > 0)) {
        strncpy(str, tmp, str_maxcount);
        str[str_maxcount - 1] = '\0';
    }
}

GLboolean yagl_host_glIsEnabled(GLenum cap)
{
    return gles_api_ts->driver->IsEnabled(cap);
}

void yagl_host_glGenTransformFeedbacks(const GLuint *ids, int32_t ids_count)
{
    int i;

    for (i = 0; i < ids_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenTransformFeedbacks(1, &global_name);

        yagl_gles_object_add(ids[i],
                             global_name,
                             yagl_get_ctx_id(),
                             &yagl_gles_transform_feedback_destroy);
    }
}

void yagl_host_glBindTransformFeedback(GLenum target,
    GLuint id)
{
    gles_api_ts->driver->BindTransformFeedback(target,
                                               yagl_gles_object_get(id));
}

void yagl_host_glBeginTransformFeedback(GLenum primitiveMode)
{
    gles_api_ts->driver->BeginTransformFeedback(primitiveMode);
}

void yagl_host_glEndTransformFeedback(void)
{
    gles_api_ts->driver->EndTransformFeedback();
}

void yagl_host_glPauseTransformFeedback(void)
{
    gles_api_ts->driver->PauseTransformFeedback();
}

void yagl_host_glResumeTransformFeedback(void)
{
    gles_api_ts->driver->ResumeTransformFeedback();
}

void yagl_host_glTransformFeedbackVaryings(GLuint program,
    const GLchar *varyings, int32_t varyings_count,
    GLenum bufferMode)
{
    const char **strings;
    int32_t num_strings = 0;

    strings = yagl_transport_get_out_string_array(varyings,
                                                  varyings_count,
                                                  &num_strings);

    gles_api_ts->driver->TransformFeedbackVaryings(yagl_gles_object_get(program),
                                                   num_strings,
                                                   strings,
                                                   bufferMode);

    g_free(strings);
}

void yagl_host_glGetTransformFeedbackVaryings(GLuint program,
    GLsizei *sizes, int32_t sizes_maxcount, int32_t *sizes_count,
    GLenum *types, int32_t types_maxcount, int32_t *types_count)
{
    GLuint obj = yagl_gles_object_get(program);
    int32_t i;

    if (sizes_maxcount != types_maxcount) {
        return;
    }

    for (i = 0; i < sizes_maxcount; ++i) {
        GLsizei length = -1;
        GLchar c[2];

        gles_api_ts->driver->GetTransformFeedbackVarying(obj,
                                                         i, sizeof(c), &length,
                                                         &sizes[i], &types[i],
                                                         c);

        if (length <= 0) {
            sizes[i] = 0;
            types[i] = 0;
        }
    }

    *sizes_count = *types_count = sizes_maxcount;
}

void yagl_host_glGenQueries(const GLuint *ids, int32_t ids_count)
{
    int i;

    for (i = 0; i < ids_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenQueries(1, &global_name);

        yagl_gles_object_add(ids[i],
                             global_name,
                             yagl_get_ctx_id(),
                             &yagl_gles_query_destroy);
    }
}

void yagl_host_glBeginQuery(GLenum target,
    GLuint id)
{
    gles_api_ts->driver->BeginQuery(target, yagl_gles_object_get(id));
}

void yagl_host_glEndQuery(GLenum target)
{
    gles_api_ts->driver->EndQuery(target);
}

GLboolean yagl_host_glGetQueryObjectuiv(GLuint id,
    GLuint *result)
{
    GLuint obj = yagl_gles_object_get(id);
    GLuint tmp = 0;

    gles_api_ts->driver->GetQueryObjectuiv(obj, GL_QUERY_RESULT_AVAILABLE, &tmp);

    if (tmp) {
        gles_api_ts->driver->GetQueryObjectuiv(obj, GL_QUERY_RESULT, result);
    }

    return tmp;
}

void yagl_host_glGenSamplers(const GLuint *samplers, int32_t samplers_count)
{
    int i;

    for (i = 0; i < samplers_count; ++i) {
        GLuint global_name;

        gles_api_ts->driver->GenSamplers(1, &global_name);

        yagl_gles_object_add(samplers[i],
                             global_name,
                             0,
                             &yagl_gles_sampler_destroy);
    }
}

void yagl_host_glBindSampler(GLuint unit,
    GLuint sampler)
{
    gles_api_ts->driver->BindSampler(unit, yagl_gles_object_get(sampler));
}

void yagl_host_glSamplerParameteri(GLuint sampler,
    GLenum pname,
    GLint param)
{
    gles_api_ts->driver->SamplerParameteri(yagl_gles_object_get(sampler), pname, param);
}

void yagl_host_glSamplerParameteriv(GLuint sampler,
    GLenum pname,
    const GLint *param, int32_t param_count)
{
    gles_api_ts->driver->SamplerParameteriv(yagl_gles_object_get(sampler), pname, param);
}

void yagl_host_glSamplerParameterf(GLuint sampler,
    GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->SamplerParameterf(yagl_gles_object_get(sampler), pname, param);
}

void yagl_host_glSamplerParameterfv(GLuint sampler,
    GLenum pname,
    const GLfloat *param, int32_t param_count)
{
    gles_api_ts->driver->SamplerParameterfv(yagl_gles_object_get(sampler), pname, param);
}

void yagl_host_glDeleteObjects(const GLuint *objects, int32_t objects_count)
{
    int i;

    for (i = 0; i < objects_count; ++i) {
        yagl_object_map_remove(cur_ts->ps->object_map, objects[i]);
    }
}

void yagl_host_glBlendEquation(GLenum mode)
{
    gles_api_ts->driver->BlendEquation(mode);
}

void yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha)
{
    gles_api_ts->driver->BlendEquationSeparate(modeRGB, modeAlpha);
}

void yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor)
{
    gles_api_ts->driver->BlendFunc(sfactor, dfactor);
}

void yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha)
{
    gles_api_ts->driver->BlendFuncSeparate(srcRGB,
                                           dstRGB,
                                           srcAlpha,
                                           dstAlpha);
}

void yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    gles_api_ts->driver->BlendColor(red, green, blue, alpha);
}

void yagl_host_glClear(GLbitfield mask)
{
    gles_api_ts->driver->Clear(mask);
}

void yagl_host_glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    gles_api_ts->driver->ClearColor(red, green, blue, alpha);
}

void yagl_host_glClearDepthf(GLclampf depth)
{
    gles_api_ts->driver->ClearDepth(depth);
}

void yagl_host_glClearStencil(GLint s)
{
    gles_api_ts->driver->ClearStencil(s);
}

void yagl_host_glColorMask(GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha)
{
    gles_api_ts->driver->ColorMask(red, green, blue, alpha);
}

void yagl_host_glCullFace(GLenum mode)
{
    gles_api_ts->driver->CullFace(mode);
}

void yagl_host_glDepthFunc(GLenum func)
{
    gles_api_ts->driver->DepthFunc(func);
}

void yagl_host_glDepthMask(GLboolean flag)
{
    gles_api_ts->driver->DepthMask(flag);
}

void yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar)
{
    gles_api_ts->driver->DepthRange(zNear, zFar);
}

void yagl_host_glEnable(GLenum cap)
{
    gles_api_ts->driver->Enable(cap);
}

void yagl_host_glDisable(GLenum cap)
{
    gles_api_ts->driver->Disable(cap);
}

void yagl_host_glFlush(void)
{
    gles_api_ts->driver->Flush();
}

void yagl_host_glFrontFace(GLenum mode)
{
    gles_api_ts->driver->FrontFace(mode);
}

void yagl_host_glGenerateMipmap(GLenum target)
{
    gles_api_ts->driver->GenerateMipmap(target);
}

void yagl_host_glHint(GLenum target,
    GLenum mode)
{
    gles_api_ts->driver->Hint(target, mode);
}

void yagl_host_glLineWidth(GLfloat width)
{
    gles_api_ts->driver->LineWidth(width);
}

void yagl_host_glPixelStorei(GLenum pname,
    GLint param)
{
    gles_api_ts->driver->PixelStorei(pname, param);
}

void yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units)
{
    gles_api_ts->driver->PolygonOffset(factor, units);
}

void yagl_host_glScissor(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    gles_api_ts->driver->Scissor(x, y, width, height);
}

void yagl_host_glStencilFunc(GLenum func,
    GLint ref,
    GLuint mask)
{
    gles_api_ts->driver->StencilFunc(func, ref, mask);
}

void yagl_host_glStencilMask(GLuint mask)
{
    gles_api_ts->driver->StencilMask(mask);
}

void yagl_host_glStencilOp(GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    gles_api_ts->driver->StencilOp(fail, zfail, zpass);
}

void yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert)
{
    gles_api_ts->driver->SampleCoverage(value, invert);
}

void yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height)
{
    gles_api_ts->driver->Viewport(x, y, width, height);
}

void yagl_host_glStencilFuncSeparate(GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask)
{
    gles_api_ts->driver->StencilFuncSeparate(face, func, ref, mask);
}

void yagl_host_glStencilMaskSeparate(GLenum face,
    GLuint mask)
{
    gles_api_ts->driver->StencilMaskSeparate(face, mask);
}

void yagl_host_glStencilOpSeparate(GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass)
{
    gles_api_ts->driver->StencilOpSeparate(face, fail, zfail, zpass);
}

void yagl_host_glPointSize(GLfloat size)
{
    gles_api_ts->driver->PointSize(size);
}

void yagl_host_glAlphaFunc(GLenum func,
    GLclampf ref)
{
    gles_api_ts->driver->AlphaFunc(func, ref);
}

void yagl_host_glMatrixMode(GLenum mode)
{
    gles_api_ts->driver->MatrixMode(mode);
}

void yagl_host_glLoadIdentity(void)
{
    gles_api_ts->driver->LoadIdentity();
}

void yagl_host_glPopMatrix(void)
{
    gles_api_ts->driver->PopMatrix();
}

void yagl_host_glPushMatrix(void)
{
    gles_api_ts->driver->PushMatrix();
}

void yagl_host_glRotatef(GLfloat angle,
    GLfloat x,
    GLfloat y,
    GLfloat z)
{
    gles_api_ts->driver->Rotatef(angle, x, y, z);
}

void yagl_host_glTranslatef(GLfloat x,
    GLfloat y,
    GLfloat z)
{
    gles_api_ts->driver->Translatef(x, y, z);
}

void yagl_host_glScalef(GLfloat x,
    GLfloat y,
    GLfloat z)
{
    gles_api_ts->driver->Scalef(x, y, z);
}

void yagl_host_glOrthof(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar)
{
    gles_api_ts->driver->Ortho(left, right, bottom, top, zNear, zFar);
}

void yagl_host_glColor4f(GLfloat red,
    GLfloat green,
    GLfloat blue,
    GLfloat alpha)
{
    gles_api_ts->driver->Color4f(red, green, blue, alpha);
}

void yagl_host_glColor4ub(GLubyte red,
    GLubyte green,
    GLubyte blue,
    GLubyte alpha)
{
    gles_api_ts->driver->Color4ub(red, green, blue, alpha);
}

void yagl_host_glNormal3f(GLfloat nx,
    GLfloat ny,
    GLfloat nz)
{
    gles_api_ts->driver->Normal3f(nx, ny, nz);
}

void yagl_host_glPointParameterf(GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->PointParameterf(pname, param);
}

void yagl_host_glPointParameterfv(GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    gles_api_ts->driver->PointParameterfv(pname, params);
}

void yagl_host_glFogf(GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->Fogf(pname, param);
}

void yagl_host_glFogfv(GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    gles_api_ts->driver->Fogfv(pname, params);
}

void yagl_host_glFrustumf(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar)
{
    gles_api_ts->driver->Frustum(left, right, bottom, top, zNear, zFar);
}

void yagl_host_glLightf(GLenum light,
    GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->Lightf(light, pname, param);
}

void yagl_host_glLightfv(GLenum light,
    GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    gles_api_ts->driver->Lightfv(light, pname, params);
}

void yagl_host_glGetLightfv(GLenum light,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    gles_api_ts->driver->GetLightfv(light, pname, params);
    *params_count = params_maxcount;
}

void yagl_host_glLightModelf(GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->LightModelf(pname, param);
}

void yagl_host_glLightModelfv(GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    gles_api_ts->driver->LightModelfv(pname, params);
}

void yagl_host_glMaterialf(GLenum face,
    GLenum pname,
    GLfloat param)
{
    gles_api_ts->driver->Materialf(face, pname, param);
}

void yagl_host_glMaterialfv(GLenum face,
    GLenum pname,
    const GLfloat *params, int32_t params_count)
{
    gles_api_ts->driver->Materialfv(face, pname, params);
}

void yagl_host_glGetMaterialfv(GLenum face,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count)
{
    gles_api_ts->driver->GetMaterialfv(face, pname, params);
    *params_count = params_maxcount;
}

void yagl_host_glShadeModel(GLenum mode)
{
    gles_api_ts->driver->ShadeModel(mode);
}

void yagl_host_glLogicOp(GLenum opcode)
{
    gles_api_ts->driver->LogicOp(opcode);
}

void yagl_host_glMultMatrixf(const GLfloat *m, int32_t m_count)
{
    gles_api_ts->driver->MultMatrixf(m);
}

void yagl_host_glLoadMatrixf(const GLfloat *m, int32_t m_count)
{
    gles_api_ts->driver->LoadMatrixf(m);
}

void yagl_host_glClipPlanef(GLenum plane,
    const GLfloat *equation, int32_t equation_count)
{
    yagl_GLdouble equationd[4];

    if (equation) {
        equationd[0] = equation[0];
        equationd[1] = equation[1];
        equationd[2] = equation[2];
        equationd[3] = equation[3];
        gles_api_ts->driver->ClipPlane(plane, equationd);
    } else {
        gles_api_ts->driver->ClipPlane(plane, NULL);
    }
}

void yagl_host_glGetClipPlanef(GLenum pname,
    GLfloat *eqn, int32_t eqn_maxcount, int32_t *eqn_count)
{
    yagl_GLdouble eqnd[4];

    gles_api_ts->driver->GetClipPlane(pname, eqnd);

    if (eqn) {
        eqn[0] = eqnd[0];
        eqn[1] = eqnd[1];
        eqn[2] = eqnd[2];
        eqn[3] = eqnd[3];
        *eqn_count = 4;
    }
}

void yagl_host_glUpdateOffscreenImageYAGL(GLuint texture,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    const void *pixels, int32_t pixels_count)
{
    GLenum format = 0;
    GLuint cur_tex = 0;
    GLsizei unpack[3];

    YAGL_LOG_FUNC_SET(glUpdateOffscreenImageYAGL);

    switch (bpp) {
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_BGRA;
        break;
    default:
        YAGL_LOG_ERROR("bad bpp - %u", bpp);
        return;
    }

    gles_api_ts->driver->GetIntegerv(GL_TEXTURE_BINDING_2D,
                                     (GLint*)&cur_tex);

    gles_api_ts->driver->GetIntegerv(GL_UNPACK_ALIGNMENT, &unpack[0]);
    gles_api_ts->driver->GetIntegerv(GL_UNPACK_ROW_LENGTH, &unpack[1]);
    gles_api_ts->driver->GetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &unpack[2]);

    gles_api_ts->driver->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gles_api_ts->driver->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gles_api_ts->driver->PixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

    gles_api_ts->driver->BindTexture(GL_TEXTURE_2D,
                                     yagl_gles_object_get(texture));

    gles_api_ts->driver->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gles_api_ts->driver->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    gles_api_ts->driver->TexImage2D(GL_TEXTURE_2D,
                                    0,
                                    GL_RGB,
                                    width,
                                    height,
                                    0,
                                    format,
                                    GL_UNSIGNED_INT_8_8_8_8_REV,
                                    pixels);

    gles_api_ts->driver->PixelStorei(GL_UNPACK_ALIGNMENT, unpack[0]);
    gles_api_ts->driver->PixelStorei(GL_UNPACK_ROW_LENGTH, unpack[1]);
    gles_api_ts->driver->PixelStorei(GL_UNPACK_IMAGE_HEIGHT, unpack[2]);

    gles_api_ts->driver->BindTexture(GL_TEXTURE_2D, cur_tex);
}

void yagl_host_glGenUniformLocationYAGL(uint32_t location,
    GLuint program,
    const GLchar *name, int32_t name_count)
{
    yagl_gles_api_ps_add_location(gles_api_ts->ps,
        location,
        gles_api_ts->driver->GetUniformLocation(yagl_gles_object_get(program), name));
}

void yagl_host_glDeleteUniformLocationsYAGL(const uint32_t *locations, int32_t locations_count)
{
    int i;

    for (i = 0; i < locations_count; ++i) {
        yagl_gles_api_ps_remove_location(gles_api_ts->ps, locations[i]);
    }
}
