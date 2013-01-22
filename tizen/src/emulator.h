/*
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
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
 * @file emulator.h
 * @brief - header file for emulator.c
 */

#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "maru_common.h"

#define MAXLEN  512
#define MAXPACKETLEN 60
#define SHMKEY	26099

void exit_emulator(void);
char *get_bin_path(void);
void set_image_and_log_path(char *qemu_argv);
void redir_output(void);
void extract_qemu_info(int qemu_argc, char **qemu_argv);
void extract_skin_info(int skin_argc, char **skin_argv);
void prepare_maru(void);
void check_shdmem(void);
void make_shdmem(void);

const gchar * prepare_maru_devices(const gchar * kernel_cmdline);
// FIXME: Necessary declaration of functions but produces error difficult to solve now
//int maru_device_check(QemuOpts *opts);
#endif /* __EMULATOR_H__ */
