/*
 * Error message
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#include "qemu-common.h"
#include "maru_err_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* This table must match the enum definition */
static char _maru_string_table[][JAVA_MAX_COMMAND_LENGTH] = {
    /* 0 */ "",
    /* 1 */ "Failed to allocate memory in qemu.",
    /* 2 */ "Failed to load a kernel file the following path.\
Check if the file is corrupted or missing.\n\n",
    /* 3 */ "Failed to load a bios file the following path.\
Check if the file is corrupted or missing.\n\n",
    /* 4 */ "Skin process cannot be initialized. Skin server is not ready.",
    /* add here.. */
    ""
};


static int maru_exit_status = MARU_EXIT_NORMAL;
static char maru_exit_msg[JAVA_MAX_COMMAND_LENGTH] = { 0, };

void maru_register_exit_msg(int maru_exit_index, char *additional_msg)
{
    int len = 0;

    if (maru_exit_index >= MARU_EXIT_NORMAL) {
        return;
    }

    maru_exit_status = maru_exit_index;

    if (maru_exit_status != MARU_EXIT_UNKNOWN) {
        if (additional_msg != NULL) {
            len = strlen(_maru_string_table[maru_exit_status])
                    + strlen(additional_msg) + 1;
            if (len > JAVA_MAX_COMMAND_LENGTH) {
                len = JAVA_MAX_COMMAND_LENGTH;
            }

            snprintf(maru_exit_msg, len, "%s%s",
                    _maru_string_table[maru_exit_status], additional_msg);
        } else {
            len = strlen(_maru_string_table[maru_exit_status]) + 1;
            if (len > JAVA_MAX_COMMAND_LENGTH) {
                len = JAVA_MAX_COMMAND_LENGTH;
            }

            snprintf(maru_exit_msg, len,
                    "%s", _maru_string_table[maru_exit_status]);
        }
    } else if (additional_msg != NULL) {
        len = strlen(additional_msg);
        if (len >= JAVA_MAX_COMMAND_LENGTH) {
            additional_msg[JAVA_MAX_COMMAND_LENGTH - 1] = '\0';
            len = JAVA_MAX_COMMAND_LENGTH - 1;
        }

        pstrcpy(maru_exit_msg, sizeof(maru_exit_msg), additional_msg);
    }
}

void maru_atexit(void)
{
    if (maru_exit_status != MARU_EXIT_NORMAL || strlen(maru_exit_msg) != 0) {
        maru_dump_backtrace(NULL, 0);
        start_simple_client(maru_exit_msg);
    }
}

char *maru_convert_path(char *msg, const char *path)
{
    char *current_path = NULL;
    char *err_msg = NULL;
#ifdef _WIN32
    char *dos_err_msg = NULL;
#endif
    int total_len = 0;
    int msg_len = 0;
    int cur_path_len = 0;
    int path_len = 0;
    int res = -1;

    res = (int)g_path_is_absolute(path);
    path_len = (strlen(path) + 1);
    if (msg) {
        msg_len = strlen(msg) + 1;
    }

    if (!res) {
        current_path = (char *)g_get_current_dir();
        cur_path_len = strlen(current_path) + strlen("/") + 1;
        total_len += cur_path_len;
    }
    total_len += (path_len + msg_len);

    err_msg = g_malloc0(total_len * sizeof(char));

    if (msg) {
        snprintf(err_msg, msg_len, "%s", msg);
        total_len = msg_len - 1;
    } else {
        total_len = 0;
    }

    if (!res) {
        snprintf(err_msg + total_len, cur_path_len, "%s%s", current_path, "/");
        total_len += (cur_path_len - 1);
    }
    snprintf(err_msg + total_len, path_len, "%s", path);

#ifdef _WIN32
    {
        int i;

        dos_err_msg = g_strdup(err_msg);
        if (!dos_err_msg) {
            printf(stderr,
                "failed to duplicate an error message from %p\n", err_msg);
            if (current_path) {
                g_free(current_path);
            }
            g_free(err_msg);
            return NULL;
        }

        for (i = (total_len - 1); dos_err_msg[i]; i++) {
            if (dos_err_msg[i] == '/') {
                dos_err_msg[i] = '\\';
            }
        }
        pstrcpy(err_msg, sizeof(err_msg), dos_err_msg);
        g_free(dos_err_msg);
    }
#endif
    if (current_path) {
        g_free(current_path);
    }

    return err_msg;
}

