/*
 * emulator controller client
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include <png.h>
#ifdef CONFIG_WEBP
#include <webp/types.h>
#include <webp/encode.h>
#endif

#include "emulator_common.h"
#include "emul_state.h"
#include "skin/maruskin_operation.h"
#include "encode_fb.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, app_tethering);

#ifdef CONFIG_WEBP
static void *encode_webp(void);
#endif
static void *encode_png(void);

void *encode_framebuffer(int encoder)
{
    void *output = NULL;

#if defined(CONFIG_LINUX) && defined(ENCODE_DEBUG)
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
#endif

#ifdef CONFIG_WEBP
    if (encoder == 0) {
        output = encode_webp();
    } else if (encoder == 1) {
        output = encode_png();
    }
#else
    output = encode_png();
#endif

#if defined(CONFIG_LINUX) && defined(ENCODE_DEBUG)
    clock_gettime(CLOCK_MONOTONIC, &end);

    INFO("encoding time: %.5f seconds\n",
        ((double)end.tv_sec + (1.0e-9 * end.tv_nsec)) -
        ((double)start.tv_sec + (1.0e-9 * start.tv_nsec)));
#endif

    return output;
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t len)
{
    struct encode_mem *p = (struct encode_mem *)png_get_io_ptr(png_ptr);
    size_t nsize = p->length + len;

    if (p->buffer) {
        p->buffer = g_realloc(p->buffer, nsize);
    } else {
        p->buffer = g_malloc(nsize);
    }

    if (!p->buffer) {
        ERR("failed to allocate \n");
    }

    memcpy(p->buffer + p->length, data, len);
    p->length += len;
}

static void user_flush_data(png_structp png_ptr)
{
}

static void *encode_png(void)
{
    int width = 0, height = 0, image_stride = 0;
    int row_index;
    /*
     * bit_depth: depending on color type
     * in case of RGB_ALPHA, 8 or 16
     */
    int bit_depth = 8;
    struct encode_mem *container = NULL;

    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytepp row_pointers = NULL;

    Framebuffer *surface = NULL;

    surface = request_screenshot();
    if (!surface) {
        ERR("failed to get framebuffer\n");
        return NULL;
    }

    width = get_emul_resolution_width();
    height = get_emul_resolution_height();

    image_stride = width * 4;
    TRACE("width %d, height %d, stride %d, raw image %d\n",
        width, height, image_stride, (image_stride * height));

    TRACE("png_create_write_struct\n");
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        ERR("png_create_write_struct failure\n");
        g_free(surface->data);
        g_free(surface);
        return NULL;
    }

    TRACE("png_create_info_struct\n");
    info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr) {
        ERR("png_create_info_struct failure\n");
        g_free(surface->data);
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return NULL;
    }

    TRACE("try png_jmpbuf\n");
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERR("png_jmpbuf failure\n");
        g_free(surface->data);
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);
        return NULL;
    }

    TRACE("png_init_io\n");
    container = g_malloc(sizeof(struct encode_mem));
    if (!container) {
        ERR("failed to allocate encode_mem\n");
        g_free(surface->data);
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);
        return NULL;
    }

    container->buffer = NULL;
    container->length = 0;

    png_set_write_fn(png_ptr, container, user_write_data, user_flush_data);

    // set image attributes
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
        PNG_COLOR_TYPE_RGB_ALPHA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    // Filp BGR pixels to RGB
    png_set_bgr(png_ptr);

    row_pointers = png_malloc(png_ptr, sizeof(png_bytep) * height);
    if (row_pointers == NULL) {
        ERR("failed to allocate png memory\n");
        g_free(surface->data);
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);
        return NULL;
    }

    for (row_index = 0; row_index < height; row_index++) {
        row_pointers[row_index] = surface->data + (row_index * image_stride);
    }

    TRACE("png_write_image\n");
    png_write_image(png_ptr, row_pointers);

    TRACE("png_write_end\n");
    png_write_end(png_ptr, info_ptr);

    g_free(surface->data);
    g_free(surface);

    TRACE("png image size %d\n", container->length);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    png_destroy_info_struct(png_ptr, &info_ptr);

    return container;
}

#ifdef CONFIG_WEBP
static void *encode_webp(void)
{
    int width = 0, height = 0, image_stride = 0;
    // float quality = 0;
    size_t ret = 0;

    struct encode_mem *container = NULL;
    Framebuffer *surface = NULL;

    container = g_malloc(sizeof(struct encode_mem));
    if (!container) {
        ERR("failed to allocate encode_mem\n");
        return NULL;
    }

    container->buffer = NULL;
    container->length = 0;

    surface = request_screenshot();
    if (!surface) {
        ERR("failed to get framebuffer\n");
        g_free(container);
        return NULL;
    }

    width = get_emul_resolution_width();
    height = get_emul_resolution_height();

    image_stride = width * 4;
    TRACE("width %d, height %d, stride %d, raw image %d\n",
        width, height, image_stride, (image_stride * height));

    ret = WebPEncodeLosslessBGRA((const uint8_t *)surface->data, width,
            height, image_stride, &container->buffer);
    TRACE("lossless encode framebuffer via webp. result %zu\n", ret);

    container->length = (int)ret;

    g_free(surface->data);
    g_free(surface);

    return container;
}
#endif
