/*
 * vigs
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

#include "vigs_gl_backend.h"
#include "vigs_surface.h"
#include "vigs_plane.h"
#include "vigs_log.h"
#include "vigs_utils.h"
#include "vigs_ref.h"
#include "winsys_gl.h"
#include "work_queue.h"

struct vigs_gl_surface;

struct vigs_gl_backend_read_pixels_work_item
{
    struct work_queue_item base;

    struct vigs_gl_backend *backend;

    vigs_composite_start_cb start_cb;
    vigs_composite_end_cb end_cb;
    void *user_data;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    vigsp_surface_format format;
};

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

    /*
     * @}
     */

    /*
     * Texture that represent this surface.
     * Used as color attachment in 'fb'.
     *
     * Allocated on first access.
     */
    GLuint tex;
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

    gl_backend->Viewport(0, 0,
                         gl_sfc->base.ws_sfc->width, gl_sfc->base.ws_sfc->height);
    gl_backend->MatrixMode(GL_PROJECTION);
    gl_backend->LoadIdentity();
    gl_backend->Ortho(0.0, gl_sfc->base.ws_sfc->width,
                      0.0, gl_sfc->base.ws_sfc->height,
                      -1.0, 1.0);
    gl_backend->MatrixMode(GL_MODELVIEW);
    gl_backend->LoadIdentity();
    gl_backend->Disable(GL_DEPTH_TEST);
    gl_backend->Disable(GL_BLEND);
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

static void vigs_winsys_gl_surface_set_dirty(struct winsys_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    if (vigs_sfc->parent) {
        vigs_sfc->parent->base.is_dirty = true;
    }
}

static GLuint vigs_winsys_gl_surface_get_texture(struct winsys_gl_surface *sfc)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    bool has_current = vigs_sfc->backend->has_current(vigs_sfc->backend);

    if (!vigs_sfc->tex &&
        (has_current ||
        vigs_sfc->backend->make_current(vigs_sfc->backend, true))) {

        vigs_winsys_gl_surface_create_texture(vigs_sfc,
                                              &vigs_sfc->tex);

        if (!has_current) {
            vigs_sfc->backend->make_current(vigs_sfc->backend, false);
        }
    }

    return vigs_sfc->tex;
}

