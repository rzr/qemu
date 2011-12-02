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

#include "debug_ch.h"
#include "sdb.h"

//DEFAULT_DEBUG_CHANNEL(tizen);
MULTI_DEBUG_CHANNEL(tizen, menu_callback);

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
		INFO( "start telephony emulator\n");
	else
		ERR( "falied to start telephony emulator\n");

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
	char *rotate[4] = {PORTRAIT, LANDSCAPE, REVERSE_PORTRAIT, REVERSE_LANDSCAPE};
	GtkWidget *pwidget = NULL;
	GtkWidget *popup_menu = get_widget(EMULATOR_ID, POPUP_MENU);

	/* 1. error checking */

	if (device == NULL || nMode < 0)
		return;

	/* 2. modify current mode */

	UISTATE.current_mode = nMode;

	mask_main_lcd(g_main_window, &PHONE, &configuration, nMode);

	pwidget = g_object_get_data((GObject *) popup_menu, rotate[nMode%4]);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pwidget), TRUE);

	INFO( "rotate called\n");
}


void scale_event_callback(PHONEMODELINFO *device, int nMode)
{
	if (device == NULL || nMode < 0)
		return;

	UISTATE.current_mode = nMode;

	mask_main_lcd(g_main_window, &PHONE, &configuration, nMode);

	INFO( "scale called\n");
}


