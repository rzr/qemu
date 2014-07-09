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
#include "emulator_common.h"
#include "maru_err_table.h"
#include "debug_ch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#ifdef CONFIG_WIN32
#include <windows.h>
#else
#include <execinfo.h>
#endif


MULTI_DEBUG_CHANNEL(qemu, backtrace);
/* This table must match the enum definition */
static char _maru_string_table[][JAVA_MAX_COMMAND_LENGTH] = {
    /* 0 */ "",
    /* 1 */ "Failed to allocate memory in qemu.",
    /* 2 */ "Failed to load a kernel file the following path."
            " Check if the file is corrupted or missing.\n\n",
    /* 3 */ "Failed to load a bios file in bios path that mentioned on document."
            " Check if the file is corrupted or missing.\n\n",
    /* 4 */ "Skin process cannot be initialized. Skin server is not ready.",
    /* 5 */ "Emulator has stopped working.\n"
            "A problem caused the program to stop working correctly.",
    /* add here.. */
    ""
};


static int maru_exit_status = MARU_EXIT_NORMAL;
static char *maru_exit_msg;

#ifdef CONFIG_WIN32
static LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionFilter;
#endif

#ifdef CONFIG_LINUX
static pthread_spinlock_t siglock;
void maru_sighandler(int sig);
#endif

char *get_canonical_path(char const *const path)
{
    if ((int)g_path_is_absolute(path)) {
        return (char *)path;
    }

    char *canonical_path;

#ifndef _WIN32
    char *current_dir = g_get_current_dir();
    canonical_path = g_strdup_printf("%s/%s", current_dir, path);
    g_free(current_dir);
#else
    canonical_path = g_malloc(MAX_PATH);
    GetFullPathName(path, MAX_PATH, canonical_path, NULL);
#endif

    return canonical_path;
}

void maru_register_exit_msg(int maru_exit_index, char const *format, ...)
{
    va_list args;

    if (maru_exit_index >= MARU_EXIT_NORMAL) {
        fprintf(stderr, "Invalid error message index = %d\n",
            maru_exit_index);
        return;
    }
    if (maru_exit_status != MARU_EXIT_NORMAL) {
        fprintf(stderr, "The error message is already registered = %d\n",
            maru_exit_status);
        return;
    }

    maru_exit_status = maru_exit_index;

    if (maru_exit_status == MARU_EXIT_HB_TIME_EXPIRED) {
        fprintf(stderr, "Skin client could not connect to Skin server."
                " The time of internal heartbeat has expired.\n");
    }

    if (!format) {
        format = "";
    }

    va_start(args, format);
    char *additional = g_strdup_vprintf(format, args);
    va_end(args);
    maru_exit_msg = g_strdup_printf("%s%s", _maru_string_table[maru_exit_status],
                        additional);
    g_free(additional);

    if (strlen(maru_exit_msg) >= JAVA_MAX_COMMAND_LENGTH) {
        maru_exit_msg[JAVA_MAX_COMMAND_LENGTH - 1] = '\0';
    }

    fprintf(stdout, "The error message is registered = %d : %s\n",
        maru_exit_status, maru_exit_msg);
}

void maru_atexit(void)
{

    if (maru_exit_status != MARU_EXIT_NORMAL || maru_exit_msg) {
        start_simple_client(maru_exit_msg);
    }
    g_free(maru_exit_msg);

    INFO("normal exit called\n");
    // dump backtrace log no matter what
    maru_dump_backtrace(NULL, 0);
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

    if (pTopFrame == NULL) {
        INFO("ebp is null, skip this for now\n");
        return ;
    }

    nCount = 0;
    currentFrame.pNext = ((struct frame_layout *)pTopFrame)->pNext;
    currentFrame.pReturnAddr = ((struct frame_layout *)pTopFrame)->pReturnAddr;
    pCurrentFrame = (struct frame_layout *)pTopFrame;

    ERR("\nBacktrace Dump Start :\n");
    if (pContext) {
        memset(module_buf, 0, sizeof(module_buf));
        hModule = aqua_get_module_handle((DWORD)((PCONTEXT)pContext)->Eip);
        if (hModule) {
            if (!GetModuleFileNameA(hModule, module_buf, sizeof(module_buf))) {
                memset(module_buf, 0, sizeof(module_buf));
            }
        }
        ERR("[%02d]Addr = 0x%p : %s\n", nCount, ((PCONTEXT)pContext)->Eip, aqua_get_filename_from_path(module_buf));
        nCount++;
    }

    while (1) {
        if (((void *)pCurrentFrame < pTopFrame)
            || ((void *)pCurrentFrame >= (void *)0xC0000000)) {
            break;
        }

        memset(module_buf, 0, sizeof(module_buf));
        hModule = aqua_get_module_handle((DWORD)currentFrame.pReturnAddr);
        if (hModule) {
            if (!GetModuleFileNameA(hModule, module_buf, sizeof(module_buf))) {
                memset(module_buf, 0, sizeof(module_buf));
            }
        }
        ERR("[%02d]Addr = 0x%p : %s\n", nCount, currentFrame.pReturnAddr, aqua_get_filename_from_path(module_buf));

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
    void *trace[1024];
    int ndepth = backtrace(trace, 1024);
    ERR("Backtrace depth is %d.\n", ndepth);

    backtrace_symbols_fd(trace, ndepth, fileno(stderr));
#endif
}

#ifdef CONFIG_WIN32
static LONG maru_unhandled_exception_filter(PEXCEPTION_POINTERS pExceptionInfo){
    char module_buf[1024];
    DWORD dwException = pExceptionInfo->ExceptionRecord->ExceptionCode;
    ERR("%d\n ", (int)dwException);

    PEXCEPTION_RECORD pExceptionRecord;
    HMODULE hModule;
    PCONTEXT pContext;

    pExceptionRecord = pExceptionInfo->ExceptionRecord;

    memset(module_buf, 0, sizeof(module_buf));
    hModule = aqua_get_module_handle((DWORD)pExceptionRecord->ExceptionAddress);
    if(hModule){
        if(!GetModuleFileNameA(hModule, module_buf, sizeof(module_buf))){
            memset(module_buf, 0, sizeof(module_buf));
        }
    }

    ERR("Exception [%X] occured at %s:0x%08x\n",
        pExceptionRecord->ExceptionCode,
        aqua_get_filename_from_path(module_buf),
        pExceptionRecord->ExceptionAddress
       );

    pContext = pExceptionInfo->ContextRecord;
    maru_dump_backtrace(pContext, 0);
    _exit(0);
    //return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#ifdef CONFIG_LINUX
void maru_sighandler(int sig)
{
    pthread_spin_lock(&siglock);
    ERR("Got signal %d\n", sig);
    maru_dump_backtrace(NULL, 0);
    pthread_spin_unlock(&siglock);
    _exit(0);
}
#endif


#ifndef CONFIG_DARWIN
void maru_register_exception_handler(void)
{
#ifdef CONFIG_WIN32
    prevExceptionFilter = SetUnhandledExceptionFilter(maru_unhandled_exception_filter);
#else // LINUX
    void *trace[1];
    struct sigaction sa;

    // make dummy call to explicitly load glibc library
    backtrace(trace, 1);

    pthread_spin_init(&siglock,0);
    sa.sa_handler = (void*) maru_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    // main thread only
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
#endif
}
#endif
