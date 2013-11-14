/*
 * SDL_WINDOWID hack
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Jinhyung Jo <jinhyung.jo@samsung.com>
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


#include <pthread.h>
#include <math.h>
#include <pixman.h>
#include "console.h"
#include "maru_sdl.h"
#include "maru_display.h"
#include "emul_state.h"
#include "emulator.h"
#include "maru_finger.h"
#include "hw/maru_pm.h"
#include "hw/maru_brightness.h"
#include "hw/maru_overlay.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, maru_sdl);

static QEMUBH *sdl_init_bh;
static QEMUBH *sdl_resize_bh;
static DisplayState *dpy_surface;

static SDL_Surface *surface_screen;
static SDL_Surface *surface_qemu;
static SDL_Surface *scaled_screen;
static SDL_Surface *rotated_screen;

static double current_scale_factor = 1.0;
static double current_screen_degree;

static int sdl_alteration;
static unsigned int sdl_skip_update;
static unsigned int sdl_skip_count;
static int blank_cnt;
#define MAX_BLANK_FRAME_CNT 120


#define SDL_THREAD

static pthread_mutex_t sdl_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef SDL_THREAD
static pthread_cond_t sdl_cond = PTHREAD_COND_INITIALIZER;
static int sdl_thread_initialized;
#endif

#define SDL_FLAGS (SDL_SWSURFACE | SDL_ASYNCBLIT | SDL_NOFRAME)
#define SDL_BPP 32

/* Image processing functions using the pixman library */
static SDL_Surface *maru_do_pixman_scale(SDL_Surface *rz_src,
                                         SDL_Surface *rz_dst)
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
    pixman_image_set_filter(src, PIXMAN_FILTER_BILINEAR, NULL, 0);
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst,
                           0, 0, 0, 0, 0, 0,
                           rz_dst->w, rz_dst->h);

    pixman_image_unref(src);
    pixman_image_unref(dst);

    SDL_UnlockSurface(rz_src);
    SDL_UnlockSurface(rz_dst);

    return rz_dst;
}

static SDL_Surface *maru_do_pixman_rotate(SDL_Surface *rz_src,
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
            fprintf(stdout, "not supported angle factor (angle=%d)\n", angle);
            break;
    }
    pixman_transform_from_pixman_f_transform(&matrix, &matrix_f);
    pixman_image_set_transform(src, &matrix);
    pixman_image_set_filter(src, PIXMAN_FILTER_BILINEAR, NULL, 0);
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst,
                           0, 0, 0, 0, 0, 0,
                           rz_dst->w, rz_dst->h);

    pixman_image_unref(src);
    pixman_image_unref(dst);

    SDL_UnlockSurface(rz_src);
    SDL_UnlockSurface(rz_dst);

    return rz_dst;
}

void qemu_ds_sdl_update(DisplayState *ds,
                               int x, int y, int w, int h)
{
    /* call sdl update */
#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);

    pthread_cond_signal(&sdl_cond);

    pthread_mutex_unlock(&sdl_mutex);
#else
    qemu_update();
#endif
}

void qemu_ds_sdl_switch(DisplayState *ds)
{
    int console_width = 0, console_height = 0;

    dpy_surface = ds;
    console_width = ds_get_width(ds);
    console_height = ds_get_height(ds);

    INFO("qemu_ds_sdl_switch : (%d, %d)\n",
        console_width, console_height);

    sdl_skip_update = 0;
    sdl_skip_count = 0;

#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    if (surface_qemu != NULL) {
        SDL_FreeSurface(surface_qemu);
        surface_qemu = NULL;
    }

    /* create surface_qemu */
    if (console_width == get_emul_lcd_width() &&
        console_height == get_emul_lcd_height()) {
        INFO("create SDL screen : (%d, %d)\n",
             console_width, console_height);

        surface_qemu = SDL_CreateRGBSurfaceFrom(
            ds_get_data(ds),
            console_width, console_height,
            ds_get_bits_per_pixel(ds),
            ds_get_linesize(ds),
            ds->surface->pf.rmask,
            ds->surface->pf.gmask,
            ds->surface->pf.bmask,
            ds->surface->pf.amask);
    } else {
        INFO("create blank screen : (%d, %d)\n",
             get_emul_lcd_width(), get_emul_lcd_height());

        surface_qemu = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            console_width, console_height,
            ds_get_bits_per_pixel(ds), 0, 0, 0, 0);
    }

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif

    if (surface_qemu == NULL) {
        ERR("Unable to set the RGBSurface: %s\n", SDL_GetError());
        return;
    }
}

