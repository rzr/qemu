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


#include <png.h>
#include "qemu/main-loop.h"
#include "emulator.h"
#include "emul_state.h"
#include "maru_display.h"
#include "maru_sdl.h"
#include "maru_sdl_processing.h"
#include "hw/pci/maru_brightness.h"
#include "debug_ch.h"

#include "tethering/encode_fb.h"

#include <SDL.h>
#ifndef CONFIG_WIN32
#include <SDL_syswm.h>
#endif

MULTI_DEBUG_CHANNEL(tizen, maru_sdl);

static QEMUBH *sdl_init_bh;
static QEMUBH *sdl_resize_bh;
static QEMUBH *sdl_update_bh;
static DisplaySurface *dpy_surface;

static SDL_Surface *surface_screen;
static SDL_Surface *surface_qemu;
static SDL_Surface *scaled_screen;
static SDL_Surface *rotated_screen;
static SDL_Surface *surface_guide; /* blank guide image */

static double current_scale_factor = 1.0;
static double current_screen_degree;
static pixman_filter_t sdl_pixman_filter;

static bool sdl_invalidate;
static int sdl_alteration;

static unsigned int sdl_skip_update;
static unsigned int sdl_skip_count;

static bool blank_guide_enable;
static unsigned int blank_cnt;
#define MAX_BLANK_FRAME_CNT 10
#define BLANK_GUIDE_IMAGE_PATH "../images/"
#define BLANK_GUIDE_IMAGE_NAME "blank-guide.png"

#ifdef SDL_THREAD
QemuMutex sdl_mutex;
QemuCond sdl_cond;
static int sdl_thread_initialized;

QemuThread sdl_thread;
static bool sdl_thread_exit;
#endif

#define SDL_FLAGS (SDL_SWSURFACE | SDL_ASYNCBLIT | SDL_NOFRAME)
#define SDL_BPP 32

static void qemu_update(void);

static void qemu_ds_sdl_update(DisplayChangeListener *dcl,
                               int x, int y, int w, int h)
{
    /* call sdl update */
#ifdef SDL_THREAD
    qemu_mutex_lock(&sdl_mutex);

    qemu_cond_signal(&sdl_cond);

    qemu_mutex_unlock(&sdl_mutex);
#else
    qemu_update();
#endif

    set_display_dirty(true);
}

static void qemu_ds_sdl_switch(DisplayChangeListener *dcl,
                               struct DisplaySurface *new_surface)
{
    int console_width = 0, console_height = 0;

    sdl_skip_update = 0;
    sdl_skip_count = 0;

    if (!new_surface) {
        ERR("qemu_ds_sdl_switch : new_surface is NULL\n");
        return;
    }

    console_width = surface_width(new_surface);
    console_height = surface_height(new_surface);

    INFO("qemu_ds_sdl_switch : (%d, %d)\n", console_width, console_height);

    /* switch */
    dpy_surface = new_surface;

    if (surface_qemu != NULL) {
        SDL_FreeSurface(surface_qemu);
        surface_qemu = NULL;
    }

    /* create surface_qemu */
    if (console_width == get_emul_resolution_width() &&
        console_height == get_emul_resolution_height()) {
        INFO("create SDL screen : (%d, %d)\n",
             console_width, console_height);

        surface_qemu = SDL_CreateRGBSurfaceFrom(
            surface_data(dpy_surface),
            console_width, console_height,
            surface_bits_per_pixel(dpy_surface),
            surface_stride(dpy_surface),
            dpy_surface->pf.rmask,
            dpy_surface->pf.gmask,
            dpy_surface->pf.bmask,
            dpy_surface->pf.amask);
    } else {
        INFO("create blank screen : (%d, %d)\n",
             get_emul_resolution_width(), get_emul_resolution_height());

        surface_qemu = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            console_width, console_height,
            surface_bits_per_pixel(dpy_surface),
            0, 0, 0, 0);
    }

    if (surface_qemu == NULL) {
        ERR("Unable to set the RGBSurface: %s\n", SDL_GetError());
        return;
    }
}

static SDL_Surface *get_blank_guide_image(void)
{
    if (surface_guide == NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        char *guide_image_path = NULL;
        void *guide_image_data = NULL;

        /* load png image */
        int path_len = strlen(get_bin_path()) +
            strlen(BLANK_GUIDE_IMAGE_PATH) +
            strlen(BLANK_GUIDE_IMAGE_NAME) + 1;
        guide_image_path = g_malloc0(sizeof(char) * path_len);
        snprintf(guide_image_path, path_len, "%s%s%s",
            get_bin_path(), BLANK_GUIDE_IMAGE_PATH,
            BLANK_GUIDE_IMAGE_NAME);

        guide_image_data = (void *) read_png_file(
            guide_image_path, &width, &height);

        if (guide_image_data != NULL) {
            surface_guide = SDL_CreateRGBSurfaceFrom(
                guide_image_data, width, height,
                get_emul_sdl_bpp(), width * 4,
                dpy_surface->pf.bmask,
                dpy_surface->pf.gmask,
                dpy_surface->pf.rmask,
                dpy_surface->pf.amask);
        } else {
            ERR("failed to draw a blank guide image\n");
        }

        g_free(guide_image_path);
    }

    return surface_guide;
}

