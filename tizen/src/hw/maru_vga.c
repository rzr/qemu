/*
 * Maru vga device
 * Based on qemu/hw/vga.c
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
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


#include "maru_common.h"

#ifdef CONFIG_DARWIN
//shared memory
#define USE_SHM
#endif

#include "hw.h"
#include "vga.h"
#include "console.h"
#include "pc.h"
#include "pci.h"
#include "vga_int.h"
#include "pixel_ops.h"
#include "qemu-timer.h"
#include "xen.h"
#include "trace.h"

#include "maru_vga_int.h"
#include "maru_brightness.h"
#include "maru_overlay.h"
#include "emul_state.h"
#include "debug_ch.h"

#ifdef USE_SHM
#include "emulator.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef USE_SHM
void *shared_memory = (void*)0;
#endif

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

#define MARU_VGA

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

static void vga_screen_dump(void *opaque, const char *filename, bool cswitch);

static void vga_update_memory_access(VGACommonState *s)
{
    MemoryRegion *region, *old_region = s->chain4_alias;
    target_phys_addr_t base, offset, size;

    s->chain4_alias = NULL;

    if ((s->sr[VGA_SEQ_PLANE_WRITE] & VGA_SR02_ALL_PLANES) ==
        VGA_SR02_ALL_PLANES && s->sr[VGA_SEQ_MEMORY_MODE] & VGA_SR04_CHN_4M) {
        offset = 0;
        switch ((s->gr[VGA_GFX_MISC] >> 2) & 3) {
        case 0:
            base = 0xa0000;
            size = 0x20000;
            break;
        case 1:
            base = 0xa0000;
            size = 0x10000;
            offset = s->bank_offset;
            break;
        case 2:
            base = 0xb0000;
            size = 0x8000;
            break;
        case 3:
        default:
            base = 0xb8000;
            size = 0x8000;
            break;
        }
        base += isa_mem_base;
        region = g_malloc(sizeof(*region));
        memory_region_init_alias(region, "vga.chain4", &s->vram, offset, size);
        memory_region_add_subregion_overlap(s->legacy_address_space, base,
                                            region, 2);
        s->chain4_alias = region;
    }
    if (old_region) {
        memory_region_del_subregion(s->legacy_address_space, old_region);
        memory_region_destroy(old_region);
        g_free(old_region);
        s->plane_updated = 0xf;
    }
}

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

#ifdef CONFIG_BOCHS_VBE
static uint32_t vbe_ioport_read_index(void *opaque, uint32_t addr)
{
    VGACommonState *s = opaque;
    uint32_t val;
    val = s->vbe_index;
    return val;
}

static uint32_t vbe_ioport_read_data(void *opaque, uint32_t addr)
{
    VGACommonState *s = opaque;
    uint32_t val;

    if (s->vbe_index < VBE_DISPI_INDEX_NB) {
        if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_GETCAPS) {
            switch(s->vbe_index) {
                /* XXX: do not hardcode ? */
            case VBE_DISPI_INDEX_XRES:
                val = VBE_DISPI_MAX_XRES;
                break;
            case VBE_DISPI_INDEX_YRES:
                val = VBE_DISPI_MAX_YRES;
                break;
            case VBE_DISPI_INDEX_BPP:
                val = VBE_DISPI_MAX_BPP;
                break;
            default:
                val = s->vbe_regs[s->vbe_index];
                break;
            }
        } else {
            val = s->vbe_regs[s->vbe_index];
        }
    } else if (s->vbe_index == VBE_DISPI_INDEX_VIDEO_MEMORY_64K) {
        val = s->vram_size / (64 * 1024);
    } else {
        val = 0;
    }
#ifdef DEBUG_BOCHS_VBE
    printf("VBE: read index=0x%x val=0x%x\n", s->vbe_index, val);
#endif
    return val;
}

static void vbe_ioport_write_index(void *opaque, uint32_t addr, uint32_t val)
{
    VGACommonState *s = opaque;
    s->vbe_index = val;
}

