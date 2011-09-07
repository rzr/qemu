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


#include "menu.h"

/**
  * @brief  create popup advanced menu
  * @return void
  */

static void create_popup_advanced_menu(GtkWidget **pMenu, PHONEMODELINFO *device, CONFIGURATION *pconfiguration)
{
	GtkWidget *Item = NULL;
	GtkWidget *image_widget = NULL;
	GtkWidget *SubMenuItem = NULL;
	GtkWidget *SubMenuItem1 = NULL;
	GtkWidget *menu_item = NULL;
	GSList *pGroup = NULL;
	gchar icon_image[128] = {0, };
	const gchar *skin_path;
	int i, j = 0;

	skin_path = get_skin_path();
	if (skin_path == NULL)
		log_msg (MSGL_WARN, "getting icon image path is failed!!\n");

	/* 3. advanced */

	Item = gtk_image_menu_item_new_with_label(_("Advanced"));
	sprintf(icon_image, "%s/icons/02_ADVANCED.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);

	gtk_container_add(GTK_CONTAINER(*pMenu), Item);

	/* 3.1 sub_menu of advanced */

	SubMenuItem = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(Item), SubMenuItem);

	/* 3.2 event injector menu of advanced */
        if(configuration.enable_telephony_emulator){
        	menu_item = gtk_image_menu_item_new_with_label(_("Telephony Emulator"));
        	sprintf(icon_image, "%s/icons/03_TELEPHONY-eMULATOR.png", skin_path);
        	image_widget = gtk_image_new_from_file (icon_image);
        	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image_widget);
		if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
        		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(menu_item),TRUE);
        	
		gtk_widget_set_tooltip_text(menu_item,
        		"Emulate receiving and sending calls, SMSs, etc");
        	gtk_container_add(GTK_CONTAINER(SubMenuItem), menu_item);
        	if (UISTATE.is_ei_run == FALSE)
	        	g_object_set(menu_item, "sensitive", TRUE, NULL);
        
        	g_signal_connect(menu_item, "activate", G_CALLBACK(menu_create_eiwidget_callback), menu_item);
        	add_widget(EMULATOR_ID, MENU_EVENT_INJECTOR, menu_item);
        	gtk_widget_show(menu_item);
        }

#ifdef ENABLE_TEST_EI
	menu_create_eiwidget_callback(NULL, menu_item);
