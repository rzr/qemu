/*
 * MARU display driver
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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


#include "emulator.h"
#include "emulator_common.h"
#include "maru_display.h"
#include "debug_ch.h"

#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
#include "maru_sdl.h"
#endif
#else
#include "maru_shm.h"
#endif

MULTI_DEBUG_CHANNEL(tizen, display);

MaruScreenShot* screenshot = NULL;

static void maru_display_fini(void)
{
    INFO("fini qemu display\n");

    g_free(screenshot);

#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_quit();
#endif
#else
    maru_shm_quit();
#endif
}

static void maru_display_notify_exit(Notifier *notifier, void *data) {
    maru_display_fini();
}
static Notifier maru_display_exit = { .notify = maru_display_notify_exit };

//TODO: interface
void maru_display_init(DisplayState *ds)
{
    INFO("init qemu display\n");

#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_pre_init();
#endif
#else
    /* do nothing */
#endif

    /*  graphics context information */
    DisplayChangeListener *dcl;

    dcl = g_malloc0(sizeof(DisplayChangeListener));
#if defined(CONFIG_USE_SHM) || defined(CONFIG_SDL)
    //FIXME
    dcl->ops = &maru_dcl_ops;
#endif
    register_displaychangelistener(dcl);

    screenshot = g_malloc0(sizeof(MaruScreenShot));
    screenshot->pixels = NULL;
    screenshot->request = false;
    screenshot->ready = false;

    emulator_add_exit_notifier(&maru_display_exit);
}

void maru_display_resize(void)
{
#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_resize();
#endif
#else
    maru_shm_resize();
#endif
}

void maru_display_update(void)
{
#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_update();
#endif
#else
    /* do nothing */
#endif
}

void maru_display_invalidate(bool on)
{
#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_invalidate(on);
#endif
#else
    /* do nothing */
#endif
}

void maru_display_interpolation(bool on)
{
#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_interpolation(on);
#endif
#else
    /* do nothing */
#endif
}

void maru_ds_surface_init(uint64 swt_handle,
    unsigned int display_width, unsigned int display_height,
    bool blank_guide)
{
#ifndef CONFIG_USE_SHM
#ifdef CONFIG_SDL
    maru_sdl_init(swt_handle,
        display_width, display_height, blank_guide);
#endif
#else
    maru_shm_init(swt_handle,
        display_width, display_height, blank_guide);
#endif
}

MaruScreenShot *get_screenshot(void)
{
    return screenshot;
}

/* save_screenshot() implemented in maruskin_operation.c */