static void vbe_ioport_write_data(void *opaque, uint32_t addr, uint32_t val)
{
    VGACommonState *s = opaque;

    if (s->vbe_index <= VBE_DISPI_INDEX_NB) {
#ifdef DEBUG_BOCHS_VBE
        printf("VBE: write index=0x%x val=0x%x\n", s->vbe_index, val);
#endif
        switch(s->vbe_index) {
        case VBE_DISPI_INDEX_ID:
            if (val == VBE_DISPI_ID0 ||
                val == VBE_DISPI_ID1 ||
                val == VBE_DISPI_ID2 ||
                val == VBE_DISPI_ID3 ||
                val == VBE_DISPI_ID4) {
                s->vbe_regs[s->vbe_index] = val;
            }
            break;
        case VBE_DISPI_INDEX_XRES:
            if ((val <= VBE_DISPI_MAX_XRES) && ((val & 7) == 0)) {
                s->vbe_regs[s->vbe_index] = val;
            }
            break;
        case VBE_DISPI_INDEX_YRES:
            if (val <= VBE_DISPI_MAX_YRES) {
                s->vbe_regs[s->vbe_index] = val;
            }
            break;
        case VBE_DISPI_INDEX_BPP:
            if (val == 0)
                val = 8;
            if (val == 4 || val == 8 || val == 15 ||
                val == 16 || val == 24 || val == 32) {
                s->vbe_regs[s->vbe_index] = val;
            }
            break;
        case VBE_DISPI_INDEX_BANK:
            if (s->vbe_regs[VBE_DISPI_INDEX_BPP] == 4) {
              val &= (s->vbe_bank_mask >> 2);
            } else {
              val &= s->vbe_bank_mask;
            }
            s->vbe_regs[s->vbe_index] = val;
            s->bank_offset = (val << 16);
            vga_update_memory_access(s);
            break;
        case VBE_DISPI_INDEX_ENABLE:
            if ((val & VBE_DISPI_ENABLED) &&
                !(s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED)) {
                int h, shift_control;

                s->vbe_regs[VBE_DISPI_INDEX_VIRT_WIDTH] =
                    s->vbe_regs[VBE_DISPI_INDEX_XRES];
                s->vbe_regs[VBE_DISPI_INDEX_VIRT_HEIGHT] =
                    s->vbe_regs[VBE_DISPI_INDEX_YRES];
                s->vbe_regs[VBE_DISPI_INDEX_X_OFFSET] = 0;
                s->vbe_regs[VBE_DISPI_INDEX_Y_OFFSET] = 0;

                if (s->vbe_regs[VBE_DISPI_INDEX_BPP] == 4)
                    s->vbe_line_offset = s->vbe_regs[VBE_DISPI_INDEX_XRES] >> 1;
                else
                    s->vbe_line_offset = s->vbe_regs[VBE_DISPI_INDEX_XRES] *
                        ((s->vbe_regs[VBE_DISPI_INDEX_BPP] + 7) >> 3);
                s->vbe_start_addr = 0;

                /* clear the screen (should be done in BIOS) */
                if (!(val & VBE_DISPI_NOCLEARMEM)) {
                    memset(s->vram_ptr, 0,
                           s->vbe_regs[VBE_DISPI_INDEX_YRES] * s->vbe_line_offset);
                }

                /* we initialize the VGA graphic mode (should be done
                   in BIOS) */
                /* graphic mode + memory map 1 */
                s->gr[VGA_GFX_MISC] = (s->gr[VGA_GFX_MISC] & ~0x0c) | 0x04 |
                    VGA_GR06_GRAPHICS_MODE;
                s->cr[VGA_CRTC_MODE] |= 3; /* no CGA modes */
                s->cr[VGA_CRTC_OFFSET] = s->vbe_line_offset >> 3;
                /* width */
                s->cr[VGA_CRTC_H_DISP] =
                    (s->vbe_regs[VBE_DISPI_INDEX_XRES] >> 3) - 1;
                /* height (only meaningful if < 1024) */
                h = s->vbe_regs[VBE_DISPI_INDEX_YRES] - 1;
                s->cr[VGA_CRTC_V_DISP_END] = h;
                s->cr[VGA_CRTC_OVERFLOW] = (s->cr[VGA_CRTC_OVERFLOW] & ~0x42) |
                    ((h >> 7) & 0x02) | ((h >> 3) & 0x40);
                /* line compare to 1023 */
                s->cr[VGA_CRTC_LINE_COMPARE] = 0xff;
                s->cr[VGA_CRTC_OVERFLOW] |= 0x10;
                s->cr[VGA_CRTC_MAX_SCAN] |= 0x40;

                if (s->vbe_regs[VBE_DISPI_INDEX_BPP] == 4) {
                    shift_control = 0;
                    s->sr[VGA_SEQ_CLOCK_MODE] &= ~8; /* no double line */
                } else {
                    shift_control = 2;
                    /* set chain 4 mode */
                    s->sr[VGA_SEQ_MEMORY_MODE] |= VGA_SR04_CHN_4M;
                    /* activate all planes */
                    s->sr[VGA_SEQ_PLANE_WRITE] |= VGA_SR02_ALL_PLANES;
                }
                s->gr[VGA_GFX_MODE] = (s->gr[VGA_GFX_MODE] & ~0x60) |
                    (shift_control << 5);
                s->cr[VGA_CRTC_MAX_SCAN] &= ~0x9f; /* no double scan */
            } else {
                /* XXX: the bios should do that */
                s->bank_offset = 0;
            }
            s->dac_8bit = (val & VBE_DISPI_8BIT_DAC) > 0;
            s->vbe_regs[s->vbe_index] = val;
            vga_update_memory_access(s);
            break;
        case VBE_DISPI_INDEX_VIRT_WIDTH:
            {
                int w, h, line_offset;

                if (val < s->vbe_regs[VBE_DISPI_INDEX_XRES])
                    return;
                w = val;
                if (s->vbe_regs[VBE_DISPI_INDEX_BPP] == 4)
                    line_offset = w >> 1;
                else
                    line_offset = w * ((s->vbe_regs[VBE_DISPI_INDEX_BPP] + 7) >> 3);
                h = s->vram_size / line_offset;
                /* XXX: support weird bochs semantics ? */
                if (h < s->vbe_regs[VBE_DISPI_INDEX_YRES])
                    return;
                s->vbe_regs[VBE_DISPI_INDEX_VIRT_WIDTH] = w;
                s->vbe_regs[VBE_DISPI_INDEX_VIRT_HEIGHT] = h;
                s->vbe_line_offset = line_offset;
            }
            break;
        case VBE_DISPI_INDEX_X_OFFSET:
        case VBE_DISPI_INDEX_Y_OFFSET:
            {
                int x;
                s->vbe_regs[s->vbe_index] = val;
                s->vbe_start_addr = s->vbe_line_offset * s->vbe_regs[VBE_DISPI_INDEX_Y_OFFSET];
                x = s->vbe_regs[VBE_DISPI_INDEX_X_OFFSET];
                if (s->vbe_regs[VBE_DISPI_INDEX_BPP] == 4)
                    s->vbe_start_addr += x >> 1;
                else
                    s->vbe_start_addr += x * ((s->vbe_regs[VBE_DISPI_INDEX_BPP] + 7) >> 3);
                s->vbe_start_addr >>= 2;
            }
            break;
        default:
            break;
        }
    }
}
#endif

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
#ifdef CONFIG_BOCHS_VBE
    if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED) {
        line_offset = s->vbe_line_offset;
        start_addr = s->vbe_start_addr;
        line_compare = 65535;
    } else
