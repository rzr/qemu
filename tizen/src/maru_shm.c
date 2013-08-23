/*
 * Shared memory
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


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "maru_shm.h"
#include "emul_state.h"
#include "hw/maru_brightness.h"
#include "hw/maru_overlay.h"
#include "skin/maruskin_server.h"
#include "debug_ch.h"
#include "maru_err_table.h"

MULTI_DEBUG_CHANNEL(tizen, maru_shm);

static DisplaySurface *dpy_surface;
static void *shared_memory = (void *) 0;
static int skin_shmid;

static int shm_skip_update;
static int shm_skip_count;
static int blank_cnt;
#define MAX_BLANK_FRAME_CNT 100

extern pthread_mutex_t mutex_draw_display;
extern int draw_display_state;

//#define INFO_FRAME_DROP_RATE
#ifdef INFO_FRAME_DROP_RATE
static unsigned int draw_frame;
static unsigned int drop_frame;
#endif

/* Image processing functions using the pixman library */
static void maru_do_pixman_dpy_surface(pixman_image_t *dst_image)
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

static void qemu_ds_shm_update(DisplayChangeListener *dcl,
                               int x, int y, int w, int h)
{
    if (shared_memory != NULL) {
        pthread_mutex_lock(&mutex_draw_display);

        if (draw_display_state == 0) {
            draw_display_state = 1;

            pthread_mutex_unlock(&mutex_draw_display);
            maru_do_pixman_dpy_surface(dpy_surface->image);
            memcpy(shared_memory,
                   surface_data(dpy_surface),
                   surface_stride(dpy_surface) *
                   surface_height(dpy_surface));

#ifdef INFO_FRAME_DROP_RATE
            draw_frame++;
#endif
            notify_draw_frame();
        } else {
#ifdef INFO_FRAME_DROP_RATE
            drop_frame++;
#endif
            pthread_mutex_unlock(&mutex_draw_display);
        }

#ifdef INFO_FRAME_DROP_RATE
        INFO("! frame drop rate = (%d/%d)\n",
             drop_frame, draw_frame + drop_frame);
#endif
    }
}

static void qemu_ds_shm_switch(DisplayChangeListener *dcl,
                        struct DisplaySurface *new_surface)
{
    TRACE("qemu_ds_shm_switch\n");

    if (new_surface) {
        dpy_surface = new_surface;
    }

    shm_skip_update = 0;
    shm_skip_count = 0;
}

static void qemu_ds_shm_refresh(DisplayChangeListener *dcl)
{
    /* If the display is turned off,
    the screen does not update until the it is turned on */
    if (shm_skip_update && brightness_off) {
        if (blank_cnt > MAX_BLANK_FRAME_CNT) {
            /* do nothing */
            return;
        } else if (blank_cnt == MAX_BLANK_FRAME_CNT) {
            /* draw guide image */
            INFO("draw a blank guide image\n");

            notify_draw_blank_guide();
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

    graphic_hw_update(NULL);

    /* Usually, continuously updated.
    But when the display is turned off,
    ten more updates the surface for a black screen. */
    if (brightness_off) {
        if (++shm_skip_count > 10) {
            shm_skip_update = 1;
        } else {
            shm_skip_update = 0;
        }
    } else {
        shm_skip_count = 0;
        shm_skip_update = 0;
    }
}

DisplayChangeListenerOps maru_dcl_ops = {
    .dpy_name          = "maru_shm",
    .dpy_refresh       = qemu_ds_shm_refresh,
    .dpy_gfx_update    = qemu_ds_shm_update,
    .dpy_gfx_switch    = qemu_ds_shm_switch,
};

void maruskin_shm_init(uint64 swt_handle,
    int lcd_size_width, int lcd_size_height, bool is_resize)
{
    INFO("maru shm initialization = %d\n", is_resize);

    if (is_resize == FALSE) { //once
        set_emul_lcd_size(lcd_size_width, lcd_size_height);
        set_emul_sdl_bpp(32);
    }

    /* byte */
    int shm_size =
        get_emul_lcd_width() * get_emul_lcd_height() * 4;

    /* base + 1 = sdb port */
    /* base + 2 = shared memory key */
    int mykey = get_emul_vm_base_port() + 2;

    INFO("shared memory key: %d, size: %d bytes\n", mykey, shm_size);

    /* make a shared framebuffer */
    skin_shmid = shmget((key_t)mykey, (size_t)shm_size, 0666 | IPC_CREAT);
    if (skin_shmid == -1) {
        ERR("shmget failed\n");
        perror("maru_vga: ");

        maru_register_exit_msg(MARU_EXIT_UNKNOWN,
            (char*) "Cannot launch this VM.\n"
            "Failed to get identifier of the shared memory segment.");
        exit(1);
    }

    shared_memory = shmat(skin_shmid, (void*)0, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        perror("maru_vga: ");

        maru_register_exit_msg(MARU_EXIT_UNKNOWN,
            (char*) "Cannot launch this VM.\n"
            "Failed to attach the shared memory segment.");
        exit(1);
    }

    /* default screen */
    memset(shared_memory, 0x00, (size_t)shm_size);
    INFO("Memory attached at 0x%X\n", (int)shared_memory);
}

void maruskin_shm_quit(void)
{
    struct shmid_ds shm_info;

    INFO("maru shm quit\n");

    if (shmctl(skin_shmid, IPC_STAT, &shm_info) == -1) {
        ERR("shmctl failed\n");
        shm_info.shm_nattch = -1;
    }

    if (shmdt(shared_memory) == -1) {
        ERR("shmdt failed\n");
        perror("maru_vga: ");
        return;
    }
    shared_memory = NULL;

    if (shm_info.shm_nattch == 1) {
        /* remove */
        if (shmctl(skin_shmid, IPC_RMID, 0) == -1) {
            INFO("segment was already removed\n");
            perror("maru_vga: ");
        } else {
            INFO("shared memory was removed\n");
        }
    } else if (shm_info.shm_nattch != -1) {
        INFO("number of current attaches = %d\n",
            (int)shm_info.shm_nattch);
    }
}

