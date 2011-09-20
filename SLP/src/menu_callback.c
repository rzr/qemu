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

#include "menu_callback.h"
#include "hw/smbus.h"
//#include "hw/smb380.h"
#include "qemu_gtk_widget.h"
#include "about_version.h"

#ifdef __MINGW32__
 #include <winsock2.h>
#else
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
#endif

extern GtkWidget *pixmap_widget;
extern GtkWidget *fixed;

extern int emul_create_process(const gchar cmd[]);


static const char telemul_name[] = "telephony_emulator";

/**
  * @brief  Event Injector call
  * @param 	wrapper function for creating event injector
  * @return void
  */
void menu_create_eiwidget_callback(GtkWidget *widget, GtkWidget *menu)
{
#ifndef _WIN32
	char* telemul_path;
	const char *path = get_bin_path();

	telemul_path = malloc(strlen(path) + 8 + strlen(telemul_name) + 1);
	sprintf(telemul_path, "%s/../bin/%s", path, telemul_name);

	if(emul_create_process(telemul_path) == TRUE)
		log_msg(MSGL_INFO, "start telephony emulator\n");
	else
		log_msg(MSGL_ERROR, "falied to start telephony emulator\n");

	free(telemul_path);
#endif
}


/**
  * @brief	show preference window
  * @param 	widget: event generation widget
  * @param 	data: user event pointer
  * @return success: 0
  */
int menu_option_callback(GtkWidget *widget, gpointer data)
{
	show_config_window(g_main_window);
//	write_config_file(SYSTEMINFO.conf_file, &configuration);

	return 0;
}


/**
  * @brief 	ajust skin image
  * @param 	widget: callback generation widget
  * @param 	pDev: pointer to structure containg phone model info
  * @param 	pref: pointer to structure containg user preference
  * @param 	nMode: mode
  * @return successs : 0
  */
int mask_main_lcd(GtkWidget *widget, PHONEMODELINFO *pDev, CONFIGURATION *pconfiguration, int nMode)
{
	GdkBitmap *SkinMask = NULL;
	GdkPixmap *SkinPixmap = NULL;
	GtkWidget *sdl_widget = NULL;

	gtk_widget_hide_all(widget);
	gtk_window_resize(GTK_WINDOW(widget), pDev->mode_SkinImg[nMode].nImgWidth, pDev->mode_SkinImg[nMode].nImgHeight);

	/*
	 * to generate the configure, expose_event, when the large image goes
	 * to the small image in expose event, it can be drawn bigger than
	 * that image, so expose event handler have to support it
	 */

	pixmap_widget = gtk_image_new_from_pixbuf (pDev->mode_SkinImg[nMode].pPixImg);
	gdk_pixbuf_render_pixmap_and_mask (pDev->mode_SkinImg[nMode].pPixImg, &SkinPixmap, &SkinMask, 1);
	gdk_pixbuf_get_has_alpha (pDev->mode_SkinImg[nMode].pPixImg);
	gtk_widget_shape_combine_mask (GTK_WIDGET(widget), SkinMask, 0, 0);
	gtk_fixed_put (GTK_FIXED (fixed), pixmap_widget, 0, 0);
	qemu_widget_new(&sdl_widget);
	gtk_fixed_move (GTK_FIXED (fixed), sdl_widget,
		PHONE.mode[UISTATE.current_mode].lcd_list[0].lcd_region.x,
		PHONE.mode[UISTATE.current_mode].lcd_list[0].lcd_region.y);

	if (SkinPixmap != NULL)
		g_object_unref(SkinPixmap);

	if (SkinMask != NULL)
		g_object_unref(SkinMask);

	gtk_window_move(GTK_WINDOW(widget), pconfiguration->main_x, pconfiguration->main_y);
	gtk_window_set_keep_above(GTK_WINDOW (widget), pconfiguration->always_on_top);
	gtk_widget_show_all(widget);

	return 0;
}


/**
  * @brief 	handler to rotate
  * @param 	device: pointe to structure containg device info
  * @param 	nMode: convert Mode
  */
void rotate_event_callback(PHONEMODELINFO *device, int nMode)
{
	/* 1. error checking */

	if (device == NULL || nMode < 0)
		return;

	/* 2. modify current mode */

	UISTATE.current_mode = nMode;

	mask_main_lcd(g_main_window, &PHONE, &configuration, nMode);

	log_msg(MSGL_INFO, "rotate called\n");
}


