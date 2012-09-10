#include <GL/gl.h>
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
        buffer->driver_ps->BufferData(buffer->driver_ps, target, size,
                                      buffer->data, buffer->usage);
    } else {
        buffer->driver_ps->BufferSubData(buffer->driver_ps, target, start,
                                         size, buffer->data + start);
    }
}

static void yagl_gles_buffer_transfer_fixed(struct yagl_gles_buffer *buffer,
                                            GLenum target,
                                            int start,
                                            int size)
{
    /*
     * TODO: Convert to GL_FLOAT
     */

    if ((start == 0) && (size == buffer->size)) {
        buffer->driver_ps->BufferData(buffer->driver_ps, target, size,
                                      buffer->data, buffer->usage);
    } else {
        buffer->driver_ps->BufferSubData(buffer->driver_ps, target, start,
                                         size, buffer->data + start);
    }
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
            buffer->driver_ps->BufferData(buffer->driver_ps, target, 0,
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

    if (!buffer->base.nodelete) {
        buffer->driver_ps->DeleteBuffers(buffer->driver_ps, 1, &buffer->default_part.global_name);
        buffer->driver_ps->DeleteBuffers(buffer->driver_ps, 1, &buffer->fixed_part.global_name);
    }

    yagl_range_list_cleanup(&buffer->default_part.range_list);
    yagl_range_list_cleanup(&buffer->fixed_part.range_list);

    qemu_mutex_destroy(&buffer->mutex);

    g_free(buffer->data);

    yagl_object_cleanup(&buffer->base);

    g_free(buffer);
}

struct yagl_gles_buffer
    *yagl_gles_buffer_create(struct yagl_gles_driver_ps *driver_ps)
{
    struct yagl_gles_buffer *buffer;

    buffer = g_malloc0(sizeof(*buffer));

    yagl_object_init(&buffer->base, &yagl_gles_buffer_destroy);

    buffer->driver_ps = driver_ps;

    qemu_mutex_init(&buffer->mutex);

    driver_ps->GenBuffers(driver_ps, 1, &buffer->default_part.global_name);
    yagl_range_list_init(&buffer->default_part.range_list);

    driver_ps->GenBuffers(driver_ps, 1, &buffer->fixed_part.global_name);
    yagl_range_list_init(&buffer->fixed_part.range_list);

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
                               void *data,
                               GLenum usage)
{
    qemu_mutex_lock(&buffer->mutex);

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

    yagl_range_list_add(&buffer->default_part.range_list, 0, buffer->size);
    yagl_range_list_add(&buffer->fixed_part.range_list, 0, buffer->size);

    qemu_mutex_unlock(&buffer->mutex);
}

bool yagl_gles_buffer_update_data(struct yagl_gles_buffer *buffer,
                                  GLint offset,
                                  GLint size,
                                  void *data)
{
    qemu_mutex_lock(&buffer->mutex);

    if ((offset < 0) || (size < 0) || ((offset + size) > buffer->size)) {
        qemu_mutex_unlock(&buffer->mutex);
        return false;
    }

    if (size == 0) {
        qemu_mutex_unlock(&buffer->mutex);
        return true;
    }

    memcpy(buffer->data + offset, data, size);

    yagl_range_list_add(&buffer->default_part.range_list, offset, size);
    yagl_range_list_add(&buffer->fixed_part.range_list, offset, size);

    qemu_mutex_unlock(&buffer->mutex);

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

    qemu_mutex_lock(&buffer->mutex);

    if ((offset < 0) || (count <= 0) || ((offset + (count * index_size)) > buffer->size)) {
        qemu_mutex_unlock(&buffer->mutex);
        return false;
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

    qemu_mutex_unlock(&buffer->mutex);

    return true;
}

bool yagl_gles_buffer_bind(struct yagl_gles_buffer *buffer,
                           GLenum type,
                           GLenum target,
                           yagl_object_name *old_buffer_name)
{
    GLenum binding;
    yagl_object_name tmp;

    if (!yagl_gles_buffer_target_to_binding(target, &binding)) {
        return false;
    }

    buffer->driver_ps->GetIntegerv(buffer->driver_ps,
                                   binding,
                                   (GLint*)&tmp);

    if (old_buffer_name) {
        *old_buffer_name = tmp;
    }

    if (type == GL_FIXED) {
        if (tmp != buffer->fixed_part.global_name) {
            buffer->driver_ps->BindBuffer(buffer->driver_ps,
                                          target,
                                          buffer->fixed_part.global_name);
        }
    } else {
        if (tmp != buffer->default_part.global_name) {
            buffer->driver_ps->BindBuffer(buffer->driver_ps,
                                          target,
                                          buffer->default_part.global_name);
        }
    }

    return true;
}

bool yagl_gles_buffer_transfer(struct yagl_gles_buffer *buffer,
                               GLenum type,
                               GLenum target)
{
    yagl_object_name old_buffer_name = 0;

    if (!yagl_gles_buffer_bind(buffer, type, target, &old_buffer_name)) {
        return false;
    }

    qemu_mutex_lock(&buffer->mutex);

    if (type == GL_FIXED) {
        yagl_gles_buffer_transfer_internal(buffer,
                                           &buffer->fixed_part.range_list,
                                           target,
                                           &yagl_gles_buffer_transfer_fixed);
    } else {
        yagl_gles_buffer_transfer_internal(buffer,
                                           &buffer->default_part.range_list,
                                           target,
                                           &yagl_gles_buffer_transfer_default);
    }

    qemu_mutex_unlock(&buffer->mutex);

    buffer->driver_ps->BindBuffer(buffer->driver_ps, target, old_buffer_name);

    return true;
}