#endif

	/* SaveVM Menu */

	GtkWidget *savevm_menu_item = gtk_image_menu_item_new_with_label(_("Save Emulator State"));
	sprintf(icon_image, "%s/icons/05_GPS.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(savevm_menu_item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);

	if (UISTATE.is_gps_run == FALSE)
		g_object_set(savevm_menu_item, "sensitive", TRUE, NULL);

	//g_signal_connect(gps_menu_item, "activate", G_CALLBACK(menu_create_gps), NULL);
	g_signal_connect(savevm_menu_item, "activate", G_CALLBACK(save_emulator_state), NULL);

	gtk_container_add(GTK_CONTAINER(SubMenuItem), savevm_menu_item);
	add_widget(EMULATOR_ID, MENU_GPS, savevm_menu_item);
	gtk_widget_show(savevm_menu_item);

    /* GPS Menu */
        if(configuration.enable_gpsd){
	        GtkWidget *gps_menu_item = gtk_image_menu_item_new_with_label(_("GPS emulator"));
        	sprintf(icon_image, "%s/icons/05_GPS.png", skin_path);
        	image_widget = gtk_image_new_from_file (icon_image);
        
        	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gps_menu_item), image_widget);
		if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
        		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(gps_menu_item),TRUE);

        	if (UISTATE.is_gps_run == FALSE)
        		g_object_set(gps_menu_item, "sensitive", TRUE, NULL);
        
        	g_signal_connect(gps_menu_item, "activate", G_CALLBACK(menu_create_gps), NULL);
        
        	gtk_container_add(GTK_CONTAINER(SubMenuItem), gps_menu_item);
        	add_widget(EMULATOR_ID, MENU_GPS, gps_menu_item);
        	gtk_widget_show(gps_menu_item);
        }

	/* Compass Emulator */

	if(qemu_arch_is_arm() && configuration.enable_compass) {
		GtkWidget *compass_menu_item = gtk_image_menu_item_new_with_label(_("Compass"));
		sprintf(icon_image, "%s/icons/15_COMPASS.png", skin_path);
		image_widget = gtk_image_new_from_file (icon_image);

		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(compass_menu_item), image_widget);
		if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
		if (UISTATE.is_compass_run == FALSE)
			g_object_set(compass_menu_item, "sensitive", TRUE, NULL);

		g_signal_connect(compass_menu_item, "activate", G_CALLBACK(menu_create_compass), NULL);

		gtk_container_add(GTK_CONTAINER(SubMenuItem), compass_menu_item);
		add_widget(EMULATOR_ID, MENU_COMPASS, compass_menu_item);
		gtk_widget_show(compass_menu_item);
	}

	/* 3.5 screen shot menu of advanced */

	menu_item = gtk_image_menu_item_new_with_label(_("Screen Shot"));
	sprintf(icon_image, "%s/icons/06_SCREEN-SHOT.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
	gtk_widget_set_tooltip_text(menu_item,
		"Capture and Save the present Screen Shot ");
	g_signal_connect(menu_item, "activate", G_CALLBACK(frame_buffer_handler), NULL);
	gtk_container_add(GTK_CONTAINER(SubMenuItem), menu_item);
	gtk_widget_show(menu_item);

	/* 3.7 event for dbi file */

	if (device->event_menu_cnt > 0) {

		/* 3.7.1 sub menu */

		for (i = 0; i < device->event_menu_cnt; i++) {

			menu_item = gtk_image_menu_item_new_with_label(device->event_menu[i].name);
			sprintf(icon_image, "%s/icons/09_ROTATE.png", skin_path);
			image_widget = gtk_image_new_from_file (icon_image);

			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image_widget);
			if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
				gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
			gtk_container_add(GTK_CONTAINER(SubMenuItem), menu_item);
			gtk_widget_show(menu_item);

			SubMenuItem1 = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), SubMenuItem1);

			pGroup = NULL;
			for (j = 0; j < device->event_menu[i].event_list_cnt; j++) {

				menu_item = gtk_radio_menu_item_new_with_label(pGroup, device->event_menu[i].event_list[j].event_evalue);
				pGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));

				gtk_container_add(GTK_CONTAINER(SubMenuItem1), menu_item);

				g_signal_connect(menu_item, "activate", G_CALLBACK(menu_event_callback), device->event_menu[i].event_list[j].event_evalue);
				gtk_widget_show(menu_item);

				/* 3.7.3 object name set */

				g_object_set_data((GObject *) * pMenu, device->event_menu[i].event_list[j].event_evalue, (GObject *) menu_item);
				g_object_set(menu_item, "name", device->event_menu[i].event_list[j].event_evalue, NULL);

			}
		}
		gtk_widget_show(Item);
	}

	gtk_widget_show(Item);
}


/**
  * @brief  create popup properties menu
  * @return void
  */

