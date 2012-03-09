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
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, maru_sdl);


// TODO : organize
SDL_Surface *surface_screen;
SDL_Surface *surface_qemu;
DisplayState *qemu_ds;

#define SDL_THREAD

#ifdef SDL_THREAD
static pthread_mutex_t sdl_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sdl_cond = PTHREAD_COND_INITIALIZER;
static int sdl_thread_initialized = 0;
#endif

static void qemu_update(void)
{
    SDL_Surface *surface = NULL;

    if (!qemu_ds) {
        return;
    }

#ifndef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    surface = SDL_GetVideoSurface();
    SDL_BlitSurface(surface_qemu, NULL, surface_screen, NULL);
    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);

#ifndef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif
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
    TRACE("%d, %d\n", ds_get_width(qemu_ds), ds_get_height(qemu_ds));

    /*if (ds_get_width(qemu_ds) == 720 && ds_get_height(qemu_ds) == 400) {
        TRACE( "blanking BIOS\n");
        surface_qemu = NULL;
        return;
    }*/

#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    /* create surface_qemu */
    surface_qemu = SDL_CreateRGBSurfaceFrom(ds_get_data(qemu_ds),
            ds_get_width(qemu_ds),
            ds_get_height(qemu_ds),
            ds_get_bits_per_pixel(qemu_ds),
            ds_get_linesize(qemu_ds),
            qemu_ds->surface->pf.rmask,
            qemu_ds->surface->pf.gmask,
            qemu_ds->surface->pf.bmask,
            qemu_ds->surface->pf.amask);

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif

    if (!surface_qemu) {
        ERR( "Unable to set the RGBSurface: %s", SDL_GetError() );
        return;
    }

}

static void qemu_ds_refresh(DisplayState *ds)
{
    vga_hw_update();
}


void maruskin_display_init(DisplayState *ds)
{
    INFO( "qemu_display_init\n");
    /*  graphics context information */
    DisplayChangeListener *dcl;

    qemu_ds = ds;

    dcl = g_malloc0(sizeof(DisplayChangeListener));
    dcl->dpy_update = qemu_ds_update;
    dcl->dpy_resize = qemu_ds_resize;
    dcl->dpy_refresh = qemu_ds_refresh;

    register_displaychangelistener(qemu_ds, dcl);

#ifdef SDL_THREAD
    if (sdl_thread_initialized == 0 ) {
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


void maruskin_sdl_init(int swt_handle)
{
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

    surface_screen = SDL_SetVideoMode(480, 800, 0,
            SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_NOFRAME);

#ifndef _WIN32
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);
    //  opengl_exec_set_parent_window(info.info.x11.display, info.info.x11.window);
#endif
}
