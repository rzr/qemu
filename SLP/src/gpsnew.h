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

#ifndef __GPSNEW_H__
#define __GPSNEW_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkexpander.h>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#ifdef __MINGW32__
 #include <winsock2.h>
#else
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <unistd.h>
 #include <netdb.h>
#endif

#include "fileio.h"

#define MAX_LEN 256
#define SLP_GPS_DEVICE      "/tmp/gpsdevice"
#define PROGRESS_LOG_TEXT	"Progress Log"

/* Function prototypes */
GtkWidget * make_progress_log_expander(void);

void on_veladj_value_changed(GtkAdjustment  *adjust, gpointer  data);
void on_altadj_value_changed(GtkAdjustment  *adjust, gpointer  data);
void on_longadj_value_changed(GtkAdjustment  *adjust, gpointer  data);
void on_latadj_value_changed(GtkAdjustment  *adjust, gpointer  data);
void on_startstop_toggled(GtkWidget *widget, gpointer data);
void on_manual_toggled(GtkWidget *widget, gpointer data);
void on_browser_clicked(void);

gboolean send_gps_data(gpointer data);
gboolean handle_connection(int mode);
void output_function(gchar *ptr);
void exit_function(void);

void menu_create_gps(GtkWidget* parent);

enum connection_mode
{
	OPEN_CONNECTION,
	CLOSE_CONNECTION,
};

enum connection_type
{
	TCP,
	UDP,
	TTY,
	PIPE,
	NONE
};
#endif
