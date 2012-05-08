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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#include "maru_err_table.h"
#include <stdio.h>
#include <string.h>


/* This table must match the enum definition */
static char _maru_string_table[][JAVA_MAX_COMMAND_LENGTH] = {
    "", //MARU_EXIT_NORMAL
    "Failed to allocate memory in qemu.", //MARU_EXIT_MEMORY_EXCEPTION
    "Fail to load kernel file. Check if the file is corrupted or missing from the following path.\n\n", //MARU_EXIT_KERNEL_FILE_EXCEPTION
    "Fail to load bios file. Check if the file is corrupted or missing from the following path.\n\n" //MARU_EXIT_BIOS_FILE_EXCEPTION
    /* add here */
};


static int maru_exit_status = MARU_EXIT_NORMAL;
static char maru_exit_msg[JAVA_MAX_COMMAND_LENGTH] = { 0, };

void maru_register_exit_msg(int maru_exit_index, char* additional_msg)
{
    int len = 0;

    maru_exit_status = maru_exit_index;

    if (maru_exit_status != MARU_EXIT_NORMAL) {
        len = strlen(_maru_string_table[maru_exit_status]) + strlen(additional_msg) + 1;
        if (len >= JAVA_MAX_COMMAND_LENGTH) {
            len = JAVA_MAX_COMMAND_LENGTH;
        }

        snprintf(maru_exit_msg, len, "%s%s", _maru_string_table[maru_exit_status], additional_msg);
    } else {
        len = strlen(additional_msg);
        if (len >= JAVA_MAX_COMMAND_LENGTH) {
            additional_msg[JAVA_MAX_COMMAND_LENGTH - 1] = '\0';
            len = JAVA_MAX_COMMAND_LENGTH - 1;
        }

        strcpy(maru_exit_msg, additional_msg);   
    }
}

void maru_atexit(void)
{
    if (maru_exit_status != MARU_EXIT_NORMAL || strlen(maru_exit_msg) != 0) {
        start_simple_client(maru_exit_msg);
    }
}

