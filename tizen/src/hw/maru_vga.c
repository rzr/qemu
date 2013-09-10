/*
 * Maru vga device
 * Based on qemu/hw/vga.c
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

#include "hw/hw.h"
#include "display/vga.h"
#include "ui/console.h"
#include "hw/i386/pc.h"
#include "hw/pci/pci.h"
#include "display/vga_int.h"
#include "ui/pixel_ops.h"
#include "qemu/timer.h"
#include "hw/xen/xen.h"
#include "trace.h"

#include <pthread.h>

#define MARU_VGA
#include "maru_common.h"
#include "maru_vga_int.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, maru_vga);

//#define DEBUG_VGA
//#define DEBUG_VGA_MEM
//#define DEBUG_VGA_REG

//#define DEBUG_BOCHS_VBE

/* 16 state changes per vertical frame @60 Hz */
#define VGA_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)

#define cbswap_32(__x) \
((uint32_t)( \
        (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
        (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
        (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
        (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) ))

#ifdef HOST_WORDS_BIGENDIAN
#define PAT(x) cbswap_32(x)
#else
#define PAT(x) (x)
#endif

#ifdef HOST_WORDS_BIGENDIAN
#define BIG 1
#else
#define BIG 0
#endif

#ifdef HOST_WORDS_BIGENDIAN
#define GET_PLANE(data, p) (((data) >> (24 - (p) * 8)) & 0xff)
#else
#define GET_PLANE(data, p) (((data) >> ((p) * 8)) & 0xff)
#endif

static const uint32_t mask16[16] = {
    PAT(0x00000000),
    PAT(0x000000ff),
    PAT(0x0000ff00),
    PAT(0x0000ffff),
    PAT(0x00ff0000),
    PAT(0x00ff00ff),
    PAT(0x00ffff00),
    PAT(0x00ffffff),
    PAT(0xff000000),
    PAT(0xff0000ff),
    PAT(0xff00ff00),
    PAT(0xff00ffff),
    PAT(0xffff0000),
    PAT(0xffff00ff),
    PAT(0xffffff00),
    PAT(0xffffffff),
};

#undef PAT

#ifdef HOST_WORDS_BIGENDIAN
#define PAT(x) (x)
#else
#define PAT(x) cbswap_32(x)
#endif

static const uint32_t dmask16[16] = {
    PAT(0x00000000),
    PAT(0x000000ff),
    PAT(0x0000ff00),
    PAT(0x0000ffff),
    PAT(0x00ff0000),
    PAT(0x00ff00ff),
    PAT(0x00ffff00),
    PAT(0x00ffffff),
    PAT(0xff000000),
    PAT(0xff0000ff),
    PAT(0xff00ff00),
    PAT(0xff00ffff),
    PAT(0xffff0000),
    PAT(0xffff00ff),
    PAT(0xffffff00),
    PAT(0xffffffff),
};

static const uint32_t dmask4[4] = {
    PAT(0x00000000),
    PAT(0x0000ffff),
    PAT(0xffff0000),
    PAT(0xffffffff),
};

static uint32_t expand4[256];
static uint16_t expand2[256];
static uint8_t expand4to8[16];

static void vga_dumb_update_retrace_info(VGACommonState *s)
{
    (void) s;
}

static void vga_precise_update_retrace_info(VGACommonState *s)
{
    int htotal_chars;
    int hretr_start_char;
    int hretr_skew_chars;
    int hretr_end_char;

    int vtotal_lines;
    int vretr_start_line;
    int vretr_end_line;

    int dots;
#if 0
    int div2, sldiv2;
#endif
    int clocking_mode;
    int clock_sel;
    const int clk_hz[] = {25175000, 28322000, 25175000, 25175000};
    int64_t chars_per_sec;
    struct vga_precise_retrace *r = &s->retrace_info.precise;

    htotal_chars = s->cr[VGA_CRTC_H_TOTAL] + 5;
    hretr_start_char = s->cr[VGA_CRTC_H_SYNC_START];
    hretr_skew_chars = (s->cr[VGA_CRTC_H_SYNC_END] >> 5) & 3;
    hretr_end_char = s->cr[VGA_CRTC_H_SYNC_END] & 0x1f;

    vtotal_lines = (s->cr[VGA_CRTC_V_TOTAL] |
                    (((s->cr[VGA_CRTC_OVERFLOW] & 1) |
                      ((s->cr[VGA_CRTC_OVERFLOW] >> 4) & 2)) << 8)) + 2;
    vretr_start_line = s->cr[VGA_CRTC_V_SYNC_START] |
        ((((s->cr[VGA_CRTC_OVERFLOW] >> 2) & 1) |
          ((s->cr[VGA_CRTC_OVERFLOW] >> 6) & 2)) << 8);
    vretr_end_line = s->cr[VGA_CRTC_V_SYNC_END] & 0xf;

    clocking_mode = (s->sr[VGA_SEQ_CLOCK_MODE] >> 3) & 1;
    clock_sel = (s->msr >> 2) & 3;
    dots = (s->msr & 1) ? 8 : 9;

    chars_per_sec = clk_hz[clock_sel] / dots;

    htotal_chars <<= clocking_mode;

    r->total_chars = vtotal_lines * htotal_chars;
    if (r->freq) {
        r->ticks_per_char = get_ticks_per_sec() / (r->total_chars * r->freq);
    } else {
        r->ticks_per_char = get_ticks_per_sec() / chars_per_sec;
    }

    r->vstart = vretr_start_line;
    r->vend = r->vstart + vretr_end_line + 1;

    r->hstart = hretr_start_char + hretr_skew_chars;
    r->hend = r->hstart + hretr_end_char + 1;
    r->htotal = htotal_chars;

#if 0
    div2 = (s->cr[VGA_CRTC_MODE] >> 2) & 1;
    sldiv2 = (s->cr[VGA_CRTC_MODE] >> 3) & 1;
    printf (
        "hz=%f\n"
        "htotal = %d\n"
        "hretr_start = %d\n"
        "hretr_skew = %d\n"
        "hretr_end = %d\n"
        "vtotal = %d\n"
        "vretr_start = %d\n"
        "vretr_end = %d\n"
        "div2 = %d sldiv2 = %d\n"
        "clocking_mode = %d\n"
        "clock_sel = %d %d\n"
        "dots = %d\n"
        "ticks/char = %" PRId64 "\n"
        "\n",
        (double) get_ticks_per_sec() / (r->ticks_per_char * r->total_chars),
        htotal_chars,
        hretr_start_char,
        hretr_skew_chars,
        hretr_end_char,
        vtotal_lines,
        vretr_start_line,
        vretr_end_line,
        div2, sldiv2,
        clocking_mode,
        clock_sel,
        clk_hz[clock_sel],
        dots,
        r->ticks_per_char
        );
#endif
}

static uint8_t vga_precise_retrace(VGACommonState *s)
{
    struct vga_precise_retrace *r = &s->retrace_info.precise;
    uint8_t val = s->st01 & ~(ST01_V_RETRACE | ST01_DISP_ENABLE);

    if (r->total_chars) {
        int cur_line, cur_line_char, cur_char;
        int64_t cur_tick;

        cur_tick = qemu_get_clock_ns(vm_clock);

        cur_char = (cur_tick / r->ticks_per_char) % r->total_chars;
        cur_line = cur_char / r->htotal;

        if (cur_line >= r->vstart && cur_line <= r->vend) {
            val |= ST01_V_RETRACE | ST01_DISP_ENABLE;
        } else {
            cur_line_char = cur_char % r->htotal;
            if (cur_line_char >= r->hstart && cur_line_char <= r->hend) {
                val |= ST01_DISP_ENABLE;
            }
        }

        return val;
    } else {
        return s->st01 ^ (ST01_V_RETRACE | ST01_DISP_ENABLE);
    }
}

static uint8_t vga_dumb_retrace(VGACommonState *s)
{
    return s->st01 ^ (ST01_V_RETRACE | ST01_DISP_ENABLE);
}

typedef void maru_vga_draw_glyph8_func(uint8_t *d, int linesize,
                             const uint8_t *font_ptr, int h,
                             uint32_t fgcol, uint32_t bgcol);
typedef void maru_vga_draw_glyph9_func(uint8_t *d, int linesize,
                                  const uint8_t *font_ptr, int h,
                                  uint32_t fgcol, uint32_t bgcol, int dup9);
typedef void maru_vga_draw_line_func(VGACommonState *s1, uint8_t *d,
                                const uint8_t *s, int width);

#define DEPTH 8
#include "maru_vga_template.h"

#define DEPTH 15
#include "maru_vga_template.h"

#define BGR_FORMAT
#define DEPTH 15
#include "maru_vga_template.h"

#define DEPTH 16
#include "maru_vga_template.h"

#define BGR_FORMAT
#define DEPTH 16
#include "maru_vga_template.h"

#define DEPTH 32
#include "maru_vga_template.h"

#define BGR_FORMAT
#define DEPTH 32
#include "maru_vga_template.h"

static unsigned int rgb_to_pixel8_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel8(r, g, b);
    col |= col << 8;
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel15_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel15(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel15bgr_dup(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    unsigned int col;
    col = rgb_to_pixel15bgr(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel16_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel16(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel16bgr_dup(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    unsigned int col;
    col = rgb_to_pixel16bgr(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel32_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel32(r, g, b);
    return col;
}

static unsigned int rgb_to_pixel32bgr_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel32bgr(r, g, b);
    return col;
}

/* return true if the palette was modified */
static int update_palette16(VGACommonState *s)
{
    int full_update, i;
    uint32_t v, col, *palette;

    full_update = 0;
    palette = s->last_palette;
    for(i = 0; i < 16; i++) {
        v = s->ar[i];
        if (s->ar[VGA_ATC_MODE] & 0x80) {
            v = ((s->ar[VGA_ATC_COLOR_PAGE] & 0xf) << 4) | (v & 0xf);
        } else {
            v = ((s->ar[VGA_ATC_COLOR_PAGE] & 0xc) << 4) | (v & 0x3f);
        }
        v = v * 3;
        col = s->rgb_to_pixel(c6_to_8(s->palette[v]),
                              c6_to_8(s->palette[v + 1]),
                              c6_to_8(s->palette[v + 2]));
        if (col != palette[i]) {
            full_update = 1;
            palette[i] = col;
        }
    }
    return full_update;
}

/* return true if the palette was modified */
static int update_palette256(VGACommonState *s)
{
    int full_update, i;
    uint32_t v, col, *palette;

    full_update = 0;
    palette = s->last_palette;
    v = 0;
    for(i = 0; i < 256; i++) {
        if (s->dac_8bit) {
          col = s->rgb_to_pixel(s->palette[v],
                                s->palette[v + 1],
                                s->palette[v + 2]);
        } else {
          col = s->rgb_to_pixel(c6_to_8(s->palette[v]),
                                c6_to_8(s->palette[v + 1]),
                                c6_to_8(s->palette[v + 2]));
        }
        if (col != palette[i]) {
            full_update = 1;
            palette[i] = col;
        }
        v += 3;
    }
    return full_update;
}

static void vga_get_offsets(VGACommonState *s,
                            uint32_t *pline_offset,
                            uint32_t *pstart_addr,
                            uint32_t *pline_compare)
{
    uint32_t start_addr, line_offset, line_compare;

    if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED) {
        line_offset = s->vbe_line_offset;
        start_addr = s->vbe_start_addr;
        line_compare = 65535;
    } else {
        /* compute line_offset in bytes */
        line_offset = s->cr[VGA_CRTC_OFFSET];
        line_offset <<= 3;

        /* starting address */
        start_addr = s->cr[VGA_CRTC_START_LO] |
            (s->cr[VGA_CRTC_START_HI] << 8);

        /* line compare */
        line_compare = s->cr[VGA_CRTC_LINE_COMPARE] |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x10) << 4) |
            ((s->cr[VGA_CRTC_MAX_SCAN] & 0x40) << 3);
    }
    *pline_offset = line_offset;
    *pstart_addr = start_addr;
    *pline_compare = line_compare;
}

/* update start_addr and line_offset. Return TRUE if modified */
static int update_basic_params(VGACommonState *s)
{
    int full_update;
    uint32_t start_addr, line_offset, line_compare;

    full_update = 0;

    s->get_offsets(s, &line_offset, &start_addr, &line_compare);

    if (line_offset != s->line_offset ||
        start_addr != s->start_addr ||
        line_compare != s->line_compare) {
        s->line_offset = line_offset;
        s->start_addr = start_addr;
        s->line_compare = line_compare;
        full_update = 1;
    }
    return full_update;
}

#define NB_DEPTHS 7

static inline int get_depth_index(DisplaySurface *s)
{
    switch (surface_bits_per_pixel(s)) {
    default:
    case 8:
        return 0;
    case 15:
        return 1;
    case 16:
        return 2;
    case 32:
        if (is_surface_bgr(s)) {
            return 4;
        } else {
            return 3;
        }
    }
}

static maru_vga_draw_glyph8_func * const maru_vga_draw_glyph8_table[NB_DEPTHS] = {
    maru_vga_draw_glyph8_8,
    maru_vga_draw_glyph8_16,
    maru_vga_draw_glyph8_16,
    maru_vga_draw_glyph8_32,
    maru_vga_draw_glyph8_32,
    maru_vga_draw_glyph8_16,
    maru_vga_draw_glyph8_16,
};

static maru_vga_draw_glyph8_func * const maru_vga_draw_glyph16_table[NB_DEPTHS] = {
    maru_vga_draw_glyph16_8,
    maru_vga_draw_glyph16_16,
    maru_vga_draw_glyph16_16,
    maru_vga_draw_glyph16_32,
    maru_vga_draw_glyph16_32,
    maru_vga_draw_glyph16_16,
    maru_vga_draw_glyph16_16,
};

static maru_vga_draw_glyph9_func * const maru_vga_draw_glyph9_table[NB_DEPTHS] = {
    maru_vga_draw_glyph9_8,
    maru_vga_draw_glyph9_16,
    maru_vga_draw_glyph9_16,
    maru_vga_draw_glyph9_32,
    maru_vga_draw_glyph9_32,
    maru_vga_draw_glyph9_16,
    maru_vga_draw_glyph9_16,
};

static const uint8_t cursor_glyph[32 * 4] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static void vga_get_text_resolution(VGACommonState *s, int *pwidth, int *pheight,
                                    int *pcwidth, int *pcheight)
{
    int width, cwidth, height, cheight;

    /* total width & height */
    cheight = (s->cr[VGA_CRTC_MAX_SCAN] & 0x1f) + 1;
    cwidth = 8;
    if (!(s->sr[VGA_SEQ_CLOCK_MODE] & VGA_SR01_CHAR_CLK_8DOTS)) {
        cwidth = 9;
    }
    if (s->sr[VGA_SEQ_CLOCK_MODE] & 0x08) {
        cwidth = 16; /* NOTE: no 18 pixel wide */
    }
    width = (s->cr[VGA_CRTC_H_DISP] + 1);
    if (s->cr[VGA_CRTC_V_TOTAL] == 100) {
        /* ugly hack for CGA 160x100x16 - explain me the logic */
        height = 100;
    } else {
        height = s->cr[VGA_CRTC_V_DISP_END] |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x02) << 7) |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x40) << 3);
        height = (height + 1) / cheight;
    }

    *pwidth = width;
    *pheight = height;
    *pcwidth = cwidth;
    *pcheight = cheight;
}

