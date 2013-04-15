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


#include "maru_shm.h"
#include "emul_state.h"
#include "debug_ch.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "maru_err_table.h"

MULTI_DEBUG_CHANNEL(tizen, maru_shm);


void *shared_memory = (void*) 0;
int skin_shmid;

void qemu_ds_shm_update(DisplayState *ds, int x, int y, int w, int h)
{
    if (shared_memory != NULL) {
        memcpy(shared_memory, ds->surface->data,
            ds->surface->linesize * ds->surface->height);
    }
}

void qemu_ds_shm_resize(DisplayState *ds)
{
    //TODO:
}

void qemu_ds_shm_refresh(DisplayState *ds)
{
    vga_hw_update();
}

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

    INFO("shared memory key: %d, vga ram_size : %d\n", mykey, shm_size);

    /* make a shared framebuffer */
    skin_shmid = shmget((key_t)mykey, (size_t)shm_size, 0666 | IPC_CREAT);
    if (skin_shmid == -1) {
        ERR("shmget failed\n");
        perror("maru_vga: ");

        maru_register_exit_msg(MARU_EXIT_UNKNOWN,
            (char*) "Cannot launch this VM.\n"
            "Shared memory is not enough.");
        exit(0);
    }

    shared_memory = shmat(skin_shmid, (void*)0, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        perror("maru_vga: ");
        exit(1);
    }

    /* default screen */
    memset(shared_memory, 0x00, (size_t)shm_size);
    printf("Memory attached at %X\n", (int)shared_memory);
}

void maruskin_shm_quit(void)
{
    if (shmdt(shared_memory) == -1) {
        ERR("shmdt failed\n");
        perror("maru_vga: ");
    }

    if (shmctl(skin_shmid, IPC_RMID, 0) == -1) {
        ERR("shmctl failed\n");
        perror("maru_vga: ");
    }
}

