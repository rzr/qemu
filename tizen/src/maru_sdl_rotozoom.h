/*
 * Rotation & Scaling of SDL surface
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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


#ifndef MARU_SDL_ROTOZOOM_H_
#define MARU_SDL_ROTOZOOM_H_

#include <SDL.h>
#include <SDL_syswm.h>


#define R_CHANNEL_MASK 0x00ff0000
#define G_CHANNEL_MASK 0x0000ff00
#define B_CHANNEL_MASK 0x000000ff

SDL_Surface *maru_rotozoom(SDL_Surface *rz_src, SDL_Surface *rz_dst, int angle);


static void interpolate_pixel_cpy(unsigned int *dst, unsigned int *src_addr, unsigned int src_w, unsigned int src_h, int x, int y)
{
#if 0
    int i, j, n, m;
    double mask[3][3] = { {0., 0.19, 0.}, {0.19, 0.24, 0.19}, {0., 0.19, 0.} };
    double sum_r = 0; //0x00ff0000
    double sum_g = 0; //0x0000ff00
    double sum_b = 0; //0x000000ff
    int index;

    n = 0;
    for(i = x - 1; i <= x + 1; i++, n++) {
        m = 0;
        for(j = y - 1; j <= y + 1; j++, m++) {
            if (mask[n][m] == 0 || i < 0 || i > src_h || j < 0 || j > src_w) { //outline
                index = (x * src_w) + y;
                sum_r += mask[n][m] * (double)((src_addr[index] & 0x00ff0000) >> 16);
                sum_g += mask[n][m] * (double)((src_addr[index] & 0x0000ff00) >> 8);
                sum_b += mask[n][m] * (double)(src_addr[index] & 0x000000ff);
            } else {
                index = (i * src_w) + j;
                sum_r += mask[n][m] * (double)((src_addr[index] & 0x00ff0000) >> 16);
                sum_g += mask[n][m] * (double)((src_addr[index] & 0x0000ff00) >> 8);
                sum_b += mask[n][m] * (double)(src_addr[index] & 0x000000ff);
            }
        }
    }

    *dst = 0 | (((unsigned int) round(sum_r)) << 16) |
        (((unsigned int)  round(sum_g)) << 8) |
        (((unsigned int) round(sum_b)) & 0x000000ff);
#endif

#if 0
    unsigned int sum_r = 0; //0x00ff0000
    unsigned int sum_g = 0; //0x0000ff00
    unsigned int sum_b = 0; //0x000000ff

    unsigned int index = (x * src_w) + y;
    unsigned int index_w = (y - 1 < 0) ? index : index - 1;
    unsigned int index_e = (y + 1 > src_w) ? index : index + 1;
    unsigned int index_n = (x - 1 < 0) ? index : ((x - 1) * src_w) + y;
    unsigned int index_s = (x + 1 > src_h) ? index : ((x + 1) * src_w) + y;

    sum_r = (src_addr[index] & R_CHANNEL_MASK) +
        (src_addr[index_w] & R_CHANNEL_MASK) + (src_addr[index_e] & R_CHANNEL_MASK) +
        (src_addr[index_n] & R_CHANNEL_MASK) + (src_addr[index_s] & R_CHANNEL_MASK);

    sum_g = (src_addr[index] & G_CHANNEL_MASK) +
        (src_addr[index_w] & G_CHANNEL_MASK) + (src_addr[index_e] & G_CHANNEL_MASK) +
        (src_addr[index_n] & G_CHANNEL_MASK) + (src_addr[index_s] & G_CHANNEL_MASK);

    sum_b = (src_addr[index] & B_CHANNEL_MASK) +
        (src_addr[index_w] & B_CHANNEL_MASK) + (src_addr[index_e] & B_CHANNEL_MASK) +
        (src_addr[index_n] & B_CHANNEL_MASK) + (src_addr[index_s] & B_CHANNEL_MASK);

    *dst = 0xff000000 | ((sum_r / 5) & R_CHANNEL_MASK) | ((sum_g / 5) & G_CHANNEL_MASK) | ((sum_b / 5) & B_CHANNEL_MASK);
#endif

    unsigned int sum_r = 0; //0x00ff0000
    unsigned int sum_g = 0; //0x0000ff00
    unsigned int sum_b = 0; //0x000000ff

    unsigned int c00 = (x * src_w) + y;
    unsigned int c01 = (y + 1 >= src_w) ? c00 : c00 + 1;
    unsigned int c10 = (x + 1 >= src_h) ? c00 : ((x + 1) * src_w) + y;
    unsigned int c11 = (y + 1 >= src_w) ? c00 : c10 + 1;

    sum_r = (src_addr[c00] & R_CHANNEL_MASK) + (src_addr[c01] & R_CHANNEL_MASK) +
        (src_addr[c10] & R_CHANNEL_MASK) + (src_addr[c11] & R_CHANNEL_MASK);

    sum_g = (src_addr[c00] & G_CHANNEL_MASK) + (src_addr[c01] & G_CHANNEL_MASK) +
        (src_addr[c10] & G_CHANNEL_MASK) + (src_addr[c11] & G_CHANNEL_MASK);

    sum_b = (src_addr[c00] & B_CHANNEL_MASK) + (src_addr[c01] & B_CHANNEL_MASK) +
        (src_addr[c10] & B_CHANNEL_MASK) + (src_addr[c11] & B_CHANNEL_MASK);

    *dst = 0xff000000 | ((sum_r / 4) & R_CHANNEL_MASK) |
        ((sum_g / 4) & G_CHANNEL_MASK) | ((sum_b / 4) & B_CHANNEL_MASK);
}

SDL_Surface *maru_rotozoom(SDL_Surface *rz_src, SDL_Surface *rz_dst, int angle)
{
#define PRECISION 4096
#define SHIFT 12

    int i, j;
    unsigned int dst_width = 0;
    unsigned int dst_height = 0;

    unsigned int sx = 0;
    unsigned int sy = 0;
    unsigned int row_index = 0;
    unsigned int col_index = 0;

    unsigned int *out = NULL;
    unsigned int *row = NULL;

    switch(angle) {
        case 90:
        case 270:
            dst_width = rz_dst->h;
            dst_height = rz_dst->w;
            break;
        case 0:
        case 180:
        default:
            dst_width = rz_dst->w;
            dst_height = rz_dst->h;
            break;
    }

    sx = (rz_src->w) * PRECISION / dst_width;
    sy = (rz_src->h) * PRECISION / dst_height;

    SDL_LockSurface(rz_src);

    switch(angle) {
        case 0:
            out = (unsigned int *) rz_dst->pixels;

            for (i = 0; i < dst_height; i++, out += dst_width) {
                row_index = (i * sy) >> SHIFT;
                row	= ((unsigned int *) rz_src->pixels) + (row_index * rz_src->w);

                for (j = 0; j < dst_width; j++) {
                    col_index = (sx * j) >> SHIFT;
                    //out[j] = row[col_index];
                    interpolate_pixel_cpy(&out[j], ((unsigned int *) rz_src->pixels),
                        rz_src->w, rz_src->h, row_index, col_index);
                }
            }
            break;

        case 90: //landscape
            for (i = 0; i < dst_height; i++) {
                row_index  = (i * sy) >> SHIFT;
                row = ((unsigned int *) rz_src->pixels) + (row_index * rz_src->w);
                
                out = ((unsigned int *) rz_dst->pixels) + i;

                for (j = 0; j < dst_width; j++, out += dst_height) {
                    col_index = (sx * j) >> SHIFT;
                    //out[0] = row[rz_src->w - col_index - 1];
                    interpolate_pixel_cpy(&out[0], ((unsigned int *) rz_src->pixels),
                        rz_src->w, rz_src->h, row_index, rz_src->w - col_index - 1);
                }
            }
            break;

        case 180:
            out = (unsigned int *) rz_dst->pixels;

            for (i = 0; i < dst_height; i++, out += dst_width) {
                row_index = ((dst_height - i - 1) * sy) >> SHIFT;
                row = ((unsigned int *) rz_src->pixels) + (row_index * rz_src->w);

                for (j = 0; j < dst_width; j++) {
                    col_index = (sx * j) >> SHIFT;
                    //out[dst_width - j - 1] = row[col_index];
                    interpolate_pixel_cpy(&out[dst_width - j - 1], ((unsigned int *) rz_src->pixels),
                        rz_src->w, rz_src->h, row_index, col_index);
                }
            }
            break;

        case 270: //reverse landscape
            for (i = 0; i < dst_height; i++) {
                row_index = ((dst_height - i - 1) * sy) >> SHIFT;
                row = ((unsigned int *) rz_src->pixels) + (row_index * rz_src->w);

                out = ((unsigned int *) rz_dst->pixels) + i;

                for (j = 0; j < dst_width; j++, out += dst_height) {
                    col_index = (sx * j) >> SHIFT;
                    //out[0] = row[col_index];
                    interpolate_pixel_cpy(&out[0], ((unsigned int *) rz_src->pixels),
                        rz_src->w, rz_src->h, row_index, col_index);
                }
            }
            break;

        default:
            fprintf(stdout, "not supported angle factor (angle=%d)\n", angle);
            return NULL;
    }

    SDL_UnlockSurface(rz_src);

    return rz_dst;
}

#endif /* MARU_SDL_ROTOZOOM_H_ */
