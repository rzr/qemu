#include "vigs_gl_backend.h"
#include "vigs_surface.h"
#include "vigs_id_gen.h"
#include "vigs_log.h"
#include "vigs_utils.h"
#include "vigs_ref.h"
#include "winsys_gl.h"

struct vigs_gl_surface;

struct vigs_winsys_gl_surface
{
    struct winsys_gl_surface base;

    struct vigs_ref ref;

    struct vigs_gl_backend *backend;

    /*
     * Will be set to NULL when orphaned.
     */
    struct vigs_gl_surface *parent;

    /*
     * GL texture format.
     * @{
     */

    GLint tex_internalformat;
    GLenum tex_format;
    GLenum tex_type;
    GLint tex_bpp;

    /*
     * @}
     */

    /*
     * Texture that represent this surface's front buffer.
     * Used as color attachment in 'fb'.
     *
     * Allocated on first access.
     */
    GLuint front_tex;

    /*
     * Texture that represents this surface's back buffer, only used by
     * winsys_gl clients. Switched with 'front_tex' on 'swap_buffers'.
     *
     * Allocated on first access.
     */
    GLuint back_tex;
};

struct vigs_gl_surface
{
    struct vigs_surface base;

    /*
     * Framebuffer that is used for rendering
     * into front buffer.
     *
     * Allocated on first access.
     *
     * TODO: Make a global framebuffer pool and use framebuffers from
     * it. Framebuffer pool must contain one framebuffer per distinct
     * surface format.
     */
    GLuint fb;

    /*
     * Texture for temporary storage.
     *
     * Allocated on first access.
     */
    GLuint tmp_tex;

    /*
     * Fence that gets inserted into GL stream
     * that comes from windowing system.
     *
     * 0 if no GL commands are pending.
     */
    GLsync fence_2d;

    /*
     * Fence that gets inserted into GL stream
     * that comes from 3D renderer.
     *
     * 0 if no GL commands are pending.
     */
    GLsync fence_3d;
};

static __inline struct vigs_winsys_gl_surface
    *get_ws_sfc(struct vigs_gl_surface *sfc)
{
    return (struct vigs_winsys_gl_surface*)sfc->base.ws_sfc;
}

/*
 * PRIVATE.
 * @{
 */

static void vigs_gl_translate_color(vigsp_color color,
                                    vigsp_surface_format format,
                                    GLubyte *red,
                                    GLubyte *green,
                                    GLubyte *blue,
                                    GLubyte *alpha)
{
    *alpha = 0xFF;
    switch (format) {
    case vigsp_surface_bgra8888:
        *alpha = (GLubyte)((color >> 24) & 0xFF);
        /* Fall through. */
    case vigsp_surface_bgrx8888:
        *red = (GLubyte)((color >> 16) & 0xFF);
        *green = (GLubyte)((color >> 8) & 0xFF);
        *blue = (GLubyte)(color & 0xFF);
        break;
    default:
        assert(false);
        *red = 0;
        *green = 0;
        *blue = 0;
    }
}

static bool vigs_winsys_gl_surface_create_texture(struct vigs_winsys_gl_surface *ws_sfc,
                                                  GLuint *tex)
{
    GLuint cur_tex = 0;

    if (*tex) {
        return true;
    }

    ws_sfc->backend->GenTextures(1, tex);

    if (!*tex) {
        goto fail;
    }

    ws_sfc->backend->GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&cur_tex);
    ws_sfc->backend->BindTexture(GL_TEXTURE_2D, *tex);

    /*
     * Workaround for problem in "Mesa DRI Intel(R) Ivybridge Desktop x86/MMX/SSE2, version 9.0.3":
     * These lines used to be in 'vigs_gl_backend_init', but it turned out that they must
     * be called after 'glBindTexture'.
     */
    ws_sfc->backend->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ws_sfc->backend->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    ws_sfc->backend->TexImage2D(GL_TEXTURE_2D, 0, ws_sfc->tex_internalformat,
                                ws_sfc->base.base.width, ws_sfc->base.base.height, 0,
                                ws_sfc->tex_format, ws_sfc->tex_type,
                                NULL);
    ws_sfc->backend->BindTexture(GL_TEXTURE_2D, cur_tex);

    return true;