void qemu_ds_sdl_refresh(DisplayState *ds)
{
    if (sdl_alteration == 1) {
        sdl_alteration = 0;
        sdl_skip_update = 0;
        sdl_skip_count = 0;
    }

    /* If the display is turned off,
       the screen does not update until the display is turned on */
    if (sdl_skip_update && brightness_off) {
        if (blank_cnt > MAX_BLANK_FRAME_CNT) {
            /* do nothing */
            return;
        } else if (blank_cnt == MAX_BLANK_FRAME_CNT) {
            /* draw guide image */
            // TODO:
        } else if (blank_cnt == 0) {
            INFO("skipping of the display updating is started\n");
        }

        blank_cnt++;

        return;
    } else {
        if (blank_cnt != 0) {
            INFO("skipping of the display updating is ended\n");
            blank_cnt = 0;
        }
    }

    vga_hw_update();

    /* Usually, continuously updated.
       When the display is turned off,
       ten more updates the screen for a black screen. */
    if (brightness_off) {
        if (++sdl_skip_count > 10) {
            sdl_skip_update = 1;
        } else {
            sdl_skip_update = 0;
        }
    } else {
        sdl_skip_count = 0;
        sdl_skip_update = 0;
    }

#ifdef TARGET_ARM
#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    /*
    * It is necessary only for exynos4210 FIMD in connection with
    * some WM (xfwm4, for example)
    */

    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif
#endif
}

static void qemu_update(void)
{
    if (sdl_alteration == -1) {
        SDL_FreeSurface(scaled_screen);
        SDL_FreeSurface(rotated_screen);
        SDL_FreeSurface(surface_qemu);
        surface_qemu = NULL;

        return;
    }

    if (surface_qemu != NULL) {
        int i = 0;

        set_maru_screenshot(dpy_surface);

        if (current_scale_factor != 1.0) {
            rotated_screen = maru_do_pixman_rotate(
                surface_qemu, rotated_screen,
                (int)current_screen_degree);
            scaled_screen = maru_do_pixman_scale(
                rotated_screen, scaled_screen);

            SDL_BlitSurface(scaled_screen, NULL, surface_screen, NULL);
        }
        else {/* current_scale_factor == 1.0 */
            if (current_screen_degree != 0.0) {
                rotated_screen = maru_do_pixman_rotate(
                    surface_qemu, rotated_screen,
                    (int)current_screen_degree);

                SDL_BlitSurface(rotated_screen, NULL, surface_screen, NULL);
            } else {
                /* as-is */
                SDL_BlitSurface(surface_qemu, NULL, surface_screen, NULL);
            }
        }

        /* draw multi-touch finger points */
        MultiTouchState *mts = get_emul_multi_touch_state();
        if (mts->multitouch_enable != 0 && mts->finger_point_surface != NULL) {
            FingerPoint *finger = NULL;
            int finger_point_size_half = mts->finger_point_size / 2;
            SDL_Rect rect;

            for (i = 0; i < mts->finger_cnt; i++) {
                finger = get_finger_point_from_slot(i);
                if (finger != NULL && finger->id != 0) {
                    rect.x = finger->origin_x - finger_point_size_half;
                    rect.y = finger->origin_y - finger_point_size_half;
                    rect.w = rect.h = mts->finger_point_size;

                    SDL_BlitSurface(
                        (SDL_Surface *)mts->finger_point_surface,
                        NULL, surface_screen, &rect);
                }
            }
        } /* end of draw multi-touch */
    }

    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);
}


#ifdef SDL_THREAD
static void *run_qemu_update(void *arg)
{
    while(1) {
        pthread_mutex_lock(&sdl_mutex);

        pthread_cond_wait(&sdl_cond, &sdl_mutex);

        qemu_update();

        pthread_mutex_unlock(&sdl_mutex);
    }

    return NULL;
}
#endif