typedef unsigned int rgb_to_pixel_dup_func(unsigned int r, unsigned int g, unsigned b);

static rgb_to_pixel_dup_func * const rgb_to_pixel_dup_table[NB_DEPTHS] = {
    rgb_to_pixel8_dup,
    rgb_to_pixel15_dup,
    rgb_to_pixel16_dup,
    rgb_to_pixel32_dup,
    rgb_to_pixel32bgr_dup,
    rgb_to_pixel15bgr_dup,
    rgb_to_pixel16bgr_dup,
};

/*
 * Text mode update
 * Missing:
 * - double scan
 * - double width
 * - underline
 * - flashing
 */
static void vga_draw_text(VGACommonState *s, int full_update)
{
    DisplaySurface *surface = qemu_console_surface(s->con);
    int cx, cy, cheight, cw, ch, cattr, height, width, ch_attr;
    int cx_min, cx_max, linesize, x_incr, line, line1;
    uint32_t offset, fgcol, bgcol, v, cursor_offset;
    uint8_t *d1, *d, *src, *dest, *cursor_ptr;
    const uint8_t *font_ptr, *font_base[2];
    int dup9, line_offset, depth_index;
    uint32_t *palette;
    uint32_t *ch_attr_ptr;
    maru_vga_draw_glyph8_func *maru_vga_draw_glyph8;
    maru_vga_draw_glyph9_func *maru_vga_draw_glyph9;
    int64_t now = qemu_get_clock_ms(vm_clock);

    /* compute font data address (in plane 2) */
    v = s->sr[VGA_SEQ_CHARACTER_MAP];
    offset = (((v >> 4) & 1) | ((v << 1) & 6)) * 8192 * 4 + 2;
    if (offset != s->font_offsets[0]) {
        s->font_offsets[0] = offset;
        full_update = 1;
    }
    font_base[0] = s->vram_ptr + offset;

    offset = (((v >> 5) & 1) | ((v >> 1) & 6)) * 8192 * 4 + 2;
    font_base[1] = s->vram_ptr + offset;
    if (offset != s->font_offsets[1]) {
        s->font_offsets[1] = offset;
        full_update = 1;
    }
    if (s->plane_updated & (1 << 2) || s->chain4_alias) {
        /* if the plane 2 was modified since the last display, it
           indicates the font may have been modified */
        s->plane_updated = 0;
        full_update = 1;
    }
    full_update |= update_basic_params(s);

    line_offset = s->line_offset;

    vga_get_text_resolution(s, &width, &height, &cw, &cheight);
    if ((height * width) <= 1) {
        /* better than nothing: exit if transient size is too small */
        return;
    }
    if ((height * width) > CH_ATTR_SIZE) {
        /* better than nothing: exit if transient size is too big */
        return;
    }

    if (width != s->last_width || height != s->last_height ||
        cw != s->last_cw || cheight != s->last_ch || s->last_depth) {
        s->last_scr_width = width * cw;
        s->last_scr_height = height * cheight;
        qemu_console_resize(s->con, s->last_scr_width, s->last_scr_height);
        surface = qemu_console_surface(s->con);
        dpy_text_resize(s->con, width, height);
        s->last_depth = 0;
        s->last_width = width;
        s->last_height = height;
        s->last_ch = cheight;
        s->last_cw = cw;
        full_update = 1;
    }
    s->rgb_to_pixel =
        rgb_to_pixel_dup_table[get_depth_index(surface)];
    full_update |= update_palette16(s);
    palette = s->last_palette;
    x_incr = cw * surface_bytes_per_pixel(surface);

    if (full_update) {
        s->full_update_text = 1;
    }
    if (s->full_update_gfx) {
        s->full_update_gfx = 0;
        full_update |= 1;
    }

    cursor_offset = ((s->cr[VGA_CRTC_CURSOR_HI] << 8) |
                     s->cr[VGA_CRTC_CURSOR_LO]) - s->start_addr;
    if (cursor_offset != s->cursor_offset ||
        s->cr[VGA_CRTC_CURSOR_START] != s->cursor_start ||
        s->cr[VGA_CRTC_CURSOR_END] != s->cursor_end) {
      /* if the cursor position changed, we update the old and new
         chars */
        if (s->cursor_offset < CH_ATTR_SIZE)
            s->last_ch_attr[s->cursor_offset] = -1;
        if (cursor_offset < CH_ATTR_SIZE)
            s->last_ch_attr[cursor_offset] = -1;
        s->cursor_offset = cursor_offset;
        s->cursor_start = s->cr[VGA_CRTC_CURSOR_START];
        s->cursor_end = s->cr[VGA_CRTC_CURSOR_END];
    }
    cursor_ptr = s->vram_ptr + (s->start_addr + cursor_offset) * 4;
    if (now >= s->cursor_blink_time) {
        s->cursor_blink_time = now + VGA_TEXT_CURSOR_PERIOD_MS / 2;
        s->cursor_visible_phase = !s->cursor_visible_phase;
    }

    depth_index = get_depth_index(surface);
    if (cw == 16)
        maru_vga_draw_glyph8 = maru_vga_draw_glyph16_table[depth_index];
    else
        maru_vga_draw_glyph8 = maru_vga_draw_glyph8_table[depth_index];
    maru_vga_draw_glyph9 = maru_vga_draw_glyph9_table[depth_index];

    dest = surface_data(surface);
    linesize = surface_stride(surface);
    ch_attr_ptr = s->last_ch_attr;
    line = 0;
    offset = s->start_addr * 4;
    for(cy = 0; cy < height; cy++) {
        d1 = dest;
        src = s->vram_ptr + offset;
        cx_min = width;
        cx_max = -1;
        for(cx = 0; cx < width; cx++) {
            ch_attr = *(uint16_t *)src;
            if (full_update || ch_attr != *ch_attr_ptr || src == cursor_ptr) {
                if (cx < cx_min)
                    cx_min = cx;
                if (cx > cx_max)
                    cx_max = cx;
                *ch_attr_ptr = ch_attr;
#ifdef HOST_WORDS_BIGENDIAN
                ch = ch_attr >> 8;
                cattr = ch_attr & 0xff;
#else
                ch = ch_attr & 0xff;
                cattr = ch_attr >> 8;
#endif
                font_ptr = font_base[(cattr >> 3) & 1];
                font_ptr += 32 * 4 * ch;
                bgcol = palette[cattr >> 4];
                fgcol = palette[cattr & 0x0f];
                if (cw != 9) {
                    maru_vga_draw_glyph8(d1, linesize,
                                    font_ptr, cheight, fgcol, bgcol);
                } else {
                    dup9 = 0;
                    if (ch >= 0xb0 && ch <= 0xdf &&
                        (s->ar[VGA_ATC_MODE] & 0x04)) {
                        dup9 = 1;
                    }
                    maru_vga_draw_glyph9(d1, linesize,
                                    font_ptr, cheight, fgcol, bgcol, dup9);
                }
                if (src == cursor_ptr &&
                    !(s->cr[VGA_CRTC_CURSOR_START] & 0x20) &&
                    s->cursor_visible_phase) {
                    int line_start, line_last, h;
                    /* draw the cursor */
                    line_start = s->cr[VGA_CRTC_CURSOR_START] & 0x1f;
                    line_last = s->cr[VGA_CRTC_CURSOR_END] & 0x1f;
                    /* XXX: check that */
                    if (line_last > cheight - 1)
                        line_last = cheight - 1;
                    if (line_last >= line_start && line_start < cheight) {
                        h = line_last - line_start + 1;
                        d = d1 + linesize * line_start;
                        if (cw != 9) {
                            maru_vga_draw_glyph8(d, linesize,
                                            cursor_glyph, h, fgcol, bgcol);
                        } else {
                            maru_vga_draw_glyph9(d, linesize,
                                            cursor_glyph, h, fgcol, bgcol, 1);
                        }
                    }
                }
            }
            d1 += x_incr;
            src += 4;
            ch_attr_ptr++;
        }
        if (cx_max != -1) {
            dpy_gfx_update(s->con, cx_min * cw, cy * cheight,
                           (cx_max - cx_min + 1) * cw, cheight);
        }
        dest += linesize * cheight;
        line1 = line + cheight;
        offset += line_offset;
        if (line < s->line_compare && line1 >= s->line_compare) {
            offset = 0;
        }
        line = line1;
    }
}

