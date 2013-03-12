/*
 * MARU display driver
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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


#include "maru_common.h"
#include "maru_display.h"
#include "debug_ch.h"

#ifndef CONFIG_USE_SHM
#include "maru_sdl.h"
#else
#include "maru_shm.h"

#if defined(TARGET_I386)
extern void maru_vga_common_fini(void);
#endif
#endif


MULTI_DEBUG_CHANNEL(tizen, display);

MaruScreenshot* maru_screenshot = NULL;

//TODO: interface
void maru_display_init(DisplayState *ds)
{
    INFO("init qemu display\n");

    /*  graphics context information */
    DisplayChangeListener *dcl;

    dcl = g_malloc0(sizeof(DisplayChangeListener));
#ifndef CONFIG_USE_SHM
    /* sdl library */
    dcl->dpy_update = qemu_ds_sdl_update;
    dcl->dpy_resize = qemu_ds_sdl_resize;
    dcl->dpy_refresh = qemu_ds_sdl_refresh;
#else
    /* shared memroy */
    dcl->dpy_update = qemu_ds_shm_update;
    dcl->dpy_resize = qemu_ds_shm_resize;
    dcl->dpy_refresh = qemu_ds_shm_refresh;
#endif

    register_displaychangelistener(ds, dcl);

    maru_screenshot = g_malloc0(sizeof(MaruScreenshot));
    maru_screenshot->pixel_data = NULL;
    maru_screenshot->request_screenshot = 0;
    maru_screenshot->isReady = 0;
}

void maru_display_fini(void)
{
    INFO("fini qemu display\n");

    g_free(maru_screenshot);

#ifndef CONFIG_USE_SHM
    maruskin_sdl_quit();
#else
#if defined(TARGET_I386)
    maru_vga_common_fini();
#endif
#endif
}

void maruskin_init(uint64 swt_handle, int lcd_size_width, int lcd_size_height, bool is_resize)
{
#ifndef CONFIG_USE_SHM
    maruskin_sdl_init(swt_handle, lcd_size_width, lcd_size_height, is_resize);
#else
    maruskin_shm_init(swt_handle, lcd_size_width, lcd_size_height, is_resize);
#endif
}

DisplaySurface* get_qemu_display_surface(void) {
#ifndef CONFIG_USE_SHM
    return maruskin_sdl_get_display();
#else
    //TODO:
#endif

    return NULL;
}

MaruScreenshot* get_maru_screenshot(void) {
    return maru_screenshot;
}

