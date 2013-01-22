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
#include <sys/sysctl.h>

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


void print_system_info_os(void)
{
  INFO("* Mac\n");

    /* uname */
    INFO("* Host machine uname :\n");
    char uname_cmd[MAXLEN] = "uname -a >> ";
    strcat(uname_cmd, log_path);
    if(system(uname_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", uname_cmd);
    }

    /* hw information */
    int mib[2];
    size_t len;
    char *sys_info;
    int sys_num = 0;

    mib[0] = CTL_HW;
    mib[1] = HW_MODEL;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    sys_info = malloc(len * sizeof(char));
    if (sysctl(mib, 2, sys_info, &len, NULL, 0) >= 0) {
        INFO("* Machine model : %s\n", sys_info);
    }
    free(sys_info);

    mib[0] = CTL_HW;
    mib[1] = HW_MACHINE;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    sys_info = malloc(len * sizeof(char));
    if (sysctl(mib, 2, sys_info, &len, NULL, 0) >= 0) {
        INFO("* Machine class : %s\n", sys_info);
    }
    free(sys_info);

    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    len = sizeof(sys_num);
    if (sysctl(mib, 2, &sys_num, &len, NULL, 0) >= 0) {
        INFO("* Number of processors : %d\n", sys_num);
    }

    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM;
    len = sizeof(sys_num);
    if (sysctl(mib, 2, &sys_num, &len, NULL, 0) >= 0) {
        INFO("* Total memory : %llu bytes\n", sys_num);
    }
}
