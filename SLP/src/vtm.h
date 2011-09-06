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


#ifndef __VTM_H__
#define __VTM_H__

#include <gtk/gtk.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <unistd.h>
#include <sys/stat.h>

#include "defines.h"
#include "ui_imageid.h"
#include "logmsg.h"
#include "fileio.h"
#include "vt_utils.h"
#include "utils.h"
#include "dialog.h"
#include "process.h"

#define MAX_LEN 256

gchar *remove_space(const gchar *str);
gboolean run_cmd(char *cmd);
void fill_virtual_target_info(void);
int create_config_file(gchar *filepath);
int write_config_file(gchar *filepath);
int name_collision_check(void);

void exit_vtm(void);
void create_window_deleted_cb(void);

void resolution_select_cb(GtkWidget *widget, gpointer data);
void sdcard_size_select_cb(void);
void set_sdcard_create_active_cb(void);
void set_sdcard_select_active_cb(void);
void set_sdcard_none_active_cb(void);
void sdcard_file_select_cb(void);
void ram_select_cb(void);
void ok_clicked_cb(void);

void setup_create_frame(void);
void setup_create_button(void);
void setup_resolution_frame(void);
void setup_sdcard_frame(void);
void setup_ram_frame(void);

void show_create_window(void);
void construct_main_window(void);


#endif 
