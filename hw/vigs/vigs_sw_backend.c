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

#include "vigs_backend.h"
#include "vigs_surface.h"
#include "vigs_log.h"
#include "vigs_utils.h"
#include "vigs_ref.h"
#include "winsys.h"

struct vigs_winsys_sw_surface
{
    struct winsys_surface base;

    struct vigs_ref ref;
};

struct vigs_sw_surface
{
    struct vigs_surface base;

    uint8_t *data;
};

/*
 * vigs_winsys_sw_surface.
 * @{
 */

static void vigs_winsys_sw_surface_acquire(struct winsys_surface *sfc)
{
    struct vigs_winsys_sw_surface *vigs_sfc = (struct vigs_winsys_sw_surface*)sfc;
    vigs_ref_acquire(&vigs_sfc->ref);
}

static void vigs_winsys_sw_surface_release(struct winsys_surface *sfc)
{
    struct vigs_winsys_sw_surface *vigs_sfc = (struct vigs_winsys_sw_surface*)sfc;
    vigs_ref_release(&vigs_sfc->ref);
}

static void vigs_winsys_sw_surface_set_dirty(struct winsys_surface *sfc)
{
}

static void vigs_winsys_sw_surface_destroy(struct vigs_ref *ref)
{
    struct vigs_winsys_sw_surface *vigs_sfc =
        container_of(ref, struct vigs_winsys_sw_surface, ref);

    vigs_ref_cleanup(&vigs_sfc->ref);

    g_free(vigs_sfc);
}

static struct vigs_winsys_sw_surface
    *vigs_winsys_sw_surface_create(uint32_t width,
                                   uint32_t height)
{
    struct vigs_winsys_sw_surface *ws_sfc;

    ws_sfc = g_malloc0(sizeof(*ws_sfc));

    ws_sfc->base.width = width;
    ws_sfc->base.height = height;
    ws_sfc->base.acquire = &vigs_winsys_sw_surface_acquire;
    ws_sfc->base.release = &vigs_winsys_sw_surface_release;
    ws_sfc->base.set_dirty = &vigs_winsys_sw_surface_set_dirty;

    vigs_ref_init(&ws_sfc->ref, &vigs_winsys_sw_surface_destroy);

    return ws_sfc;
}

/*
 * @}
 */

static void vigs_sw_backend_batch_start(struct vigs_backend *backend)
{
}

/*
 * vigs_sw_surface.
 * @{
 */

static void vigs_sw_surface_read_pixels(struct vigs_surface *sfc,
                                        uint8_t *pixels)
{
    struct vigs_sw_surface *sw_sfc = (struct vigs_sw_surface*)sfc;

    memcpy(pixels, sw_sfc->data, sfc->stride * sfc->ws_sfc->height);
}

static void vigs_sw_surface_draw_pixels(struct vigs_surface *sfc,
                                        uint8_t *pixels,
                                        const struct vigsp_rect *entries,
                                        uint32_t num_entries)
{
    struct vigs_sw_surface *sw_sfc = (struct vigs_sw_surface*)sfc;
    uint32_t bpp = vigs_format_bpp(sfc->format);
    uint32_t i, j;

    for (i = 0; i < num_entries; ++i) {
        uint32_t x = entries[i].pos.x;
        uint32_t y = entries[i].pos.y;
        uint32_t width = entries[i].size.w;
        uint32_t height = entries[i].size.h;
        uint32_t row_length = width * bpp;
        const uint8_t *src;
        uint8_t *dest;

        VIGS_LOG_TRACE("x = %u, y = %u, width = %u, height = %u",
                       x, y,
                       width, height);

        src = pixels + y * sfc->stride + x * bpp;
        dest = sw_sfc->data + y * sfc->stride + x * bpp;

        if (width == sfc->ws_sfc->width) {
            row_length = sfc->stride * height;
            height = 1;
        }

        for (j = 0; j < height; ++j) {
            memcpy(dest, src, row_length);
            src += sfc->stride;
            dest += sfc->stride;
        }
    }
}

static void vigs_sw_surface_copy(struct vigs_surface *dst,
                                 struct vigs_surface *src,
                                 const struct vigsp_copy *entries,
                                 uint32_t num_entries)
{
    struct vigs_sw_surface *sw_dst = (struct vigs_sw_surface*)dst;
    struct vigs_sw_surface *sw_src = (struct vigs_sw_surface*)src;
    uint32_t dst_stride = dst->stride;
    uint32_t src_stride = src->stride;
    uint32_t bpp = vigs_format_bpp(dst->format);
    uint8_t *dst_data;
    const uint8_t *src_data;
    uint32_t i, line;

    VIGS_LOG_TRACE("copy %d regions of surface %d to surface %d",
                    num_entries, src->id, dst->id);

    if (dst->format != src->format) {
        VIGS_LOG_ERROR("Source and destination formats differ");
        return;
    }

    for (i = 0; i < num_entries; ++i) {
        /*
         * In case we're copying overlapping regions of the same image.
         */
        if (entries[i].from.y < entries[i].to.y) {
            dst_data = sw_dst->data +
                       (entries[i].to.y + entries[i].size.h - 1) * dst_stride +
                       entries[i].to.x * bpp;
            src_data = sw_src->data +
                       (entries[i].from.y + entries[i].size.h - 1) * src_stride +
                       entries[i].from.x * bpp;

            for (line = entries[i].size.h; line; --line) {
                memcpy(dst_data, src_data, entries[i].size.w * bpp);
                dst_data -= dst_stride;
                src_data -= src_stride;
            }
        } else {
            dst_data = sw_dst->data + entries[i].to.y * dst_stride +
                       entries[i].to.x * bpp;
            src_data = sw_src->data + entries[i].from.y * src_stride +
                       entries[i].from.x * bpp;

            for (line = 0; line < entries[i].size.h; ++line) {
                memmove(dst_data, src_data, entries[i].size.w * bpp);
                dst_data += dst_stride;
                src_data += src_stride;
            }
        }
    }
}

