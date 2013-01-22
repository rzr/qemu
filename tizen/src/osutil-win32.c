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
  @file     osutil-win32.c
  @brief    Collection of utilities for win32
 */

#include "maru_common.h"
#include "osutil.h"
#include "emulator.h"
#include "debug_ch.h"
#include "maru_err_table.h"
#include "sdb.h"

#include <windows.h>

MULTI_DEBUG_CHANNEL (emulator, osutil);

extern char tizen_target_img_path[];
extern int tizen_base_port;

void check_vm_lock_os(void)
{
    uint32_t port;
    char *base_port = NULL;
    char *pBuf;
    HANDLE hMapFile;
    for (port = 26100; port < 26200; port += 10) {
        base_port = g_strdup_printf("%d", port);
        hMapFile = OpenFileMapping(FILE_MAP_READ, TRUE, base_port);
        if (hMapFile == NULL) {
            INFO("port %s is not used.\n", base_port);
            continue;
        } else {
             pBuf = (char *)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 50);
            if (pBuf == NULL) {
                ERR("Could not map view of file (%d).\n", GetLastError());
                CloseHandle(hMapFile);
            }

            if (strcmp(pBuf, tizen_target_img_path) == 0) {
                maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                    "Can not execute this VM.\n"
                    "The same name is running now.");
                UnmapViewOfFile(pBuf);
                CloseHandle(hMapFile);
                free(base_port);
                exit(0);
            } else {
                UnmapViewOfFile(pBuf);
            }
        }

        CloseHandle(hMapFile);
        free(base_port);
    }
}

void make_vm_lock_os(void)
{
    HANDLE hMapFile;
    char *pBuf;
    char *port_in_use;
    char *shared_memory;

    shared_memory = g_strdup_printf("%s", tizen_target_img_path);
    port_in_use =  g_strdup_printf("%d", tizen_base_port);
    hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE, /* use paging file */
                 NULL,                 /* default security */
                 PAGE_READWRITE,       /* read/write access */
                 0,                /* maximum object size (high-order DWORD) */
                 50,               /* maximum object size (low-order DWORD) */
                 port_in_use);         /* name of mapping object */
    if (hMapFile == NULL) {
        ERR("Could not create file mapping object (%d).\n", GetLastError());
        return;
    }
    pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 50);

    if (pBuf == NULL) {
        ERR("Could not map view of file (%d).\n", GetLastError());
        CloseHandle(hMapFile);
        return;
    }

    CopyMemory((PVOID)pBuf, shared_memory, strlen(shared_memory));
    free(port_in_use);
    free(shared_memory);
}

void set_bin_path_os(gchar * exec_argv)
{
    gchar link_path[PATH_MAX] = { 0, };
    gchar *file_name = NULL;

    if (!GetModuleFileName(NULL, link_path, PATH_MAX)) {
        return;
    }

    file_name = g_strrstr(link_path, "\\");
    g_strlcpy(bin_path, link_path, strlen(link_path) - strlen(file_name) + 1);

    g_strlcat(bin_path, "\\", PATH_MAX);
}

void print_system_info_os(void)
{
    INFO("* Windows\n");

    /* Retrieves information about the current os */
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osvi)) {
        INFO("* MajorVersion : %d, MinorVersion : %d, BuildNumber : %d, "
            "PlatformId : %d, CSDVersion : %s\n", osvi.dwMajorVersion,
            osvi.dwMinorVersion, osvi.dwBuildNumber,
            osvi.dwPlatformId, osvi.szCSDVersion);
    }

    /* Retrieves information about the current system */
    SYSTEM_INFO sysi;
    ZeroMemory(&sysi, sizeof(SYSTEM_INFO));

    GetSystemInfo(&sysi);
    INFO("* Processor type : %d, Number of processors : %d\n",
            sysi.dwProcessorType, sysi.dwNumberOfProcessors);

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    INFO("* Total Ram : %llu kB, Free: %lld kB\n",
            memInfo.ullTotalPhys / 1024, memInfo.ullAvailPhys / 1024);
}