fail:
    return false;
}

static bool vigs_gl_surface_create_framebuffer(struct vigs_gl_surface *gl_sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    if (gl_sfc->fb) {
        return true;
    }

    gl_backend->GenFramebuffers(1, &gl_sfc->fb);

    if (!gl_sfc->fb) {
        goto fail;
    }

    return true;

fail:
    return false;
}

static void vigs_gl_surface_setup_framebuffer(struct vigs_gl_surface *gl_sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    gl_backend->Viewport(0, 0, gl_sfc->base.width, gl_sfc->base.height);
    gl_backend->MatrixMode(GL_PROJECTION);
    gl_backend->LoadIdentity();
    gl_backend->Ortho(0.0, gl_sfc->base.width, 0.0, gl_sfc->base.height, -1.0, 1.0);
    gl_backend->MatrixMode(GL_MODELVIEW);
    gl_backend->LoadIdentity();
    gl_backend->Disable(GL_DEPTH_TEST);
    gl_backend->Disable(GL_BLEND);
}

static void vigs_gl_surface_set_fence_2d(struct vigs_gl_surface *gl_sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    if (!gl_backend->has_arb_sync) {
        gl_backend->Finish();
        return;
    }

    if (gl_sfc->fence_2d) {
        gl_backend->DeleteSync(gl_sfc->fence_2d);
    }

    gl_sfc->fence_2d = gl_backend->FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    gl_backend->Flush();
}

static void vigs_gl_surface_wait_fence_2d(struct vigs_gl_surface *gl_sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    if (!gl_backend->has_arb_sync) {
        return;
    }

    if (gl_sfc->fence_2d) {
        /*
         * Using glClientWaitSync instead of glWaitSync because of
         * nVidia bug on windows.
         */
        gl_backend->ClientWaitSync(gl_sfc->fence_2d, 0, GL_TIMEOUT_IGNORED);
        gl_backend->DeleteSync(gl_sfc->fence_2d);
        gl_sfc->fence_2d = 0;
    }
}

static void vigs_gl_surface_set_fence_3d(struct vigs_gl_surface *gl_sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    if (!gl_backend->has_arb_sync) {
        gl_backend->Finish();
        return;
    }

    if (gl_sfc->fence_3d) {
        gl_backend->DeleteSync(gl_sfc->fence_3d);
    }

    gl_sfc->fence_3d = gl_backend->FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    gl_backend->Flush();
}

static void vigs_gl_surface_wait_fence_3d(struct vigs_gl_surface *gl_sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    if (!gl_backend->has_arb_sync) {
        return;
    }

    if (gl_sfc->fence_3d) {
        /*
         * Using glClientWaitSync instead of glWaitSync because of
         * nVidia bug on windows.
         */
        gl_backend->ClientWaitSync(gl_sfc->fence_3d, 0, GL_TIMEOUT_IGNORED);
        gl_backend->DeleteSync(gl_sfc->fence_3d);
        gl_sfc->fence_3d = 0;
    }
}

/*
 * @}
 */

/*
 * vigs_winsys_gl_surface.
 * @{
 */

static void vigs_winsys_gl_surface_acquire(struct winsys_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    vigs_ref_acquire(&vigs_sfc->ref);
}

static void vigs_winsys_gl_surface_release(struct winsys_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    vigs_ref_release(&vigs_sfc->ref);
}

static GLuint vigs_winsys_gl_surface_get_front_texture(struct winsys_gl_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    bool has_current = vigs_sfc->backend->has_current(vigs_sfc->backend);

    if (!vigs_sfc->front_tex &&
        (has_current ||
        vigs_sfc->backend->make_current(vigs_sfc->backend, true))) {

        vigs_winsys_gl_surface_create_texture(vigs_sfc,
                                              &vigs_sfc->front_tex);

        if (!has_current) {
            vigs_sfc->backend->make_current(vigs_sfc->backend, false);
        }
    }

    return vigs_sfc->front_tex;
}