enum {
    maru_vga_draw_line2,
    maru_vga_draw_line2D2,
    maru_vga_draw_line4,
    maru_vga_draw_line4D2,
    maru_vga_draw_line8D2,
    maru_vga_draw_line8,
    maru_vga_draw_line15,
    maru_vga_draw_line16,
    maru_vga_draw_line24,
    maru_vga_draw_line32,
    maru_vga_draw_line_NB,
};

static maru_vga_draw_line_func * const maru_vga_draw_line_table[NB_DEPTHS * maru_vga_draw_line_NB] = {
    maru_vga_draw_line2_8,
    maru_vga_draw_line2_16,
    maru_vga_draw_line2_16,
    maru_vga_draw_line2_32,
    maru_vga_draw_line2_32,
    maru_vga_draw_line2_16,
    maru_vga_draw_line2_16,

    maru_vga_draw_line2d2_8,
    maru_vga_draw_line2d2_16,
    maru_vga_draw_line2d2_16,
    maru_vga_draw_line2d2_32,
    maru_vga_draw_line2d2_32,
    maru_vga_draw_line2d2_16,
    maru_vga_draw_line2d2_16,

    maru_vga_draw_line4_8,
    maru_vga_draw_line4_16,
    maru_vga_draw_line4_16,
    maru_vga_draw_line4_32,
    maru_vga_draw_line4_32,
    maru_vga_draw_line4_16,
    maru_vga_draw_line4_16,

    maru_vga_draw_line4d2_8,
    maru_vga_draw_line4d2_16,
    maru_vga_draw_line4d2_16,
    maru_vga_draw_line4d2_32,
    maru_vga_draw_line4d2_32,
    maru_vga_draw_line4d2_16,
    maru_vga_draw_line4d2_16,

    maru_vga_draw_line8d2_8,
    maru_vga_draw_line8d2_16,
    maru_vga_draw_line8d2_16,
    maru_vga_draw_line8d2_32,
    maru_vga_draw_line8d2_32,
    maru_vga_draw_line8d2_16,
    maru_vga_draw_line8d2_16,

    maru_vga_draw_line8_8,
    maru_vga_draw_line8_16,
    maru_vga_draw_line8_16,
    maru_vga_draw_line8_32,
    maru_vga_draw_line8_32,
    maru_vga_draw_line8_16,
    maru_vga_draw_line8_16,

    maru_vga_draw_line15_8,
    maru_vga_draw_line15_15,
    maru_vga_draw_line15_16,
    maru_vga_draw_line15_32,
    maru_vga_draw_line15_32bgr,
    maru_vga_draw_line15_15bgr,
    maru_vga_draw_line15_16bgr,

    maru_vga_draw_line16_8,
    maru_vga_draw_line16_15,
    maru_vga_draw_line16_16,
    maru_vga_draw_line16_32,
    maru_vga_draw_line16_32bgr,
    maru_vga_draw_line16_15bgr,
    maru_vga_draw_line16_16bgr,

    maru_vga_draw_line24_8,
    maru_vga_draw_line24_15,
    maru_vga_draw_line24_16,
    maru_vga_draw_line24_32,
    maru_vga_draw_line24_32bgr,
    maru_vga_draw_line24_15bgr,
    maru_vga_draw_line24_16bgr,

    maru_vga_draw_line32_8,
    maru_vga_draw_line32_15,
    maru_vga_draw_line32_16,
    maru_vga_draw_line32_32,
    maru_vga_draw_line32_32bgr,
    maru_vga_draw_line32_15bgr,
    maru_vga_draw_line32_16bgr,
};

