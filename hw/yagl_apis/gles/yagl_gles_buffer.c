#include <GLES/gl.h>
#include "yagl_gles_buffer.h"
#include "yagl_gles_driver.h"
#include "yagl_gles_validate.h"

typedef void (*yagl_gles_buffer_transfer_func)(struct yagl_gles_buffer */*buffer*/,
                                               GLenum /*target*/,
                                               int /*start*/,
                                               int /*size*/);

static void yagl_gles_buffer_transfer_default(struct yagl_gles_buffer *buffer,
                                              GLenum target,
                                              int start,
                                              int size)
{
    if ((start == 0) && (size == buffer->size)) {
        buffer->driver->BufferData(target, size,
                                   buffer->data, buffer->usage);
    } else {
        buffer->driver->BufferSubData(target, start,
                                      size, buffer->data + start);
    }
}

/*
 * Here we directly substitute target GLfixed data with GLfloat data.
 * We can do that because GLfloat and GLfixed have the same data length
 * and because buffer data is not directly visible to guest system.
 * In the future, if something like glMapBuffer would have to be implemented,
 * this approach should be changed to allocating intermediate buffer for
 * passing to glBuferData call. The only two users who access buffer data
 * directly is yagl_gles1_draw_arrays_psize/yagl_gles1_draw_elem_psize,
 * they would have to be modified to perform manual conversion from GL_FIXED
 * to GL_FLOAT, or to use glMapBuffer call themselves.
 */

static void yagl_gles_buffer_transfer_fixed(struct yagl_gles_buffer *buffer,
                                            GLenum target,
                                            int start,
                                            int size)
{
    void *data = buffer->data + start;;
    int i;

    assert(sizeof(GLfixed) == sizeof(GLfloat));

    for (i = 0; i < (size / sizeof(GLfloat)); ++i) {
        ((GLfloat *)data)[i] = yagl_fixed_to_float(((GLfixed *)data)[i]);
    }

    if ((start == 0) && (size == buffer->size)) {
        buffer->driver->BufferData(target, size, data, buffer->usage);
    } else {
        buffer->driver->BufferSubData(target, start, size, data);
    }
}

static void yagl_gles_buffer_transfer_byte(struct yagl_gles_buffer *buffer,
                                           GLenum target,
                                           int start,
                                           int size)
{
    int host_size = size * sizeof(GLshort);
    GLbyte *target_data = buffer->data + start;
    GLshort *host_data;
    int i;

    host_data = g_malloc(host_size);

    for (i = 0; i < size; ++i) {
        host_data[i] = target_data[i];
    }

    if ((start == 0) && (size == buffer->size)) {
        buffer->driver->BufferData(target, host_size, host_data, buffer->usage);
    } else {
        buffer->driver->BufferSubData(target, start * sizeof(GLshort),
                host_size, host_data);
    }

    g_free(host_data);
}

static void yagl_gles_buffer_transfer_internal(struct yagl_gles_buffer *buffer,
                                               struct yagl_range_list *range_list,
                                               GLenum target,
                                               yagl_gles_buffer_transfer_func transfer_func)
{
    int num_ranges = yagl_range_list_size(range_list);
    int i, start, size;

    if (num_ranges <= 0) {
        return;
    }

    if (num_ranges == 1) {
        yagl_range_list_get(range_list,
                            0,
                            &start,
                            &size);
        if (size == 0) {
            /*
             * Buffer clear.
             */
            assert(start == 0);
            buffer->driver->BufferData(target, 0,
                                       NULL, buffer->usage);
            yagl_range_list_clear(range_list);
            return;
        } else if ((start == 0) && (size == buffer->size)) {
            /*
             * Buffer full update.
             */
            transfer_func(buffer, target, 0, size);
            yagl_range_list_clear(range_list);
            return;
        }
    }

    /*
     * Buffer partial updates.
     */

    for (i = 0; i < num_ranges; ++i) {
        yagl_range_list_get(range_list,
                            i,
                            &start,
                            &size);
        transfer_func(buffer, target, start, size);
    }
    yagl_range_list_clear(range_list);
}