static void maru_sdl_resize_bh(void *opaque)
{
    int surface_width = 0, surface_height = 0;
    int display_width = 0, display_height = 0;
    int temp = 0;

    INFO("Set up a video mode with the specified width, "
        "height and bits-per-pixel\n");

#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    sdl_alteration = 1;
    sdl_skip_update = 0;

    /* get current setting information and calculate screen size */
    display_width = get_emul_lcd_width();
    display_height = get_emul_lcd_height();
    current_scale_factor = get_emul_win_scale();

    short rotaton_type = get_emul_rotation();
    if (rotaton_type == ROTATION_PORTRAIT) {
        current_screen_degree = 0.0;
    } else if (rotaton_type == ROTATION_LANDSCAPE) {
        current_screen_degree = 90.0;
        temp = display_width;
        display_width = display_height;
        display_height = temp;
    } else if (rotaton_type == ROTATION_REVERSE_PORTRAIT) {
        current_screen_degree = 180.0;
    } else if (rotaton_type == ROTATION_REVERSE_LANDSCAPE) {
        current_screen_degree = 270.0;
        temp = display_width;
        display_width = display_height;
        display_height = temp;
    }

    surface_width = display_width * current_scale_factor;
    surface_height = display_height * current_scale_factor;

    surface_screen = SDL_SetVideoMode(
        surface_width, surface_height,
        get_emul_sdl_bpp(), SDL_FLAGS);

    INFO("SDL_SetVideoMode\n");

    if (surface_screen == NULL) {
        ERR("Could not open SDL display (%dx%dx%d) : %s\n",
            surface_width, surface_height,
            get_emul_sdl_bpp(), SDL_GetError());
        return;
    }

    /* create buffer for image processing */
    SDL_FreeSurface(scaled_screen);
    scaled_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        surface_width, surface_height,
        get_emul_sdl_bpp(),
        surface_qemu->format->Rmask,
        surface_qemu->format->Gmask,
        surface_qemu->format->Bmask,
        surface_qemu->format->Amask);

    SDL_FreeSurface(rotated_screen);
    rotated_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        display_width, display_height,
        get_emul_sdl_bpp(),
        surface_qemu->format->Rmask,
        surface_qemu->format->Gmask,
        surface_qemu->format->Bmask,
        surface_qemu->format->Amask);

    /* rearrange multi-touch finger points */
    if (get_emul_multi_touch_state()->multitouch_enable == 1 ||
            get_emul_multi_touch_state()->multitouch_enable == 2) {
        rearrange_finger_points(get_emul_lcd_width(), get_emul_lcd_height(),
            current_scale_factor, rotaton_type);
    }

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif
}

static void maru_sdl_init_bh(void *opaque)
{
    SDL_SysWMinfo info;

    INFO("SDL_Init\n");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ERR("unable to init SDL: %s\n", SDL_GetError());
        //TODO:
    }

#ifndef _WIN32
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);
#endif

    qemu_bh_schedule(sdl_resize_bh);

#ifdef SDL_THREAD
    if (sdl_thread_initialized == 0) {
        sdl_thread_initialized = 1;

        INFO("sdl update thread create\n");

        pthread_t thread_id;
        if (pthread_create(
            &thread_id, NULL, run_qemu_update, NULL) != 0) {
            ERR("pthread_create fail\n");
            return;
        }
    }
#endif
}

void maruskin_sdl_init(uint64 swt_handle,
    int lcd_size_width, int lcd_size_height)
{
    gchar SDL_windowhack[32] = { 0, };
    long window_id = swt_handle;

    INFO("maru sdl init\n");

    sdl_init_bh = qemu_bh_new(maru_sdl_init_bh, NULL);
    sdl_resize_bh = qemu_bh_new(maru_sdl_resize_bh, NULL);

    sprintf(SDL_windowhack, "%ld", window_id);
    g_setenv("SDL_WINDOWID", SDL_windowhack, 1);

    INFO("register SDL environment variable. "
        "(SDL_WINDOWID = %s)\n", SDL_windowhack);

    set_emul_lcd_size(lcd_size_width, lcd_size_height);
    set_emul_sdl_bpp(SDL_BPP);
    init_multi_touch_state();

    qemu_bh_schedule(sdl_init_bh);
}

void maruskin_sdl_quit(void)
{
    INFO("maru sdl quit\n");

    /* remove multi-touch finger points */
    cleanup_multi_touch_state();

    if (sdl_init_bh != NULL) {
        qemu_bh_delete(sdl_init_bh);
    }
    if (sdl_resize_bh != NULL) {
        qemu_bh_delete(sdl_resize_bh);
    }

#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    sdl_alteration = -1;

    SDL_Quit();

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
    pthread_cond_destroy(&sdl_cond);
#endif

    pthread_mutex_destroy(&sdl_mutex);
}

void maruskin_sdl_resize(void)
{
    INFO("maru sdl resize\n");

    qemu_bh_schedule(sdl_resize_bh);
}