static void vigs_sw_surface_solid_fill(struct vigs_surface *sfc,
                                       vigsp_color color,
                                       const struct vigsp_rect *entries,
                                       uint32_t num_entries)
{
    struct vigs_sw_surface *sw_sfc = (struct vigs_sw_surface*)sfc;
    uint32_t bpp = vigs_format_bpp(sfc->format);
    uint32_t stride = sfc->stride;
    uint8_t *first_line, *line_data;
    uint32_t i, entry;

    VIGS_LOG_TRACE("Fill %d regions of surface %d with color 0x%x",
                    num_entries, sfc->id, color);

    switch (sfc->format) {
    case vigsp_surface_bgra8888:
        break;
    case vigsp_surface_bgrx8888:
        color |= 0xff << 24;
        break;
    default:
        VIGS_LOG_ERROR("Unknown color format %d\n", sfc->format);
        break;
    }

    for (entry = 0; entry < num_entries; ++entry) {
        first_line = sw_sfc->data + entries[entry].pos.y * stride;
        line_data = first_line;
        i = entries[entry].pos.x;
        switch (bpp) {
        case 4:
            for (; i < (entries[entry].pos.x + entries[entry].size.w); ++i) {
                ((uint32_t*)line_data)[i] = color;
            }
            break;
        default:
            memset(&line_data[i], color, entries[entry].size.w);
            break;
        }

        line_data += stride;
        for (i = 1; i < entries[entry].size.h; ++i) {
            memcpy(&line_data[entries[entry].pos.x * bpp],
                   &first_line[entries[entry].pos.x * bpp],
                   entries[entry].size.w * bpp);
            line_data += stride;
        }
    }
}

static void vigs_sw_surface_destroy(struct vigs_surface *sfc)
{
    struct vigs_sw_surface *sw_sfc = (struct vigs_sw_surface*)sfc;

    g_free(sw_sfc->data);

    vigs_surface_cleanup(&sw_sfc->base);

    g_free(sw_sfc);
}

/*
 * @}
 */

static struct vigs_surface *vigs_sw_backend_create_surface(struct vigs_backend *backend,
                                                           uint32_t width,
                                                           uint32_t height,
                                                           uint32_t stride,
                                                           vigsp_surface_format format,
                                                           vigsp_surface_id id)
{
    struct vigs_sw_surface *sw_sfc = NULL;
    struct vigs_winsys_sw_surface *ws_sfc = NULL;

    sw_sfc = g_malloc0(sizeof(*sw_sfc));

    sw_sfc->data = g_malloc(stride * height);

    ws_sfc = vigs_winsys_sw_surface_create(width, height);

    vigs_surface_init(&sw_sfc->base,
                      &ws_sfc->base,
                      backend,
                      stride,
                      format,
                      id);

    ws_sfc->base.release(&ws_sfc->base);

    sw_sfc->base.read_pixels = &vigs_sw_surface_read_pixels;
    sw_sfc->base.draw_pixels = &vigs_sw_surface_draw_pixels;
    sw_sfc->base.copy = &vigs_sw_surface_copy;
    sw_sfc->base.solid_fill = &vigs_sw_surface_solid_fill;
    sw_sfc->base.destroy = &vigs_sw_surface_destroy;

    return &sw_sfc->base;
}

static void vigs_sw_backend_composite(struct vigs_surface *surface,
                                      const struct vigs_plane *planes,
                                      vigs_composite_start_cb start_cb,
                                      vigs_composite_end_cb end_cb,
                                      void *user_data)
{
    struct vigs_sw_surface *sw_sfc = (struct vigs_sw_surface*)surface;
    uint8_t *buff;

    /*
     * TODO: Render planes.
     */

    buff = start_cb(user_data, surface->ws_sfc->width, surface->ws_sfc->height,
                    surface->stride, surface->format);

    if (surface->ptr) {
        memcpy(buff,
               surface->ptr,
               surface->stride * surface->ws_sfc->height);
    } else if (surface->is_dirty) {
        memcpy(buff,
               sw_sfc->data,
               surface->stride * surface->ws_sfc->height);
    }

    end_cb(user_data, true);
}

static void vigs_sw_backend_batch_end(struct vigs_backend *backend)
{
}

static void vigs_sw_backend_destroy(struct vigs_backend *backend)
{
    vigs_backend_cleanup(backend);
    g_free(backend);
}

struct vigs_backend *vigs_sw_backend_create(void)
{
    struct vigs_backend *backend;

    backend = g_malloc0(sizeof(*backend));

    vigs_backend_init(backend, NULL);

    backend->batch_start = &vigs_sw_backend_batch_start;
    backend->create_surface = &vigs_sw_backend_create_surface;
    backend->composite = &vigs_sw_backend_composite;
    backend->batch_end = &vigs_sw_backend_batch_end;
    backend->destroy = &vigs_sw_backend_destroy;

    return backend;
}