static int vga_get_bpp(VGACommonState *s)
{
    int ret;

    if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED) {
        ret = s->vbe_regs[VBE_DISPI_INDEX_BPP];
    } else {
        ret = 0;
    }
    return ret;
}

static void vga_get_resolution(VGACommonState *s, int *pwidth, int *pheight)
{
    int width, height;

    if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED) {
        width = s->vbe_regs[VBE_DISPI_INDEX_XRES];
        height = s->vbe_regs[VBE_DISPI_INDEX_YRES];
    } else {
        width = (s->cr[VGA_CRTC_H_DISP] + 1) * 8;
        height = s->cr[VGA_CRTC_V_DISP_END] |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x02) << 7) |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x40) << 3);
        height = (height + 1);
    }
    *pwidth = width;
    *pheight = height;
}

/*
 * graphic modes
 */
static void vga_draw_graphic(VGACommonState *s, int full_update)
{
    DisplaySurface *surface = qemu_console_surface(s->con);
    int y1, y, update, linesize, y_start, double_scan, mask, depth;
    int width, height, shift_control, line_offset, bwidth, bits;
    ram_addr_t page0, page1, page_min, page_max;
    int disp_width, multi_scan, multi_run;
    uint8_t *d;
    uint32_t v, addr1, addr;
    maru_vga_draw_line_func *maru_vga_draw_line;
#if defined(HOST_WORDS_BIGENDIAN) == defined(TARGET_WORDS_BIGENDIAN)
    static const bool byteswap = false;
#else
    static const bool byteswap = true;
#endif

    full_update |= update_basic_params(s);

    if (!full_update)
        vga_sync_dirty_bitmap(s);

    s->get_resolution(s, &width, &height);
    disp_width = width;

    shift_control = (s->gr[VGA_GFX_MODE] >> 5) & 3;
    double_scan = (s->cr[VGA_CRTC_MAX_SCAN] >> 7);
    if (shift_control != 1) {
        multi_scan = (((s->cr[VGA_CRTC_MAX_SCAN] & 0x1f) + 1) << double_scan)
            - 1;
    } else {
        /* in CGA modes, multi_scan is ignored */
        /* XXX: is it correct ? */
        multi_scan = double_scan;
    }
    multi_run = multi_scan;
    if (shift_control != s->shift_control ||
        double_scan != s->double_scan) {
        full_update = 1;
        s->shift_control = shift_control;
        s->double_scan = double_scan;
    }

    if (shift_control == 0) {
        if (s->sr[VGA_SEQ_CLOCK_MODE] & 8) {
            disp_width <<= 1;
        }
    } else if (shift_control == 1) {
        if (s->sr[VGA_SEQ_CLOCK_MODE] & 8) {
            disp_width <<= 1;
        }
    }

    depth = s->get_bpp(s);
    if (s->line_offset != s->last_line_offset ||
        disp_width != s->last_width ||
        height != s->last_height ||
        s->last_depth != depth) {
        if (depth == 32 || (depth == 16 && !byteswap)) {
#ifdef MARU_VGA /* create new sufrace by g_new0 in MARU VGA */
            TRACE("create the display surface\n");
            surface = qemu_create_displaysurface(disp_width, height);
#else /* MARU_VGA */
            surface = qemu_create_displaysurface_from(disp_width,
                    height, depth, s->line_offset,
                    s->vram_ptr + (s->start_addr * 4), byteswap);
#endif /* MARU_VGA */
            dpy_gfx_replace_surface(s->con, surface);
        } else {
            qemu_console_resize(s->con, disp_width, height);
            surface = qemu_console_surface(s->con);
        }
        s->last_scr_width = disp_width;
        s->last_scr_height = height;
        s->last_width = disp_width;
        s->last_height = height;
        s->last_line_offset = s->line_offset;
        s->last_depth = depth;
        full_update = 1;
    } else if (is_buffer_shared(surface) &&
               (full_update || surface_data(surface) != s->vram_ptr
                + (s->start_addr * 4))) {
        DisplaySurface *surface;
        surface = qemu_create_displaysurface_from(disp_width,
                height, depth, s->line_offset,
                s->vram_ptr + (s->start_addr * 4), byteswap);
        dpy_gfx_replace_surface(s->con, surface);
    }

    s->rgb_to_pixel =
        rgb_to_pixel_dup_table[get_depth_index(surface)];

    if (shift_control == 0) {
        full_update |= update_palette16(s);
        if (s->sr[VGA_SEQ_CLOCK_MODE] & 8) {
            v = maru_vga_draw_line4D2;
        } else {
            v = maru_vga_draw_line4;
        }
        bits = 4;
    } else if (shift_control == 1) {
        full_update |= update_palette16(s);
        if (s->sr[VGA_SEQ_CLOCK_MODE] & 8) {
            v = maru_vga_draw_line2D2;
        } else {
            v = maru_vga_draw_line2;
        }
        bits = 4;
    } else {
        switch(s->get_bpp(s)) {
        default:
        case 0:
            full_update |= update_palette256(s);
            v = maru_vga_draw_line8D2;
            bits = 4;
            break;
        case 8:
            full_update |= update_palette256(s);
            v = maru_vga_draw_line8;
            bits = 8;
            break;
        case 15:
            v = maru_vga_draw_line15;
            bits = 16;
            break;
        case 16:
            v = maru_vga_draw_line16;
            bits = 16;
            break;
        case 24:
            v = maru_vga_draw_line24;
            bits = 24;
            break;
        case 32:
            v = maru_vga_draw_line32;
            bits = 32;
            break;
        }
    }
    maru_vga_draw_line = maru_vga_draw_line_table[v * NB_DEPTHS +
                                        get_depth_index(surface)];

    if (!is_buffer_shared(surface) && s->cursor_invalidate) {
        s->cursor_invalidate(s);
    }

    line_offset = s->line_offset;
#if 0
    printf("w=%d h=%d v=%d line_offset=%d cr[0x09]=0x%02x cr[0x17]=0x%02x linecmp=%d sr[0x01]=0x%02x\n",
           width, height, v, line_offset, s->cr[9], s->cr[VGA_CRTC_MODE],
           s->line_compare, s->sr[VGA_SEQ_CLOCK_MODE]);
#endif
    addr1 = (s->start_addr * 4);
    bwidth = (width * bits + 7) / 8;
    y_start = -1;
    page_min = -1;
    page_max = 0;
    d = surface_data(surface);
    linesize = surface_stride(surface);
    y1 = 0;
    for(y = 0; y < height; y++) {
        addr = addr1;
        if (!(s->cr[VGA_CRTC_MODE] & 1)) {
            int shift;
            /* CGA compatibility handling */
            shift = 14 + ((s->cr[VGA_CRTC_MODE] >> 6) & 1);
            addr = (addr & ~(1 << shift)) | ((y1 & 1) << shift);
        }
        if (!(s->cr[VGA_CRTC_MODE] & 2)) {
            addr = (addr & ~0x8000) | ((y1 & 2) << 14);
        }
        update = full_update;
        page0 = addr;
        page1 = addr + bwidth - 1;
        update |= memory_region_get_dirty(&s->vram, page0, page1 - page0,
                                          DIRTY_MEMORY_VGA);
        /* explicit invalidation for the hardware cursor */
        update |= (s->invalidated_y_table[y >> 5] >> (y & 0x1f)) & 1;
#ifdef MARU_VGA /* needs full update */
        update |= 1;
#endif
        if (update) {
            if (y_start < 0)
                y_start = y;
            if (page0 < page_min)
                page_min = page0;
            if (page1 > page_max)
                page_max = page1;
            if (!(is_buffer_shared(surface))) {
                maru_vga_draw_line(s, d, s->vram_ptr + addr, width);
                if (s->cursor_draw_line)
                    s->cursor_draw_line(s, d, y);
            }
        } else {
            if (y_start >= 0) {
                /* flush to display */
                dpy_gfx_update(s->con, 0, y_start,
                               disp_width, y - y_start);
                y_start = -1;
            }
        }
        if (!multi_run) {
            mask = (s->cr[VGA_CRTC_MODE] & 3) ^ 3;
            if ((y1 & mask) == mask)
                addr1 += line_offset;
            y1++;
            multi_run = multi_scan;
        } else {
            multi_run--;
        }
        /* line compare acts on the displayed lines */
        if (y == s->line_compare)
            addr1 = 0;
        d += linesize;
    }
    if (y_start >= 0) {
        /* flush to display */
        dpy_gfx_update(s->con, 0, y_start,
                       disp_width, y - y_start);
    }
    /* reset modified pages */
    if (page_max >= page_min) {
        memory_region_reset_dirty(&s->vram,
                                  page_min,
                                  page_max - page_min,
                                  DIRTY_MEMORY_VGA);
    }
    memset(s->invalidated_y_table, 0, ((height + 31) >> 5) * 4);
}

