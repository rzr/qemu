/*
 *  vga device
 * Based on qemu/hw/vga_template.h
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * Hyunjun Son
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

#if DEPTH == 8
#define BPP 1
#define PIXEL_TYPE uint8_t
#elif DEPTH == 15 || DEPTH == 16
#define BPP 2
#define PIXEL_TYPE uint16_t
#elif DEPTH == 32
#define BPP 4
#define PIXEL_TYPE uint32_t
#else
#error unsupport depth
#endif

#ifdef BGR_FORMAT
#define PIXEL_NAME glue(DEPTH, bgr)
#else
#define PIXEL_NAME DEPTH
#endif /* BGR_FORMAT */

#if DEPTH != 15 && !defined(BGR_FORMAT)

static inline void glue(maru_vga_draw_glyph_line_, DEPTH)(uint8_t *d,
                                                     uint32_t font_data,
                                                     uint32_t xorcol,
                                                     uint32_t bgcol)
{
#if BPP == 1
        ((uint32_t *)d)[0] = (dmask16[(font_data >> 4)] & xorcol) ^ bgcol;
        ((uint32_t *)d)[1] = (dmask16[(font_data >> 0) & 0xf] & xorcol) ^ bgcol;
#elif BPP == 2
        ((uint32_t *)d)[0] = (dmask4[(font_data >> 6)] & xorcol) ^ bgcol;
        ((uint32_t *)d)[1] = (dmask4[(font_data >> 4) & 3] & xorcol) ^ bgcol;
        ((uint32_t *)d)[2] = (dmask4[(font_data >> 2) & 3] & xorcol) ^ bgcol;
        ((uint32_t *)d)[3] = (dmask4[(font_data >> 0) & 3] & xorcol) ^ bgcol;
#else
        ((uint32_t *)d)[0] = (-((font_data >> 7)) & xorcol) ^ bgcol;
        ((uint32_t *)d)[1] = (-((font_data >> 6) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[2] = (-((font_data >> 5) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[3] = (-((font_data >> 4) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[4] = (-((font_data >> 3) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[5] = (-((font_data >> 2) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[6] = (-((font_data >> 1) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[7] = (-((font_data >> 0) & 1) & xorcol) ^ bgcol;
#endif
}

static void glue(maru_vga_draw_glyph8_, DEPTH)(uint8_t *d, int linesize,
                                          const uint8_t *font_ptr, int h,
                                          uint32_t fgcol, uint32_t bgcol)
{
    uint32_t font_data, xorcol;

    xorcol = bgcol ^ fgcol;
    do {
        font_data = font_ptr[0];
        glue(maru_vga_draw_glyph_line_, DEPTH)(d, font_data, xorcol, bgcol);
        font_ptr += 4;
        d += linesize;
    } while (--h);
}

static void glue(maru_vga_draw_glyph16_, DEPTH)(uint8_t *d, int linesize,
                                          const uint8_t *font_ptr, int h,
                                          uint32_t fgcol, uint32_t bgcol)
{
    uint32_t font_data, xorcol;

    xorcol = bgcol ^ fgcol;
    do {
        font_data = font_ptr[0];
        glue(maru_vga_draw_glyph_line_, DEPTH)(d,
                                          expand4to8[font_data >> 4],
                                          xorcol, bgcol);
        glue(maru_vga_draw_glyph_line_, DEPTH)(d + 8 * BPP,
                                          expand4to8[font_data & 0x0f],
                                          xorcol, bgcol);
        font_ptr += 4;
        d += linesize;
    } while (--h);
}

static void glue(maru_vga_draw_glyph9_, DEPTH)(uint8_t *d, int linesize,
                                          const uint8_t *font_ptr, int h,
                                          uint32_t fgcol, uint32_t bgcol, int dup9)
{
    uint32_t font_data, xorcol, v;

    xorcol = bgcol ^ fgcol;
    do {
        font_data = font_ptr[0];
#if BPP == 1
        stl_p((uint32_t *)d, (dmask16[(font_data >> 4)] & xorcol) ^ bgcol);
        v = (dmask16[(font_data >> 0) & 0xf] & xorcol) ^ bgcol;
        stl_p(((uint32_t *)d)+1, v);
        if (dup9)
            ((uint8_t *)d)[8] = v >> (24 * (1 - BIG));
        else
            ((uint8_t *)d)[8] = bgcol;

#elif BPP == 2
        stl_p(((uint32_t *)d)+0, (dmask4[(font_data >> 6)] & xorcol) ^ bgcol);
        stl_p(((uint32_t *)d)+1, (dmask4[(font_data >> 4) & 3] & xorcol) ^ bgcol);
        stl_p(((uint32_t *)d)+2, (dmask4[(font_data >> 2) & 3] & xorcol) ^ bgcol);
        v = (dmask4[(font_data >> 0) & 3] & xorcol) ^ bgcol;
        stl_p(((uint32_t *)d)+3, v);
        if (dup9)
            ((uint16_t *)d)[8] = v >> (16 * (1 - BIG));
        else
            ((uint16_t *)d)[8] = bgcol;
#else
        ((uint32_t *)d)[0] = (-((font_data >> 7)) & xorcol) ^ bgcol;
        ((uint32_t *)d)[1] = (-((font_data >> 6) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[2] = (-((font_data >> 5) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[3] = (-((font_data >> 4) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[4] = (-((font_data >> 3) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[5] = (-((font_data >> 2) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[6] = (-((font_data >> 1) & 1) & xorcol) ^ bgcol;
        v = (-((font_data >> 0) & 1) & xorcol) ^ bgcol;
        ((uint32_t *)d)[7] = v;
        if (dup9)
            ((uint32_t *)d)[8] = v;
        else
            ((uint32_t *)d)[8] = bgcol;
#endif
        font_ptr += 4;
        d += linesize;
    } while (--h);
}

/*
 * 4 color mode
 */
static void glue(maru_vga_draw_line2_, DEPTH)(VGACommonState *s1, uint8_t *d,
                                         const uint8_t *s, int width)
{
    uint32_t plane_mask, *palette, data, v;
    int x;

    palette = s1->last_palette;
    plane_mask = mask16[s1->ar[0x12] & 0xf];
    width >>= 3;
    for(x = 0; x < width; x++) {
        data = ((uint32_t *)s)[0];
        data &= plane_mask;
        v = expand2[GET_PLANE(data, 0)];
        v |= expand2[GET_PLANE(data, 2)] << 2;
        ((PIXEL_TYPE *)d)[0] = palette[v >> 12];
        ((PIXEL_TYPE *)d)[1] = palette[(v >> 8) & 0xf];
        ((PIXEL_TYPE *)d)[2] = palette[(v >> 4) & 0xf];
        ((PIXEL_TYPE *)d)[3] = palette[(v >> 0) & 0xf];

        v = expand2[GET_PLANE(data, 1)];
        v |= expand2[GET_PLANE(data, 3)] << 2;
        ((PIXEL_TYPE *)d)[4] = palette[v >> 12];
        ((PIXEL_TYPE *)d)[5] = palette[(v >> 8) & 0xf];
        ((PIXEL_TYPE *)d)[6] = palette[(v >> 4) & 0xf];
        ((PIXEL_TYPE *)d)[7] = palette[(v >> 0) & 0xf];
        d += BPP * 8;
        s += 4;
    }
}

#if BPP == 1
#define PUT_PIXEL2(d, n, v) ((uint16_t *)d)[(n)] = (v)
#elif BPP == 2
#define PUT_PIXEL2(d, n, v) ((uint32_t *)d)[(n)] = (v)
#else
#define PUT_PIXEL2(d, n, v) \
((uint32_t *)d)[2*(n)] = ((uint32_t *)d)[2*(n)+1] = (v)
#endif

/*
 * 4 color mode, dup2 horizontal
 */
static void glue(maru_vga_draw_line2d2_, DEPTH)(VGACommonState *s1, uint8_t *d,
                                           const uint8_t *s, int width)
{
    uint32_t plane_mask, *palette, data, v;
    int x;

    palette = s1->last_palette;
    plane_mask = mask16[s1->ar[0x12] & 0xf];
    width >>= 3;
    for(x = 0; x < width; x++) {
        data = ((uint32_t *)s)[0];
        data &= plane_mask;
        v = expand2[GET_PLANE(data, 0)];
        v |= expand2[GET_PLANE(data, 2)] << 2;
        PUT_PIXEL2(d, 0, palette[v >> 12]);
        PUT_PIXEL2(d, 1, palette[(v >> 8) & 0xf]);
        PUT_PIXEL2(d, 2, palette[(v >> 4) & 0xf]);
        PUT_PIXEL2(d, 3, palette[(v >> 0) & 0xf]);

        v = expand2[GET_PLANE(data, 1)];
        v |= expand2[GET_PLANE(data, 3)] << 2;
        PUT_PIXEL2(d, 4, palette[v >> 12]);
        PUT_PIXEL2(d, 5, palette[(v >> 8) & 0xf]);
        PUT_PIXEL2(d, 6, palette[(v >> 4) & 0xf]);
        PUT_PIXEL2(d, 7, palette[(v >> 0) & 0xf]);
        d += BPP * 16;
        s += 4;
    }
}

/*
 * 16 color mode
 */
static void glue(maru_vga_draw_line4_, DEPTH)(VGACommonState *s1, uint8_t *d,
                                         const uint8_t *s, int width)
{
    uint32_t plane_mask, data, v, *palette;
    int x;

    palette = s1->last_palette;
    plane_mask = mask16[s1->ar[0x12] & 0xf];
    width >>= 3;
    for(x = 0; x < width; x++) {
        data = ((uint32_t *)s)[0];
        data &= plane_mask;
        v = expand4[GET_PLANE(data, 0)];
        v |= expand4[GET_PLANE(data, 1)] << 1;
        v |= expand4[GET_PLANE(data, 2)] << 2;
        v |= expand4[GET_PLANE(data, 3)] << 3;
        ((PIXEL_TYPE *)d)[0] = palette[v >> 28];
        ((PIXEL_TYPE *)d)[1] = palette[(v >> 24) & 0xf];
        ((PIXEL_TYPE *)d)[2] = palette[(v >> 20) & 0xf];
        ((PIXEL_TYPE *)d)[3] = palette[(v >> 16) & 0xf];
        ((PIXEL_TYPE *)d)[4] = palette[(v >> 12) & 0xf];
        ((PIXEL_TYPE *)d)[5] = palette[(v >> 8) & 0xf];
        ((PIXEL_TYPE *)d)[6] = palette[(v >> 4) & 0xf];
        ((PIXEL_TYPE *)d)[7] = palette[(v >> 0) & 0xf];
        d += BPP * 8;
        s += 4;
    }
}

/*
 * 16 color mode, dup2 horizontal
 */
static void glue(maru_vga_draw_line4d2_, DEPTH)(VGACommonState *s1, uint8_t *d,
                                           const uint8_t *s, int width)
{
    uint32_t plane_mask, data, v, *palette;
    int x;

    palette = s1->last_palette;
    plane_mask = mask16[s1->ar[0x12] & 0xf];
    width >>= 3;
    for(x = 0; x < width; x++) {
        data = ((uint32_t *)s)[0];
        data &= plane_mask;
        v = expand4[GET_PLANE(data, 0)];
        v |= expand4[GET_PLANE(data, 1)] << 1;
        v |= expand4[GET_PLANE(data, 2)] << 2;
        v |= expand4[GET_PLANE(data, 3)] << 3;
        PUT_PIXEL2(d, 0, palette[v >> 28]);
        PUT_PIXEL2(d, 1, palette[(v >> 24) & 0xf]);
        PUT_PIXEL2(d, 2, palette[(v >> 20) & 0xf]);
        PUT_PIXEL2(d, 3, palette[(v >> 16) & 0xf]);
        PUT_PIXEL2(d, 4, palette[(v >> 12) & 0xf]);
        PUT_PIXEL2(d, 5, palette[(v >> 8) & 0xf]);
        PUT_PIXEL2(d, 6, palette[(v >> 4) & 0xf]);
        PUT_PIXEL2(d, 7, palette[(v >> 0) & 0xf]);
        d += BPP * 16;
        s += 4;
    }
}

/*
 * 256 color mode, double pixels
 *
 * XXX: add plane_mask support (never used in standard VGA modes)
 */
static void glue(maru_vga_draw_line8d2_, DEPTH)(VGACommonState *s1, uint8_t *d,
                                           const uint8_t *s, int width)
{
    uint32_t *palette;
    int x;

    palette = s1->last_palette;
    width >>= 3;
    for(x = 0; x < width; x++) {
        PUT_PIXEL2(d, 0, palette[s[0]]);
        PUT_PIXEL2(d, 1, palette[s[1]]);
        PUT_PIXEL2(d, 2, palette[s[2]]);
        PUT_PIXEL2(d, 3, palette[s[3]]);
        d += BPP * 8;
        s += 4;
    }
}

/*
 * standard 256 color mode
 *
 * XXX: add plane_mask support (never used in standard VGA modes)
 */
static void glue(maru_vga_draw_line8_, DEPTH)(VGACommonState *s1, uint8_t *d,
                                         const uint8_t *s, int width)
{
    uint32_t *palette;
    int x;

    palette = s1->last_palette;
    width >>= 3;
    for(x = 0; x < width; x++) {
        ((PIXEL_TYPE *)d)[0] = palette[s[0]];
        ((PIXEL_TYPE *)d)[1] = palette[s[1]];
        ((PIXEL_TYPE *)d)[2] = palette[s[2]];
        ((PIXEL_TYPE *)d)[3] = palette[s[3]];
        ((PIXEL_TYPE *)d)[4] = palette[s[4]];
        ((PIXEL_TYPE *)d)[5] = palette[s[5]];
        ((PIXEL_TYPE *)d)[6] = palette[s[6]];
        ((PIXEL_TYPE *)d)[7] = palette[s[7]];
        d += BPP * 8;
        s += 8;
    }
}

#endif /* DEPTH != 15 */


/* XXX: optimize */

/*
 * 15 bit color
 */
static void glue(maru_vga_draw_line15_, PIXEL_NAME)(VGACommonState *s1, uint8_t *d,
                                          const uint8_t *s, int width)
{
#if DEPTH == 15 && defined(HOST_WORDS_BIGENDIAN) == defined(TARGET_WORDS_BIGENDIAN)
    memcpy(d, s, width * 2);
#else
    int w;
    uint32_t v, r, g, b;

    w = width;
    do {
        v = lduw_raw((void *)s);
        r = (v >> 7) & 0xf8;
        g = (v >> 2) & 0xf8;
        b = (v << 3) & 0xf8;
        ((PIXEL_TYPE *)d)[0] = glue(rgb_to_pixel, PIXEL_NAME)(r, g, b);
        s += 2;
        d += BPP;
    } while (--w != 0);
#endif
}

/*
 * 16 bit color
 */
static void glue(maru_vga_draw_line16_, PIXEL_NAME)(VGACommonState *s1, uint8_t *d,
                                          const uint8_t *s, int width)
{
#if DEPTH == 16 && defined(HOST_WORDS_BIGENDIAN) == defined(TARGET_WORDS_BIGENDIAN)
    memcpy(d, s, width * 2);
#else
    int w;
    uint32_t v, r, g, b;

    w = width;
    do {
        v = lduw_raw((void *)s);
        r = (v >> 8) & 0xf8;
        g = (v >> 3) & 0xfc;
        b = (v << 3) & 0xf8;
        ((PIXEL_TYPE *)d)[0] = glue(rgb_to_pixel, PIXEL_NAME)(r, g, b);
        s += 2;
        d += BPP;
    } while (--w != 0);
#endif
}

/*
 * 24 bit color
 */
static void glue(maru_vga_draw_line24_, PIXEL_NAME)(VGACommonState *s1, uint8_t *d,
                                          const uint8_t *s, int width)
{
    int w;
    uint32_t r, g, b;

    w = width;
    do {
#if defined(TARGET_WORDS_BIGENDIAN)
        r = s[0];
        g = s[1];
        b = s[2];
#else
        b = s[0];
        g = s[1];
        r = s[2];
#endif
        ((PIXEL_TYPE *)d)[0] = glue(rgb_to_pixel, PIXEL_NAME)(r, g, b);
        s += 3;
        d += BPP;
    } while (--w != 0);
}

/*
 * 32 bit color
 */
static void glue(maru_vga_draw_line32_, PIXEL_NAME)(VGACommonState *s1, uint8_t *d,
                                          const uint8_t *s, int width)
{
#if DEPTH == 32 && defined(HOST_WORDS_BIGENDIAN) == defined(TARGET_WORDS_BIGENDIAN) && !defined(BGR_FORMAT)
    memcpy(d, s, width * 4);
#else
    int w;
    uint32_t r, g, b;

    w = width;
    do {
#if defined(TARGET_WORDS_BIGENDIAN)
        r = s[1];
        g = s[2];
        b = s[3];
#else
        b = s[0];
        g = s[1];
        r = s[2];
#endif
        ((PIXEL_TYPE *)d)[0] = glue(rgb_to_pixel, PIXEL_NAME)(r, g, b);
        s += 4;
        d += BPP;
    } while (--w != 0);
#endif
}

#undef PUT_PIXEL2
#undef DEPTH
#undef BPP
#undef PIXEL_TYPE
#undef PIXEL_NAME
#undef BGR_FORMAT
