/*
 * Error message
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
<<<<<<< HEAD
 * Contact: 
=======
 * Contact:
>>>>>>> remotes/private-qm/develop
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
<<<<<<< HEAD
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
=======
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
>>>>>>> remotes/private-qm/develop
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#ifndef __EMUL_ERR_TABLE_H__
#define __EMUL_ERR_TABLE_H__

#include "skin/maruskin_client.h"


/* TODO: define macro for fair of definition */
<<<<<<< HEAD

enum { //This enum must match the table definition
=======
/* This enum must match the table definition */
enum {
>>>>>>> remotes/private-qm/develop
    /* 0 */ MARU_EXIT_UNKNOWN = 0,
    /* 1 */ MARU_EXIT_MEMORY_EXCEPTION,
    /* 2 */ MARU_EXIT_KERNEL_FILE_EXCEPTION,
    /* 3 */ MARU_EXIT_BIOS_FILE_EXCEPTION,
    /* 4 */ MARU_EXIT_SKIN_SERVER_FAILED,
    /* add here.. */
    MARU_EXIT_NORMAL
};


<<<<<<< HEAD
void maru_register_exit_msg(int maru_exit_status, char* additional_msg);
void maru_atexit(void);
char* maru_convert_path(char *msg, const char *path);
void maru_dump_backtrace(void* ptr, int depth);
=======
void maru_register_exit_msg(int maru_exit_status, char *additional_msg);
void maru_atexit(void);
char *maru_convert_path(char *msg, const char *path);
void maru_dump_backtrace(void *ptr, int depth);
>>>>>>> remotes/private-qm/develop

#endif /* __EMUL_ERR_TABLE_H__ */