static void vga_draw_blank(VGACommonState *s, int full_update)
{
    DisplaySurface *surface = qemu_console_surface(s->con);
    int i, w, val;
    uint8_t *d;

    if (!full_update)
        return;
    if (s->last_scr_width <= 0 || s->last_scr_height <= 0)
        return;

    s->rgb_to_pixel =
        rgb_to_pixel_dup_table[get_depth_index(surface)];
    if (surface_bits_per_pixel(surface) == 8) {
        val = s->rgb_to_pixel(0, 0, 0);
    } else {
        val = 0;
    }
    w = s->last_scr_width * surface_bytes_per_pixel(surface);
    d = surface_data(surface);
    for(i = 0; i < s->last_scr_height; i++) {
        memset(d, val, w);
        d += surface_stride(surface);
    }
    dpy_gfx_update(s->con, 0, 0,
                   s->last_scr_width, s->last_scr_height);
}

#define GMODE_TEXT     0
#define GMODE_GRAPH    1
#define GMODE_BLANK 2

static void vga_update_display(void *opaque)
{
    VGACommonState *s = opaque;
    DisplaySurface *surface = qemu_console_surface(s->con);
    int full_update, graphic_mode;

    qemu_flush_coalesced_mmio_buffer();

    if (surface_bits_per_pixel(surface) == 0) {
        /* nothing to do */
    } else {
        full_update = 0;
        if (!(s->ar_index & 0x20)) {
            graphic_mode = GMODE_BLANK;
        } else {
            graphic_mode = s->gr[VGA_GFX_MISC] & VGA_GR06_GRAPHICS_MODE;
        }
        if (graphic_mode != s->graphic_mode) {
            s->graphic_mode = graphic_mode;
            s->cursor_blink_time = qemu_get_clock_ms(vm_clock);
            full_update = 1;
        }
        switch(graphic_mode) {
        case GMODE_TEXT:
            vga_draw_text(s, full_update);
            break;
        case GMODE_GRAPH:
            vga_draw_graphic(s, full_update);
            break;
        case GMODE_BLANK:
        default:
            vga_draw_blank(s, full_update);
            break;
        }
    }
}

