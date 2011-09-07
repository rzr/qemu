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

#include "compass.h"
//#define DEBUG

int compass_update(uint8_t x, uint8_t y, uint8_t z, uint8_t temp);

static void menu_compass_apply(GtkWidget* widget, gpointer user_data)
{

	GtkWidget * entry;
	const char *x = NULL, *y = NULL, *z=NULL, *temp = NULL;
	int nx, ny, nz, ntemp;

	entry = get_widget(EMULATOR_ID, ENTRY_COMPASS_X);
	x = gtk_entry_get_text(GTK_ENTRY(entry));

	if ((x == NULL) || (strlen(x) < 1)) {
		show_message("Warning", "Please, Input X value ");
		return;
	}

#define MAX_COMPASS_VALUE 1000

	nx = atoi(x);
	if ((nx < 0) || (nx > MAX_COMPASS_VALUE)) {
		show_message("Warning", "Please, check X value");
	}

	entry = get_widget(EMULATOR_ID, ENTRY_COMPASS_Y);
	y = gtk_entry_get_text(GTK_ENTRY(entry));
	
	if ((y == NULL) || (strlen(y) < 1)) {
		show_message("Warning", "Please, Input Y value ");
		return;
	}

	ny = atoi(y);
	if ((ny < 0) || (ny > MAX_COMPASS_VALUE)) {
		show_message("Warning", "Please, check Y value");
	}


	entry = get_widget(EMULATOR_ID, ENTRY_COMPASS_Z);
	z = gtk_entry_get_text(GTK_ENTRY(entry));

	if ((z == NULL) || (strlen(z) < 1)) {
		show_message("Warning", "Please, Input Z value ");
		return;
	}

	nz = atoi(z);
	if ((nz < 0) || (nz > MAX_COMPASS_VALUE)) {
		show_message("Warning", "Please, check Z value");
	}


	entry = get_widget(EMULATOR_ID, ENTRY_COMPASS_TEMP);
	temp = gtk_entry_get_text(GTK_ENTRY(entry));

	if ((temp == NULL) || (strlen(temp) < 1)) {
		show_message("Warning", "Please, Input Temp value ");
		return;
	}

	ntemp = atoi(temp);
	if ((ntemp < 0) || (ntemp > MAX_COMPASS_VALUE)) {
		show_message("Warning", "Please, check Tempature value");
	}

	/* comment out for linking
	compass_update(nx, ny, nz, ntemp);
	*/
	show_message("Warning", "Not support: compass_update => abort");
	abort();

#ifdef DEBUG
	printf("%s, %s, %s, %s\n", x, y, z, temp);
#endif

}

/**
  * @brief 	close compass window.
  * @param 	widget	dummy
  * @param	data	compass menu widget
  */
static void close_compass(GtkWidget* widget, gpointer data)
{
	UISTATE.is_compass_run = FALSE;
	g_object_set(data, "sensitive", TRUE, NULL);
}