static GLuint vigs_winsys_gl_surface_get_back_texture(struct winsys_gl_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    bool has_current = vigs_sfc->backend->has_current(vigs_sfc->backend);

    if (!vigs_sfc->back_tex &&
        (has_current ||
        vigs_sfc->backend->make_current(vigs_sfc->backend, true))) {

        vigs_winsys_gl_surface_create_texture(vigs_sfc,
                                              &vigs_sfc->back_tex);

        if (!has_current) {
            vigs_sfc->backend->make_current(vigs_sfc->backend, false);
        }
    }

    return vigs_sfc->back_tex;
}

static void vigs_winsys_gl_surface_swap_buffers(struct winsys_gl_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    GLuint cur_fb = 0;

    if (!vigs_sfc->parent) {
        return;
    }

    if (!vigs_gl_surface_create_framebuffer(vigs_sfc->parent)) {
        return;
    }

    if (!vigs_winsys_gl_surface_create_texture(vigs_sfc, &vigs_sfc->front_tex)) {
        return;
    }

    vigs_gl_surface_wait_fence_2d(vigs_sfc->parent);

    vigs_sfc->backend->GetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&cur_fb);

    vigs_sfc->backend->BindFramebuffer(GL_READ_FRAMEBUFFER,
                                       cur_fb);
    vigs_sfc->backend->BindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                       vigs_sfc->parent->fb);
    vigs_sfc->backend->FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_TEXTURE_2D, vigs_sfc->front_tex, 0);

    vigs_sfc->backend->BlitFramebuffer(0, 0, sfc->base.width, sfc->base.height,
                                       0, 0, sfc->base.width, sfc->base.height,
                                       GL_COLOR_BUFFER_BIT,
                                       GL_LINEAR);

    vigs_sfc->backend->BindFramebuffer(GL_FRAMEBUFFER,
                                       cur_fb);

    vigs_gl_surface_set_fence_3d(vigs_sfc->parent);

    vigs_sfc->parent->base.is_dirty = true;
}

static void vigs_winsys_gl_surface_copy_buffers(uint32_t width,
                                                uint32_t height,
                                                struct winsys_gl_surface *target,
                                                bool is_loop)
{
    struct vigs_winsys_gl_surface *vigs_target = (struct vigs_winsys_gl_surface*)target;
    GLuint cur_fb = 0;

    if (!vigs_target->parent) {
        return;
    }

    vigs_gl_surface_wait_fence_2d(vigs_target->parent);

    if (is_loop) {
        /*
         * Feedback loop is possible, no-op.
         */
    } else {
        if (!vigs_gl_surface_create_framebuffer(vigs_target->parent)) {
            return;
        }

        if (!vigs_winsys_gl_surface_create_texture(vigs_target, &vigs_target->front_tex)) {
            return;
        }

        vigs_target->backend->GetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&cur_fb);

        vigs_target->backend->BindFramebuffer(GL_READ_FRAMEBUFFER,
                                              cur_fb);
        vigs_target->backend->BindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                              vigs_target->parent->fb);
        vigs_target->backend->FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                   GL_TEXTURE_2D, vigs_target->front_tex, 0);

        vigs_target->backend->BlitFramebuffer(0, 0, width, height,
                                              0, 0, width, height,
                                              GL_COLOR_BUFFER_BIT,
                                              GL_LINEAR);

        vigs_target->backend->BindFramebuffer(GL_FRAMEBUFFER,
                                              cur_fb);
    }

    vigs_gl_surface_set_fence_3d(vigs_target->parent);

    vigs_target->parent->base.is_dirty = true;
}

static void vigs_winsys_gl_surface_destroy(struct vigs_ref *ref)
{
    struct vigs_winsys_gl_surface *vigs_sfc =
        container_of(ref, struct vigs_winsys_gl_surface, ref);
    bool has_current = vigs_sfc->backend->has_current(vigs_sfc->backend);

    if (has_current ||
        vigs_sfc->backend->make_current(vigs_sfc->backend, true)) {
        if (vigs_sfc->front_tex) {
            vigs_sfc->backend->DeleteTextures(1, &vigs_sfc->front_tex);
        }
        if (vigs_sfc->back_tex) {
            vigs_sfc->backend->DeleteTextures(1, &vigs_sfc->back_tex);
        }

        if (!has_current) {
            vigs_sfc->backend->make_current(vigs_sfc->backend, false);
        }
    }

    vigs_ref_cleanup(&vigs_sfc->ref);

    g_free(vigs_sfc);
}

