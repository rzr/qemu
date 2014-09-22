/*
 * emulator controller client
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  SangHo Park <sangho1206.park@samsung.com>
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

#include "emulator_common.h"
#include "emul_state.h"
#include "skin/maruskin_operation.h"
#include "encode_fb.h"

#if defined(CONFIG_LINUX) && defined(ENCODE_DEBUG)
#include <time.h>
#endif
#ifdef CONFIG_WEBP
#include <webp/types.h>
#include <webp/encode.h>
#endif

#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(app_tethering);

#ifdef CONFIG_WEBP
/*
 *  webp functions
 */
static void *encode_webp(void)
{
    int width = 0, height = 0, image_stride = 0;
    size_t ret = 0;

    struct encode_mem *container = NULL;
    uint8_t *surface = NULL;
    uint32_t surface_size = 0;

    container = g_malloc(sizeof(struct encode_mem));
    if (!container) {
        LOG_SEVERE("failed to allocate encode_mem\n");
        return NULL;
    }

    container->buffer = NULL;
    container->length = 0;

    width = get_emul_resolution_width();
    height = get_emul_resolution_height();

    image_stride = width * 4;
    LOG_TRACE("width %d, height %d, stride %d, raw image %d\n",
        width, height, image_stride, (image_stride * height));

    surface_size = width * height * 4;

    surface = g_malloc0(surface_size);
    if (!surface) {
        LOG_SEVERE("failed to allocate framebuffer\n");
        return NULL;
    }

    if (!maru_extract_framebuffer(surface)) {
        LOG_SEVERE("failed to extract framebuffer\n");
        g_free(surface);
        return NULL;
    }

    container = g_malloc(sizeof(struct encode_mem));
    if (!container) {
        LOG_SEVERE("failed to allocate encode_mem\n");
        g_free(surface);
        return NULL;
    }

    container->buffer = NULL;
    container->length = 0;

    ret = WebPEncodeLosslessBGRA((const uint8_t *)surface, width,
            height, image_stride, &container->buffer);
    LOG_TRACE("lossless encode framebuffer via webp. result %zu\n", ret);

    container->length = (int)ret;

    g_free(surface);

    return container;
}
#endif

#ifdef CONFIG_PNG
/*
 *  png functions
 */
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
        LOG_SEVERE("failed to allocate \n");
    } else {
        memcpy(p->buffer + p->length, data, len);
        p->length += len;
    }
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
    uint8_t *surface = NULL;
    uint32_t surface_size = 0;

    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytepp row_pointers = NULL;

    width = get_emul_resolution_width();
    height = get_emul_resolution_height();

    image_stride = width * 4;
    LOG_TRACE("width %d, height %d, stride %d, raw image %d\n",
        width, height, image_stride, (image_stride * height));

    surface_size = width * height * 4;

    surface = g_malloc0(surface_size);
    if (!surface) {
        LOG_SEVERE("failed to allocate framebuffer\n");
        return NULL;
    }

    if (!maru_extract_framebuffer(surface)) {
        LOG_SEVERE("failed to extract framebuffer\n");
        g_free(surface);
        return NULL;
    }

    LOG_TRACE("png_create_write_struct\n");
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        LOG_SEVERE("png_create_write_struct failure\n");
        g_free(surface);
        return NULL;
    }

    LOG_TRACE("png_create_info_struct\n");
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        LOG_SEVERE("png_create_info_struct failure\n");
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return NULL;
    }

    LOG_TRACE("try png_jmpbuf\n");
    if (setjmp(png_jmpbuf(png_ptr))) {
        LOG_SEVERE("png_jmpbuf failure\n");
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);
        return NULL;
    }

    LOG_TRACE("png_init_io\n");
    container = g_malloc(sizeof(struct encode_mem));
    if (!container) {
        LOG_SEVERE("failed to allocate encode_mem\n");
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
        LOG_SEVERE("failed to allocate png memory\n");
        g_free(surface);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);
        return NULL;
    }

    for (row_index = 0; row_index < height; row_index++) {
        row_pointers[row_index] = surface + (row_index * image_stride);
    }

    LOG_TRACE("png_write_image\n");
    png_write_image(png_ptr, row_pointers);

    LOG_TRACE("png_write_end\n");
    png_write_end(png_ptr, info_ptr);

    g_free(surface);

    LOG_TRACE("png image size %d\n", container->length);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    png_destroy_info_struct(png_ptr, &info_ptr);

    return container;
}
#endif

void *encode_framebuffer(int encoder)
{
    void *output = NULL;

#ifdef CONFIG_PNG
#if defined(CONFIG_LINUX) && defined(ENCODE_DEBUG)
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
#endif

    output = encode_png();

#if defined(CONFIG_LINUX) && defined(ENCODE_DEBUG)
    clock_gettime(CLOCK_MONOTONIC, &end);

    LOG_TRACE("encoding time: %.5f seconds\n",
        ((double)end.tv_sec + (1.0e-9 * end.tv_nsec)) -
        ((double)start.tv_sec + (1.0e-9 * start.tv_nsec)));
#endif
#endif

    return output;
}

static bool display_dirty = false;

void set_display_dirty(bool dirty)
{
    LOG_TRACE("qemu display update: %d\n", display_dirty);
    display_dirty = dirty;
}

bool is_display_dirty(void)
{
    return display_dirty;
}
