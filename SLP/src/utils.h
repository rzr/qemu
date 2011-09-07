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


#ifndef __UTILS_H__
#define __UTILS_H__
#include <gtk/gtk.h>

#include "fileio.h"
#include "logmsg.h"

GHashTable *window_hash_init (void);
void add_window (GtkWidget * win, gint id);
void raise_window (GtkWidget * win);
void remove_window (gint id);
GtkWidget *get_window (gint id);
void window_hash_destroy (void);
void add_widget (gint window_id, gchar * widget_name, GtkWidget * widget);
GtkWidget *get_widget (gint window_id, gchar * widget_name);

int set_config_type(gchar *filepath, const gchar *group, const gchar *field, const int type);
int set_config_value(gchar *filepath, const gchar *group, const gchar *field, const gchar *value);
int get_config_type(gchar *filepath, const gchar *group, const gchar *field);
char *get_config_value(gchar *filepath, const gchar *group, const gchar *field);

#ifndef _WIN32
void strlwr (char *string);
#endif

#endif