static void qemu_ds_sdl_refresh(DisplayChangeListener *dcl)
{
    if (sdl_alteration == 1) {
        sdl_alteration = 0;
        sdl_skip_update = 0;
        sdl_skip_count = 0;
    }

    /* draw cover image */
    if (sdl_skip_update && display_off) {
        if (blank_cnt > MAX_BLANK_FRAME_CNT) {
#ifdef CONFIG_WIN32
            if (sdl_invalidate && get_emul_skin_enable()) {
                draw_image(surface_screen, get_blank_guide_image());
            }
#endif

            return;
        } else if (blank_cnt == MAX_BLANK_FRAME_CNT) {
            if (blank_guide_enable == true && get_emul_skin_enable()) {
                INFO("draw a blank guide image\n");

                draw_image(surface_screen, get_blank_guide_image());
            }
        } else if (blank_cnt == 0) {
            /* If the display is turned off,
            the screen does not update until the display is turned on */
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

    /* draw framebuffer */
    if (sdl_invalidate) {
        graphic_hw_invalidate(NULL);
    }
    graphic_hw_update(NULL);

    /* Usually, continuously updated.
       When the display is turned off,
       ten more updates the screen for a black screen. */
    if (display_off) {
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
    qemu_mutex_lock(&sdl_mutex);
#endif

    /*
    * It is necessary only for exynos4210 FIMD in connection with
    * some WM (xfwm4, for example)
    */

    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);

#ifdef SDL_THREAD
    qemu_mutex_unlock(&sdl_mutex);
#endif
#endif
}

DisplayChangeListenerOps maru_dcl_ops = {
    .dpy_name          = "maru_sdl",
    .dpy_gfx_update    = qemu_ds_sdl_update,
    .dpy_gfx_switch    = qemu_ds_sdl_switch,
    .dpy_refresh       = qemu_ds_sdl_refresh,
};

void maru_sdl_interpolation(bool on)
{
    if (on == true) {
        INFO("set PIXMAN_FILTER_BEST filter for image processing\n");

        /* PIXMAN_FILTER_BILINEAR */
        sdl_pixman_filter = PIXMAN_FILTER_BEST;
    } else {
        INFO("set PIXMAN_FILTER_FAST filter for image processing\n");

        /* PIXMAN_FILTER_NEAREST */
        sdl_pixman_filter = PIXMAN_FILTER_FAST;
    }
}

static void qemu_update(void)
{
    if (sdl_alteration < 0) {
        SDL_FreeSurface(scaled_screen);
        SDL_FreeSurface(rotated_screen);
        SDL_FreeSurface(surface_qemu);
        surface_qemu = NULL;

        return;
    }

    if (surface_qemu != NULL) {
        maru_do_pixman_dpy_surface(dpy_surface->image);

        save_screenshot(dpy_surface);

        if (current_scale_factor != 1.0) {
            rotated_screen = maru_do_pixman_rotate(
                surface_qemu, rotated_screen,
                (int)current_screen_degree);
            scaled_screen = maru_do_pixman_scale(
                rotated_screen, scaled_screen, sdl_pixman_filter);

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
            int i = 0;
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
    qemu_mutex_lock(&sdl_mutex);

    while (1) {
        qemu_cond_wait(&sdl_cond, &sdl_mutex);
        if (sdl_thread_exit) {
            INFO("make SDL Thread exit\n");
            break;
        }
        qemu_update();
    }

    qemu_mutex_unlock(&sdl_mutex);

    INFO("finish qemu_update routine\n");
    return NULL;
}
#endif

static void maru_sdl_update_bh(void *opaque)
{
    graphic_hw_invalidate(NULL);
}

static void maru_sdl_resize_bh(void *opaque)
{
    int surface_width = 0, surface_height = 0;
    int display_width = 0, display_height = 0;
    int temp = 0;

    INFO("Set up a video mode with the specified width, "
         "height and bits-per-pixel\n");

    sdl_alteration = 1;
    sdl_skip_update = 0;

#ifdef SDL_THREAD
    qemu_mutex_lock(&sdl_mutex);
#endif

    /* get current setting information and calculate screen size */
    display_width = get_emul_resolution_width();
    display_height = get_emul_resolution_height();
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

#ifdef SDL_THREAD
        qemu_mutex_unlock(&sdl_mutex);
#endif

        return;
    }

    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);

    /* create buffer for image processing */
    SDL_FreeSurface(scaled_screen);
    scaled_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        surface_width, surface_height,
        surface_qemu->format->BitsPerPixel,
        surface_qemu->format->Rmask,
        surface_qemu->format->Gmask,
        surface_qemu->format->Bmask,
        surface_qemu->format->Amask);

    SDL_FreeSurface(rotated_screen);
    rotated_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        display_width, display_height,
        surface_qemu->format->BitsPerPixel,
        surface_qemu->format->Rmask,
        surface_qemu->format->Gmask,
        surface_qemu->format->Bmask,
        surface_qemu->format->Amask);

    /* rearrange multi-touch finger points */
    if (get_emul_multi_touch_state()->multitouch_enable == 1 ||
            get_emul_multi_touch_state()->multitouch_enable == 2) {
        rearrange_finger_points(get_emul_resolution_width(), get_emul_resolution_height(),
            current_scale_factor, rotaton_type);
    }

#ifdef SDL_THREAD
    qemu_mutex_unlock(&sdl_mutex);
#endif

    graphic_hw_invalidate(NULL);
}