static struct vigs_winsys_gl_surface
    *vigs_winsys_gl_surface_create(struct vigs_gl_backend *backend,
                                   struct vigs_gl_surface *parent,
                                   uint32_t width,
                                   uint32_t height,
                                   GLint tex_internalformat,
                                   GLenum tex_format,
                                   GLenum tex_type,
                                   GLint tex_bpp)
{
    struct vigs_winsys_gl_surface *ws_sfc;

    ws_sfc = g_malloc0(sizeof(*ws_sfc));

    ws_sfc->base.base.width = width;
    ws_sfc->base.base.height = height;
    ws_sfc->base.base.acquire = &vigs_winsys_gl_surface_acquire;
    ws_sfc->base.base.release = &vigs_winsys_gl_surface_release;
    ws_sfc->base.get_front_texture = &vigs_winsys_gl_surface_get_front_texture;
    ws_sfc->base.get_back_texture = &vigs_winsys_gl_surface_get_back_texture;
    ws_sfc->base.swap_buffers = &vigs_winsys_gl_surface_swap_buffers;
    ws_sfc->base.copy_buffers = &vigs_winsys_gl_surface_copy_buffers;
    ws_sfc->tex_internalformat = tex_internalformat;
    ws_sfc->tex_format = tex_format;
    ws_sfc->tex_type = tex_type;
    ws_sfc->tex_bpp = tex_bpp;
    ws_sfc->backend = backend;
    ws_sfc->parent = parent;

    vigs_ref_init(&ws_sfc->ref, &vigs_winsys_gl_surface_destroy);

    return ws_sfc;
}

static void vigs_winsys_gl_surface_orphan(struct vigs_winsys_gl_surface *sfc)
{
    sfc->parent = NULL;
}

/*
 * @}
 */

/*
 * vigs_gl_surface.
 * @{
 */

static void vigs_gl_surface_update(struct vigs_surface *sfc,
                                   vigsp_offset vram_offset,
                                   uint8_t *data)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);

    assert(data != NULL);

    if (!gl_backend->make_current(gl_backend, true)) {
        return;
    }

    sfc->vram_offset = vram_offset;
    sfc->data = data;

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->front_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    vigs_gl_surface_wait_fence_3d(gl_sfc);

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_sfc);

    gl_backend->Disable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->front_tex, 0);

    gl_backend->PixelZoom(1.0f, -1.0f);
    gl_backend->RasterPos2f(0.0f, gl_sfc->base.height);
    gl_backend->DrawPixels(gl_sfc->base.width,
                           gl_sfc->base.height,
                           ws_sfc->tex_format,
                           ws_sfc->tex_type,
                           data);

    vigs_gl_surface_set_fence_2d(gl_sfc);

    sfc->is_dirty = false;

    VIGS_LOG_TRACE("updated");

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_backend->make_current(gl_backend, false);
}

static void vigs_gl_surface_set_data(struct vigs_surface *sfc,
                                     vigsp_offset vram_offset,
                                     uint8_t *data)
{
    sfc->vram_offset = vram_offset;
    sfc->data = data;
}