#endif
    {
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

static inline int get_depth_index(DisplayState *s)
{
    switch(ds_get_bits_per_pixel(s)) {
    default:
    case 8:
        return 0;
    case 15:
        return 1;
    case 16:
        return 2;
    case 32:
        if (is_surface_bgr(s->surface))
            return 4;
        else
            return 3;
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
        qemu_console_resize(s->ds, s->last_scr_width, s->last_scr_height);
        s->last_depth = 0;
        s->last_width = width;
        s->last_height = height;
        s->last_ch = cheight;
        s->last_cw = cw;
        full_update = 1;
    }
    s->rgb_to_pixel =
        rgb_to_pixel_dup_table[get_depth_index(s->ds)];
    full_update |= update_palette16(s);
    palette = s->last_palette;
    x_incr = cw * ((ds_get_bits_per_pixel(s->ds) + 7) >> 3);

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

    depth_index = get_depth_index(s->ds);
    if (cw == 16)
        maru_vga_draw_glyph8 = maru_vga_draw_glyph16_table[depth_index];
    else
        maru_vga_draw_glyph8 = maru_vga_draw_glyph8_table[depth_index];
    maru_vga_draw_glyph9 = maru_vga_draw_glyph9_table[depth_index];

    dest = ds_get_data(s->ds);
    linesize = ds_get_linesize(s->ds);
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
            dpy_update(s->ds, cx_min * cw, cy * cheight,
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
#ifdef CONFIG_BOCHS_VBE
    if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED) {
        ret = s->vbe_regs[VBE_DISPI_INDEX_BPP];
    } else
#endif
    {
        ret = 0;
    }
    return ret;
}

static void vga_get_resolution(VGACommonState *s, int *pwidth, int *pheight)
{
    int width, height;

#ifdef CONFIG_BOCHS_VBE
    if (s->vbe_regs[VBE_DISPI_INDEX_ENABLE] & VBE_DISPI_ENABLED) {
        width = s->vbe_regs[VBE_DISPI_INDEX_XRES];
        height = s->vbe_regs[VBE_DISPI_INDEX_YRES];
    } else
#endif
    {
        width = (s->cr[VGA_CRTC_H_DISP] + 1) * 8;
        height = s->cr[VGA_CRTC_V_DISP_END] |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x02) << 7) |
            ((s->cr[VGA_CRTC_OVERFLOW] & 0x40) << 3);
        height = (height + 1);
    }
    *pwidth = width;
    *pheight = height;
}