static void yagl_gles_buffer_destroy(struct yagl_ref *ref)
{
    struct yagl_gles_buffer *buffer = (struct yagl_gles_buffer*)ref;

    yagl_ensure_ctx();
    buffer->driver->DeleteBuffers(1, &buffer->default_part.global_name);
    buffer->driver->DeleteBuffers(1, &buffer->fixed_part.global_name);
    buffer->driver->DeleteBuffers(1, &buffer->byte_part.global_name);
    yagl_unensure_ctx();

    yagl_range_list_cleanup(&buffer->default_part.range_list);
    yagl_range_list_cleanup(&buffer->fixed_part.range_list);
    yagl_range_list_cleanup(&buffer->byte_part.range_list);

    g_free(buffer->data);

    yagl_object_cleanup(&buffer->base);

    g_free(buffer);
}

struct yagl_gles_buffer
    *yagl_gles_buffer_create(struct yagl_gles_driver *driver)
{
    struct yagl_gles_buffer *buffer;

    buffer = g_malloc0(sizeof(*buffer));

    yagl_object_init(&buffer->base, &yagl_gles_buffer_destroy);

    buffer->driver = driver;

    driver->GenBuffers(1, &buffer->default_part.global_name);
    yagl_range_list_init(&buffer->default_part.range_list);

    driver->GenBuffers(1, &buffer->fixed_part.global_name);
    yagl_range_list_init(&buffer->fixed_part.range_list);

    driver->GenBuffers(1, &buffer->byte_part.global_name);
    yagl_range_list_init(&buffer->byte_part.range_list);

    return buffer;
}

void yagl_gles_buffer_acquire(struct yagl_gles_buffer *buffer)
{
    if (buffer) {
        yagl_object_acquire(&buffer->base);
    }
}

void yagl_gles_buffer_release(struct yagl_gles_buffer *buffer)
{
    if (buffer) {
        yagl_object_release(&buffer->base);
    }
}

void yagl_gles_buffer_set_data(struct yagl_gles_buffer *buffer,
                               GLint size,
                               const void *data,
                               GLenum usage)
{
    if (size > 0) {
        if (size > buffer->size) {
            g_free(buffer->data);
            buffer->data = g_malloc(size);
        }
        buffer->size = size;
        if (data) {
            memcpy(buffer->data, data, buffer->size);
        }
    } else {
        g_free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0;
    }

    buffer->usage = usage;

    yagl_range_list_clear(&buffer->default_part.range_list);
    yagl_range_list_clear(&buffer->fixed_part.range_list);
    yagl_range_list_clear(&buffer->byte_part.range_list);

    yagl_range_list_add(&buffer->default_part.range_list, 0, buffer->size);
    yagl_range_list_add(&buffer->fixed_part.range_list, 0, buffer->size);
    yagl_range_list_add(&buffer->byte_part.range_list, 0, buffer->size);

    buffer->cached_minmax_idx = false;
}

bool yagl_gles_buffer_update_data(struct yagl_gles_buffer *buffer,
                                  GLint offset,
                                  GLint size,
                                  const void *data)
{
    if ((offset < 0) || (size < 0) || ((offset + size) > buffer->size)) {
        return false;
    }

    if (size == 0) {
        return true;
    }

    memcpy(buffer->data + offset, data, size);

    yagl_range_list_add(&buffer->default_part.range_list, offset, size);
    yagl_range_list_add(&buffer->fixed_part.range_list, offset, size);
    yagl_range_list_add(&buffer->byte_part.range_list, offset, size);

    buffer->cached_minmax_idx = false;

    return true;
}

