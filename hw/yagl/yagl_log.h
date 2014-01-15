/*
 * yagl
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
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

#ifndef _QEMU_YAGL_LOG_H
#define _QEMU_YAGL_LOG_H

#ifdef _WIN32
#include <windows.h>
#endif

#include "yagl_types.h"

//#define YAGL_LOG_DISABLE

typedef enum
{
    yagl_log_level_off = 0,
    yagl_log_level_error = 1,
    yagl_log_level_warn = 2,
    yagl_log_level_info = 3,
    yagl_log_level_debug = 4,
    yagl_log_level_trace = 5
} yagl_log_level;

#define yagl_log_level_max yagl_log_level_trace

void yagl_log_init(void);

void yagl_log_cleanup(void);

void yagl_log_event(yagl_log_level log_level,
                    yagl_pid process_id,
                    yagl_tid thread_id,
                    const char* facility,
                    int line,
                    const char* format, ...);

void yagl_log_func_enter(yagl_pid process_id,
                         yagl_tid thread_id,
                         const char* func,
                         int line,
                         const char* format, ...);

void yagl_log_func_exit(yagl_pid process_id,
                        yagl_tid thread_id,
                        const char* func,
                        int line,
                        const char* format, ...);

/*
 * Convenience function that uses datatypes instead of format strings, one for each
 * function argument.
 *
 * 'num_args' is number of arguments to the function, then come 'num_args'
 * (datatype, arg) strings, then come 'num_args' arguments. i.e. the call is
 * like this:
 * yagl_log_func_enter_split(0, 0, "my_func", 123, 2, "EGLint", "arg_1", "EGLDisplay", "arg_2", 12, my_ptr);
 *
 * This function uses constant size storage for final format string creation,
 * be sure not to pass too many arguments, the overall size must not
 * exceed 1024 characters.
 *
 * If one of the argument datatypes is not a supported datatype, then argument
 * formatting is skipped and "..." is printed instead.
 */
void yagl_log_func_enter_split(yagl_pid process_id,
                               yagl_tid thread_id,
                               const char* func,
                               int line,
                               int num_args, ...);

/*
 * Same as above, but only one datatype is required and only one argument
 * is needed. i.e.:
 * yagl_log_func_exit_split(0, 0, "my_func", 234, "EGLint", 123);
 */
void yagl_log_func_exit_split(yagl_pid process_id,
                              yagl_tid thread_id,
                              const char* func,
                              int line,
                              const char* datatype, ...);

bool yagl_log_is_enabled_for_level(yagl_log_level log_level);

bool yagl_log_is_enabled_for_facility(const char* facility);

bool yagl_log_is_enabled_for_func_tracing(void);

