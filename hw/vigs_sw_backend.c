#include "vigs_surface.h"
#include "vigs_id_gen.h"
#include "vigs_log.h"
#include "vigs_utils.h"
#include "vigs_ref.h"
#include "vigs_backend.h"
#include "winsys.h"
#include "vigs_vector.h"

typedef struct VigsWinsysSWSurface {
    struct winsys_surface base;
    struct vigs_ref ref;
} VigsWinsysSWSurface;

typedef struct VigsSWSurface
{
    struct vigs_surface base;
    uint8_t *data;
} VigsSWSurface;

static void vigs_winsys_sw_surface_acquire(struct winsys_surface *sfc)
{
    VigsWinsysSWSurface *vigs_sfc = (VigsWinsysSWSurface *)sfc;
    vigs_ref_acquire(&vigs_sfc->ref);
}

static void vigs_winsys_sw_surface_release(struct winsys_surface *sfc)
{
    VigsWinsysSWSurface *vigs_sfc = (VigsWinsysSWSurface *)sfc;
    vigs_ref_release(&vigs_sfc->ref);
}

static void vigs_winsys_sw_surface_destroy(struct vigs_ref *ref)
{
    VigsWinsysSWSurface *vigs_sfc = container_of(ref, VigsWinsysSWSurface, ref);

    vigs_ref_cleanup(&vigs_sfc->ref);
    g_free(vigs_sfc);
}

static void vigs_sw_surface_update(struct vigs_surface *sfc,
                                   vigsp_offset vram_offset,
                                   uint8_t *data)
{
    VigsSWSurface *sw_sfc = (VigsSWSurface *)sfc;

    VIGS_LOG_TRACE("update surface %d: VRAM offset=%d, addr=%p",
                   sfc->id, vram_offset, data);

    assert(data && vram_offset >= 0);

    if (vram_offset == sfc->vram_offset) {
        return;
    }

    memcpy(data, sw_sfc->data, sfc->stride * sfc->height);

    if (sfc->vram_offset < 0) {
        g_free(sw_sfc->data);
        sw_sfc->data = NULL;
    }

    sfc->vram_offset = vram_offset;
    sw_sfc->data = sfc->data = data;
}

static void vigs_sw_surface_set_data(struct vigs_surface *sfc,
                                     vigsp_offset vram_offset,
                                     uint8_t *data)
{
    VigsSWSurface *sw_sfc = (VigsSWSurface *)sfc;

    VIGS_LOG_TRACE("set data of surface %d: VRAM offset=%d, addr=%p",
                   sfc->id, vram_offset, data);

    assert(data && vram_offset >= 0);

    if (sfc->vram_offset < 0) {
        g_free(sw_sfc->data);
        sw_sfc->data = NULL;
    }
    sfc->vram_offset = vram_offset;
    sw_sfc->data = sfc->data = data;
}

static void vigs_sw_surface_read_pixels(struct vigs_surface *sfc,
                                        uint32_t x,
                                        uint32_t y,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t stride,
                                        uint8_t *pixels)
{
    VigsSWSurface *sw_sfc = (VigsSWSurface *)sfc;
    const unsigned bpp = vigs_format_bpp(sfc->format);
    unsigned line;
    const uint8_t *copy_from;

    VIGS_LOG_TRACE("read %dx%d region from (%d, %d) of surface %d (stride=%d)",
                   width, height, x, y, sfc->id, stride);

    if (pixels == sw_sfc->data) {
        return;
    }

    copy_from = sw_sfc->data + y * sfc->stride + x * bpp;

    for (line = 0; line < height; ++line) {
        memcpy(pixels, copy_from, width * bpp);
        pixels += stride;
        copy_from += sfc->stride;
    }
}