static void create_popup_properties_menu(GtkWidget **pMenu, CONFIGURATION *pconfiguration)
{

	GtkWidget *Item = NULL;
	GtkWidget *SubMenuItem = NULL;
	GtkWidget *menu_item = NULL;
	GtkWidget *image_widget = NULL;
	gchar icon_image[128] = {0, };
	const gchar *skin_path;

	skin_path = get_skin_path();
	if (skin_path == NULL)
		log_msg (MSGL_WARN, "getting icon image path is failed!!\n");


	Item = gtk_image_menu_item_new_with_label(_("Properties"));
	sprintf(icon_image, "%s/icons/10_PROPERTIES.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
	gtk_container_add(GTK_CONTAINER(*pMenu), Item);

	/* 4.1 sub_menu of properties */

	SubMenuItem = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(Item), SubMenuItem);

	/* 4.2 option menu of properties */

	menu_item = gtk_image_menu_item_new_with_label(_("Options"));
	sprintf(icon_image, "%s/icons/11_OPTION.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
	gtk_widget_set_tooltip_text(menu_item, _("Set the present Emulator's Configuration(position, terminal type, fps, path)"));

	g_signal_connect(menu_item, "activate", G_CALLBACK(menu_option_callback), NULL);
	gtk_container_add(GTK_CONTAINER(SubMenuItem), menu_item);
	gtk_widget_show(menu_item);

	/* 4.4 device info menu of properties */

	menu_item = gtk_image_menu_item_new_with_label(_("Device Info"));
	sprintf(icon_image, "%s/icons/12_DEVICE-INFO.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
	gtk_widget_set_tooltip_text(menu_item, _("Show Target information like LCD Size"));

	g_signal_connect(menu_item, "activate", G_CALLBACK(menu_device_info_callback), NULL);
	gtk_container_add(GTK_CONTAINER(SubMenuItem), menu_item);
	gtk_widget_show(menu_item);

	gtk_widget_show(Item);
}


/**
  * @brief  create popup menu
  * @return void
  */

void create_popup_menu(GtkWidget **pMenu, PHONEMODELINFO *device, CONFIGURATION *pconfiguration)
{
	GtkWidget *Item = NULL;
	GtkWidget *menu_item = NULL;
	GtkWidget *image_widget = NULL;
	gchar icon_image[128] = {0, };
	const gchar *skin_path;

	*pMenu = gtk_menu_new();

	skin_path = get_skin_path();
	if (skin_path == NULL)
		log_msg (MSGL_WARN, "getting icon image path is failed!!\n");

	/* 2. shell menu */
        if(configuration.enable_shell){
	        Item = gtk_image_menu_item_new_with_label(_("Shell"));
	        sprintf(icon_image, "%s/icons/01_SHELL.png", skin_path);
        	image_widget = gtk_image_new_from_file (icon_image);

	        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Item), image_widget);
		if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
        		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
        	gtk_widget_set_tooltip_text(Item, _("Run Command Window (ssh)"));

        	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Item), image_widget);
		if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
        		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);
        	gtk_widget_set_tooltip_text(Item, _("Run Command Window (ssh)"));

        	g_signal_connect(Item, "activate", G_CALLBACK(create_cmdwindow), NULL);
        	gtk_container_add(GTK_CONTAINER(*pMenu), Item);
	        gtk_widget_show(Item);
        }
	/* 3. advanced menu */

	create_popup_advanced_menu(pMenu, device, pconfiguration);

	MENU_ADD_SEPARTOR(*pMenu);

	/* 4. properties menu */

	create_popup_properties_menu(pMenu, pconfiguration);

	/* 5. about menu */

	Item = gtk_image_menu_item_new_with_label(_("About"));
	sprintf(icon_image, "%s/icons/13_ABOUT.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);

	g_signal_connect(Item, "activate", G_CALLBACK(menu_about_callback), NULL);
	gtk_container_add(GTK_CONTAINER(*pMenu), Item);
	gtk_widget_show(Item);

	/* 6. exit menu */

	Item = gtk_image_menu_item_new_with_label(_("Close"));
	sprintf(icon_image, "%s/icons/14_CLOSE.png", skin_path);
	image_widget = gtk_image_new_from_file (icon_image);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Item), image_widget);
	if(GTK_MAJOR_VERSION >=2 && GTK_MINOR_VERSION >= 16)
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM(Item),TRUE);

	g_signal_connect(Item, "activate", G_CALLBACK(exit_emulator), NULL);
	gtk_container_add(GTK_CONTAINER(*pMenu), Item);
	gtk_widget_show(Item);
}

