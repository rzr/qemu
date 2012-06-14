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
#include "console.h"
#include "maru_sdl.h"
#include "emul_state.h"
#include "sdl_rotate.h"
#include "maru_finger.h"
#include "hw/maru_pm.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, maru_sdl);


// TODO : organize
SDL_Surface *surface_screen;
SDL_Surface *surface_qemu;

static double scale_factor = 1.0;
static double screen_degree = 0.0;
static int sdl_initialized = 0;

#define SDL_THREAD

static pthread_mutex_t sdl_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef SDL_THREAD
static pthread_cond_t sdl_cond = PTHREAD_COND_INITIALIZER;
static int sdl_thread_initialized = 0;
#endif

#define SDL_FLAGS (SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_NOFRAME)
#define SDL_BPP 32

DisplaySurface* qemu_display_surface = NULL;

static void qemu_update(void)
{
    int i;
    SDL_Surface *processing_screen = NULL;

    if (surface_qemu != NULL) {
        if (scale_factor != 1.0 || screen_degree != 0.0) {

            // workaround
            // set color key 'magenta'
            surface_qemu->format->colorkey = 0xFF00FF;

            //image processing
            processing_screen = rotozoomSurface(surface_qemu, screen_degree, scale_factor, 1);
            SDL_BlitSurface(processing_screen, NULL, surface_screen, NULL);
        } else {
            SDL_BlitSurface(surface_qemu, NULL, surface_screen, NULL);
        }
    }

    /* draw multi-touch finger points */
    MultiTouchState *mts = get_emul_multi_touch_state();
    if (mts->multitouch_enable == 1 && mts->finger_point_surface != NULL) {
        FingerPoint *finger = NULL;
        int finger_point_size_half = mts->finger_point_size / 2;
        SDL_Rect rect;

        for (i = 0; i < mts->finger_cnt; i++) {
            finger = get_finger_point_from_slot(i);
            if (finger != NULL && finger->id != 0) {
                rect.x = (finger->x * get_emul_win_scale()) - finger_point_size_half;
                rect.y = (finger->y * get_emul_win_scale()) - finger_point_size_half;
                rect.w = rect.h = mts->finger_point_size;

                SDL_BlitSurface((SDL_Surface *)mts->finger_point_surface, NULL, surface_screen, &rect);
            }
        }
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

static int maru_sdl_poll_event(SDL_Event *ev)
{
    int ret = 0;

    if (sdl_initialized == 1) {
        //pthread_mutex_lock(&sdl_mutex);
        ret = SDL_PollEvent(ev);
        //pthread_mutex_unlock(&sdl_mutex);
    }

    return ret;
}

static void put_hardkey_code( SDL_UserEvent event ) {

    // use pointer as integer
    int event_type = (int) event.data1;
    int keycode = (int) event.data2;

    if ( KEY_PRESSED == event_type ) {

        if ( kbd_mouse_is_absolute() ) {
            ps2kbd_put_keycode( keycode & 0x7f );
        }

    } else if ( KEY_RELEASED == event_type ) {

        if ( kbd_mouse_is_absolute() ) {
            ps2kbd_put_keycode( keycode | 0x80 );
        }

    } else {
        ERR( "Unknown hardkey event type.[event_type:%d]", event_type );
    }

}

static void handle_sdl_user_event ( SDL_UserEvent event ) {

    int code = event.code;

    switch ( code ) {
    case SDL_USER_EVENT_CODE_HARDKEY: {
        put_hardkey_code( event );
        break;
    }
    default: {
        ERR( "Unknown sdl user event.[event code:%d]\n", code );
        break;
    }
    }

}

static void qemu_ds_refresh(DisplayState *ds)
{
    SDL_Event ev1, *ev = &ev1;

    vga_hw_update();

    // surface may be NULL in init func.
    qemu_display_surface = ds->surface;

    while (maru_sdl_poll_event(ev)) {
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
            case SDL_USEREVENT: {
                handle_sdl_user_event( ev->user );
                break;
            }

            default:
                break;
        }
    }
}

void maruskin_display_init(DisplayState *ds)
{
    INFO( "qemu display initialization\n");

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

void maruskin_sdl_init(uint64 swt_handle, int lcd_size_width, int lcd_size_height)
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

    set_emul_sdl_bpp(SDL_BPP);

    INFO( "maru sdl initialization\n");
    surface_screen = SDL_SetVideoMode(w, h, get_emul_sdl_bpp(), SDL_FLAGS);
    if (surface_screen == NULL) {
        ERR("Could not open SDL display (%dx%dx%d): %s\n", w, h, get_emul_sdl_bpp(), SDL_GetError());
    }

#ifndef _WIN32
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);
#endif

    sdl_initialized = 1;
    init_multi_touch_state();
}

void maruskin_sdl_resize(void)
{
    SDL_Event ev;

    /* this fails if SDL is not initialized */
    memset(&ev, 0, sizeof(ev));
    ev.resize.type = SDL_VIDEORESIZE;

    //This function is thread safe, and can be called from other threads safely.
    SDL_PushEvent(&ev);
}

DisplaySurface* get_qemu_display_surface( void ) {
    return qemu_display_surface;
}