void menu_rotate_callback(PHONEMODELINFO *device, int nMode)
{
	int send_s;
#ifndef _WIN32
	socklen_t send_len = (socklen_t)sizeof(struct sockaddr_in);
#else
	int send_len = (int)sizeof(struct sockaddr_in);
#endif
	char buf[255];
	struct sockaddr_in servaddr;

#ifdef __MINGW32__
	WSADATA wsadata;
	if(WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
		printf("Error creating socket.");
		return;
	}
#endif

	if((send_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("Create the socket error: ");
		goto cleanup;
	}

	memset(&servaddr, '\0', sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(SENSOR_PORT);

	switch(nMode)
	{
	case 0:
		sprintf(buf, "1\n0\nn");
		break;
	case 1:
		sprintf(buf, "1\n90\n");
		break;
	case 2:
		sprintf(buf, "1\n180\n");
		break;
	case 3:
		sprintf(buf, "1\n270\n");
		break;
	case 7:
		sprintf(buf, "7\n1\n");
		break;
	case 8:
		sprintf(buf, "7\n0\n");
		break;
	}

	if(sendto(send_s, buf, 255, 0, (struct sockaddr *)&servaddr, send_len) <= 0)
	{
		perror("sendto error: ");
	}

cleanup:
#ifdef __MINGW32__
	if(send_s)
		closesocket(send_s);
	WSACleanup();
#else
	if(send_s)
		close(send_s);
#endif
	return;
}


/**
 * @brief	pop-up menu callback for event menu
 * @param 	widget: event generation widget
 * @param 	data: user-defined data
 * @return	void
 */
void menu_event_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *pWidget = NULL;
	char *buf = NULL;
	buf = data;

	/* 1. getting popup_menu */

	GtkWidget *popup_menu = get_widget(EMULATOR_ID, POPUP_MENU);

	pWidget = widget;

	/* 2. LED ON/OFF menu */

	if (g_strcmp0(buf, LED_ON) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			gtk_widget_hide_all(g_main_window);
			gtk_window_resize(GTK_WINDOW(g_main_window), PHONE.mode_SkinImg[UISTATE.current_mode].nImgWidth, PHONE.mode_SkinImg[UISTATE.current_mode].nImgHeight);
			gtk_widget_show_all(g_main_window);
		}
	}

	else if (g_strcmp0(buf, LED_OFF) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			gtk_widget_hide_all(g_main_window);
			gtk_window_resize(GTK_WINDOW(g_main_window), PHONE.mode_SkinImg[UISTATE.current_mode].nImgWidth, PHONE.mode_SkinImg[UISTATE.current_mode].nImgHeight);
			gtk_widget_show_all(g_main_window);
		}
	}

	/* 3. Folder open/ close menu */

	else if (g_strcmp0(buf, FOLDER_OPEN) == 0) {
		GdkBitmap *SkinMask = NULL;
		GdkPixmap *SkinPixmap = NULL;

		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {

			pWidget = g_object_get_data((GObject *) popup_menu, "SubLCD");
			if (pWidget != NULL) {
				gtk_widget_show(pWidget);
			}
			pWidget = g_object_get_data((GObject *) popup_menu, "Simple Mode");
			if (pWidget != NULL) {
				gtk_widget_show(pWidget);
			}

			gdk_pixbuf_render_pixmap_and_mask(PHONE.mode_SkinImg[UISTATE.current_mode].pPixImg, &SkinPixmap, &SkinMask, 1);
			gtk_widget_shape_combine_mask(g_main_window, NULL, 0, 0);
			gtk_widget_shape_combine_mask(g_main_window, SkinMask, 0, 0);

			if (SkinPixmap != NULL) {
				g_object_unref(SkinPixmap);
			}
			if (SkinMask != NULL) {
				g_object_unref(SkinMask);
			}
			gtk_widget_hide_all(g_main_window);
			gtk_window_resize(GTK_WINDOW(g_main_window), PHONE.mode_SkinImg[UISTATE.current_mode].nImgWidth, PHONE.mode_SkinImg[UISTATE.current_mode].nImgHeight);
			gtk_widget_show_all(g_main_window);

		}
	}

	else if (g_strcmp0(buf, FOLDER_CLOSE) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			GdkBitmap *SkinMask = NULL;
			GdkPixmap *SkinPixmap = NULL;

			pWidget = g_object_get_data((GObject *) popup_menu, "Simple Mode");
			if (pWidget != NULL) {
				gtk_widget_hide(pWidget);
			}

			pWidget = g_object_get_data((GObject *) popup_menu, "SubLCD");
			if (pWidget != NULL) {
				gtk_widget_hide(pWidget);
			}

			// window MASK
			gdk_pixbuf_render_pixmap_and_mask(PHONE.cover_mode_SkinImg.pPixImg, &SkinPixmap, &SkinMask, 1);
			gtk_widget_shape_combine_mask(g_main_window, NULL, 0, 0);
			gtk_widget_shape_combine_mask(g_main_window, SkinMask, 0, 0);

			if (SkinPixmap != NULL) {
				g_object_unref(SkinPixmap);
			}
			if (SkinMask != NULL) {
				g_object_unref(SkinMask);
			}

			gtk_widget_hide_all(g_main_window);
			gtk_window_resize(GTK_WINDOW(g_main_window), PHONE.cover_mode_SkinImg.nImgWidth, PHONE.cover_mode_SkinImg.nImgHeight);
			gtk_widget_show_all(g_main_window);
		}
	}

	else if (g_strcmp0(buf, KEYBOARD_ON) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			menu_rotate_callback(&PHONE, 7);
		}
	}

	else if (g_strcmp0(buf, KEYBOARD_OFF) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			menu_rotate_callback(&PHONE, 8);
		}
	}

	/* 4.1 Portrait menu */

	if (g_strcmp0(buf, PORTRAIT) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {

			int i = 0;
			for (i = 0; i < PHONE.mode_cnt; i++) {
				if (g_strcmp0(PHONE.mode[i].name, PORTRAIT) == 0) {
					menu_rotate_callback(&PHONE, i);
					break;
				}
			}

			device_set_rotation(0);
		}
	}

	/* 4.2 Landscape menu */

	else if (g_strcmp0(buf, LANDSCAPE) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {

			int i = 0;
			for (i = 0; i < PHONE.mode_cnt; i++) {
				if (g_strcmp0(PHONE.mode[i].name, LANDSCAPE) == 0) {
					menu_rotate_callback(&PHONE, i);
					break;
				}
			}

			device_set_rotation(90);
		}
	}

	/* 4.3 Reverse Portrait menu */

	else if (g_strcmp0(buf, REVERSE_PORTRAIT) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {

			int i = 0;
			for (i = 0; i < PHONE.mode_cnt; i++) {
				if (g_strcmp0(PHONE.mode[i].name, REVERSE_PORTRAIT) == 0) {
					menu_rotate_callback(&PHONE, i);
					break;
				}
			}

			device_set_rotation(180);
		}
	}

	/* 4.4 Reverse Landscape menu */

	else if (g_strcmp0(buf, REVERSE_LANDSCAPE) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {

			int i = 0;
			for (i = 0; i < PHONE.mode_cnt; i++) {
				if (g_strcmp0(PHONE.mode[i].name, REVERSE_LANDSCAPE) == 0) {
					menu_rotate_callback(&PHONE, i);
					break;
				}
			}

			device_set_rotation(270);
		}
	}

	else {
//		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE)
//			show_message(buf, buf);
	}

	log_msg(MSGL_INFO, "menu_event_callback called\n");
}