bool yagl_gles_buffer_get_minmax_index(struct yagl_gles_buffer *buffer,
                                       GLenum type,
                                       GLint offset,
                                       GLint count,
                                       uint32_t *min_idx,
                                       uint32_t *max_idx)
{
    int index_size, i;
    char tmp[4];

    *min_idx = UINT32_MAX;
    *max_idx = 0;

    if (!yagl_gles_get_index_size(type, &index_size)) {
        return false;
    }

    if ((offset < 0) || (count <= 0) || ((offset + (count * index_size)) > buffer->size)) {
        return false;
    }

    if (buffer->cached_minmax_idx &&
        (buffer->cached_type == type) &&
        (buffer->cached_offset == offset) &&
        (buffer->cached_count == count)) {
        *min_idx = buffer->cached_min_idx;
        *max_idx = buffer->cached_max_idx;
        return true;
    }

    for (i = 0; i < count; ++i) {
        /*
         * We don't respect target endian here, but we don't care for now,
         * we're on little endian anyway.
         */
        memcpy(&tmp[0], buffer->data + offset + (i * index_size), index_size);
        uint32_t idx = 0;
        switch (type) {
        case GL_UNSIGNED_BYTE:
            idx = *(uint8_t*)&tmp[0];
            break;
        case GL_UNSIGNED_SHORT:
            idx = *(uint16_t*)&tmp[0];
            break;
        default:
            assert(0);
            break;
        }
        if (idx < *min_idx) {
            *min_idx = idx;
        }
        if (idx > *max_idx) {
            *max_idx = idx;
        }
    }

    buffer->cached_minmax_idx = true;
    buffer->cached_type = type;
    buffer->cached_offset = offset;
    buffer->cached_count = count;
    buffer->cached_min_idx = *min_idx;
    buffer->cached_max_idx = *max_idx;

    return true;
}

bool yagl_gles_buffer_bind(struct yagl_gles_buffer *buffer,
                           GLenum type,
                           bool need_convert,
                           GLenum target,
                           yagl_object_name *old_buffer_name)
{
    GLenum binding;
    yagl_object_name tmp;
    struct yagl_gles_buffer_part *bufpart = &buffer->default_part;

    if (!yagl_gles_buffer_target_to_binding(target, &binding)) {
        return false;
    }

    buffer->driver->GetIntegerv(binding, (GLint*)&tmp);

    if (old_buffer_name) {
        *old_buffer_name = tmp;
    }

    if (need_convert) {
        switch (type) {
        case GL_BYTE:
            bufpart = &buffer->byte_part;
            break;
        case GL_FIXED:
            bufpart = &buffer->fixed_part;
            break;
        }
    }

    if (tmp != bufpart->global_name) {
        buffer->driver->BindBuffer(target, bufpart->global_name);
    }

    return true;
}

bool yagl_gles_buffer_transfer(struct yagl_gles_buffer *buffer,
                               GLenum type,
                               GLenum target,
                               bool need_convert)
{
    yagl_object_name old_buffer_name = 0;

    if (!yagl_gles_buffer_bind(buffer, type, need_convert, target, &old_buffer_name)) {
        return false;
    }

    if (need_convert) {
        switch (type) {
        case GL_BYTE:
            yagl_gles_buffer_transfer_internal(buffer,
                                               &buffer->byte_part.range_list,
                                               target,
                                               &yagl_gles_buffer_transfer_byte);
            break;
        case GL_FIXED:
            yagl_gles_buffer_transfer_internal(buffer,
                                               &buffer->fixed_part.range_list,
                                               target,
                                               &yagl_gles_buffer_transfer_fixed);
            break;
        }
    } else {
        yagl_gles_buffer_transfer_internal(buffer,
                                           &buffer->default_part.range_list,
                                           target,
                                           &yagl_gles_buffer_transfer_default);
    }

    buffer->driver->BindBuffer(target, old_buffer_name);

    return true;
}

bool yagl_gles_buffer_get_parameter(struct yagl_gles_buffer *buffer,
                                    GLenum pname,
                                    GLint *param)
{
    switch (pname) {
    case GL_BUFFER_SIZE:
        *param = buffer->size;
        break;
    case GL_BUFFER_USAGE:
        *param = buffer->usage;
        break;
    default:
        return false;
    }

    return true;
}

void yagl_gles_buffer_set_bound(struct yagl_gles_buffer *buffer)
{
    buffer->was_bound = true;
}

bool yagl_gles_buffer_was_bound(struct yagl_gles_buffer *buffer)
{
    return buffer->was_bound;
}