#ifndef YAGL_LOG_DISABLE
#define YAGL_LOG_EVENT(log_level, pid, tid, facility, format, ...) \
    do \
    { \
        if ( yagl_log_is_enabled_for_level(yagl_log_level_##log_level) && \
             yagl_log_is_enabled_for_facility(facility) ) \
        { \
            yagl_log_event(yagl_log_level_##log_level, pid, tid, facility, __LINE__, format,##__VA_ARGS__); \
        } \
    } while(0)

#define YAGL_LOG_FUNC_SET(func) \
    const char* _yagl_log_current_func = #func; \
    yagl_pid _yagl_log_current_pid = (cur_ts ? cur_ts->ps->id : 0); \
    yagl_tid _yagl_log_current_tid = (cur_ts ? cur_ts->id : 0)

#define YAGL_LOG_FUNC_ENTER(func, format, ...) \
    YAGL_LOG_FUNC_SET(func); \
    do \
    { \
        if ( yagl_log_is_enabled_for_func_tracing() && \
             yagl_log_is_enabled_for_facility(_yagl_log_current_func) ) \
        { \
            yagl_log_func_enter(_yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, __LINE__, format,##__VA_ARGS__); \
        } \
    } while(0)

#define YAGL_LOG_FUNC_ENTER_SPLIT(func, num_args, ...) \
    YAGL_LOG_FUNC_SET(func); \
    do \
    { \
        if ( yagl_log_is_enabled_for_func_tracing() && \
             yagl_log_is_enabled_for_facility(_yagl_log_current_func) ) \
        { \
            yagl_log_func_enter_split(_yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, __LINE__, num_args,##__VA_ARGS__); \
        } \
    } while(0)

#define YAGL_LOG_FUNC_EXIT(format, ...) \
    do \
    { \
        if ( yagl_log_is_enabled_for_func_tracing() && \
             yagl_log_is_enabled_for_facility(_yagl_log_current_func) ) \
        { \
            yagl_log_func_exit(_yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, __LINE__, format,##__VA_ARGS__); \
        } \
    } while(0)

#define YAGL_LOG_FUNC_EXIT_SPLIT(ret_type, ret) \
    do \
    { \
        if ( yagl_log_is_enabled_for_func_tracing() && \
             yagl_log_is_enabled_for_facility(_yagl_log_current_func) ) \
        { \
            yagl_log_func_exit_split(_yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, __LINE__, #ret_type, ret); \
        } \
    } while(0)

#ifdef _WIN32
#define YAGL_LOG_EVENT_WIN(log_level, pid, tid, facility, format) \
    do \
    { \
    	DWORD err = GetLastError(); \
        if ( yagl_log_is_enabled_for_level(yagl_log_level_##log_level) && \
             yagl_log_is_enabled_for_facility(facility) ) \
        { \
            LPVOID err_msg; \
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
                          NULL, err, 0, (LPTSTR)&err_msg, 0 , NULL); \
            yagl_log_event(yagl_log_level_##log_level, pid, tid, facility, __LINE__, format"%s", err_msg); \
            LocalFree(err_msg); \
        } \
    } while(0)
#endif
#else
#define YAGL_LOG_EVENT(log_level, pid, tid, facility, format, ...)
#define YAGL_LOG_FUNC_SET(func)
#define YAGL_LOG_FUNC_ENTER(func, format, ...)
#define YAGL_LOG_FUNC_ENTER_SPLIT(func, num_args, ...)
#define YAGL_LOG_FUNC_EXIT(format, ...)
#define YAGL_LOG_FUNC_EXIT_SPLIT(ret_type, ret)
#define YAGL_LOG_EVENT_WIN(log_level, pid, tid, facility, format)
#endif

#define YAGL_LOG_FUNC_ENTER_SPLIT0(func) YAGL_LOG_FUNC_ENTER_SPLIT(func, 0)

#define YAGL_LOG_FUNC_ENTER_SPLIT1(func, arg0_type, arg0) \
    YAGL_LOG_FUNC_ENTER_SPLIT(func, 1, #arg0_type, #arg0, arg0)

#define YAGL_LOG_FUNC_ENTER_SPLIT2(func, arg0_type, arg1_type, arg0, arg1) \
    YAGL_LOG_FUNC_ENTER_SPLIT(func, 2, #arg0_type, #arg0, #arg1_type, #arg1, arg0, arg1)

#define YAGL_LOG_FUNC_ENTER_SPLIT3( func, \
                                    arg0_type, arg1_type, arg2_type, \
                                    arg0, arg1, arg2 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 3, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, \
                               arg0, arg1, arg2 )

#define YAGL_LOG_FUNC_ENTER_SPLIT4( func, \
                                    arg0_type, arg1_type, arg2_type, arg3_type, \
                                    arg0, arg1, arg2, arg3 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 4, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, \
                               arg0, arg1, arg2, arg3 )

#define YAGL_LOG_FUNC_ENTER_SPLIT5( func, \
                                    arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, \
                                    arg0, arg1, arg2, arg3, arg4 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 5, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, #arg4_type, #arg4, \
                               arg0, arg1, arg2, arg3, arg4 )

#define YAGL_LOG_FUNC_ENTER_SPLIT6( func, \
                                    arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, \
                                    arg0, arg1, arg2, arg3, arg4, arg5 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 6, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, #arg4_type, #arg4, #arg5_type, #arg5, \
                               arg0, arg1, arg2, arg3, arg4, arg5 )

#define YAGL_LOG_FUNC_ENTER_SPLIT7( func, \
                                    arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, \
                                    arg0, arg1, arg2, arg3, arg4, arg5, arg6 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 7, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, #arg4_type, #arg4, #arg5_type, #arg5, #arg6_type, #arg6, \
                               arg0, arg1, arg2, arg3, arg4, arg5, arg6 )

#define YAGL_LOG_FUNC_ENTER_SPLIT8( func, \
                                    arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, \
                                    arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 8, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, #arg4_type, #arg4, #arg5_type, #arg5, #arg6_type, #arg6, #arg7_type, #arg7, \
                               arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7 )

#define YAGL_LOG_FUNC_ENTER_SPLIT9( func, \
                                    arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, \
                                    arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 9, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, #arg4_type, #arg4, #arg5_type, #arg5, #arg6_type, #arg6, #arg7_type, #arg7, #arg8_type, #arg8, \
                               arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 )

#define YAGL_LOG_FUNC_ENTER_SPLIT10( func, \
                                     arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, arg9_type, \
                                     arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
    YAGL_LOG_FUNC_ENTER_SPLIT( func, 10, \
                               #arg0_type, #arg0, #arg1_type, #arg1, #arg2_type, #arg2, #arg3_type, #arg3, #arg4_type, #arg4, #arg5_type, #arg5, #arg6_type, #arg6, #arg7_type, #arg7, #arg8_type, #arg8, #arg9_type, #arg9, \
                               arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 )

#define YAGL_LOG_TRACE(format, ...)    YAGL_LOG_EVENT(trace, _yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, format,##__VA_ARGS__)
#define YAGL_LOG_DEBUG(format, ...)    YAGL_LOG_EVENT(debug, _yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, format,##__VA_ARGS__)
#define YAGL_LOG_INFO(format, ...)     YAGL_LOG_EVENT(info, _yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, format,##__VA_ARGS__)
#define YAGL_LOG_WARN(format, ...)     YAGL_LOG_EVENT(warn, _yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, format,##__VA_ARGS__)
#define YAGL_LOG_ERROR(format, ...)    YAGL_LOG_EVENT(error, _yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, format,##__VA_ARGS__)
#define YAGL_LOG_ERROR_WIN()           YAGL_LOG_EVENT_WIN(error, _yagl_log_current_pid, _yagl_log_current_tid, _yagl_log_current_func, "OS ERROR: ")
#define YAGL_LOG_CRITICAL(format, ...) \
    yagl_log_event(yagl_log_level_error, 0, 0, __FUNCTION__, __LINE__, format,##__VA_ARGS__)

#endif