void menu_rotate_callback(PHONEMODELINFO *device, int nMode)
{
	int send_s;
	uint16_t port;

#ifndef _WIN32
	socklen_t send_len = (socklen_t)sizeof(struct sockaddr_in);
#else
	int send_len = (int)sizeof(struct sockaddr_in);
#endif
	char buf[32];
	struct sockaddr_in servaddr;

#ifdef __MINGW32__
	WSADATA wsadata;
	if(WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
		ERR("Error creating socket.");
		return;
	}
#endif

	if((send_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		ERR("Create the socket error: ");
		goto cleanup;
	}

	memset(&servaddr, '\0', sizeof(servaddr));
	port = get_sdb_base_port() + SDB_UDP_SENSOR_INDEX;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(port);

	INFO("sendto data for rotation [127.0.0.1:%d/udp] \n", port);

	switch(nMode)
	{
		case 0:
			sprintf(buf, "1\n0\n");
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

	if(sendto(send_s, buf, 32, 0, (struct sockaddr *)&servaddr, send_len) <= 0)
	{
		ERR("sendto data for rotation [127.0.0.1:%d/udp] fail \n", port);
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


void menu_keyboard_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *pWidget = NULL;
	char *buf = NULL;
	buf = data;

	pWidget = widget;

	if (g_strcmp0(buf, "On") == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			menu_rotate_callback(&PHONE, 7);
		}
	}
	else if (g_strcmp0(buf, "Off") == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			menu_rotate_callback(&PHONE, 8);
		}
	}
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

	/* 5. Scale menu */

	else if (g_strcmp0(buf, HALF_SIZE) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			if(UISTATE.current_mode > 3) {
				UISTATE.scale = 2.0;
				scale_event_callback(&PHONE, UISTATE.current_mode - 4);			
			}
		}
	}

	else if (g_strcmp0(buf, ACTUAL_SIZE) == 0) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			if(UISTATE.current_mode <= 3) {
				UISTATE.scale = 1.0;
				scale_event_callback(&PHONE, UISTATE.current_mode + 4);			
			}	
		}
	}

	else {
		//		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE)
		//			show_message(buf, buf);
	}

	INFO( "menu_event_callback called\n");
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
	//const gchar *version = build_version;
	const gchar *copyright =
		"Copyright (C) 2011 Samsung Electronics Co., LTD. All Rights Reserved.\n";
	gchar comments[512] = {0,};
	/* from qemu/MAINTAINERS, alphabetically */
	/*
	   const gchar *authors[] = {	
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
	 */
	//const gchar *website = "http://innovator.samsungmobile.com";

	sprintf(comments, "Tizen Emulator.\n"
			"Version: %s\n"
			//		"Based upon QEMU 0.10.5 (http://qemu.org)\n"
			"Build date: %s\nGit version: %s\n",
			build_version, build_date, build_git);

	GtkWidget* about_dialog = gtk_about_dialog_new();

	//	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), version);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), comments);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), copyright);
	//	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), website);
	gtk_show_about_dialog(GTK_WINDOW(parent),
			"program-name", "Tizen Emulator",
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

void show_info_window(GtkWidget *widget, gpointer data)
{
	char *target_name;
	char *virtual_target_path;
	char *info_file;
	int info_file_status;
	char *resolution = NULL;
	char *sdcard_type = NULL;
	char *sdcard_path = NULL;
	char *ram_size = NULL;
	char *dpi = NULL;
	char *disk_path = NULL;
	char *arch = NULL;
	char *sdcard_detail = NULL;
	char *ram_size_detail = NULL;
	char *disk_path_detail = NULL;
	char *sdcard_path_detail = NULL;
	char *details = NULL;

	target_name = (char*)data;
	virtual_target_path = get_virtual_target_path(target_name);
	info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);
	info_file_status = is_exist_file(info_file);
	//if targetlist exist but config file not exists
	if(info_file_status == -1 || info_file_status == FILE_NOT_EXISTS)
	{
		ERR( "target info file not exists : %s\n", target_name);
		return;
	}
	resolution= get_config_value(info_file, HARDWARE_GROUP, RESOLUTION_KEY);
	sdcard_type= get_config_value(info_file, HARDWARE_GROUP, SDCARD_TYPE_KEY);
	sdcard_path= get_config_value(info_file, HARDWARE_GROUP, SDCARD_PATH_KEY);
	ram_size = get_config_value(info_file, HARDWARE_GROUP, RAM_SIZE_KEY);
	dpi = get_config_value(info_file, HARDWARE_GROUP, DPI_KEY);
	disk_path = get_config_value(info_file, HARDWARE_GROUP, DISK_PATH_KEY);

	arch = getenv("EMULATOR_ARCH");
	if(!arch) /* for stand alone */
	{
		const gchar *vtm_path = get_vtm_path();
		char *binary = g_path_get_basename(vtm_path);
		if(strcmp(binary, "emulator-x86") == 0)
			arch = g_strdup_printf("x86");
		else if(strcmp(binary, "emulator-arm") == 0)
			arch = g_strdup_printf("arm");
		else 
		{
			ERR( "binary setting failed\n");
			exit(1);
		}
		free(binary);
	}

	if(strcmp(sdcard_type, "0") == 0)
	{
		sdcard_detail = g_strdup_printf("Not supported");
		sdcard_path_detail = g_strdup_printf(" ");
	}
	else
	{
		sdcard_detail = g_strdup_printf("Supported");
		sdcard_path_detail = g_strdup_printf("%s%s", get_bin_path(), sdcard_path); 
	}

	ram_size_detail = g_strdup_printf("%sMB", ram_size); 
	disk_path_detail = g_strdup_printf("%s/%s", get_bin_path(), disk_path);
#ifndef _WIN32		
	details = g_strdup_printf("Name: %s\nCPU: %s\nResolution: %s\nRam size: %s\nDPI: %s\nSD card: %s\nSD path: %s\nDisk path: %s",
			target_name, arch, resolution, ram_size_detail, dpi, sdcard_detail, sdcard_path_detail, disk_path_detail);
	show_message("Emulator Details", details);
#else /* _WIN32 */
	gchar *details_win = NULL;
	details = g_strdup_printf("Name: %s\nCPU: %s\nResolution: %s\nRam size: %s\nDPI: %s\nSD card: %s\nSD path: %s\nDisk path: %s",
			target_name, arch, resolution, ram_size_detail, dpi, sdcard_detail, sdcard_path_detail, disk_path_detail);
	details_win = change_path_from_slash(details);
	show_message("Emulator Details", details_win);
	free(details_win);
#endif
	g_free(resolution);
	g_free(sdcard_type);
	g_free(sdcard_path);
	g_free(ram_size);
	g_free(dpi);
	g_free(disk_path);
	g_free(sdcard_detail);
	g_free(ram_size_detail);
	g_free(disk_path_detail);
	g_free(sdcard_path_detail);
	g_free(details);
	return;
//	show_about_window(win);
}