static void vigs_winsys_gl_surface_destroy(struct vigs_ref *ref)
{
    struct vigs_winsys_gl_surface *vigs_sfc =
        container_of(ref, struct vigs_winsys_gl_surface, ref);
    bool has_current = vigs_sfc->backend->has_current(vigs_sfc->backend);

    if (has_current ||
        vigs_sfc->backend->make_current(vigs_sfc->backend, true)) {
        if (vigs_sfc->tex) {
            vigs_sfc->backend->DeleteTextures(1, &vigs_sfc->tex);
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
                                   GLenum tex_type)
{
    struct vigs_winsys_gl_surface *ws_sfc;

    ws_sfc = g_malloc0(sizeof(*ws_sfc));

    ws_sfc->base.base.width = width;
    ws_sfc->base.base.height = height;
    ws_sfc->base.base.acquire = &vigs_winsys_gl_surface_acquire;
    ws_sfc->base.base.release = &vigs_winsys_gl_surface_release;
    ws_sfc->base.base.set_dirty = &vigs_winsys_gl_surface_set_dirty;
    ws_sfc->base.get_texture = &vigs_winsys_gl_surface_get_texture;
    ws_sfc->tex_internalformat = tex_internalformat;
    ws_sfc->tex_format = tex_format;
    ws_sfc->tex_type = tex_type;
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

static void vigs_gl_backend_batch_start(struct vigs_backend *backend)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)backend;

    if (!gl_backend->make_current(gl_backend, true)) {
        VIGS_LOG_CRITICAL("make_current failed");
    }
}

/*
 * vigs_gl_surface.
 * @{
 */

static void vigs_gl_surface_read_pixels(struct vigs_surface *sfc,
                                        uint8_t *pixels)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);
    GLfloat sfc_w, sfc_h;
    GLfloat *vert_coords;
    GLfloat *tex_coords;

    if (!ws_sfc->tex) {
        VIGS_LOG_TRACE("skipping blank read");
        goto out;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->tex)) {
        goto out;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &gl_sfc->tmp_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    sfc_w = ws_sfc->base.base.width;
    sfc_h = ws_sfc->base.base.height;

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

    gl_backend->BindTexture(GL_TEXTURE_2D, ws_sfc->tex);

    gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

    gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));
    gl_backend->TexCoordPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v2));

    gl_backend->DrawArrays(GL_QUADS, 0, 4);

    gl_backend->DisableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

    gl_backend->PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl_backend->ReadPixels(0, 0, sfc_w, sfc_h,
                           ws_sfc->tex_format, ws_sfc->tex_type,
                           pixels);

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void vigs_gl_surface_draw_pixels(struct vigs_surface *sfc,
                                        uint8_t *pixels,
                                        const struct vigsp_rect *entries,
                                        uint32_t num_entries)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);
    GLfloat sfc_w, sfc_h;
    GLfloat *vert_coords;
    GLfloat *tex_coords;
    uint32_t i;

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->tex)) {
        goto out;
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &gl_sfc->tmp_tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    sfc_w = ws_sfc->base.base.width;
    sfc_h = ws_sfc->base.base.height;

    gl_backend->BindTexture(GL_TEXTURE_2D, gl_sfc->tmp_tex);

    gl_backend->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl_backend->PixelStorei(GL_UNPACK_ROW_LENGTH, sfc_w);

    for (i = 0; i < num_entries; ++i) {
        uint32_t x = entries[i].pos.x;
        uint32_t y = entries[i].pos.y;
        uint32_t width = entries[i].size.w;
        uint32_t height = entries[i].size.h;
        VIGS_LOG_TRACE("x = %u, y = %u, width = %u, height = %u",
                       x, y,
                       width, height);
        gl_backend->PixelStorei(GL_UNPACK_SKIP_PIXELS, x);
        gl_backend->PixelStorei(GL_UNPACK_SKIP_ROWS, y);
        gl_backend->TexSubImage2D(GL_TEXTURE_2D, 0, x, y,
                                  width, height,
                                  ws_sfc->tex_format,
                                  ws_sfc->tex_type,
                                  pixels);
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_sfc);

    gl_backend->Enable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->tex, 0);

    for (i = 0; i < num_entries; ++i) {
        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (8 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (8 * sizeof(GLfloat)));

        vert_coords[0] = entries[i].pos.x;
        vert_coords[1] = sfc_h - entries[i].pos.y;
        vert_coords[2] = entries[i].pos.x + entries[i].size.w;
        vert_coords[3] = sfc_h - entries[i].pos.y;
        vert_coords[4] = entries[i].pos.x + entries[i].size.w;
        vert_coords[5] = sfc_h - (entries[i].pos.y + entries[i].size.h);
        vert_coords[6] = entries[i].pos.x;
        vert_coords[7] = sfc_h - (entries[i].pos.y + entries[i].size.h);

        tex_coords[0] = (GLfloat)entries[i].pos.x / sfc_w;
        tex_coords[1] = (GLfloat)entries[i].pos.y / sfc_h;
        tex_coords[2] = (GLfloat)(entries[i].pos.x + entries[i].size.w) / sfc_w;
        tex_coords[3] = (GLfloat)entries[i].pos.y / sfc_h;
        tex_coords[4] = (GLfloat)(entries[i].pos.x + entries[i].size.w) / sfc_w;
        tex_coords[5] = (GLfloat)(entries[i].pos.y + entries[i].size.h) / sfc_h;
        tex_coords[6] = (GLfloat)entries[i].pos.x / sfc_w;
        tex_coords[7] = (GLfloat)(entries[i].pos.y + entries[i].size.h) / sfc_h;
    }

    gl_backend->EnableClientState(GL_VERTEX_ARRAY);
    gl_backend->EnableClientState(GL_TEXTURE_COORD_ARRAY);

    gl_backend->BindTexture(GL_TEXTURE_2D, gl_sfc->tmp_tex);

    gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

    gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));
    gl_backend->TexCoordPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v2));

    gl_backend->DrawArrays(GL_QUADS, 0, num_entries * 4);

    gl_backend->DisableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);
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

    if (!vigs_winsys_gl_surface_create_texture(ws_dst, &ws_dst->tex)) {
        goto out;
    }

    if (!ws_src->tex) {
        VIGS_LOG_WARN("copying garbage ???");
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_src, &ws_src->tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_dst)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    src_w = ws_src->base.base.width;
    src_h = ws_src->base.base.height;
    dst_h = ws_dst->base.base.height;

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
                                         GL_TEXTURE_2D, ws_dst->tex, 0);

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

    gl_backend->BindTexture(GL_TEXTURE_2D, ws_src->tex);

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
                                         GL_TEXTURE_2D, ws_dst->tex, 0);

        gl_backend->BindTexture(GL_TEXTURE_2D, gl_dst->tmp_tex);

        gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

        gl_backend->VertexPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v1));
        gl_backend->TexCoordPointer(2, GL_FLOAT, 0, vigs_vector_data(&gl_backend->v2));

        gl_backend->DrawArrays(GL_QUADS, 0, 4);
    }

    gl_backend->DisableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);
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

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->tex)) {
        goto out;
    }

    if (!vigs_gl_surface_create_framebuffer(gl_sfc)) {
        goto out;
    }

    sfc_h = ws_sfc->base.base.height;

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_sfc);

    gl_backend->Disable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->tex, 0);

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

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void vigs_gl_surface_destroy(struct vigs_surface *sfc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)sfc->backend;
    struct vigs_gl_surface *gl_sfc = (struct vigs_gl_surface*)sfc;
    struct vigs_winsys_gl_surface *ws_sfc = get_ws_sfc(gl_sfc);

    vigs_winsys_gl_surface_orphan(ws_sfc);

    if (gl_sfc->fb) {
        gl_backend->DeleteFramebuffers(1, &gl_sfc->fb);
    }
    if (gl_sfc->tmp_tex) {
        gl_backend->DeleteTextures(1, &gl_sfc->tmp_tex);
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
                                                           vigsp_surface_id id)
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
        tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
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
                                           tex_type);

    vigs_surface_init(&gl_sfc->base,
                      &ws_sfc->base.base,
                      backend,
                      stride,
                      format,
                      id);

    ws_sfc->base.base.release(&ws_sfc->base.base);

    gl_sfc->base.read_pixels = &vigs_gl_surface_read_pixels;
    gl_sfc->base.draw_pixels = &vigs_gl_surface_draw_pixels;
    gl_sfc->base.copy = &vigs_gl_surface_copy;
    gl_sfc->base.solid_fill = &vigs_gl_surface_solid_fill;
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