static void vigs_sw_surface_copy(struct vigs_surface *dst,
                                 struct vigs_surface *src,
                                 const struct vigsp_copy *entries,
                                 uint32_t num_entries)
{
    VigsSWSurface *sw_dst = (VigsSWSurface *)dst;
    VigsSWSurface *sw_src = (VigsSWSurface *)src;
    const unsigned dst_stride = dst->stride;
    const unsigned src_stride = src->stride;
    const unsigned bpp = vigs_format_bpp(dst->format);
    uint8_t *dst_data;
    const uint8_t *src_data;
    unsigned i, line;

    VIGS_LOG_TRACE("copy %d regions of surface %d to surface %d",
                    num_entries, src->id, dst->id);

    if (dst->format != src->format) {
        VIGS_LOG_ERROR("Source and destination formats differ");
        return;
    }

    for (i = 0; i < num_entries; ++i) {
        /* In case we're copying overlapping regions of the same image */
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
    VigsSWSurface *sw_sfc = (VigsSWSurface *)sfc;
    const unsigned bpp = vigs_format_bpp(sfc->format);
    const unsigned stride = sfc->stride;
    uint8_t *first_line, *line_data;
    unsigned i, entry;

    VIGS_LOG_TRACE("fill %d regions of surface %d with color 0x%x",
                    num_entries, sfc->id, color);

    switch (sfc->format) {
    case vigsp_surface_bgra8888:
        break;
    case vigsp_surface_bgrx8888:
        color |= 0xff << 24;
        break;
    default:
        hw_error("Unknown color format %d\n", sfc->format);
        break;
    }

    for (entry = 0; entry < num_entries; ++entry) {
        first_line = sw_sfc->data + entries[entry].pos.y * stride;
        line_data = first_line;
        i = entries[entry].pos.x;
        switch (bpp) {
        case 4:
            for (; i < (entries[entry].pos.x + entries[entry].size.w); ++i) {
                ((uint32_t *)line_data)[i] = color;
            }
            break;
        case 2:
            for (; i < (entries[entry].pos.x + entries[entry].size.w); ++i) {
                ((uint16_t *)line_data)[i] = color;
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

static void vigs_sw_surface_put_image(struct vigs_surface *sfc,
                                      const void *src,
                                      uint32_t src_stride,
                                      const struct vigsp_rect *rect)
{
    VigsSWSurface *sw_sfc = (VigsSWSurface *)sfc;
    const unsigned bpp = vigs_format_bpp(sfc->format);
    unsigned line;
    uint8_t *sfc_data;

    VIGS_LOG_TRACE("image %dx%d put at (%d, %d) of surface %d",
        rect->size.w, rect->size.h, rect->pos.x, rect->pos.y, sfc->id);

    sfc_data = sw_sfc->data + sfc->stride * rect->pos.y + rect->pos.x * bpp;
    for (line = 0; line < rect->size.h; ++line) {
        memcpy(sfc_data, src, rect->size.w * bpp);
        sfc_data += sfc->stride;
        src += src_stride;
    }
}

static void vigs_sw_surface_destroy(struct vigs_surface *sfc)
{
    VigsSWSurface *sw_sfc = (VigsSWSurface *)sfc;

    VIGS_LOG_TRACE("surface %d destroyed", sfc->id);

    if (sfc->vram_offset < 0) {
        g_free(sw_sfc->data);
        sw_sfc->data = NULL;
    }
    vigs_surface_cleanup(&sw_sfc->base);
    g_free(sw_sfc);
}

static VigsWinsysSWSurface *vigs_winsys_sw_surface_create(uint32_t width,
                                                          uint32_t height)
{
    VigsWinsysSWSurface *ws_sfc;

    ws_sfc = g_new0(VigsWinsysSWSurface, 1);

    ws_sfc->base.width = width;
    ws_sfc->base.height = height;
    ws_sfc->base.acquire = &vigs_winsys_sw_surface_acquire;
    ws_sfc->base.release = &vigs_winsys_sw_surface_release;

    vigs_ref_init(&ws_sfc->ref, &vigs_winsys_sw_surface_destroy);

    return ws_sfc;
}

static struct vigs_surface *vigs_sw_backend_create_surface(struct vigs_backend *backend,
                                                           uint32_t width,
                                                           uint32_t height,
                                                           uint32_t stride,
                                                           vigsp_surface_format format,
                                                           vigsp_offset vram_offset,
                                                           uint8_t *data)
{
    VigsSWSurface *sw_sfc = NULL;
    VigsWinsysSWSurface *ws_sfc;

    sw_sfc = g_new0(VigsSWSurface, 1);
    sw_sfc->base.update = &vigs_sw_surface_update;
    sw_sfc->base.set_data = &vigs_sw_surface_set_data;
    sw_sfc->base.read_pixels = &vigs_sw_surface_read_pixels;
    sw_sfc->base.copy = &vigs_sw_surface_copy;
    sw_sfc->base.solid_fill = &vigs_sw_surface_solid_fill;
    sw_sfc->base.put_image = &vigs_sw_surface_put_image;
    sw_sfc->base.destroy = &vigs_sw_surface_destroy;
    ws_sfc = vigs_winsys_sw_surface_create(width, height);
    vigs_surface_init(&sw_sfc->base,
                      &ws_sfc->base,
                      backend,
                      vigs_id_gen(),
                      stride,
                      format,
                      vram_offset,
                      data);

    if (vram_offset < 0) {
        sw_sfc->data = g_malloc(height * stride);
    } else {
        sw_sfc->data = sw_sfc->base.data;
    }

    assert(sw_sfc->data);
    ws_sfc->base.release(&ws_sfc->base);

    VIGS_LOG_TRACE("surface %d created: %dx%d stride=%d, format=%d, offset=%d",
        sw_sfc->base.id, width, height, stride, format, vram_offset);

    return &sw_sfc->base;
}

static void vigs_sw_backend_destroy(struct vigs_backend *sw_backend)
{
    vigs_backend_cleanup(sw_backend);
    g_free(sw_backend);
    VIGS_LOG_DEBUG("VIGS software backend destroyed");
}

struct vigs_backend *vigs_sw_backend_create(void)
{
    struct vigs_backend *sw_backend;

    sw_backend = g_new0(struct vigs_backend, 1);
    vigs_backend_init(sw_backend, NULL);
    sw_backend->destroy = &vigs_sw_backend_destroy;
    sw_backend->create_surface = &vigs_sw_backend_create_surface;
    VIGS_LOG_DEBUG("VIGS software backend created");

    return sw_backend;
}