/* Print 'backtrace' */
#ifdef _WIN32
struct frame_layout {
    void *pNext;
    void *pReturnAddr;
};

static char *aqua_get_filename_from_path(char *path_buf)
{
    char *ret_slash;
    char *ret_rslash;

    ret_slash = strrchr(path_buf, '/');
    ret_rslash = strrchr(path_buf, '\\');

    if (ret_slash || ret_rslash) {
        if (ret_slash > ret_rslash) {
            return ret_slash + 1;
        } else{
            return ret_rslash + 1;
        }
    }

    return path_buf;
}


static HMODULE aqua_get_module_handle(DWORD dwAddress)
{
    MEMORY_BASIC_INFORMATION Buffer;
    return VirtualQuery((LPCVOID) dwAddress, &Buffer, sizeof(Buffer))
            ? (HMODULE) Buffer.AllocationBase : (HMODULE) 0;
}
#endif

void maru_dump_backtrace(void *ptr, int depth)
{
#ifdef _WIN32
    int nCount;
    void *pTopFrame;
    struct frame_layout currentFrame;
    struct frame_layout *pCurrentFrame;

    char module_buf[1024];
    HMODULE hModule;

    PCONTEXT pContext = ptr;
    if (!pContext) {
        __asm__ __volatile__ ("movl   %%ebp, %0" : "=m" (pTopFrame));
    } else {
        pTopFrame = (void *)((PCONTEXT)pContext)->Ebp;
    }

    nCount = 0;
    currentFrame.pNext = ((struct frame_layout *)pTopFrame)->pNext;
    currentFrame.pReturnAddr = ((struct frame_layout *)pTopFrame)->pReturnAddr;
    pCurrentFrame = (struct frame_layout *)pTopFrame;

    fprintf(stderr, "\nBacktrace Dump Start :\n");
    if (pContext) {
        fprintf(stderr, "[%02d]Addr = 0x%p", nCount, ((PCONTEXT)pContext)->Eip);
        memset(module_buf, 0, sizeof(module_buf));
        hModule = aqua_get_module_handle((DWORD)((PCONTEXT)pContext)->Eip);
        if (hModule) {
            if (!GetModuleFileNameA(hModule, module_buf, sizeof(module_buf))) {
                memset(module_buf, 0, sizeof(module_buf));
            }
        }
        fprintf(stderr, " : %s\n", aqua_get_filename_from_path(module_buf));
        nCount++;
    }

    while (1) {
        if (((void *)pCurrentFrame < pTopFrame)
            || ((void *)pCurrentFrame >= (void *)0xC0000000)) {
            break;
        }

        fprintf(stderr, "[%02d]Addr = 0x%p", nCount, currentFrame.pReturnAddr);
        memset(module_buf, 0, sizeof(module_buf));
        hModule = aqua_get_module_handle((DWORD)currentFrame.pReturnAddr);
        if (hModule) {
            if (!GetModuleFileNameA(hModule, module_buf, sizeof(module_buf))) {
                memset(module_buf, 0, sizeof(module_buf));
            }
        }
        fprintf(stderr, " : %s\n", aqua_get_filename_from_path(module_buf));

    if (!ReadProcessMemory(GetCurrentProcess(), currentFrame.pNext,
            (void *)&currentFrame, sizeof(struct frame_layout), NULL)) {
            break;
        }
        pCurrentFrame = (struct frame_layout *)pCurrentFrame->pNext;

        if (depth) {
            if (!--depth) {
                break;
            }
        }
        nCount++;
    }
#else
    int i, ndepth;
    void *trace[1024];
    char **symbols;

    fprintf(stderr, "\n Backtrace Dump Start :\n");

    ndepth = backtrace(trace, 1024);
    fprintf(stderr, "Backtrace depth is %d.\n", ndepth);

    symbols = backtrace_symbols(trace, ndepth);
    if (symbols == NULL) {
        fprintf(stderr, "'backtrace_symbols()' return error");
        return;
    }

    for (i = 0; i < ndepth; i++) {
        fprintf(stderr, "%s\n", symbols[i]);
    }
    free(symbols);
#endif
}
