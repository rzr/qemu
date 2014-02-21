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

    /*
     * Ortho matrix for this surface.
     */
    GLfloat ortho[16];
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

static const char *g_vs_tex_source =
    "attribute vec4 vertCoord;\n"
    "uniform mat4 proj;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 v_texCoord;\n"
    "void main()\n"
    "{\n"
    "    v_texCoord = texCoord;\n"
    "    gl_Position = proj * vertCoord;\n"
    "}\n";

static const char *g_fs_tex_source =
    "uniform sampler2D tex;\n"
    "varying vec2 v_texCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(tex, v_texCoord);\n"
    "}\n";

static const char *g_vs_color_source =
    "attribute vec4 vertCoord;\n"
    "uniform mat4 proj;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = proj * vertCoord;\n"
    "}\n";

static const char *g_fs_color_source =
    "uniform vec4 color;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = color;\n"
    "}\n";

static GLuint vigs_gl_create_shader(struct vigs_gl_backend *backend,
                                    const char *source,
                                    GLenum type)
{
    GLuint shader = backend->CreateShader(type);
    GLint tmp = 0;

    if (!shader) {
        VIGS_LOG_CRITICAL("Unable to create shader type %d", type);
        return 0;
    }

    backend->ShaderSource(shader, 1, &source, NULL);
    backend->CompileShader(shader);
    backend->GetShaderiv(shader, GL_COMPILE_STATUS, &tmp);

    if (!tmp) {
        char *buff;

        tmp = 0;

        backend->GetShaderiv(shader, GL_INFO_LOG_LENGTH, &tmp);

        buff = g_malloc0(tmp);

        backend->GetShaderInfoLog(shader, tmp, NULL, buff);

        VIGS_LOG_CRITICAL("Unable to compile shader (type = %d) - %s",
                          type, buff);

        backend->DeleteShader(shader);

        g_free(buff);

        return 0;
    }

    return shader;
}

static GLuint vigs_gl_create_program(struct vigs_gl_backend *backend,
                                     GLuint vs_id,
                                     GLuint fs_id)
{
    GLuint program = backend->CreateProgram();
    GLint tmp = 0;

    if (!program) {
        VIGS_LOG_CRITICAL("Unable to create program");
        return 0;
    }

    backend->AttachShader(program, vs_id);
    backend->AttachShader(program, fs_id);
    backend->LinkProgram(program);
    backend->GetProgramiv(program, GL_LINK_STATUS, &tmp);

    if (!tmp) {
        char *buff;

        tmp = 0;

        backend->GetProgramiv(program, GL_INFO_LOG_LENGTH, &tmp);

        buff = g_malloc0(tmp);

        backend->GetProgramInfoLog(program, tmp, NULL, buff);

        VIGS_LOG_CRITICAL("Unable to link program - %s", buff);

        backend->DeleteProgram(program);

        g_free(buff);

        return 0;
    }

    return program;
}

static void vigs_gl_draw_tex_prog(struct vigs_gl_backend *backend,
                                  uint32_t count)
{
    uint32_t size = count * 16;

    if (size > backend->vbo_size) {
        backend->vbo_size = size;
        backend->BufferData(GL_ARRAY_BUFFER,
                            size,
                            0,
                            GL_STREAM_DRAW);
    }

    backend->BufferSubData(GL_ARRAY_BUFFER, 0,
                           (size / 2), vigs_vector_data(&backend->v1));
    backend->BufferSubData(GL_ARRAY_BUFFER, (size / 2),
                           (size / 2), vigs_vector_data(&backend->v2));

    backend->EnableVertexAttribArray(backend->tex_prog_vertCoord_loc);
    backend->EnableVertexAttribArray(backend->tex_prog_texCoord_loc);

    backend->VertexAttribPointer(backend->tex_prog_vertCoord_loc,
                                 2, GL_FLOAT, GL_FALSE, 0, NULL);
    backend->VertexAttribPointer(backend->tex_prog_texCoord_loc,
                                 2, GL_FLOAT, GL_FALSE, 0, NULL + (size / 2));

    backend->DrawArrays(GL_TRIANGLES, 0, count);

    backend->DisableVertexAttribArray(backend->tex_prog_texCoord_loc);
    backend->DisableVertexAttribArray(backend->tex_prog_vertCoord_loc);
}