static void maru_sdl_init_bh(void *opaque)
{
    INFO("SDL_Init\n");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ERR("unable to init SDL: %s\n", SDL_GetError());
        // TODO:
    }

#ifndef CONFIG_WIN32
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);
#endif

    qemu_bh_schedule(sdl_resize_bh);

#ifdef SDL_THREAD
    if (sdl_thread_initialized == 0) {
        sdl_thread_initialized = 1;

        INFO("sdl update thread create\n");

        sdl_thread_exit = false;
        qemu_thread_create(&sdl_thread, "sdl-workthread", run_qemu_update,
            NULL, QEMU_THREAD_JOINABLE);
    }
#endif
}

void maru_sdl_pre_init(void) {
    sdl_init_bh = qemu_bh_new(maru_sdl_init_bh, NULL);
    sdl_resize_bh = qemu_bh_new(maru_sdl_resize_bh, NULL);
    sdl_update_bh = qemu_bh_new(maru_sdl_update_bh, NULL);

#ifdef SDL_THREAD
    qemu_mutex_init(&sdl_mutex);
    qemu_cond_init(&sdl_cond);
#endif
}

void maru_sdl_init(uint64 swt_handle,
    unsigned int display_width, unsigned int display_height,
    bool blank_guide)
{
    gchar SDL_windowhack[32] = { 0, };
    long window_id = swt_handle;
    blank_guide_enable = blank_guide;

    INFO("maru sdl init\n");

    sprintf(SDL_windowhack, "%ld", window_id);
    g_setenv("SDL_WINDOWID", SDL_windowhack, 1);

    INFO("register SDL environment variable. "
        "(SDL_WINDOWID = %s)\n", SDL_windowhack);

    set_emul_resolution(display_width, display_height);
    set_emul_sdl_bpp(SDL_BPP);
    maru_sdl_interpolation(false);
    init_multi_touch_state();

    if (blank_guide_enable == true) {
        INFO("blank guide is on\n");
    }

    qemu_bh_schedule(sdl_init_bh);
}

void maru_sdl_quit(void)
{
    INFO("maru sdl quit\n");

    if (surface_guide != NULL) {
        g_free(surface_guide->pixels);
        SDL_FreeSurface(surface_guide);
    }

    /* remove multi-touch finger points */
    cleanup_multi_touch_state();

    if (sdl_init_bh != NULL) {
        qemu_bh_delete(sdl_init_bh);
    }
    if (sdl_resize_bh != NULL) {
        qemu_bh_delete(sdl_resize_bh);
    }

    sdl_alteration = -1;

#ifdef SDL_THREAD
    qemu_mutex_lock(&sdl_mutex);
#endif

    SDL_Quit();

#ifdef SDL_THREAD
    sdl_thread_exit = true;
    qemu_cond_signal(&sdl_cond);
    qemu_mutex_unlock(&sdl_mutex);

    INFO("join SDL thread\n");
    qemu_thread_join(&sdl_thread);

    INFO("destroy cond and mutex of SDL thread\n");
    qemu_cond_destroy(&sdl_cond);
    qemu_mutex_destroy(&sdl_mutex);
#endif
}

void maru_sdl_resize(void)
{
    INFO("maru sdl resize\n");

    qemu_bh_schedule(sdl_resize_bh);
}

void maru_sdl_update(void)
{
    if (sdl_update_bh != NULL) {
        qemu_bh_schedule(sdl_update_bh);
    }
}

void maru_sdl_invalidate(bool on)
{
    sdl_invalidate = on;
}

bool maru_extract_framebuffer(void *buffer)
{
    uint32_t buffer_size = 0;

    if (!buffer) {
        ERR("given buffer is null\n");
        return false;
    }

    if (!surface_qemu) {
        ERR("surface_qemu is null\n");
        return false;
    }

    maru_do_pixman_dpy_surface(dpy_surface->image);

    buffer_size = surface_stride(dpy_surface) * surface_height(dpy_surface);
    INFO("extract framebuffer %d\n", buffer_size);

    memcpy(buffer, surface_data(dpy_surface), buffer_size);
    return true;
}
