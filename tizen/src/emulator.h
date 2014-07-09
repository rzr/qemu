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

#include "emulator_common.h"
#include "qapi/qmp/qlist.h"
#include "qemu/option.h"
#include "sysemu/sysemu.h"

#define MAXLEN  512
#define MAXPACKETLEN 60
//#define SHMKEY 26099

extern gchar bin_path[];
extern gchar log_path[];

char *get_bin_path(void);
const gchar * prepare_maru(const gchar * const kernel_cmdline);
void start_skin(void);

const gchar * get_log_path(void);

void emulator_add_exit_notifier(Notifier *notify);

#endif /* __EMULATOR_H__ */
