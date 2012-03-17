/*
 * MARU SDL display driver
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


#include <pthread.h>
#include "maru_sdl.h"
#include "emul_state.h"
#include "sdl_rotate.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, maru_sdl);


// TODO : organize
SDL_Surface *surface_screen;
SDL_Surface *surface_qemu;

static double scale_factor = 1.0;
static double screen_degree = 0.0;

#define SDL_THREAD

static pthread_mutex_t sdl_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef SDL_THREAD
static pthread_cond_t sdl_cond = PTHREAD_COND_INITIALIZER;
static int sdl_thread_initialized = 0;
#endif

#define SDL_FLAGS (SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_NOFRAME)
#define SDL_BPP 32

static void qemu_update(void)
{
    SDL_Surface *processing_screen = NULL;

    if (scale_factor != 1.0 || screen_degree != 0.0) {
        //image processing
        processing_screen = rotozoomSurface(surface_qemu, screen_degree, scale_factor, 1);
        SDL_BlitSurface(processing_screen, NULL, surface_screen, NULL);
    } else {
        SDL_BlitSurface(surface_qemu, NULL, surface_screen, NULL);
    }
    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);

    SDL_FreeSurface(processing_screen);
}


#ifdef SDL_THREAD
static void* run_qemu_update(void* arg)
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

static void qemu_ds_update(DisplayState *ds, int x, int y, int w, int h)
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

static void qemu_ds_resize(DisplayState *ds)
{
    TRACE("%d, %d\n", ds_get_width(ds), ds_get_height(ds));

#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    /* create surface_qemu */
    surface_qemu = SDL_CreateRGBSurfaceFrom(ds_get_data(ds),
            ds_get_width(ds),
            ds_get_height(ds),
            ds_get_bits_per_pixel(ds),
            ds_get_linesize(ds),
            ds->surface->pf.rmask,
            ds->surface->pf.gmask,
            ds->surface->pf.bmask,
            ds->surface->pf.amask);

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif

    if (surface_qemu == NULL) {
        ERR( "Unable to set the RGBSurface: %s", SDL_GetError() );
        return;
    }

}

static void qemu_ds_refresh(DisplayState *ds)
{
    SDL_Event ev1, *ev = &ev1;

    vga_hw_update();

    while (SDL_PollEvent(ev)) {
        switch (ev->type) {
            case SDL_VIDEORESIZE:
            {
                int w, h, temp;

                //get current setting information and calculate screen size
                scale_factor = get_emul_win_scale();
                w = get_emul_lcd_width() * scale_factor;
                h = get_emul_lcd_height() * scale_factor;

                short rotaton_type = get_emul_rotation();
                if (rotaton_type == ROTATION_PORTRAIT) {
                    screen_degree = 0.0;
                } else if (rotaton_type == ROTATION_LANDSCAPE) {
                    screen_degree = 90.0;
                    temp = w;
                    w = h;
                    h = temp;
                } else if (rotaton_type == ROTATION_REVERSE_PORTRAIT) {
                    screen_degree = 180.0;
                } else if (rotaton_type == ROTATION_REVERSE_LANDSCAPE) {
                    screen_degree = 270.0;
                    temp = w;
                    w = h;
                    h = temp;
                }

                pthread_mutex_lock(&sdl_mutex);

                SDL_Quit(); //The returned surface is freed by SDL_Quit and must not be freed by the caller
                surface_screen = SDL_SetVideoMode(w, h, SDL_BPP, SDL_FLAGS);
                if (surface_screen == NULL) {
                    ERR("Could not open SDL display (%dx%dx%d): %s\n", w, h, SDL_BPP, SDL_GetError());
                }

                pthread_mutex_unlock(&sdl_mutex);

                break;
            }

            default:
                break;
        }
    }
}

void maruskin_display_init(DisplayState *ds)
{
    INFO( "qemu display initialize\n");

    /*  graphics context information */
    DisplayChangeListener *dcl;

    dcl = g_malloc0(sizeof(DisplayChangeListener));
    dcl->dpy_update = qemu_ds_update;
    dcl->dpy_resize = qemu_ds_resize;
    dcl->dpy_refresh = qemu_ds_refresh;

    register_displaychangelistener(ds, dcl);

#ifdef SDL_THREAD
    if (sdl_thread_initialized == 0) {
        sdl_thread_initialized = 1;
        pthread_t thread_id;
        INFO( "sdl update thread create\n");
        if (pthread_create(&thread_id, NULL, run_qemu_update, NULL) != 0) {
            ERR( "pthread_create fail\n");
            return;
        }
    }
#endif
}

void maruskin_sdl_init(int swt_handle, int lcd_size_width, int lcd_size_height)
{
    int w, h;
    gchar SDL_windowhack[32];
    SDL_SysWMinfo info;
    long window_id = swt_handle;

    sprintf(SDL_windowhack, "%ld", window_id);
    g_setenv("SDL_WINDOWID", SDL_windowhack, 1);
    INFO("register SDL environment variable. (SDL_WINDOWID = %s)\n", SDL_windowhack);

    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        ERR( "unable to init SDL: %s", SDL_GetError() );
        exit(1);
    }

    set_emul_lcd_size(lcd_size_width, lcd_size_height);

    //get current setting information and calculate screen size
    scale_factor = get_emul_win_scale();
    w = lcd_size_width * scale_factor;
    h = lcd_size_height * scale_factor;

    INFO( "qemu_sdl_initialize\n");
    surface_screen = SDL_SetVideoMode(w, h, SDL_BPP, SDL_FLAGS);
    if (surface_screen == NULL) {
        ERR("Could not open SDL display (%dx%dx%d): %s\n", w, h, SDL_BPP, SDL_GetError());
    }

#ifndef _WIN32
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);
#endif
}

void maruskin_sdl_resize()
{
    SDL_Event ev;

    /* this fails if SDL is not initialized */
    memset(&ev, 0, sizeof(ev));
    ev.resize.type = SDL_VIDEORESIZE;

    SDL_PushEvent(&ev);
}
