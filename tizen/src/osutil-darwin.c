/* 
 * Emulator
 *
 * Copyright (C) 2012, 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

/**
  @file     osutil-darwin.c
  @brief    Collection of utilities for darwin
 */

#include "osutil.h"
#include "emulator.h"
#include "debug_ch.h"
#include "maru_err_table.h"
#include "sdb.h"

#include <string.h>
#include <sys/shm.h>

MULTI_DEBUG_CHANNEL(qemu, osutil);

extern int tizen_base_port;

void check_vm_lock_os(void)
{
    /* TODO: */
}

void make_vm_lock_os(void)
{
    int shmid;
    char *shared_memory;

    shmid = shmget((key_t)SHMKEY, MAXLEN, 0666|IPC_CREAT);
    if (shmid == -1) {
        ERR("shmget failed\n");
        return;
    }
    shared_memory = shmat(shmid, (char *)0x00, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        return;
    }
    sprintf(shared_memory, "%d", tizen_base_port + 2);
    INFO("shared memory key: %d, value: %s\n", SHMKEY, (char *)shared_memory);
    shmdt(shared_memory);
}

void set_bin_path_os(gchar * exec_argv)
{
    gchar *file_name = NULL;

    if (!exec_argv) {
        return;
    }

    char *data = g_strdup(exec_argv);
    if (!data) {
        ERR("Fail to strdup for paring a binary directory.\n");
        return;
    }

    file_name = g_strrstr(data, "/");
    if (!file_name) {
        free(data);
        return;
    }

    g_strlcpy(bin_path, data, strlen(data) - strlen(file_name) + 1);

    g_strlcat(bin_path, "/", PATH_MAX);
    free(data);
}
