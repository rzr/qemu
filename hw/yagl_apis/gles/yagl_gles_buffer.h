#ifndef _QEMU_YAGL_GLES_BUFFER_H
#define _QEMU_YAGL_GLES_BUFFER_H

#include "yagl_types.h"
#include "yagl_object.h"
#include "yagl_range_list.h"

 /*
  * VBO implementation is somewhat tricky because
  * we must correctly handle GL_FIXED data type, which must be
  * converted to GL_FLOAT, and GL_BYTE data type, which (in some cases)
  * must be converted to GL_SHORT. Buffer objects may be bound to
  * multiple contexts, so there may be several VBOs using the
  * same buffer object, but having different data types. We handle this
  * by having three 'yagl_gles_buffer_part' objects: one for GL_FIXED type,
  * one for GL_BYTE type and the other for all other types.
  * Each 'yagl_gles_buffer_part' object has a separate global buffer name
  * and a range list that consists of 'glBufferData' and 'glBufferSubData'
  * ranges which haven't been processed yet. When a VBO is used we walk
  * the appropriate range list, make conversions, clear it and call
  * 'glBufferData' or 'glBufferSubData' as many times as needed.
  */

#define YAGL_NS_BUFFER 0

struct yagl_gles_driver;

struct yagl_gles_buffer_part
{
    yagl_object_name global_name;

    struct yagl_range_list range_list;
};

struct yagl_gles_buffer
{
    struct yagl_object base;

    struct yagl_gles_driver *driver;

    struct yagl_gles_buffer_part default_part;
    struct yagl_gles_buffer_part fixed_part;
    struct yagl_gles_buffer_part byte_part;

    GLint size;
    void *data;
    GLenum usage;

    bool was_bound;

    bool cached_minmax_idx;
    GLenum cached_type;
    GLint cached_offset;
    GLint cached_count;
    uint32_t cached_min_idx;
    uint32_t cached_max_idx;
};

struct yagl_gles_buffer
    *yagl_gles_buffer_create(struct yagl_gles_driver *driver);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_buffer_acquire(struct yagl_gles_buffer *buffer);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles_buffer_release(struct yagl_gles_buffer *buffer);

void yagl_gles_buffer_set_data(struct yagl_gles_buffer *buffer,
                               GLint size,
                               const void *data,
                               GLenum usage);

bool yagl_gles_buffer_update_data(struct yagl_gles_buffer *buffer,
                                  GLint offset,
                                  GLint size,
                                  const void *data);

bool yagl_gles_buffer_get_minmax_index(struct yagl_gles_buffer *buffer,
                                       GLenum type,
                                       GLint offset,
                                       GLint count,
                                       uint32_t *min_idx,
                                       uint32_t *max_idx);

bool yagl_gles_buffer_bind(struct yagl_gles_buffer *buffer,
                           GLenum type,
                           bool need_convert,
                           GLenum target,
                           yagl_object_name *old_buffer_name);

bool yagl_gles_buffer_transfer(struct yagl_gles_buffer *buffer,
                               GLenum type,
                               GLenum target,
                               bool need_convert);

bool yagl_gles_buffer_get_parameter(struct yagl_gles_buffer *buffer,
                                    GLenum pname,
                                    GLint *param);

void yagl_gles_buffer_set_bound(struct yagl_gles_buffer *buffer);

bool yagl_gles_buffer_was_bound(struct yagl_gles_buffer *buffer);

#endif