static void vigs_gl_backend_read_pixels_work(struct work_queue_item *wq_item)
{
    struct vigs_gl_backend_read_pixels_work_item *item = (struct vigs_gl_backend_read_pixels_work_item*)wq_item;
    struct vigs_gl_backend *backend = item->backend;
    uint8_t *dst = NULL;

    VIGS_LOG_TRACE("enter");

    if (backend->read_pixels_make_current(backend, true)) {
        uint8_t *src;

        backend->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, backend->pbo);

        src = backend->MapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);

        if (src) {
            dst = item->start_cb(item->user_data,
                                 item->width, item->height,
                                 item->stride, item->format);

            memcpy(dst, src, item->stride * item->height);

            if (!backend->UnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB)) {
                VIGS_LOG_CRITICAL("glUnmapBuffer failed");
            }
        } else {
            VIGS_LOG_CRITICAL("glMapBuffer failed");
        }

        backend->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

        backend->read_pixels_make_current(backend, false);
    }

    item->end_cb(item->user_data, (dst != NULL));

    g_free(item);
}

static void vigs_gl_backend_composite(struct vigs_surface *surface,
                                      const struct vigs_plane *planes,
                                      vigs_composite_start_cb start_cb,
                                      vigs_composite_end_cb end_cb,
                                      void *user_data)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)surface->backend;
    struct vigs_gl_surface *gl_root_sfc = (struct vigs_gl_surface*)surface;
    struct vigs_winsys_gl_surface *ws_root_sfc = get_ws_sfc(gl_root_sfc);
    uint32_t i;
    GLfloat *vert_coords;
    GLfloat *tex_coords;
    const struct vigs_plane *sorted_planes[VIGS_MAX_PLANES];
    uint32_t size = surface->stride * surface->ws_sfc->height;
    struct vigs_gl_backend_read_pixels_work_item *item;

    VIGS_LOG_TRACE("enter");

    if (!surface->ptr) {
        if (!ws_root_sfc->tex) {
            VIGS_LOG_WARN("compositing garbage (root surface) ???");
        }

        if (!vigs_winsys_gl_surface_create_texture(ws_root_sfc, &ws_root_sfc->tex)) {
            goto out;
        }
    }

    if (!vigs_winsys_gl_surface_create_texture(ws_root_sfc, &gl_root_sfc->tmp_tex)) {
        goto out;
    }

    for (i = 0; i < VIGS_MAX_PLANES; ++i) {
        struct vigs_gl_surface *gl_sfc;
        struct vigs_winsys_gl_surface *ws_sfc;

        if (!planes[i].sfc) {
            continue;
        }

        gl_sfc = (struct vigs_gl_surface*)planes[i].sfc;
        ws_sfc = get_ws_sfc(gl_sfc);

        if (!ws_sfc->tex) {
            VIGS_LOG_WARN("compositing garbage (plane %u) ???", i);
        }

        if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->tex)) {
            goto out;
        }
    }

    if (!vigs_gl_surface_create_framebuffer(gl_root_sfc)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    vert_coords = vigs_vector_append(&gl_backend->v1,
                                     (8 * sizeof(GLfloat)));
    tex_coords = vigs_vector_append(&gl_backend->v2,
                                    (8 * sizeof(GLfloat)));

    if (surface->ptr) {
        /*
         * Root surface is scanout, upload it to texture.
         * Slow path.
         */

        gl_backend->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
        gl_backend->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        gl_backend->PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        gl_backend->PixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        gl_backend->BindTexture(GL_TEXTURE_2D, gl_root_sfc->tmp_tex);

        gl_backend->TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                                  ws_root_sfc->base.base.width,
                                  ws_root_sfc->base.base.height,
                                  ws_root_sfc->tex_format,
                                  ws_root_sfc->tex_type,
                                  surface->ptr);
    }

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_root_sfc->fb);

    vigs_gl_surface_setup_framebuffer(gl_root_sfc);

    gl_backend->Enable(GL_TEXTURE_2D);

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, gl_root_sfc->tmp_tex, 0);

    gl_backend->EnableClientState(GL_VERTEX_ARRAY);
    gl_backend->EnableClientState(GL_TEXTURE_COORD_ARRAY);

    gl_backend->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

    if (!surface->ptr) {
        /*
         * If root surface is not scanout then we must render
         * it.
         */

        vert_coords[0] = 0;
        vert_coords[1] = ws_root_sfc->base.base.height;
        vert_coords[2] = ws_root_sfc->base.base.width;
        vert_coords[3] = ws_root_sfc->base.base.height;
        vert_coords[4] = ws_root_sfc->base.base.width;
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

        gl_backend->BindTexture(GL_TEXTURE_2D, ws_root_sfc->tex);

        gl_backend->VertexPointer(2, GL_FLOAT, 0, vert_coords);
        gl_backend->TexCoordPointer(2, GL_FLOAT, 0, tex_coords);

        gl_backend->DrawArrays(GL_QUADS, 0, 4);
    }

    /*
     * Sort planes, only 2 of them now, don't bother...
     */

    assert(VIGS_MAX_PLANES == 2);

    if (planes[0].z_pos <= planes[1].z_pos) {
        sorted_planes[0] = &planes[0];
        sorted_planes[1] = &planes[1];
    } else {
        sorted_planes[0] = &planes[1];
        sorted_planes[1] = &planes[0];
    }

    /*
     * Now render planes, respect z-order.
     */

    for (i = 0; i < VIGS_MAX_PLANES; ++i) {
        const struct vigs_plane *plane = sorted_planes[i];
        struct vigs_gl_surface *gl_sfc;
        struct vigs_winsys_gl_surface *ws_sfc;
        GLfloat src_w, src_h;

        if (!plane->sfc) {
            continue;
        }

        gl_sfc = (struct vigs_gl_surface*)plane->sfc;
        ws_sfc = get_ws_sfc(gl_sfc);

        src_w = ws_sfc->base.base.width;
        src_h = ws_sfc->base.base.height;

        vert_coords[0] = plane->dst_x;
        vert_coords[1] = plane->dst_y;
        vert_coords[2] = plane->dst_x + (int)plane->dst_size.w;
        vert_coords[3] = plane->dst_y;
        vert_coords[4] = plane->dst_x + (int)plane->dst_size.w;
        vert_coords[5] = plane->dst_y + (int)plane->dst_size.h;
        vert_coords[6] = plane->dst_x;
        vert_coords[7] = plane->dst_y + (int)plane->dst_size.h;

        tex_coords[0] = (GLfloat)plane->src_rect.pos.x / src_w;
        tex_coords[1] = (GLfloat)(src_h - plane->src_rect.pos.y) / src_h;
        tex_coords[2] = (GLfloat)(plane->src_rect.pos.x + plane->src_rect.size.w) / src_w;
        tex_coords[3] = (GLfloat)(src_h - plane->src_rect.pos.y) / src_h;
        tex_coords[4] = (GLfloat)(plane->src_rect.pos.x + plane->src_rect.size.w) / src_w;
        tex_coords[5] = (GLfloat)(src_h - (plane->src_rect.pos.y + plane->src_rect.size.h)) / src_h;
        tex_coords[6] = (GLfloat)plane->src_rect.pos.x / src_w;
        tex_coords[7] = (GLfloat)(src_h - (plane->src_rect.pos.y + plane->src_rect.size.h)) / src_h;

        gl_backend->BindTexture(GL_TEXTURE_2D, ws_sfc->tex);

        gl_backend->VertexPointer(2, GL_FLOAT, 0, vert_coords);
        gl_backend->TexCoordPointer(2, GL_FLOAT, 0, tex_coords);

        gl_backend->DrawArrays(GL_QUADS, 0, 4);
    }

    gl_backend->DisableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_backend->DisableClientState(GL_VERTEX_ARRAY);

    /*
     * Now schedule asynchronous glReadPixels.
     */

    gl_backend->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, gl_backend->pbo);

    if (size > gl_backend->pbo_size) {
        gl_backend->pbo_size = size;
        gl_backend->BufferData(GL_PIXEL_PACK_BUFFER_ARB,
                               size,
                               0,
                               GL_STREAM_READ);
    }

    gl_backend->PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl_backend->ReadPixels(0, 0,
                           surface->ws_sfc->width, surface->ws_sfc->height,
                           ws_root_sfc->tex_format, ws_root_sfc->tex_type,
                           NULL);

    gl_backend->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

    /*
     * That's a tricky one, if we don't do this then it's not
     * guaranteed that PBO will actually be updated by the time
     * 'vigs_gl_backend_read_pixels_work' runs and since
     * 'vigs_gl_backend_read_pixels_work' uses another OpenGL context
     * we might get old results.
     */
    gl_backend->Finish();

    item = g_malloc(sizeof(*item));

    work_queue_item_init(&item->base, &vigs_gl_backend_read_pixels_work);

    item->backend = gl_backend;

    item->start_cb = start_cb;
    item->end_cb = end_cb;
    item->user_data = user_data;
    item->width = surface->ws_sfc->width;
    item->height = surface->ws_sfc->height;
    item->stride = surface->stride;
    item->format = surface->format;

    work_queue_add_item(gl_backend->read_pixels_queue, &item->base);

