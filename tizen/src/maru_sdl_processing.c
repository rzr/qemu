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


#include "maru_sdl_processing.h"
#include "hw/maru_overlay.h"
#include "hw/maru_brightness.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, sdl_processing);


/* Image processing functions using the pixman library */
void maru_do_pixman_dpy_surface(pixman_image_t *dst_image)
{
    /* overlay0 */
    if (overlay0_power) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               overlay0_image, NULL, dst_image,
                               0, 0, 0, 0, overlay0_left, overlay0_top,
                               overlay0_width, overlay0_height);
    }
    /* overlay1 */
    if (overlay1_power) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               overlay1_image, NULL, dst_image,
                               0, 0, 0, 0, overlay1_left, overlay1_top,
                               overlay1_width, overlay1_height);
    }
    /* apply the brightness level */
    if (brightness_level < BRIGHTNESS_MAX) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               brightness_image, NULL, dst_image,
                               0, 0, 0, 0, 0, 0,
                               pixman_image_get_width(dst_image),
                               pixman_image_get_height(dst_image));
    }
}

SDL_Surface *maru_do_pixman_scale(SDL_Surface *rz_src,
                                  SDL_Surface *rz_dst,
                                  pixman_filter_t filter)
{
    pixman_image_t *src = NULL;
    pixman_image_t *dst = NULL;
    double sx = 0;
    double sy = 0;
    pixman_transform_t matrix;
    struct pixman_f_transform matrix_f;

    SDL_LockSurface(rz_src);
    SDL_LockSurface(rz_dst);

    src = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_src->w, rz_src->h, rz_src->pixels, rz_src->w * 4);
    dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_dst->w, rz_dst->h, rz_dst->pixels, rz_dst->w * 4);

    sx = (double)rz_src->w / (double)rz_dst->w;
    sy = (double)rz_src->h / (double)rz_dst->h;
    pixman_f_transform_init_identity(&matrix_f);
    pixman_f_transform_scale(&matrix_f, NULL, sx, sy);
    pixman_transform_from_pixman_f_transform(&matrix, &matrix_f);
    pixman_image_set_transform(src, &matrix);
    pixman_image_set_filter(src, filter, NULL, 0);
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst,
                           0, 0, 0, 0, 0, 0,
                           rz_dst->w, rz_dst->h);

    pixman_image_unref(src);
    pixman_image_unref(dst);

    SDL_UnlockSurface(rz_src);
    SDL_UnlockSurface(rz_dst);

    return rz_dst;
}

SDL_Surface *maru_do_pixman_rotate(SDL_Surface *rz_src,
                                   SDL_Surface *rz_dst,
                                   int angle)
{
    pixman_image_t *src = NULL;
    pixman_image_t *dst = NULL;
    pixman_transform_t matrix;
    struct pixman_f_transform matrix_f;

    SDL_LockSurface(rz_src);
    SDL_LockSurface(rz_dst);

    src = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_src->w, rz_src->h, rz_src->pixels, rz_src->w * 4);
    dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_dst->w, rz_dst->h, rz_dst->pixels, rz_dst->w * 4);

    pixman_f_transform_init_identity(&matrix_f);
    switch(angle) {
        case 0:
            pixman_f_transform_rotate(&matrix_f, NULL, 1.0, 0.0);
            pixman_f_transform_translate(&matrix_f, NULL, 0.0, 0.0);
            break;
        case 90:
            pixman_f_transform_rotate(&matrix_f, NULL, 0.0, 1.0);
            pixman_f_transform_translate(&matrix_f, NULL,
                                         (double)rz_dst->h, 0.0);
            break;
        case 180:
            pixman_f_transform_rotate(&matrix_f, NULL, -1.0, 0.0);
            pixman_f_transform_translate(&matrix_f, NULL,
                                         (double)rz_dst->w, (double)rz_dst->h);
            break;
        case 270:
            pixman_f_transform_rotate(&matrix_f, NULL, 0.0, -1.0);
            pixman_f_transform_translate(&matrix_f, NULL,
                                         0.0, (double)rz_dst->w);
            break;
        default:
            ERR("Not supported angle factor (angle=%d)\n", angle);
            break;
    }
    pixman_transform_from_pixman_f_transform(&matrix, &matrix_f);
    pixman_image_set_transform(src, &matrix);
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst,
                           0, 0, 0, 0, 0, 0,
                           rz_dst->w, rz_dst->h);

    pixman_image_unref(src);
    pixman_image_unref(dst);

    SDL_UnlockSurface(rz_src);
    SDL_UnlockSurface(rz_dst);

    return rz_dst;
}

