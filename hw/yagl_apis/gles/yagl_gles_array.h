#ifndef _QEMU_YAGL_GLES_ARRAY_H
#define _QEMU_YAGL_GLES_ARRAY_H

#include "yagl_types.h"

/*
 * This structure represents GLES array, such arrays are
 * typically set up by calls to 'glColorPointer', 'glVertexPointer',
 * 'glVertexAttribPointer', etc.
 *
 * Note that we can't call host 'glVertexAttribPointer' right away
 * when target 'glVertexAttribPointer' call is made. We can't just
 * transfer the data from target to host and feed it to
 * host 'glVertexAttribPointer' since the data that pointer points to
 * may be changed later, thus, we must do the transfer right before host
 * OpenGL will attempt to use it. Host OpenGL will attempt to use
 * the array pointer on 'glDrawArrays' and 'glDrawElements', thus, we must
 * do the transfer right before those calls. For this to work, we'll
 * store 'apply' function together with an array, this function will be
 * responsible for calling 'glVertexAttribPointer', 'glColorPointer'
 * or whatever.
 */

struct yagl_gles_context;
struct yagl_gles_array;
struct yagl_gles_buffer;

/*
 * Calls the corresponding host array function.
 */
typedef void (*yagl_gles_array_apply_func)(struct yagl_gles_array */*array*/);

struct yagl_gles_array
{
    /*
     * Owning context.
     */
    struct yagl_gles_context *ctx;

    GLuint index;
    GLint size;
    GLenum type;
    int el_size;
    GLboolean normalized;
    GLsizei stride;

    /*
     * Is array enabled by 'glEnableClientState'/'glEnableVertexAttribArray'.
     */
    bool enabled;

    struct yagl_gles_buffer *vbo;
    yagl_object_name vbo_local_name;

    union
    {
        /*
         * Array data.
         */
        struct
        {
            target_ulong target_data;

            void *host_data;
            uint32_t host_data_size;
        };
        /*
         * VBO data.
         */
        struct
        {
            GLint offset;
        };
    };

    yagl_gles_array_apply_func apply;
};

void yagl_gles_array_init(struct yagl_gles_array *array,
                          GLuint index,
                          struct yagl_gles_context *ctx,
                          yagl_gles_array_apply_func apply);

void yagl_gles_array_cleanup(struct yagl_gles_array *array);

void yagl_gles_array_enable(struct yagl_gles_array *array, bool enable);

bool yagl_gles_array_update(struct yagl_gles_array *array,
                            GLint size,
                            GLenum type,
                            GLboolean normalized,
                            GLsizei stride,
                            target_ulong target_data);

bool yagl_gles_array_update_vbo(struct yagl_gles_array *array,
                                GLint size,
                                GLenum type,
                                GLboolean normalized,
                                GLsizei stride,
                                struct yagl_gles_buffer *vbo,
                                yagl_object_name vbo_local_name,
                                GLint offset);

/*
 * Transfers and applies 'count' array elements starting from 'first' element.
 * Transferred data is kept in 'host_data'.
 * On error returns false.
 *
 * Note that the current thread MUST be in sync with target while making this
 * call, since this call might transfer data from target to host.
 */
bool yagl_gles_array_transfer(struct yagl_gles_array *array,
                              uint32_t first,
                              uint32_t count);

#endif
