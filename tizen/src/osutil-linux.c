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
  @file     osutil-linux.c
  @brief    Collection of utilities for linux
 */

#include "maru_common.h"
#include "osutil.h"
#include "emulator.h"
#include "debug_ch.h"
#include "maru_err_table.h"
#include "sdb.h"

#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

MULTI_DEBUG_CHANNEL(emulator, osutil);

extern char tizen_target_img_path[];
extern int tizen_base_port;

void check_vm_lock_os(void)
{
    int shm_id;
    void *shm_addr;
    uint32_t port;
    int val;
    struct shmid_ds shm_info;

    for (port = 26100; port < 26200; port += 10) {
        shm_id = shmget((key_t)port, 0, 0);
        if (shm_id != -1) {
            shm_addr = shmat(shm_id, (void *)0, 0);
            if ((void *)-1 == shm_addr) {
                ERR("error occured at shmat()\n");
                break;
            }

            val = shmctl(shm_id, IPC_STAT, &shm_info);
            if (val != -1) {
                INFO("count of process that use shared memory : %d\n",
                    shm_info.shm_nattch);
                if ((shm_info.shm_nattch > 0) &&
                    g_strcmp0(tizen_target_img_path, (char *)shm_addr) == 0) {
                    if (check_port_bind_listen(port + 1) > 0) {
                        shmdt(shm_addr);
                        continue;
                    }
                    shmdt(shm_addr);
                    maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                                        "Can not execute this VM.\n"
                                        "The same name is running now.");
                    exit(0);
                } else {
                    shmdt(shm_addr);
                }
            }
        }
    }
}

void make_vm_lock_os(void)
{
    int shmid;
    char *shared_memory;

    shmid = shmget((key_t)tizen_base_port, MAXLEN, 0666|IPC_CREAT);
    if (shmid == -1) {
        ERR("shmget failed\n");
        return;
    }

    shared_memory = shmat(shmid, (char *)0x00, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        return;
    }
    g_sprintf(shared_memory, "%s", tizen_target_img_path);
    INFO("shared memory key: %d value: %s\n",
        tizen_base_port, (char *)shared_memory);
}

void set_bin_path_os(gchar * exec_argv)
{
    gchar link_path[PATH_MAX] = { 0, };
    char *file_name = NULL;

    ssize_t len = readlink("/proc/self/exe", link_path, sizeof(link_path) - 1);

    if (len < 0 || len > sizeof(link_path)) {
        perror("set_bin_path error : ");
        return;
    }

    link_path[len] = '\0';

    file_name = g_strrstr(link_path, "/");
    g_strlcpy(bin_path, link_path, strlen(link_path) - strlen(file_name) + 1);

    g_strlcat(bin_path, "/", PATH_MAX);
}

void print_system_info_os(void)
{
    INFO("* Linux\n");

    /* depends on building */
    INFO("* QEMU build machine linux kernel version : (%d, %d, %d)\n",
            LINUX_VERSION_CODE >> 16,
            (LINUX_VERSION_CODE >> 8) & 0xff,
            LINUX_VERSION_CODE & 0xff);

     /* depends on launching */
    struct utsname host_uname_buf;
    if (uname(&host_uname_buf) == 0) {
        INFO("* Host machine uname : %s %s %s %s %s\n",
            host_uname_buf.sysname, host_uname_buf.nodename,
            host_uname_buf.release, host_uname_buf.version,
            host_uname_buf.machine);
    }

    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        INFO("* Total Ram : %llu kB, Free: %llu kB\n",
            sys_info.totalram * (unsigned long long)sys_info.mem_unit / 1024,
            sys_info.freeram * (unsigned long long)sys_info.mem_unit / 1024);
    }

    /* get linux distribution information */
    INFO("* Linux distribution infomation :\n");
    char lsb_release_cmd[MAXLEN] = "lsb_release -d -r -c >> ";
    strcat(lsb_release_cmd, log_path);
    if(system(lsb_release_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lsb_release_cmd);
    }

    /* pci device description */
    INFO("* PCI devices :\n");
    char lspci_cmd[MAXLEN] = "lspci >> ";
    strcat(lspci_cmd, log_path);
    if(system(lspci_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lspci_cmd);
    }
}