static void vigs_gl_draw_color_prog(struct vigs_gl_backend *backend,
                                    const GLfloat color[4],
                                    uint32_t count)
{
    uint32_t size = count * 8;

    if (size > backend->vbo_size) {
        backend->vbo_size = size;
        backend->BufferData(GL_ARRAY_BUFFER,
                            size,
                            0,
                            GL_STREAM_DRAW);
    }

    backend->BufferSubData(GL_ARRAY_BUFFER, 0, size,
                           vigs_vector_data(&backend->v1));

    backend->Uniform4fv(backend->color_prog_color_loc, 1, color);

    backend->EnableVertexAttribArray(backend->color_prog_vertCoord_loc);

    backend->VertexAttribPointer(backend->color_prog_vertCoord_loc,
                                 2, GL_FLOAT, GL_FALSE, 0, NULL);

    backend->DrawArrays(GL_TRIANGLES, 0, count);

    backend->DisableVertexAttribArray(backend->color_prog_vertCoord_loc);
}

static void vigs_gl_create_ortho(GLfloat left, GLfloat right,
                                 GLfloat bottom, GLfloat top,
                                 GLfloat near, GLfloat far,
                                 GLfloat ortho[16])
{
    ortho[0] = 2.0f / (right - left);
    ortho[5] = 2.0f / (top - bottom);
    ortho[10] = -2.0f / (far - near);
    ortho[12] = -(right + left) / (right - left);
    ortho[13] = -(top + bottom) / (top - bottom);
    ortho[14] = -(far + near) / (far - near);
    ortho[15] = 1.0f;
}

