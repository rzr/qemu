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

#ifndef __QEMU__GTK_WIDGET_H__
#define __QEMU__GTK_WIDGET_H__

#include <gtk/gtk.h>
#include <stdlib.h>
#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#ifndef _WIN32
#include <gdk/gdkx.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_syswm.h>
#else
#include <windows.h>
#include <winbase.h>
#include <gdk/gdkwin32.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_rotozoom.h>
#include <SDL_syswm.h>
#include <SDL_getenv.h>
#endif


#include "../../qemu-common.h"
#include "../../console.h"
#include "../../sysemu.h"

#include "extern.h"
#include "ui_imageid.h"
#include "logmsg.h"

#define GTK_TYPE_QEMU (qemu_get_type ())
#define GTK_QEMU(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_QEMU, qemu_state_t))
#define GTK_QEMU_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_QEMU, qemu_class_t))
#define GTK_IS_QEMU(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_QEMU))
#define GTK_IS_QEMU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_QEMU))

typedef struct _qemu_state {
  GtkWidget qemu_widget;

  SDL_Surface *surface_screen, *surface_qemu;
  int width, height, bpp;
  float scale;
  Uint16 flags;
  DisplayState *ds;
} qemu_state_t;


typedef struct _qemu_class {
  GtkWidgetClass parent_class;
} qemu_class_t;


void qemu_widget_size (qemu_state_t *qemu_state, gint width, gint height);
void qemu_widget_resize (qemu_state_t *qemu_state, gint width, gint height);
void qemu_display_init (DisplayState *ds);
gint qemu_widget_new (GtkWidget **widget);
gint qemu_widget_expose (GtkWidget *widget, GdkEventExpose *event);

#endif