static void vga_sync_dirty_bitmap(VGACommonState *s)
{
    memory_region_sync_dirty_bitmap(&s->vram);
}

/*
 * graphic modes
 */
static void vga_draw_graphic(VGACommonState *s, int full_update)
{
    int y1, y, update, linesize, y_start, double_scan, mask, depth;
    int width, height, shift_control, line_offset, bwidth, bits;
    ram_addr_t page0, page1, page_min, page_max;
    int disp_width, multi_scan, multi_run;
    uint8_t *d;
    uint32_t v, addr1, addr;
    maru_vga_draw_line_func *maru_vga_draw_line;

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
#if defined(HOST_WORDS_BIGENDIAN) == defined(TARGET_WORDS_BIGENDIAN)
        if (depth == 16 || depth == 32) {
#else
        if (depth == 32) {
#endif
            qemu_free_displaysurface(s->ds);

#ifdef MARU_VGA // create new sufrace by malloc in MARU VGA

#ifdef USE_SHM
            s->ds->surface = qemu_create_displaysurface_from(disp_width, height, depth,
                    disp_width * 4, (uint8_t*)shared_memory);
#else
            s->ds->surface = qemu_create_displaysurface(s->ds, disp_width, height);
#endif //USE_SHM

#else //MARU_VGA
            s->ds->surface = qemu_create_displaysurface_from(disp_width, height, depth,
                    s->line_offset,
                    s->vram_ptr + (s->start_addr * 4));
#endif //MARU_VGA

#if defined(HOST_WORDS_BIGENDIAN) != defined(TARGET_WORDS_BIGENDIAN)
            s->ds->surface->pf = qemu_different_endianness_pixelformat(depth);
#endif
            dpy_resize(s->ds);
        } else {
            qemu_console_resize(s->ds, disp_width, height);
        }
        s->last_scr_width = disp_width;
        s->last_scr_height = height;
        s->last_width = disp_width;
        s->last_height = height;
        s->last_line_offset = s->line_offset;
        s->last_depth = depth;
        full_update = 1;
#ifndef USE_SHM
    } else if (is_buffer_shared(s->ds->surface) &&
               (full_update || s->ds->surface->data != s->vram_ptr + (s->start_addr * 4))) {
        s->ds->surface->data = s->vram_ptr + (s->start_addr * 4);
        dpy_setdata(s->ds);
    }
#else
    }