static void vigs_gl_translate_color(vigsp_color color,
                                    vigsp_surface_format format,
                                    GLfloat res[4])
{
    res[3] = 1.0f;
    switch (format) {
    case vigsp_surface_bgra8888:
        res[3] = (GLfloat)((color >> 24) & 0xFF) / 255.0f;
        /* Fall through. */
    case vigsp_surface_bgrx8888:
        res[0] = (GLfloat)((color >> 16) & 0xFF) / 255.0f;
        res[1] = (GLfloat)((color >> 8) & 0xFF) / 255.0f;
        res[2] = (GLfloat)(color & 0xFF) / 255.0f;
        break;
    default:
        assert(false);
        res[0] = 0.0f;
        res[1] = 0.0f;
        res[2] = 0.0f;
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

static bool vigs_gl_surface_setup_framebuffer(struct vigs_gl_surface *gl_sfc,
                                              GLuint prog_id,
                                              GLint proj_loc)
{
    struct vigs_gl_backend *gl_backend = (struct vigs_gl_backend*)gl_sfc->base.backend;

    if (!gl_sfc->fb) {
        gl_backend->GenFramebuffers(1, &gl_sfc->fb);

        if (!gl_sfc->fb) {
            return false;
        }
    }

    if (gl_backend->cur_prog_id != prog_id) {
        gl_backend->UseProgram(prog_id);
        gl_backend->cur_prog_id = prog_id;
    }

    gl_backend->Viewport(0, 0,
                         gl_sfc->base.ws_sfc->width,
                         gl_sfc->base.ws_sfc->height);

    gl_backend->UniformMatrix4fv(proj_loc, 1, GL_FALSE, gl_sfc->ortho);

    gl_backend->BindFramebuffer(GL_FRAMEBUFFER, gl_sfc->fb);

    return true;
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

static void vigs_winsys_gl_surface_draw_pixels(struct winsys_surface *sfc,
                                               uint8_t *pixels)
{
    struct vigs_winsys_gl_surface *vigs_sfc = (struct vigs_winsys_gl_surface*)sfc;
    bool has_current = vigs_sfc->backend->has_current(vigs_sfc->backend);
    if (!vigs_sfc->parent) {
        return;
    }

    if (has_current ||
        vigs_sfc->backend->make_current(vigs_sfc->backend, true)) {
        struct vigsp_rect rect;

        rect.pos.x = 0;
        rect.pos.y = 0;
        rect.size.w = sfc->width;
        rect.size.h = sfc->height;

        vigs_sfc->parent->base.draw_pixels(&vigs_sfc->parent->base,
                                           pixels,
                                           &rect,
                                           1);

        vigs_sfc->parent->base.is_dirty = true;

        vigs_sfc->backend->Finish();

        if (!has_current) {
            vigs_sfc->backend->make_current(vigs_sfc->backend, false);
        }
    } else {
        VIGS_LOG_CRITICAL("make_current failed");
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
    ws_sfc->base.base.draw_pixels = &vigs_winsys_gl_surface_draw_pixels;
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

    if (!vigs_gl_surface_setup_framebuffer(gl_sfc,
                                           gl_backend->tex_prog_id,
                                           gl_backend->tex_prog_proj_loc)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    sfc_w = ws_sfc->base.base.width;
    sfc_h = ws_sfc->base.base.height;

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, gl_sfc->tmp_tex, 0);

    vert_coords = vigs_vector_append(&gl_backend->v1,
                                     (12 * sizeof(GLfloat)));
    tex_coords = vigs_vector_append(&gl_backend->v2,
                                    (12 * sizeof(GLfloat)));

    vert_coords[6] = vert_coords[0] = 0;
    vert_coords[7] = vert_coords[1] = sfc_h;
    vert_coords[2] = sfc_w;
    vert_coords[3] = sfc_h;
    vert_coords[8] = vert_coords[4] = sfc_w;
    vert_coords[9] = vert_coords[5] = 0;
    vert_coords[10] = 0;
    vert_coords[11] = 0;

    tex_coords[6] = tex_coords[0] = 0;
    tex_coords[7] = tex_coords[1] = 0;
    tex_coords[2] = 1;
    tex_coords[3] = 0;
    tex_coords[8] = tex_coords[4] = 1;
    tex_coords[9] = tex_coords[5] = 1;
    tex_coords[10] = 0;
    tex_coords[11] = 1;

    gl_backend->BindTexture(GL_TEXTURE_2D, ws_sfc->tex);

    vigs_gl_draw_tex_prog(gl_backend, 6);

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

    if (!vigs_gl_surface_setup_framebuffer(gl_sfc,
                                           gl_backend->tex_prog_id,
                                           gl_backend->tex_prog_proj_loc)) {
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

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->tex, 0);

    for (i = 0; i < num_entries; ++i) {
        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (12 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (12 * sizeof(GLfloat)));

        vert_coords[6] = vert_coords[0] = entries[i].pos.x;
        vert_coords[7] = vert_coords[1] = sfc_h - entries[i].pos.y;
        vert_coords[2] = entries[i].pos.x + entries[i].size.w;
        vert_coords[3] = sfc_h - entries[i].pos.y;
        vert_coords[8] = vert_coords[4] = entries[i].pos.x + entries[i].size.w;
        vert_coords[9] = vert_coords[5] = sfc_h - (entries[i].pos.y + entries[i].size.h);
        vert_coords[10] = entries[i].pos.x;
        vert_coords[11] = sfc_h - (entries[i].pos.y + entries[i].size.h);

        tex_coords[6] = tex_coords[0] = (GLfloat)entries[i].pos.x / sfc_w;
        tex_coords[7] = tex_coords[1] = (GLfloat)entries[i].pos.y / sfc_h;
        tex_coords[2] = (GLfloat)(entries[i].pos.x + entries[i].size.w) / sfc_w;
        tex_coords[3] = (GLfloat)entries[i].pos.y / sfc_h;
        tex_coords[8] = tex_coords[4] = (GLfloat)(entries[i].pos.x + entries[i].size.w) / sfc_w;
        tex_coords[9] = tex_coords[5] = (GLfloat)(entries[i].pos.y + entries[i].size.h) / sfc_h;
        tex_coords[10] = (GLfloat)entries[i].pos.x / sfc_w;
        tex_coords[11] = (GLfloat)(entries[i].pos.y + entries[i].size.h) / sfc_h;
    }

    vigs_gl_draw_tex_prog(gl_backend, num_entries * 6);

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

    if (!vigs_gl_surface_setup_framebuffer(gl_dst,
                                           gl_backend->tex_prog_id,
                                           gl_backend->tex_prog_proj_loc)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    src_w = ws_src->base.base.width;
    src_h = ws_src->base.base.height;
    dst_h = ws_dst->base.base.height;

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
                                         (12 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (12 * sizeof(GLfloat)));

        vert_coords[6] = vert_coords[0] = 0;
        vert_coords[7] = vert_coords[1] = src_h;
        vert_coords[2] = src_w;
        vert_coords[3] = src_h;
        vert_coords[8] = vert_coords[4] = src_w;
        vert_coords[9] = vert_coords[5] = 0;
        vert_coords[10] = 0;
        vert_coords[11] = 0;

        tex_coords[6] = tex_coords[0] = 0;
        tex_coords[7] = tex_coords[1] = 1;
        tex_coords[2] = 1;
        tex_coords[3] = 1;
        tex_coords[8] = tex_coords[4] = 1;
        tex_coords[9] = tex_coords[5] = 0;
        tex_coords[10] = 0;
        tex_coords[11] = 0;
    } else {
        /*
         * No feedback loop possible, render to 'front_tex'.
         */

        gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_TEXTURE_2D, ws_dst->tex, 0);
    }

    for (i = 0; i < num_entries; ++i) {
        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (12 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (12 * sizeof(GLfloat)));

        vert_coords[6] = vert_coords[0] = entries[i].to.x;
        vert_coords[7] = vert_coords[1] = dst_h - entries[i].to.y;
        vert_coords[2] = entries[i].to.x + entries[i].size.w;
        vert_coords[3] = dst_h - entries[i].to.y;
        vert_coords[8] = vert_coords[4] = entries[i].to.x + entries[i].size.w;
        vert_coords[9] = vert_coords[5] = dst_h - (entries[i].to.y + entries[i].size.h);
        vert_coords[10] = entries[i].to.x;
        vert_coords[11] = dst_h - (entries[i].to.y + entries[i].size.h);

        tex_coords[6] = tex_coords[0] = (GLfloat)entries[i].from.x / src_w;
        tex_coords[7] = tex_coords[1] = (GLfloat)(src_h - entries[i].from.y) / src_h;
        tex_coords[2] = (GLfloat)(entries[i].from.x + entries[i].size.w) / src_w;
        tex_coords[3] = (GLfloat)(src_h - entries[i].from.y) / src_h;
        tex_coords[8] = tex_coords[4] = (GLfloat)(entries[i].from.x + entries[i].size.w) / src_w;
        tex_coords[9] = tex_coords[5] = (GLfloat)(src_h - (entries[i].from.y + entries[i].size.h)) / src_h;
        tex_coords[10] = (GLfloat)entries[i].from.x / src_w;
        tex_coords[11] = (GLfloat)(src_h - (entries[i].from.y + entries[i].size.h)) / src_h;
    }

    gl_backend->BindTexture(GL_TEXTURE_2D, ws_src->tex);

    vigs_gl_draw_tex_prog(gl_backend, total_entries * 6);

    if (src == dst) {
        vigs_vector_resize(&gl_backend->v1, 0);
        vigs_vector_resize(&gl_backend->v2, 0);

        vert_coords = vigs_vector_append(&gl_backend->v1,
                                         (12 * sizeof(GLfloat)));
        tex_coords = vigs_vector_append(&gl_backend->v2,
                                        (12 * sizeof(GLfloat)));

        vert_coords[6] = vert_coords[0] = 0;
        vert_coords[7] = vert_coords[1] = src_h;
        vert_coords[2] = src_w;
        vert_coords[3] = src_h;
        vert_coords[8] = vert_coords[4] = src_w;
        vert_coords[9] = vert_coords[5] = 0;
        vert_coords[10] = 0;
        vert_coords[11] = 0;

        tex_coords[6] = tex_coords[0] = 0;
        tex_coords[7] = tex_coords[1] = 1;
        tex_coords[2] = 1;
        tex_coords[3] = 1;
        tex_coords[8] = tex_coords[4] = 1;
        tex_coords[9] = tex_coords[5] = 0;
        tex_coords[10] = 0;
        tex_coords[11] = 0;

        gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_TEXTURE_2D, ws_dst->tex, 0);

        gl_backend->BindTexture(GL_TEXTURE_2D, gl_dst->tmp_tex);

        vigs_gl_draw_tex_prog(gl_backend, 6);
    }

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
    GLfloat colorf[4];
    GLfloat sfc_h;

    if (!vigs_winsys_gl_surface_create_texture(ws_sfc, &ws_sfc->tex)) {
        goto out;
    }

    if (!vigs_gl_surface_setup_framebuffer(gl_sfc,
                                           gl_backend->color_prog_id,
                                           gl_backend->color_prog_proj_loc)) {
        goto out;
    }

    sfc_h = ws_sfc->base.base.height;

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ws_sfc->tex, 0);

    vigs_vector_resize(&gl_backend->v1, 0);

    for (i = 0; i < num_entries; ++i) {
        GLfloat *vert_coords = vigs_vector_append(&gl_backend->v1,
                                                  (12 * sizeof(GLfloat)));

        vert_coords[6] = vert_coords[0] = entries[i].pos.x;
        vert_coords[7] = vert_coords[1] = sfc_h - entries[i].pos.y;
        vert_coords[2] = entries[i].pos.x + entries[i].size.w;
        vert_coords[3] = sfc_h - entries[i].pos.y;
        vert_coords[8] = vert_coords[4] = entries[i].pos.x + entries[i].size.w;
        vert_coords[9] = vert_coords[5] = sfc_h - (entries[i].pos.y + entries[i].size.h);
        vert_coords[10] = entries[i].pos.x;
        vert_coords[11] = sfc_h - (entries[i].pos.y + entries[i].size.h);
    }

    vigs_gl_translate_color(color, sfc->format, colorf);

    vigs_gl_draw_color_prog(gl_backend, colorf, num_entries * 6);

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

    vigs_gl_create_ortho(0.0f, width, 0.0f, height,
                         -1.0f, 1.0f, gl_sfc->ortho);

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

    if (!vigs_gl_surface_setup_framebuffer(gl_root_sfc,
                                           gl_backend->tex_prog_id,
                                           gl_backend->tex_prog_proj_loc)) {
        goto out;
    }

    vigs_vector_resize(&gl_backend->v1, 0);
    vigs_vector_resize(&gl_backend->v2, 0);

    vert_coords = vigs_vector_append(&gl_backend->v1,
                                     (12 * sizeof(GLfloat)));
    tex_coords = vigs_vector_append(&gl_backend->v2,
                                    (12 * sizeof(GLfloat)));

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

    gl_backend->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, gl_root_sfc->tmp_tex, 0);

    if (!surface->ptr) {
        /*
         * If root surface is not scanout then we must render
         * it.
         */

        vert_coords[6] = vert_coords[0] = 0;
        vert_coords[7] = vert_coords[1] = ws_root_sfc->base.base.height;
        vert_coords[2] = ws_root_sfc->base.base.width;
        vert_coords[3] = ws_root_sfc->base.base.height;
        vert_coords[8] = vert_coords[4] = ws_root_sfc->base.base.width;
        vert_coords[9] = vert_coords[5] = 0;
        vert_coords[10] = 0;
        vert_coords[11] = 0;

        tex_coords[6] = tex_coords[0] = 0;
        tex_coords[7] =tex_coords[1] = 0;
        tex_coords[2] = 1;
        tex_coords[3] = 0;
        tex_coords[8] = tex_coords[4] = 1;
        tex_coords[9] = tex_coords[5] = 1;
        tex_coords[10] = 0;
        tex_coords[11] = 1;

        gl_backend->BindTexture(GL_TEXTURE_2D, ws_root_sfc->tex);

        vigs_gl_draw_tex_prog(gl_backend, 6);
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

        vert_coords[6] = vert_coords[0] = plane->dst_x;
        vert_coords[7] = vert_coords[1] = plane->dst_y;
        vert_coords[2] = plane->dst_x + (int)plane->dst_size.w;
        vert_coords[3] = plane->dst_y;
        vert_coords[8] = vert_coords[4] = plane->dst_x + (int)plane->dst_size.w;
        vert_coords[9] = vert_coords[5] = plane->dst_y + (int)plane->dst_size.h;
        vert_coords[10] = plane->dst_x;
        vert_coords[11] = plane->dst_y + (int)plane->dst_size.h;

        tex_coords[6] = tex_coords[0] = (GLfloat)plane->src_rect.pos.x / src_w;
        tex_coords[7] = tex_coords[1] = (GLfloat)(src_h - plane->src_rect.pos.y) / src_h;
        tex_coords[2] = (GLfloat)(plane->src_rect.pos.x + plane->src_rect.size.w) / src_w;
        tex_coords[3] = (GLfloat)(src_h - plane->src_rect.pos.y) / src_h;
        tex_coords[8] = tex_coords[4] = (GLfloat)(plane->src_rect.pos.x + plane->src_rect.size.w) / src_w;
        tex_coords[9] = tex_coords[5] = (GLfloat)(src_h - (plane->src_rect.pos.y + plane->src_rect.size.h)) / src_h;
        tex_coords[10] = (GLfloat)plane->src_rect.pos.x / src_w;
        tex_coords[11] = (GLfloat)(src_h - (plane->src_rect.pos.y + plane->src_rect.size.h)) / src_h;

        gl_backend->BindTexture(GL_TEXTURE_2D, ws_sfc->tex);

        vigs_gl_draw_tex_prog(gl_backend, 6);
    }

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
    if (!gl_backend->make_current(gl_backend, true)) {
        return false;
    }

    if (!gl_backend->is_gl_2) {
        gl_backend->GenVertexArrays(1, &gl_backend->vao);

        if (!gl_backend->vao) {
            VIGS_LOG_CRITICAL("cannot create VAO");
            goto fail;
        }

        gl_backend->BindVertexArray(gl_backend->vao);
    }

    gl_backend->tex_prog_vs_id = vigs_gl_create_shader(gl_backend,
                                                       g_vs_tex_source,
                                                       GL_VERTEX_SHADER);

    if (!gl_backend->tex_prog_vs_id) {
        goto fail;
    }

    gl_backend->tex_prog_fs_id = vigs_gl_create_shader(gl_backend,
                                                       g_fs_tex_source,
                                                       GL_FRAGMENT_SHADER);

    if (!gl_backend->tex_prog_fs_id) {
        goto fail;
    }

    gl_backend->tex_prog_id = vigs_gl_create_program(gl_backend,
                                                     gl_backend->tex_prog_vs_id,
                                                     gl_backend->tex_prog_fs_id);

    if (!gl_backend->tex_prog_id) {
        goto fail;
    }

    gl_backend->tex_prog_proj_loc = gl_backend->GetUniformLocation(gl_backend->tex_prog_id, "proj");
    gl_backend->tex_prog_vertCoord_loc = gl_backend->GetAttribLocation(gl_backend->tex_prog_id, "vertCoord");
    gl_backend->tex_prog_texCoord_loc = gl_backend->GetAttribLocation(gl_backend->tex_prog_id, "texCoord");

    gl_backend->color_prog_vs_id = vigs_gl_create_shader(gl_backend,
                                                         g_vs_color_source,
                                                         GL_VERTEX_SHADER);

    if (!gl_backend->color_prog_vs_id) {
        goto fail;
    }

    gl_backend->color_prog_fs_id = vigs_gl_create_shader(gl_backend,
                                                         g_fs_color_source,
                                                         GL_FRAGMENT_SHADER);

    if (!gl_backend->color_prog_fs_id) {
        goto fail;
    }

    gl_backend->color_prog_id = vigs_gl_create_program(gl_backend,
                                                       gl_backend->color_prog_vs_id,
                                                       gl_backend->color_prog_fs_id);

    if (!gl_backend->color_prog_id) {
        goto fail;
    }

    gl_backend->color_prog_proj_loc = gl_backend->GetUniformLocation(gl_backend->color_prog_id, "proj");
    gl_backend->color_prog_vertCoord_loc = gl_backend->GetAttribLocation(gl_backend->color_prog_id, "vertCoord");
    gl_backend->color_prog_color_loc = gl_backend->GetUniformLocation(gl_backend->color_prog_id, "color");

    gl_backend->GenBuffers(1, &gl_backend->vbo);

    if (!gl_backend->vbo) {
        VIGS_LOG_CRITICAL("cannot create VBO");
        goto fail;
    }

    gl_backend->BindBuffer(GL_ARRAY_BUFFER, gl_backend->vbo);

    gl_backend->UseProgram(gl_backend->tex_prog_id);
    gl_backend->cur_prog_id = gl_backend->tex_prog_id;

    gl_backend->GenBuffers(1, &gl_backend->pbo);

    if (!gl_backend->pbo) {
        VIGS_LOG_CRITICAL("cannot create read_pixels PBO");
        goto fail;
    }

    gl_backend->Disable(GL_DEPTH_TEST);
    gl_backend->Disable(GL_BLEND);

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
        gl_backend->DeleteBuffers(1, &gl_backend->vbo);
        gl_backend->DetachShader(gl_backend->color_prog_id,
                                 gl_backend->color_prog_vs_id);
        gl_backend->DetachShader(gl_backend->color_prog_id,
                                 gl_backend->color_prog_fs_id);
        gl_backend->DeleteShader(gl_backend->color_prog_vs_id);
        gl_backend->DeleteShader(gl_backend->color_prog_fs_id);
        gl_backend->DeleteProgram(gl_backend->color_prog_id);
        gl_backend->DetachShader(gl_backend->tex_prog_id,
                                 gl_backend->tex_prog_vs_id);
        gl_backend->DetachShader(gl_backend->tex_prog_id,
                                 gl_backend->tex_prog_fs_id);
        gl_backend->DeleteShader(gl_backend->tex_prog_vs_id);
        gl_backend->DeleteShader(gl_backend->tex_prog_fs_id);
        gl_backend->DeleteProgram(gl_backend->tex_prog_id);

        if (!gl_backend->is_gl_2) {
            gl_backend->DeleteVertexArrays(1, &gl_backend->vao);
        }

        gl_backend->make_current(gl_backend, false);
    }

    vigs_vector_cleanup(&gl_backend->v2);
    vigs_vector_cleanup(&gl_backend->v1);
}