out:
    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void vigs_gl_backend_batch_end(struct vigs_backend *backend)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)backend;

    gl_backend->Finish();
    gl_backend->make_current(gl_backend, false);
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

    gl_backend->GenBuffers(1, &gl_backend->pbo);

    if (!gl_backend->pbo) {
        VIGS_LOG_CRITICAL("cannot create read_pixels PBO");
        goto fail;
    }

    gl_backend->base.batch_start = &vigs_gl_backend_batch_start;
    gl_backend->base.create_surface = &vigs_gl_backend_create_surface;
    gl_backend->base.composite = &vigs_gl_backend_composite;
    gl_backend->base.batch_end = &vigs_gl_backend_batch_end;

    gl_backend->make_current(gl_backend, false);

    vigs_vector_init(&gl_backend->v1, 0);
    vigs_vector_init(&gl_backend->v2, 0);

    gl_backend->read_pixels_queue = work_queue_create();

    return true;

fail:
    gl_backend->make_current(gl_backend, false);

    return false;
}

void vigs_gl_backend_cleanup(struct vigs_gl_backend *gl_backend)
{
    work_queue_destroy(gl_backend->read_pixels_queue);

    if (gl_backend->make_current(gl_backend, true)) {
        gl_backend->DeleteBuffers(1, &gl_backend->pbo);

        gl_backend->make_current(gl_backend, false);
    }

    vigs_vector_cleanup(&gl_backend->v2);
    vigs_vector_cleanup(&gl_backend->v1);
}