void menu_create_compass(GtkWidget* parent)
{
    GtkWidget* compass_win;
    GtkWidget* label;
    GtkWidget *hbox_a, *hbox_b, *hbox_z, *hbox_temp;
    GtkWidget* vbox;

    GtkWidget* entry_x;
    GtkWidget* entry_y;
    GtkWidget* entry_z;
    GtkWidget* entry_temp;
    GtkWidget* separator;
    GtkWidget* button2;		

	compass_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gchar icon_image[128] = {0, };
	const gchar *skin = get_skin_path();	
	if (skin == NULL) 
		log_msg (MSGL_WARN, "getting icon image path is failed!!\n");

	sprintf(icon_image, "%s/icons/15_COMPASS.png", skin);	
	
	gtk_window_set_default_icon_from_file (icon_image, NULL);
	gtk_window_set_resizable(GTK_WINDOW(compass_win), FALSE);
	gtk_window_set_position (GTK_WINDOW (compass_win), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (compass_win), _("Compass Emulator"));
	
	/* Vbox - main box */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_container_add(GTK_CONTAINER(compass_win), vbox);

	/* hbox - X */

	hbox_a = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox_a, FALSE, FALSE, 0);

	/* X : label */
	label = gtk_label_new(_(" X component "));
	gtk_label_set_width_chars((GtkLabel *)label, 13);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox_a), label, FALSE, FALSE, 10);

	/* X : text */
	entry_x = gtk_entry_new (); 
	gtk_entry_set_text (GTK_ENTRY (entry_x), "");
	gtk_box_pack_start (GTK_BOX (hbox_a), entry_x, TRUE, TRUE, 2);
    add_widget(EMULATOR_ID, ENTRY_COMPASS_X, entry_x);
    gtk_widget_set_size_request(GTK_WIDGET(entry_x), 220, -1);

	/* hbox : Y */

	hbox_b = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox_b, FALSE, FALSE, 0);

	/* Y : label */
	label = gtk_label_new(_(" Y component "));
	gtk_label_set_width_chars((GtkLabel *)label, 13);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox_b), label, FALSE, FALSE, 10);

	/* Y : text */
	entry_y = gtk_entry_new (); 
	gtk_entry_set_text (GTK_ENTRY (entry_y), "");
	gtk_box_pack_start (GTK_BOX (hbox_b), entry_y, TRUE, TRUE, 2);
    add_widget(EMULATOR_ID, ENTRY_COMPASS_Y, entry_y);
    gtk_widget_set_size_request(GTK_WIDGET(entry_y), 220, -1);

	/* hbox : Z */

	hbox_z = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox_z, FALSE, FALSE, 0);

	/* Z : label */
	label = gtk_label_new(_(" Z component "));
	gtk_label_set_width_chars((GtkLabel *)label, 13);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox_z), label, FALSE, FALSE, 10);

	/* Z : text */
	entry_z = gtk_entry_new (); 
	gtk_entry_set_text (GTK_ENTRY (entry_z), "");
	gtk_box_pack_start (GTK_BOX (hbox_z), entry_z, TRUE, TRUE, 2);
    add_widget(EMULATOR_ID, ENTRY_COMPASS_Z, entry_z);
    gtk_widget_set_size_request(GTK_WIDGET(entry_z), 220, -1);

	/* hbox : Temp */

	hbox_temp = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox_temp, FALSE, FALSE, 0);

	/* Temp : label */
	label = gtk_label_new(_(" Temperature "));
	gtk_label_set_width_chars((GtkLabel *)label, 13);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox_temp), label, FALSE, FALSE, 10);

	/* Temp : text */
	entry_temp = gtk_entry_new (); 
	gtk_entry_set_text (GTK_ENTRY (entry_temp), "");
	gtk_box_pack_start (GTK_BOX (hbox_temp), entry_temp, TRUE, TRUE, 2);
    add_widget(EMULATOR_ID, ENTRY_COMPASS_TEMP, entry_temp);
    gtk_widget_set_size_request(GTK_WIDGET(entry_temp), 220, -1);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 5);

    button2 =  gtk_button_new_with_label(_("Apply"));
    gtk_box_pack_end(GTK_BOX(vbox), button2, FALSE, FALSE, 10);
    gtk_widget_set_size_request(GTK_WIDGET(button2), 120, -1);
    g_signal_connect((gpointer)button2, "clicked",
                     G_CALLBACK(menu_compass_apply),
                     (gpointer)button2);

	GtkWidget* emulator_widget;
	emulator_widget = get_widget(EMULATOR_ID, MENU_COMPASS);
	g_object_set(emulator_widget, "sensitive", FALSE, NULL);
	g_signal_connect (compass_win, "destroy", G_CALLBACK(close_compass), emulator_widget);
	gtk_widget_show_all(compass_win);
	
	return;
}