static void vigs_gl_surface_read_pixels(struct vigs_surface *sfc,
                                        uint32_t x,
                                        uint32_t y,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t stride,
                                        uint8_t *pixels)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);
    GLfloat sfc_w, sfc_h;
    GLfloat *vert_coords;
    GLfloat *tex_coords;

    VIGS_LOG_TRACE("x = %u, y = %u, width = %u, height = %u",
                   x, y, width, height);

    if ((width * ws_sfc->tex_bpp) != stride) {
        VIGS_LOG_ERROR("Custom strides not supported yet");
        return;
    }

    if (!gl_backend->make_current(gl_backend, true)) {
        return;
    }

    if (!ws_sfc->front_tex) {
        VIGS_LOG_WARN("drawing garbage ???");
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->front_tex)) {
        goto out;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &gl_sfc->tmp_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    vigs_gl_surface_wait_fence_3d(gl_sfc);

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    sfc_w = gl_sfc->base.width;
    sfc_h = gl_sfc->base.height;

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_sfc);

    gl_backend->Enable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, gl_sfc->tmp_tex, 0);

    vert_coords = vigs_vector_append(&gl_backend->v1,
                                     (8 * sizeof(GLfloat)));
    tex_coords = vigs_vector_append(&gl_backend->v2,
                                    (8 * sizeof(GLfloat)));

    vert_coords[0] = 0;
    vert_coords[1] = sfc_h;
    vert_coords[2] = sfc_w;
    vert_coords[3] = sfc_h;
    vert_coords[4] = sfc_w;
    vert_coords[5] = 0;
    vert_coords[6] = 0;
    vert_coords[7] = 0;

    tex_coords[0] = 0;
    tex_coords[1] = 0;
    tex_coords[2] = 1;
    tex_coords[3] = 0;
    tex_coords[4] = 1;
    tex_coords[5] = 1;
    tex_coords[6] = 0;
    tex_coords[7] = 1;

    gl_backend->EnableClientState(GL_VERTEX_ARRAY);
    gl_backend->EnableClientState(GL_TEXTURE_COORD_ARRAY);

    gl_backend->BindTexture(GL_TEXTURE_2D, ws_sfc->front_tex);

    gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

    gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));
    gl_backend->TexCoordPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v2));

    gl_backend->DrawArrays(GL_QUADS, 0, 4);

    gl_backend->DisableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

    gl_backend->PixelStorei(GL_PACK_ALIGNMENT, ws_sfc->tex_bpp);
    gl_backend->ReadPixels(x, y, width, height,
                           ws_sfc->tex_format, ws_sfc->tex_type,
                           pixels);

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_backend->make_current(gl_backend, false);
}