/* force a full display refresh */
static void vga_invalidate_display(void *opaque)
{
    VGACommonState *s = opaque;

    s->last_width = -1;
    s->last_height = -1;
}

#define TEXTMODE_X(x)    ((x) % width)
#define TEXTMODE_Y(x)    ((x) / width)
#define VMEM2CHTYPE(v)   ((v & 0xff0007ff) | \
        ((v & 0x00000800) << 10) | ((v & 0x00007000) >> 1))
/* relay text rendering to the display driver
 * instead of doing a full vga_update_display() */
static void vga_update_text(void *opaque, console_ch_t *chardata)
{
    VGACommonState *s =  opaque;
    int graphic_mode, i, cursor_offset, cursor_visible;
    int cw, cheight, width, height, size, c_min, c_max;
    uint32_t *src;
    console_ch_t *dst, val;
    char msg_buffer[80];
    int full_update = 0;

    qemu_flush_coalesced_mmio_buffer();

    if (!(s->ar_index & 0x20)) {
        graphic_mode = GMODE_BLANK;
    } else {
        graphic_mode = s->gr[VGA_GFX_MISC] & VGA_GR06_GRAPHICS_MODE;
    }
    if (graphic_mode != s->graphic_mode) {
        s->graphic_mode = graphic_mode;
        full_update = 1;
    }
    if (s->last_width == -1) {
        s->last_width = 0;
        full_update = 1;
    }

    switch (graphic_mode) {
    case GMODE_TEXT:
        /* TODO: update palette */
        full_update |= update_basic_params(s);

        /* total width & height */
        cheight = (s->cr[VGA_CRTC_MAX_SCAN] & 0x1f) + 1;
        cw = 8;
        if (!(s->sr[VGA_SEQ_CLOCK_MODE] & VGA_SR01_CHAR_CLK_8DOTS)) {
            cw = 9;
        }
        if (s->sr[VGA_SEQ_CLOCK_MODE] & 0x08) {
            cw = 16; /* NOTE: no 18 pixel wide */
        }
        width = (s->cr[VGA_CRTC_H_DISP] + 1);
        if (s->cr[VGA_CRTC_V_TOTAL] == 100) {
            /* ugly hack for CGA 160x100x16 - explain me the logic */
            height = 100;
        } else {
            height = s->cr[VGA_CRTC_V_DISP_END] |
                ((s->cr[VGA_CRTC_OVERFLOW] & 0x02) << 7) |
                ((s->cr[VGA_CRTC_OVERFLOW] & 0x40) << 3);
            height = (height + 1) / cheight;
        }

        size = (height * width);
        if (size > CH_ATTR_SIZE) {
            if (!full_update)
                return;

            snprintf(msg_buffer, sizeof(msg_buffer), "%i x %i Text mode",
                     width, height);
            break;
        }

        if (width != s->last_width || height != s->last_height ||
            cw != s->last_cw || cheight != s->last_ch) {
            s->last_scr_width = width * cw;
            s->last_scr_height = height * cheight;
            qemu_console_resize(s->con, s->last_scr_width, s->last_scr_height);
            dpy_text_resize(s->con, width, height);
            s->last_depth = 0;
            s->last_width = width;
            s->last_height = height;
            s->last_ch = cheight;
            s->last_cw = cw;
            full_update = 1;
        }

        if (full_update) {
            s->full_update_gfx = 1;
        }
        if (s->full_update_text) {
            s->full_update_text = 0;
            full_update |= 1;
        }

        /* Update "hardware" cursor */
        cursor_offset = ((s->cr[VGA_CRTC_CURSOR_HI] << 8) |
                         s->cr[VGA_CRTC_CURSOR_LO]) - s->start_addr;
        if (cursor_offset != s->cursor_offset ||
            s->cr[VGA_CRTC_CURSOR_START] != s->cursor_start ||
            s->cr[VGA_CRTC_CURSOR_END] != s->cursor_end || full_update) {
            cursor_visible = !(s->cr[VGA_CRTC_CURSOR_START] & 0x20);
            if (cursor_visible && cursor_offset < size && cursor_offset >= 0)
                dpy_text_cursor(s->con,
                                TEXTMODE_X(cursor_offset),
                                TEXTMODE_Y(cursor_offset));
            else
                dpy_text_cursor(s->con, -1, -1);
            s->cursor_offset = cursor_offset;
            s->cursor_start = s->cr[VGA_CRTC_CURSOR_START];
            s->cursor_end = s->cr[VGA_CRTC_CURSOR_END];
        }

        src = (uint32_t *) s->vram_ptr + s->start_addr;
        dst = chardata;

        if (full_update) {
            for (i = 0; i < size; src ++, dst ++, i ++)
                console_write_ch(dst, VMEM2CHTYPE(le32_to_cpu(*src)));

            dpy_text_update(s->con, 0, 0, width, height);
        } else {
            c_max = 0;

            for (i = 0; i < size; src ++, dst ++, i ++) {
                console_write_ch(&val, VMEM2CHTYPE(le32_to_cpu(*src)));
                if (*dst != val) {
                    *dst = val;
                    c_max = i;
                    break;
                }
            }
            c_min = i;
            for (; i < size; src ++, dst ++, i ++) {
                console_write_ch(&val, VMEM2CHTYPE(le32_to_cpu(*src)));
                if (*dst != val) {
                    *dst = val;
                    c_max = i;
                }
            }

            if (c_min <= c_max) {
                i = TEXTMODE_Y(c_min);
                dpy_text_update(s->con, 0, i, width, TEXTMODE_Y(c_max) - i + 1);
            }
        }

        return;
    case GMODE_GRAPH:
        if (!full_update)
            return;

        s->get_resolution(s, &width, &height);
        snprintf(msg_buffer, sizeof(msg_buffer), "%i x %i Graphic mode",
                 width, height);
        break;
    case GMODE_BLANK:
    default:
        if (!full_update)
            return;

        snprintf(msg_buffer, sizeof(msg_buffer), "VGA Blank mode");
        break;
    }

    /* Display a message */
    s->last_width = 60;
    s->last_height = height = 3;
    dpy_text_cursor(s->con, -1, -1);
    dpy_text_resize(s->con, s->last_width, height);

    for (dst = chardata, i = 0; i < s->last_width * height; i ++)
        console_write_ch(dst ++, ' ');

    size = strlen(msg_buffer);
    width = (s->last_width - size) / 2;
    dst = chardata + s->last_width + width;
    for (i = 0; i < size; i ++)
        console_write_ch(dst ++, 0x00200100 | msg_buffer[i]);

    dpy_text_update(s->con, 0, 0, s->last_width, height);
}

