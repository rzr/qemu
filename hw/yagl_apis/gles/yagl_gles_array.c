#include <GLES/gl.h>
#include "yagl_gles_array.h"
#include "yagl_gles_context.h"
#include "yagl_gles_buffer.h"
#include "yagl_mem.h"
#include "yagl_sharegroup.h"

static __inline bool yagl_get_el_size(GLenum type, int *el_size)
{
    switch (type) {
    case GL_BYTE:
        *el_size = 1;
        break;
    case GL_UNSIGNED_BYTE:
        *el_size = 1;
        break;
    case GL_SHORT:
        *el_size = 2;
        break;
    case GL_UNSIGNED_SHORT:
        *el_size = 2;
        break;
    case GL_FLOAT:
        *el_size = 4;
        break;
    case GL_FIXED:
        *el_size = 4;
        break;
    default:
        return false;
    }
    return true;
}

static void yagl_gles_array_reset(struct yagl_gles_array *array)
{
    if (array->vbo) {
        yagl_gles_buffer_release(array->vbo);
        array->vbo = NULL;
        array->offset = 0;
    } else {
        array->target_data = 0;
        g_free(array->host_data);
        array->host_data = NULL;
        array->host_data_size = 0;
    }
}

void yagl_gles_array_init(struct yagl_gles_array *array,
                          GLuint index,
                          struct yagl_gles_context *ctx,
                          yagl_gles_array_apply_func apply)
{
    memset(array, 0, sizeof(*array));

    array->index = index;
    array->ctx = ctx;
    array->apply = apply;
}

void yagl_gles_array_cleanup(struct yagl_gles_array *array)
{
    if (array->vbo) {
        yagl_sharegroup_reap_object(array->ctx->base.sg, &array->vbo->base);
        array->vbo = NULL;
        array->offset = 0;
    } else {
        array->target_data = 0;
        g_free(array->host_data);
        array->host_data = NULL;
        array->host_data_size = 0;
    }
}

void yagl_gles_array_enable(struct yagl_gles_array *array, bool enable)
{
    array->enabled = enable;
}

bool yagl_gles_array_update(struct yagl_gles_array *array,
                            GLint size,
                            GLenum type,
                            GLboolean normalized,
                            GLsizei stride,
                            target_ulong target_data)
{
    if (!yagl_get_el_size(type, &array->el_size)) {
        return false;
    }

    yagl_gles_array_reset(array);

    array->size = size;
    array->type = type;
    array->normalized = normalized;
    array->stride = stride;

    if (!array->stride) {
        array->stride = array->size * array->el_size;
    }

    array->target_data = target_data;

    return true;
}

bool yagl_gles_array_update_vbo(struct yagl_gles_array *array,
                                GLint size,
                                GLenum type,
                                GLboolean normalized,
                                GLsizei stride,
                                struct yagl_gles_buffer *vbo,
                                GLint offset)
{
    if (!yagl_get_el_size(type, &array->el_size)) {
        return false;
    }

    yagl_gles_buffer_acquire(vbo);

    yagl_gles_array_reset(array);

    array->size = size;
    array->type = type;
    array->normalized = normalized;
    array->stride = stride;

    if (!array->stride) {
        array->stride = array->size * array->el_size;
    }

    array->vbo = vbo;
    array->offset = offset;

    return true;
}

bool yagl_gles_array_transfer(struct yagl_gles_array *array,
                              uint32_t first,
                              uint32_t count)
{
    uint32_t host_data_size;

    if (!array->enabled) {
        return true;
    }

    if (array->vbo) {
        yagl_gles_buffer_transfer(array->vbo, array->type, GL_ARRAY_BUFFER);

        array->apply(array);
    } else {
        if (!array->target_data) {
            return true;
        }

        /*
         * We must take 'first' into account since we're going to
         * feed 'host_data' to glVertexAttribPointer or whatever, so
         * the data before 'first' can be garbage, the host won't touch it,
         * what matters is data starting at 'first' and 'count' elements long.
         *
         * Nokia dgles has an implementation of this where there's an extra
         * variable called 'delta' = first * stride, which is being subtracted
         * from 'host_data', thus, giving host implementation a bad pointer which
         * it will increment according to GL rules and will get to what we need.
         * Though works, I find this approach awkward, so I'll keep with
         * overallocating for now...
         */

        host_data_size = (first + count) * array->stride;

        if (host_data_size > array->host_data_size) {
            array->host_data_size = host_data_size;
            g_free(array->host_data);
            array->host_data = g_malloc(array->host_data_size);
        }

        /*
         * TODO: fix for GL_FIXED case.
         */

        if (!yagl_mem_get(array->ctx->ts,
                          array->target_data + (first * array->stride),
                          (count * array->stride),
                          array->host_data + (first * array->stride))) {
            return false;
        }

        array->apply(array);
    }

    return true;
}