static void vigs_gl_surface_copy(struct vigs_surface *dst,
                                 struct vigs_surface *src,
                                 const struct vigsp_copy *entries,
                                 uint32_t num_entries)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)dst->backend;
    struct vigs_gl_surface *gl_dst = (struct vigs_gl_surface*)dst;
    struct vigs_gl_surface *gl_src = (struct vigs_gl_surface*)src;
    struct vigs_winsys_gl_surface *ws_dst = get_ws_sfc(gl_dst);
    struct vigs_winsys_gl_surface *ws_src = get_ws_sfc(gl_src);
    uint32_t total_entries = num_entries, i;
    GLfloat src_w, src_h;
    GLfloat dst_h;
    GLfloat *vert_coords;
    GLfloat *tex_coords;

    if (!gl_backend->make_current(gl_backend, true)) {
        return;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_dst, &ws_dst->front_tex)) {
        goto out;
    }

    if (!ws_src->front_tex) {
        VIGS_LOG_WARN("copying garbage ???");
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_src, &ws_src->front_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_dst)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    src_w = gl_src->base.width;
    src_h = gl_src->base.height;
    dst_h = gl_dst->base.height;

    vigs_gl_surface_wait_fence_3d(gl_src);
    vigs_gl_surface_wait_fence_3d(gl_dst);

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_dst->fb);

    vigs_gl_surface_setup_framebuffer(gl_dst);

    gl_backend->Enable(GL_TEXTURE_2D);

    if (src == dst) {
        /*
         * Feedback loop is possible, use 'tmp_tex' instead.
         */

        if (!vigs_winsys_gl_surface_create_texture(ws_dst, &gl_dst->tmp_tex)) {
            goto out;
        }

        gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_TEXTURE_2D, gl_dst->tmp_tex, 0);

        ++total_entries;

        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (8 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (8 * sizeof(GLfloat)));

        vert_coords[0] = 0;
        vert_coords[1] = src_h;
        vert_coords[2] = src_w;
        vert_coords[3] = src_h;
        vert_coords[4] = src_w;
        vert_coords[5] = 0;
        vert_coords[6] = 0;
        vert_coords[7] = 0;

        tex_coords[0] = 0;
        tex_coords[1] = 1;
        tex_coords[2] = 1;
        tex_coords[3] = 1;
        tex_coords[4] = 1;
        tex_coords[5] = 0;
        tex_coords[6] = 0;
        tex_coords[7] = 0;
    } else {
        /*
         * No feedback loop possible, render to 'front_tex'.
         */

        gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_TEXTURE_2D, ws_dst->front_tex, 0);

    }

    for (i = 0; i < num_entries; ++i) {
        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (8 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (8 * sizeof(GLfloat)));

        vert_coords[0] = entries[i].to.x;
        vert_coords[1] = dst_h - entries[i].to.y;
        vert_coords[2] = entries[i].to.x + entries[i].size.w;
        vert_coords[3] = dst_h - entries[i].to.y;
        vert_coords[4] = entries[i].to.x + entries[i].size.w;
        vert_coords[5] = dst_h - (entries[i].to.y + entries[i].size.h);
        vert_coords[6] = entries[i].to.x;
        vert_coords[7] = dst_h - (entries[i].to.y + entries[i].size.h);

        tex_coords[0] = (GLfloat)entries[i].from.x / src_w;
        tex_coords[1] = (GLfloat)(src_h - entries[i].from.y) / src_h;
        tex_coords[2] = (GLfloat)(entries[i].from.x + entries[i].size.w) / src_w;
        tex_coords[3] = (GLfloat)(src_h - entries[i].from.y) / src_h;
        tex_coords[4] = (GLfloat)(entries[i].from.x + entries[i].size.w) / src_w;
        tex_coords[5] = (GLfloat)(src_h - (entries[i].from.y + entries[i].size.h)) / src_h;
        tex_coords[6] = (GLfloat)entries[i].from.x / src_w;
        tex_coords[7] = (GLfloat)(src_h - (entries[i].from.y + entries[i].size.h)) / src_h;
    }

    gl_backend->EnableClientState(GL_VERTEX_ARRAY);
    gl_backend->EnableClientState(GL_TEXTURE_COORD_ARRAY);

    gl_backend->BindTexture(GL_TEXTURE_2D, ws_src->front_tex);

    gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

    gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));
    gl_backend->TexCoordPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v2));

    gl_backend->DrawArrays(GL_QUADS, 0, total_entries * 4);

    if (src == dst) {
        vigs_vector_resize(&gl_backend->v1, 0);
        vigs_vector_resize(&gl_backend->v2, 0);

        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (8 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (8 * sizeof(GLfloat)));

        vert_coords[0] = 0;
        vert_coords[1] = src_h;
        vert_coords[2] = src_w;
        vert_coords[3] = src_h;
        vert_coords[4] = src_w;
        vert_coords[5] = 0;
        vert_coords[6] = 0;
        vert_coords[7] = 0;

        tex_coords[0] = 0;
        tex_coords[1] = 1;
        tex_coords[2] = 1;
        tex_coords[3] = 1;
        tex_coords[4] = 1;
        tex_coords[5] = 0;
        tex_coords[6] = 0;
        tex_coords[7] = 0;

        gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_TEXTURE_2D, ws_dst->front_tex, 0);

        gl_backend->BindTexture(GL_TEXTURE_2D, gl_dst->tmp_tex);

        gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

        gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));
        gl_backend->TexCoordPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v2));

        gl_backend->DrawArrays(GL_QUADS, 0, 4);
    }

    gl_backend->DisableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

    vigs_gl_surface_set_fence_2d(gl_dst);

    gl_dst->base.is_dirty = true;

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_backend->make_current(gl_backend, false);
}

