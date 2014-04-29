/*
 * Image Processing
 *
 * Copyright (C) 2011 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * SangHo Park <sangho1206.park@samsung.com>
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


#ifndef MARU_SDL_PROCESSING_H_
#define MARU_SDL_PROCESSING_H_

#include <SDL.h>
#include <png.h>
#include "ui/console.h"

void maru_do_pixman_dpy_surface(pixman_image_t *dst);
SDL_Surface *maru_do_pixman_scale(SDL_Surface *src, SDL_Surface *dst, pixman_filter_t filter);
SDL_Surface *maru_do_pixman_rotate(SDL_Surface *src, SDL_Surface *dst, int angle);

png_bytep read_png_file(const char *file_name,
    unsigned int *width_out, unsigned int *height_out);
void draw_image(SDL_Surface *screen, SDL_Surface *image);

#endif /* MARU_SDL_PROCESSING_H_ */