#endif
    s->rgb_to_pixel =
        rgb_to_pixel_dup_table[get_depth_index(s->ds)];

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
    maru_vga_draw_line = maru_vga_draw_line_table[v * NB_DEPTHS + get_depth_index(s->ds)];

#ifndef USE_SHM
    if (!is_buffer_shared(s->ds->surface) && s->cursor_invalidate)
#else
    if (s->cursor_invalidate)
#endif
        s->cursor_invalidate(s);

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
    d = ds_get_data(s->ds);
    linesize = ds_get_linesize(s->ds);
    y1 = 0;
    for(y = 0; y < height; y++) {
        addr = addr1;
        if (!(s->cr[0x17] & 1)) {
            int shift;
            /* CGA compatibility handling */
            shift = 14 + ((s->cr[0x17] >> 6) & 1);
            addr = (addr & ~(1 << shift)) | ((y1 & 1) << shift);
        }
        if (!(s->cr[0x17] & 2)) {
            addr = (addr & ~0x8000) | ((y1 & 2) << 14);
        }
        update = full_update;
        page0 = addr;
        page1 = addr + bwidth - 1;
        update |= memory_region_get_dirty(&s->vram, page0, page1 - page0,
                                          DIRTY_MEMORY_VGA);
        /* explicit invalidation for the hardware cursor */
        update |= (s->invalidated_y_table[y >> 5] >> (y & 0x1f)) & 1;

#ifdef MARU_VGA // needs full update
        update |= 1;
#endif

        if (update) {
            if (y_start < 0)
                y_start = y;
            if (page0 < page_min)
                page_min = page0;
            if (page1 > page_max)
                page_max = page1;
#ifndef USE_SHM
            if (!(is_buffer_shared(s->ds->surface))) {
#endif
                maru_vga_draw_line(s, d, s->vram_ptr + addr, width);
                if (s->cursor_draw_line)
                    s->cursor_draw_line(s, d, y);
#ifndef USE_SHM
            }
#endif

#ifdef MARU_VGA

            int i;
            uint8_t *fb_sub;
            uint8_t *over_sub;
            uint8_t *dst_sub;
            uint8_t alpha, c_alpha;
            uint32_t *dst;
            uint16_t overlay_bottom;

            if ( overlay0_power ) {

                overlay_bottom = overlay0_top + overlay0_height;

                if ( overlay0_top <= y && y < overlay_bottom ) {

                    fb_sub = s->vram_ptr + addr + overlay0_left * 4;
                    over_sub = overlay_ptr + ( y - overlay0_top ) * overlay0_width * 4;
                    dst = (uint32_t*) ( s->ds->surface->data + addr + overlay0_left * 4 );

                    for ( i = 0; i < overlay0_width; i++, fb_sub += 4, over_sub += 4, dst++ ) {

                        alpha = fb_sub[3];
                        c_alpha = 0xff - alpha;

                        *dst = ( ( c_alpha * over_sub[0] + alpha * fb_sub[0] ) >> 8 )
                            | ( ( c_alpha * over_sub[1] + alpha * fb_sub[1] ) & 0xFF00 )
                            | ( ( ( c_alpha * over_sub[2] + alpha * fb_sub[2] ) & 0xFF00 ) << 8 );
                    }

                }

            }

            if ( overlay1_power ) {

                overlay_bottom = overlay1_top + overlay1_height;

                if ( overlay1_top <= y && y < overlay_bottom ) {

                    fb_sub = s->vram_ptr + addr + overlay1_left * 4;
                    over_sub = overlay_ptr + ( y - overlay1_top ) * overlay1_width * 4 + 0x00400000;
                    dst = (uint32_t*) ( s->ds->surface->data + addr + overlay1_left * 4 );

                    for ( i = 0; i < overlay1_width; i++, fb_sub += 4, over_sub += 4, dst++ ) {

                        alpha = fb_sub[3];
                        c_alpha = 0xff - alpha;

                        *dst = ( ( c_alpha * over_sub[0] + alpha * fb_sub[0] ) >> 8 )
                            | ( ( c_alpha * over_sub[1] + alpha * fb_sub[1] ) & 0xFF00 )
                            | ( ( ( c_alpha * over_sub[2] + alpha * fb_sub[2] ) & 0xFF00 ) << 8 );
                    }

                }

            }

            if ( brightness_off ) {

                dst_sub = s->ds->surface->data + addr;
                dst = (uint32_t*) ( s->ds->surface->data + addr );

                for ( i = 0; i < disp_width; i++, dst_sub += 4, dst++ ) {
                    *dst = 0xFF000000; // black
                }

            } else  {

                if ( brightness_level < BRIGHTNESS_MAX ) {

                    alpha = brightness_tbl[brightness_level];

                    dst_sub = s->ds->surface->data + addr;
                    dst = (uint32_t*) ( s->ds->surface->data + addr );

                    for ( i = 0; i < disp_width; i++, dst_sub += 4, dst++ ) {
                        *dst = ( ( alpha * dst_sub[0] ) >> 8 )
                                | ( ( alpha * dst_sub[1] ) & 0xFF00 )
                                | ( ( ( alpha * dst_sub[2] ) & 0xFF00 ) << 8 );
                    }
                }

            }

#endif /* MARU_VGA */

        } else {
            if (y_start >= 0) {
                /* flush to display */
                dpy_update(s->ds, 0, y_start,
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
        dpy_update(s->ds, 0, y_start,
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
    int i, w, val;
    uint8_t *d;

    if (!full_update)
        return;
    if (s->last_scr_width <= 0 || s->last_scr_height <= 0)
        return;

    s->rgb_to_pixel =
        rgb_to_pixel_dup_table[get_depth_index(s->ds)];
    if (ds_get_bits_per_pixel(s->ds) == 8)
        val = s->rgb_to_pixel(0, 0, 0);
    else
        val = 0;
    w = s->last_scr_width * ((ds_get_bits_per_pixel(s->ds) + 7) >> 3);
    d = ds_get_data(s->ds);
    for(i = 0; i < s->last_scr_height; i++) {
        memset(d, val, w);
        d += ds_get_linesize(s->ds);
    }
    dpy_update(s->ds, 0, 0,
               s->last_scr_width, s->last_scr_height);
}

#define GMODE_TEXT     0
#define GMODE_GRAPH    1
#define GMODE_BLANK 2

static void vga_update_display(void *opaque)
{
    VGACommonState *s = opaque;
    int full_update, graphic_mode;

    qemu_flush_coalesced_mmio_buffer();

    if (ds_get_bits_per_pixel(s->ds) == 0) {
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

static void vga_reset(void *opaque)
{
    VGACommonState *s =  opaque;
    vga_common_reset(s);
}

#define TEXTMODE_X(x)   ((x) % width)
#define TEXTMODE_Y(x)   ((x) / width)
#define VMEM2CHTYPE(v)  ((v & 0xff0007ff) | \
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
            s->ds->surface->width = width;
            s->ds->surface->height = height;
            dpy_resize(s->ds);
            s->last_width = width;
            s->last_height = height;
            s->last_ch = cheight;
            s->last_cw = cw;
            full_update = 1;
        }

        /* Update "hardware" cursor */
        cursor_offset = ((s->cr[VGA_CRTC_CURSOR_HI] << 8) |
                         s->cr[VGA_CRTC_CURSOR_LO]) - s->start_addr;
        if (cursor_offset != s->cursor_offset ||
            s->cr[VGA_CRTC_CURSOR_START] != s->cursor_start ||
            s->cr[VGA_CRTC_CURSOR_END] != s->cursor_end || full_update) {
            cursor_visible = !(s->cr[VGA_CRTC_CURSOR_START] & 0x20);
            if (cursor_visible && cursor_offset < size && cursor_offset >= 0)
                dpy_cursor(s->ds,
                           TEXTMODE_X(cursor_offset),
                           TEXTMODE_Y(cursor_offset));
            else
                dpy_cursor(s->ds, -1, -1);
            s->cursor_offset = cursor_offset;
            s->cursor_start = s->cr[VGA_CRTC_CURSOR_START];
            s->cursor_end = s->cr[VGA_CRTC_CURSOR_END];
        }

        src = (uint32_t *) s->vram_ptr + s->start_addr;
        dst = chardata;

        if (full_update) {
            for (i = 0; i < size; src ++, dst ++, i ++)
                console_write_ch(dst, VMEM2CHTYPE(le32_to_cpu(*src)));

            dpy_update(s->ds, 0, 0, width, height);
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
                dpy_update(s->ds, 0, i, width, TEXTMODE_Y(c_max) - i + 1);
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
    dpy_cursor(s->ds, -1, -1);
    s->ds->surface->width = s->last_width;
    s->ds->surface->height = height;
    dpy_resize(s->ds);

    for (dst = chardata, i = 0; i < s->last_width * height; i ++)
        console_write_ch(dst ++, ' ');

    size = strlen(msg_buffer);
    width = (s->last_width - size) / 2;
    dst = chardata + s->last_width + width;
    for (i = 0; i < size; i ++)
        console_write_ch(dst ++, 0x00200100 | msg_buffer[i]);

    dpy_update(s->ds, 0, 0, s->last_width, height);
}


static uint64_t vga_mem_read(void *opaque, target_phys_addr_t addr,
                             unsigned size)
{
    VGACommonState *s = opaque;

    return vga_mem_readb(s, addr);
}

static void vga_mem_write(void *opaque, target_phys_addr_t addr,
                          uint64_t data, unsigned size)
{
    VGACommonState *s = opaque;

    return vga_mem_writeb(s, addr, data);
}

static int vga_common_post_load(void *opaque, int version_id)
{
    VGACommonState *s = opaque;

    /* force refresh */
    s->graphic_mode = -1;
    return 0;
}

void maru_vga_common_init(VGACommonState *s)
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

#ifdef CONFIG_BOCHS_VBE
    s->is_vbe_vmstate = 1;
#else
    s->is_vbe_vmstate = 0;
#endif
    memory_region_init_ram(&s->vram, "maru_vga.vram", s->vram_size);
    vmstate_register_ram_global(&s->vram);
    xen_register_framebuffer(&s->vram);
    s->vram_ptr = memory_region_get_ram_ptr(&s->vram);
    s->get_bpp = vga_get_bpp;
    s->get_offsets = vga_get_offsets;
    s->get_resolution = vga_get_resolution;
    s->update = vga_update_display;
    s->invalidate = vga_invalidate_display;
    s->screen_dump = vga_screen_dump;
    s->text_update = vga_update_text;
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

#ifdef USE_SHM
    int mykey;
    void *temp; 
    int shmid;

    shmid = shmget((key_t)SHMKEY, (size_t)MAXLEN, 0666 | IPC_CREAT);
    if (shmid == -1) {
        ERR( "shmget failed\n");
        perror("maru_vga: ");
        exit(1);
    }
    
    temp = shmat(shmid, (char*)0x0, 0);
    if (temp == (void *)-1) {
        ERR( "shmat failed\n");
        perror("maru_vga: ");
        exit(1);
    }
    mykey = atoi(temp);
    shmdt(temp);
    INFO("shared memory key: %d, ram_size : %d\n", mykey, vga_ram_size);
    shmid = shmget((key_t)mykey, (size_t)vga_ram_size, 0666 | IPC_CREAT);
    if (shmid == -1) {
	ERR( "shmget failed\n");
        perror("maru_vga: ");
        exit(1);
    }
    
    shared_memory = shmat(shmid, (void*)0, 0);
    if (shared_memory == (void *)-1) {
        ERR( "shmat failed\n");
        perror("maru_vga: ");
        exit(1);
    }

    memset(shared_memory, 0x00, (size_t)vga_ram_size); 
    printf("Memory attached at %X\n", (int)shared_memory);
#endif

}


static const MemoryRegionPortio vga_portio_list[] = {
    { 0x04,  2, 1, .read = vga_ioport_read, .write = vga_ioport_write }, /* 3b4 */
    { 0x0a,  1, 1, .read = vga_ioport_read, .write = vga_ioport_write }, /* 3ba */
    { 0x10, 16, 1, .read = vga_ioport_read, .write = vga_ioport_write }, /* 3c0 */
    { 0x24,  2, 1, .read = vga_ioport_read, .write = vga_ioport_write }, /* 3d4 */
    { 0x2a,  1, 1, .read = vga_ioport_read, .write = vga_ioport_write }, /* 3da */
    PORTIO_END_OF_LIST(),
};

#ifdef CONFIG_BOCHS_VBE
static const MemoryRegionPortio vbe_portio_list[] = {
    { 0, 1, 2, .read = vbe_ioport_read_index, .write = vbe_ioport_write_index },
# ifdef TARGET_I386
    { 1, 1, 2, .read = vbe_ioport_read_data, .write = vbe_ioport_write_data },
# else
    { 2, 1, 2, .read = vbe_ioport_read_data, .write = vbe_ioport_write_data },
# endif
    PORTIO_END_OF_LIST(),
};
#endif /* CONFIG_BOCHS_VBE */

/********************************************************/
/* vga screen dump */

static int maru_ppm_save(const char *filename, struct DisplaySurface *ds)
{
    FILE *f;
    uint8_t *d, *d1;
    uint32_t v;
    int y, x;
    uint8_t r, g, b;
    int ret;
    char *linebuf, *pbuf;

    trace_ppm_save(filename, ds);
    f = fopen(filename, "wb");
    if (!f)
        return -1;
    fprintf(f, "P6\n%d %d\n%d\n",
            ds->width, ds->height, 255);
    linebuf = g_malloc(ds->width * 3);
    d1 = ds->data;
    for(y = 0; y < ds->height; y++) {
        d = d1;
        pbuf = linebuf;
        for(x = 0; x < ds->width; x++) {
            if (ds->pf.bits_per_pixel == 32)
                v = *(uint32_t *)d;
            else
                v = (uint32_t) (*(uint16_t *)d);
            /* Limited to 8 or fewer bits per channel: */
            r = ((v >> ds->pf.rshift) & ds->pf.rmax) << (8 - ds->pf.rbits);
            g = ((v >> ds->pf.gshift) & ds->pf.gmax) << (8 - ds->pf.gbits);
            b = ((v >> ds->pf.bshift) & ds->pf.bmax) << (8 - ds->pf.bbits);
            *pbuf++ = r;
            *pbuf++ = g;
            *pbuf++ = b;
            d += ds->pf.bytes_per_pixel;
        }
        d1 += ds->linesize;
        ret = fwrite(linebuf, 1, pbuf - linebuf, f);
        (void)ret;
    }
    g_free(linebuf);
    fclose(f);
    return 0;
}

/* save the vga display in a PPM image even if no display is
   available */
static void vga_screen_dump(void *opaque, const char *filename, bool cswitch)
{
    VGACommonState *s = opaque;

    if (cswitch) {
        vga_invalidate_display(s);
    }
    vga_hw_update();
    ppm_save(filename, s->ds->surface);
}