static void vigs_gl_surface_solid_fill(struct vigs_surface *sfc,
                                       vigsp_color color,
                                       const struct vigsp_rect *entries,
                                       uint32_t num_entries)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);
    uint32_t i;
    GLubyte red, green, blue, alpha;
    GLfloat sfc_h;

    if (!gl_backend->make_current(gl_backend, true)) {
        return;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->front_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    sfc_h = gl_sfc->base.height;

    vigs_gl_surface_wait_fence_3d(gl_sfc);

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_sfc);

    gl_backend->Disable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->front_tex, 0);

    vigs_vector_resize(&gl_backend->v1, 0);

    for (i = 0; i < num_entries; ++i) {
        GLfloat *vert_coords = vigs_vector_append(&gl_backend->v1,
                                                  (8 * sizeof(GLfloat)));

        vert_coords[0] = entries[i].pos.x;
        vert_coords[1] = sfc_h - entries[i].pos.y;
        vert_coords[2] = entries[i].pos.x + entries[i].size.w;
        vert_coords[3] = sfc_h - entries[i].pos.y;
        vert_coords[4] = entries[i].pos.x + entries[i].size.w;
        vert_coords[5] = sfc_h - (entries[i].pos.y + entries[i].size.h);
        vert_coords[6] = entries[i].pos.x;
        vert_coords[7] = sfc_h - (entries[i].pos.y + entries[i].size.h);
    }

    gl_backend->EnableClientState(GL_VERTEX_ARRAY);

    vigs_gl_translate_color(color,
                            sfc->format,
                            &red,
                            &green,
                            &blue,
                            &alpha);

    gl_backend->Color4ub(red, green, blue, alpha);

    gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));

    gl_backend->DrawArrays(GL_QUADS, 0, num_entries * 4);

    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

    vigs_gl_surface_set_fence_2d(gl_sfc);

    gl_sfc->base.is_dirty = true;

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_backend->make_current(gl_backend, false);
}

static void vigs_gl_surface_put_image(struct vigs_surface *sfc,
                                      const void *src,
                                      uint32_t src_stride,
                                      const struct vigsp_rect *rect)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);

    if (!gl_backend->make_current(gl_backend, true)) {
        return;
    }

    if ((rect->size.w * ws_sfc->tex_bpp) != src_stride) {
        VIGS_LOG_ERROR("Custom strides not supported yet");
        goto out;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->front_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    vigs_gl_surface_wait_fence_3d(gl_sfc);

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_sfc);

    gl_backend->Disable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->front_tex, 0);

    gl_backend->PixelZoom(1.0f, -1.0f);
    gl_backend->RasterPos2f(rect->pos.x, gl_sfc->base.height - rect->pos.y);
    gl_backend->DrawPixels(rect->size.w,
                           rect->size.h,
                           ws_sfc->tex_format,
                           ws_sfc->tex_type,
                           src);

    vigs_gl_surface_set_fence_2d(gl_sfc);

    gl_sfc->base.is_dirty = true;

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_backend->make_current(gl_backend, false);
}

static void vigs_gl_surface_destroy(struct vigs_surface *sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);

    vigs_winsys_gl_surface_orphan(ws_sfc);

    if (gl_backend->make_current(gl_backend, true)) {
        if (gl_sfc->fb) {
            gl_backend->DeleteFramebuffers(1, &gl_sfc->fb);
        }
        if (gl_sfc->tmp_tex) {
            gl_backend->DeleteTextures(1, &gl_sfc->tmp_tex);
        }

        if (gl_backend->has_arb_sync) {
            if (gl_sfc->fence_2d) {
                gl_backend->DeleteSync(gl_sfc->fence_2d);
            }
            if (gl_sfc->fence_3d) {
                gl_backend->DeleteSync(gl_sfc->fence_3d);
            }
        }

        gl_backend->make_current(gl_backend, false);
    }

    vigs_surface_cleanup(&gl_sfc->base);

    g_free(gl_sfc);
}

/*
 * @}
 */