/**
  * @brief 	pop-up menu callback for showing gerneal emulation info
  * @param 	widget: event generation widget
  * @param 	data: user defined data
  * @return	void
  */
void menu_device_info_callback(GtkWidget * widget, gpointer data)
{
	char buf[MAXBUF];
	memset(buf, 0x00, sizeof(buf));

	snprintf(buf, sizeof(buf), "LCD SIZE    : %d x %d\nColor Depth : %d(bits per pixel)\nKey Count   : %d", PHONE.mode[UISTATE.current_mode].lcd_list[0].lcd_region.w,
			 PHONE.mode[UISTATE.current_mode].lcd_list[0].lcd_region.h, PHONE.mode[UISTATE.current_mode].lcd_list[0].bitsperpixel,
			 PHONE.mode[UISTATE.current_mode].key_map_list_cnt);

	show_message("Device Information", buf);
}


/**
	@brief 	called when cliced about popup menu, it opens a about_dialog
	@param	widget: emulator_id
	@see	show_about_dialog
*/
void show_about_window(GtkWidget *parent)
{
	const gchar *version = build_version;
	const gchar *copyright =
		"Copyright (C) 2011 Samsung Electronics Co., LTD. All Rights Reserved.\n";
	gchar comments[512] = {0,};
	const gchar *authors[] = {	/* from qemu/MAINTAINERS, alphabetically */
		"Fabrice Bellard",
		"Paul Brook",
		"Magnus Damm",
		"Edgar E. Iglesias",
		"Aurelien Jarno",
		"Vassili Karpov (malc)",
		"Jan Kiszka",
		"Armin Kuster",
		"Hervé Poussineau",
		"Thiemo Seufer",
		"Blue Swirl",
		"Andrzej Zaborowski",
		"Thorsten Zitterell",
		" and many more",
		 NULL};
	const gchar *website = "http://innovator.samsungmobile.com";

	sprintf(comments, "SLP Emulator.\n"
//		"Based upon QEMU 0.10.5 (http://qemu.org)\n"
		"Build date: %s\nGit version: %s\n",
		build_date, build_git);

	GtkWidget* about_dialog = gtk_about_dialog_new();

//	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), version);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), comments);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), copyright);
//	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), website);
	gtk_show_about_dialog(GTK_WINDOW(parent),
							"program-name", "SLP Emulator",
//							"version", version,
							"comments", comments,
							"copyright", copyright,
//							"website", website,
//							"authors", authors,
							"license", license_text,
							NULL);

}


/**
  * @brief	show emulator about info
  * @param 	widget: event generation widget
  * @param 	data: user defined data
  * @return	void
  */
void menu_about_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *win = get_window(EMULATOR_ID);
	show_about_window(win);
}

