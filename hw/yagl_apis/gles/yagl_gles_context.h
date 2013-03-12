#ifndef _QEMU_YAGL_GLES_CONTEXT_H
#define _QEMU_YAGL_GLES_CONTEXT_H

#include "yagl_gles_types.h"
#include "yagl_client_context.h"

struct yagl_gles_driver_ps;
struct yagl_gles_array;
struct yagl_gles_buffer;
struct yagl_gles_framebuffer;
struct yagl_gles_texture_unit;

struct yagl_gles_context
{
    struct yagl_client_context base;

    struct yagl_gles_driver_ps *driver_ps;

    bool (*get_param_count)(struct yagl_gles_context */*ctx*/,
                            GLenum /*pname*/,
                            int */*count*/);

    bool (*get_booleanv)(struct yagl_gles_context */*ctx*/,
                         GLenum /*pname*/,
                         GLboolean */*params*/);

    bool (*get_integerv)(struct yagl_gles_context */*ctx*/,
                         GLenum /*pname*/,
                         GLint */*params*/);

    bool (*get_floatv)(struct yagl_gles_context */*ctx*/,
                       GLenum /*pname*/,
                       GLfloat */*params*/);

    GLchar *(*get_extensions)(struct yagl_gles_context */*ctx*/);

    void (*pre_draw)(struct yagl_gles_context */*ctx*/, GLenum mode);

    void (*post_draw)(struct yagl_gles_context */*ctx*/, GLenum mode);

    /*
     * Pixel Buffer Object (PBO) for quick access to current surface pixels.
     * 'read_pixels' will automatically detect surface size changes and
     * will recreate this pbo accordingly.
     */
    struct yagl_gles_buffer *rp_pbo;
    uint32_t rp_pbo_width;
    uint32_t rp_pbo_height;
    uint32_t rp_pbo_bpp;

    GLenum error;

    /*
     * Buffer that is used for reading target data that is
     * accessed via target pointers. Instead of doing
     * g_malloc/g_free every time we'll just use this.
     */
    void *malloc_buff;
    GLsizei malloc_buff_size;

    /*
     * GLES arrays, the number of arrays is different depending on
     * GLES version, 'num_arrays' holds that number.
     */
    struct yagl_gles_array *arrays;
    int num_arrays;

    /*
     * GLES texture units, the number of texture units is determined
     * at runtime.
     */
    struct yagl_gles_texture_unit *texture_units;
    int num_texture_units;

    int num_compressed_texture_formats;

    bool pack_depth_stencil;

    bool texture_npot;

    bool texture_rectangle;

    bool texture_filter_anisotropic;

    int active_texture_unit;

    struct yagl_gles_buffer *vbo;
    yagl_object_name vbo_local_name;

    struct yagl_gles_buffer *ebo;
    yagl_object_name ebo_local_name;

    struct yagl_gles_framebuffer *fbo;
    yagl_object_name fbo_local_name;

    yagl_object_name rbo_local_name;

    /*
     * Things below are changed on 'activate'/'deactivate'.
     * @{
     */

    /*
     * The thread this context is currently running on.
     */
    struct yagl_thread_state *ts;

    /*
     * @}
     */
};

void yagl_gles_context_init(struct yagl_gles_context *ctx,
                            struct yagl_gles_driver_ps *driver_ps);

/*
 * Called when the context is being activated for the first time.
 * Called before 'yagl_gles_context_activate'.
 *
 * Takes ownership of 'arrays'.
 */
void yagl_gles_context_prepare(struct yagl_gles_context *ctx,
                               struct yagl_thread_state *ts,
                               struct yagl_gles_array *arrays,
                               int num_arrays,
                               int num_texture_units);

void yagl_gles_context_activate(struct yagl_gles_context *ctx,
                                struct yagl_thread_state *ts);

void yagl_gles_context_deactivate(struct yagl_gles_context *ctx);

void yagl_gles_context_cleanup(struct yagl_gles_context *ctx);

void yagl_gles_context_set_error(struct yagl_gles_context *ctx, GLenum error);

GLenum yagl_gles_context_get_error(struct yagl_gles_context *ctx);

/*
 * Allocate per-call memory, no need to free it.
 */
void *yagl_gles_context_malloc(struct yagl_gles_context *ctx, GLsizei size);
void *yagl_gles_context_malloc0(struct yagl_gles_context *ctx, GLsizei size);

struct yagl_gles_array
    *yagl_gles_context_get_array(struct yagl_gles_context *ctx, GLuint index);

/*
 * For all arrays: transfers and applies 'count' array elements starting
 * from 'first' element.
 * On error returns false, some arrays might not get
 * transferred on error.
 *
 * Note that the current thread MUST be in sync with target while making this
 * call, since this call might transfer data from target to host.
 */
bool yagl_gles_context_transfer_arrays(struct yagl_gles_context *ctx,
                                       uint32_t first,
                                       uint32_t count);

bool yagl_gles_context_set_active_texture(struct yagl_gles_context *ctx,
                                          GLenum texture);

struct yagl_gles_texture_unit
    *yagl_gles_context_get_active_texture_unit(struct yagl_gles_context *ctx);

struct yagl_gles_texture_target_state
    *yagl_gles_context_get_active_texture_target_state(struct yagl_gles_context *ctx,
                                                       yagl_gles_texture_target texture_target);

void yagl_gles_context_bind_texture(struct yagl_gles_context *ctx,
                                    yagl_gles_texture_target texture_target,
                                    yagl_object_name texture_local_name);

void yagl_gles_context_unbind_texture(struct yagl_gles_context *ctx,
                                      yagl_object_name texture_local_name);

bool yagl_gles_context_bind_buffer(struct yagl_gles_context *ctx,
                                   GLenum target,
                                   struct yagl_gles_buffer *buffer,
                                   yagl_object_name buffer_local_name);

void yagl_gles_context_unbind_buffer(struct yagl_gles_context *ctx,
                                     struct yagl_gles_buffer *buffer);

bool yagl_gles_context_bind_framebuffer(struct yagl_gles_context *ctx,
                                        GLenum target,
                                        struct yagl_gles_framebuffer *fbo,
                                        yagl_object_name fbo_local_name);

void yagl_gles_context_unbind_framebuffer(struct yagl_gles_context *ctx,
                                          yagl_object_name fbo_local_name);

bool yagl_gles_context_bind_renderbuffer(struct yagl_gles_context *ctx,
                                         GLenum target,
                                         yagl_object_name rbo_local_name);

void yagl_gles_context_unbind_renderbuffer(struct yagl_gles_context *ctx,
                                           yagl_object_name rbo_local_name);

struct yagl_gles_buffer
    *yagl_gles_context_acquire_binded_buffer(struct yagl_gles_context *ctx,
                                             GLenum target);

struct yagl_gles_framebuffer
    *yagl_gles_context_acquire_binded_framebuffer(struct yagl_gles_context *ctx,
                                                  GLenum target);

#endif