static struct vigs_surface *vigs_gl_backend_create_surface(struct vigs_backend *backend,
                                                           uint32_t width,
                                                           uint32_t height,
                                                           uint32_t stride,
                                                           vigsp_surface_format format,
                                                           vigsp_offset vram_offset,
                                                           uint8_t *data)
{
    struct vigs_gl_surface *gl_sfc = NULL;
    struct vigs_winsys_gl_surface *ws_sfc = NULL;
    GLint tex_internalformat;
    GLenum tex_format;
    GLenum tex_type;
    GLint tex_bpp;

    switch (format) {
    case vigsp_surface_bgrx8888:
    case vigsp_surface_bgra8888:
        tex_internalformat = GL_RGBA8;
        tex_format = GL_BGRA;
        tex_type = GL_UNSIGNED_BYTE;
        break;
    default:
        assert(false);
        goto fail;
    }

    tex_bpp = vigs_format_bpp(format);

    if ((width * tex_bpp) != stride) {
        VIGS_LOG_ERROR("Custom strides not supported yet");
        goto fail;
    }

    gl_sfc = g_malloc0(sizeof(*gl_sfc));

    ws_sfc = vigs_winsys_gl_surface_create((struct vigs_gl_backend*)backend,
                                           gl_sfc,
                                           width,
                                           height,
                                           tex_internalformat,
                                           tex_format,
                                           tex_type,
                                           tex_bpp);

    vigs_surface_init(&gl_sfc->base,
                      &ws_sfc->base.base,
                      backend,
                      vigs_id_gen(),
                      stride,
                      format,
                      vram_offset,
                      data);

    ws_sfc->base.base.release(&ws_sfc->base.base);

    gl_sfc->base.update = &vigs_gl_surface_update;
    gl_sfc->base.set_data = &vigs_gl_surface_set_data;
    gl_sfc->base.read_pixels = &vigs_gl_surface_read_pixels;
    gl_sfc->base.copy = &vigs_gl_surface_copy;
    gl_sfc->base.solid_fill = &vigs_gl_surface_solid_fill;
    gl_sfc->base.put_image = &vigs_gl_surface_put_image;
    gl_sfc->base.destroy = &vigs_gl_surface_destroy;

    return &gl_sfc->base;

fail:
    if (ws_sfc) {
        vigs_winsys_gl_surface_orphan(ws_sfc);
    }

    if (gl_sfc) {
        vigs_surface_cleanup(&gl_sfc->base);

        g_free(gl_sfc);
    }

    return NULL;
}

bool vigs_gl_backend_init(struct vigs_gl_backend *gl_backend)
{
    const char *extensions;

    if (!gl_backend->make_current(gl_backend, true)) {
        return false;
    }

    extensions = (const char*)gl_backend->GetString(GL_EXTENSIONS);

    if (!extensions) {
        VIGS_LOG_CRITICAL("Unable to get extension string");
        goto fail;
    }

    /*
     * NPOT is needed in order to
     * store custom sized surfaces as textures.
     */
    if ((strstr(extensions, "GL_OES_texture_npot ") == NULL) &&
        (strstr(extensions, "GL_ARB_texture_non_power_of_two ") == NULL)) {
        VIGS_LOG_CRITICAL("non power of 2 textures not supported");
        goto fail;
    }

    /*
     * Currently we shouldn't do this. Consider this scenario:
     * 1. Host OpenGL has a large GL commands buffer
     * 2. Target renders a lot of frames continuously
     * 3. QEMU makes a glReadPixels call 50 times per second (to update its display)
     * Thus, we might have a situation where target will queue up thousands
     * of frames and glReadPixels will take really long until all of these
     * frames are rendered which is foolish, since we only need the last one.
     * We might have really large FPS, but true performance will degrade.
     * The right way to implement all of this is via vsync, which is a todo.
     * TODO: Implement vsync.
     * @{
     */

    /*
     * ARB_sync is not mandatory, but if present gives additional
     * performance.
     */
    /*gl_backend->has_arb_sync = (strstr(extensions, "GL_ARB_sync ") != NULL) &&
                                 gl_backend->FenceSync &&
                                 gl_backend->DeleteSync &&
                                 gl_backend->WaitSync &&
                                 gl_backend->ClientWaitSync;
    if (gl_backend->has_arb_sync) {
        VIGS_LOG_INFO("ARB_sync supported");
    } else {
        VIGS_LOG_WARN("ARB_sync not supported!");
    }*/
    gl_backend->has_arb_sync = false;

    /*
     * @}
     */

    gl_backend->base.create_surface = &vigs_gl_backend_create_surface;

    gl_backend->make_current(gl_backend, false);

    vigs_vector_init(&gl_backend->v1, 0);
    vigs_vector_init(&gl_backend->v2, 0);

    return true;

fail:
    gl_backend->make_current(gl_backend, false);

    return false;
}

void vigs_gl_backend_cleanup(struct vigs_gl_backend *gl_backend)
{
    vigs_vector_cleanup(&gl_backend->v2);
    vigs_vector_cleanup(&gl_backend->v1);
}