static const GraphicHwOps vga_ops = {
    .invalidate  = vga_invalidate_display,
    .gfx_update  = vga_update_display,
    .text_update = vga_update_text,
};

void maru_vga_common_init(VGACommonState *s, Object *obj)
{
    int i, j, v, b;

    for(i = 0;i < 256; i++) {
        v = 0;
        for(j = 0; j < 8; j++) {
            v |= ((i >> j) & 1) << (j * 4);
        }
        expand4[i] = v;

        v = 0;
        for(j = 0; j < 4; j++) {
            v |= ((i >> (2 * j)) & 3) << (j * 4);
        }
        expand2[i] = v;
    }
    for(i = 0; i < 16; i++) {
        v = 0;
        for(j = 0; j < 4; j++) {
            b = ((i >> j) & 1);
            v |= b << (2 * j);
            v |= b << (2 * j + 1);
        }
        expand4to8[i] = v;
    }

    /* valid range: 1 MB -> 256 MB */
    s->vram_size = 1024 * 1024;
    while (s->vram_size < (s->vram_size_mb << 20) &&
           s->vram_size < (256 << 20)) {
        s->vram_size <<= 1;
    }
    s->vram_size_mb = s->vram_size >> 20;

    s->is_vbe_vmstate = 1;
    memory_region_init_ram(&s->vram, obj, "maru_vga.vram", s->vram_size);
    vmstate_register_ram_global(&s->vram);
    xen_register_framebuffer(&s->vram);
    s->vram_ptr = memory_region_get_ram_ptr(&s->vram);
    s->get_bpp = vga_get_bpp;
    s->get_offsets = vga_get_offsets;
    s->get_resolution = vga_get_resolution;
    s->hw_ops = &vga_ops;
    switch (vga_retrace_method) {
    case VGA_RETRACE_DUMB:
        s->retrace = vga_dumb_retrace;
        s->update_retrace_info = vga_dumb_update_retrace_info;
        break;

    case VGA_RETRACE_PRECISE:
        s->retrace = vga_precise_retrace;
        s->update_retrace_info = vga_precise_update_retrace_info;
        break;
    }
    vga_dirty_log_start(s);
    TRACE("%s\n", __func__);
}

void maru_vga_common_fini(void)
{
    /* do nothing */
    TRACE("%s\n", __func__);
}
