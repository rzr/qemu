/*
 * Maru vga device
 * Based on qemu/hw/vga_int.h
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

#ifndef MARU_VGA_INT_H_
#define MARU_VGA_INT_H_

void maru_vga_common_init(VGACommonState *s);

void maru_vga_draw_cursor_line_8(uint8_t *d1, const uint8_t *src1,
                            int poffset, int w,
                            unsigned int color0, unsigned int color1,
                            unsigned int color_xor);
void maru_vga_draw_cursor_line_16(uint8_t *d1, const uint8_t *src1,
                             int poffset, int w,
                             unsigned int color0, unsigned int color1,
                             unsigned int color_xor);
void maru_vga_draw_cursor_line_32(uint8_t *d1, const uint8_t *src1,
                             int poffset, int w,
                             unsigned int color0, unsigned int color1,
                             unsigned int color_xor);

#endif /* MARU_VGA_INT_H_ */