png_bytep read_png_file(const char *file_name,
    unsigned int *width_out, unsigned int *height_out)
{
#define PNG_HEADER_SIZE 8

    FILE *fp = NULL;
    png_byte header[PNG_HEADER_SIZE] = { 0, };
    png_structp png_ptr = NULL;

    png_infop info_ptr = NULL;
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    png_byte channels = 0;
    unsigned int stride = 0;
    int bit_depth = 0;
    int color_type = 0;
    int i = 0;

    png_bytep pixel_data = NULL;
    png_bytepp row_ptr_data = NULL;

    if (file_name == NULL) {
        ERR("file name is empty\n");
        return NULL;
    }

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        ERR("file %s could not be opened\n", file_name);
        return NULL;
    }

    if (fread(header, sizeof(png_byte), PNG_HEADER_SIZE, fp) != PNG_HEADER_SIZE) {
        ERR("failed to read header from png file\n");
        fclose(fp);
        return NULL;
    }

    if (png_sig_cmp(header, 0, PNG_HEADER_SIZE) != 0) {
        ERR("file %s is not recognized as a PNG image\n", file_name);
        fclose(fp);
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        ERR("failed to allocate png read struct\n");
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        ERR("failed to allocate png info struct\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)) != 0) {
        ERR("error during init_io\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        png_destroy_info_struct(png_ptr, &info_ptr);
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, PNG_HEADER_SIZE);

    /* read the PNG image information */
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr,
        &width, &height, &bit_depth, &color_type,
        NULL, NULL, NULL);

    channels = png_get_channels(png_ptr, info_ptr);
    stride = width * bit_depth * channels / 8;

    pixel_data = (png_bytep) g_malloc0(stride * height);
    if (pixel_data == NULL) {
        ERR("could not allocate data buffer for pixels\n");

        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        png_destroy_info_struct(png_ptr, &info_ptr);
        fclose(fp);
        return NULL;
    }

    row_ptr_data = (png_bytepp) g_malloc0(sizeof(png_bytep) * height);
    if (row_ptr_data == NULL) {
        ERR("could not allocate data buffer for row_ptr\n");

        g_free(pixel_data);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        png_destroy_info_struct(png_ptr, &info_ptr);
        fclose(fp);
        return NULL;
    }

    switch(color_type) {
        case PNG_COLOR_TYPE_PALETTE :
            png_set_palette_to_rgb(png_ptr);
            break;
        case PNG_COLOR_TYPE_RGB :
            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
                /* transparency data for image */
                png_set_tRNS_to_alpha(png_ptr);
            } else {
                png_set_filter(png_ptr, 0xff, PNG_FILLER_AFTER);
            }
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA :
            break;
        default :
            INFO("png file has an unsupported color type\n");
            break;
    }

    for (i = 0; i < height; i++) {
        row_ptr_data[i] = pixel_data + (stride * i);
    }

    /* read the entire image into memory */
    png_read_image(png_ptr, row_ptr_data);

    /* image information */
    INFO("=== blank guide image was loaded ===============\n");
    INFO("file path : %s\n", file_name);
    INFO("width : %d, height : %d, stride : %d\n",
        width, height, stride);
    INFO("color type : %d, channels : %d, bit depth : %d\n",
        color_type, channels, bit_depth);
    INFO("================================================\n");

    if (width_out != NULL) {
        *width_out = (unsigned int) width;
    }
    if (height_out != NULL) {
        *height_out = (unsigned int) height;
    }

    g_free(row_ptr_data);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    png_destroy_info_struct(png_ptr, &info_ptr);
    fclose(fp);

    return pixel_data;
}

void draw_image(SDL_Surface *screen, SDL_Surface *image)
{
    if (screen == NULL || image == NULL) {
        return;
    }

    int dst_x = 0; int dst_y = 0;
    int dst_w = 0; int dst_h = 0;

    int margin_w = screen->w - image->w;
    int margin_h = screen->h - image->h;

    if (margin_w < 0 || margin_h < 0) {
        /* guide image scaling */
        int margin = (margin_w < margin_h)? margin_w : margin_h;
        dst_w = image->w + margin;
        dst_h = image->h + margin;

        SDL_Surface *scaled_image = SDL_CreateRGBSurface(
                SDL_SWSURFACE, dst_w, dst_h,
                image->format->BitsPerPixel,
                image->format->Rmask, image->format->Gmask,
                image->format->Bmask, image->format->Amask);

        scaled_image = maru_do_pixman_scale(
                image, scaled_image, PIXMAN_FILTER_BEST);

        dst_x = (screen->w - dst_w) / 2;
        dst_y = (screen->h - dst_h) / 2;
        SDL_Rect dst_rect = { dst_x, dst_y, dst_w, dst_h };

        SDL_BlitSurface(scaled_image, NULL, screen, &dst_rect);
        SDL_UpdateRect(screen, 0, 0, 0, 0);

        SDL_FreeSurface(scaled_image);
    } else {
        dst_w = image->w;
        dst_h = image->h;
        dst_x = (screen->w - dst_w) / 2;
        dst_y = (screen->h - dst_h) / 2;
        SDL_Rect dst_rect = { dst_x, dst_y, dst_w, dst_h };

        SDL_BlitSurface(image, NULL, screen, &dst_rect);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
    }
}
