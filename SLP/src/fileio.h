/*
 * Emulator
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * JinKyu Kim <fredrick.kim@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YuYeon Oh <yuyeon.oh@samsung.com>
 * WooJin Jung <woojin2.jung@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
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


#ifndef __EMULATOR_FILEIO_H__
#define __EMULATOR_FILEIO_H__

#include <unistd.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <shlobj.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "process.h"
#include "defines.h"

int is_exist_file(gchar *filepath);
const gchar *get_emulator_path(void);
const gchar *get_bin_path(void);
const gchar *get_path(void);
const gchar *get_skin_path(void);
const gchar *get_data_path(void);
const gchar *get_conf_path(void);
gchar *get_emulator_conf_filepath(void);
gchar *get_target_list_filepath(void);
gchar *get_virtual_target_path(gchar *virtual_target_name);
#ifdef _WIN32	
gchar *change_path_to_slash(gchar *org_path);
gchar *change_path_from_slash(gchar *org_path);
#endif
int check_port(const char *ip_address, int port);
//#define MIDBUF	64

#ifndef _WIN32
#define PATH_SEP "/"
#else
#define PATH_SEP "\\"
#endif

#endif //__EMULATOR_FILEIO_H__
