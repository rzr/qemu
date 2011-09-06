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

#include "gps.h"

static gchar *g_gps_full_path;
static gchar *g_gps_basename;
static gchar gps_target_file[512] = "";
static gchar tmp_file[512] = "";

//scp mount.sh root@192.168.128.3:/root
static int safe_system(const char *command, const char *args1, const char *args2)
{
#ifndef _WIN32
	pid_t pid;
	if ((pid = vfork()) < 0) {
		log_msg(MSGL_WARN, "fork error\n");
		return -1;
	}

	if (pid == 0) { /* child */
		//printf("%s %s %s\n", command, args1, args2);
		execlp(command, command, args1, args2, (char *)0);
		exit(0);
	}

	else if (pid > 0) { /* parent */
		int status = 0;
		wait4(pid, &status, 0, NULL);
		log_msg(MSGL_DEBUG, "ssh pid = %d\n", pid);
	}
#else
	STARTUPINFO si;
	PROCESS_INFORMATION  pi;
	char *parameter;

	parameter = (char *)malloc(512);

	snprintf(parameter, "%s %s %s", command, args1, args2);	

	GetStartupInfo(&si);

	CreateProcess(command, parameter, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
#endif

	return 0;

}

#ifdef _WIN32
int secure_copy(const char *file1, const char * file2)
{
	//STARTUPINFO si;
	//PROCESS_INFORMATION  pi;
	char parameter[1024] = "";
	gchar *bin_path = NULL;
	
	//parameter = (char *)malloc(512);
	bin_path = get_path();
	memset(parameter, 0 , sizeof(1024));
	
	//sprintf(skin_path, "%s%s", path,"\\_skins");
	//sprintf(parameter, "%s%s %s root@192.168.128.3:%s", get_path(), "\\scp.exe", file1, file2);
	sprintf(parameter, "%s%s %s root@192.168.128.3:%s", bin_path, "\\scp.exe", file1+2, file2);
	//printf("pram=%s\N", parameter);
		
	//printf("parameter=%s\n", parameter);
	system(parameter);

	if (bin_path)
		g_free(bin_path);
	/*
	char asdf[512] = "";
	sprintf(asdf, "%s\\scp.exe", get_path());
	GetStartupInfo(&si);

	//CreateProcess("scp", parameter, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	CreateProcess(asdf, parameter, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	*/
	return 0;
}
#endif

//file_chooser_dialog
static void menu_gps_file_search(GtkWidget* widget, gpointer user_data)
{
	GtkWidget *file_chooser_dialog = NULL;
	gchar	gps_path[512] = "";
    GtkWidget * entry;
	const gchar *data_path = get_data_path();

	if (data_path == NULL) {
		log_msg(MSGL_ERROR, "Fail to get data path\n");
	}

	sprintf(gps_path, "%s/gps.log", data_path);

	file_chooser_dialog = gtk_file_chooser_dialog_new (_("Select GPS File"),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT,
			NULL);

	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_chooser_dialog), gps_path);

	/* Skin Preview */

	if (gtk_dialog_run (GTK_DIALOG (file_chooser_dialog)) == GTK_RESPONSE_ACCEPT) {
		g_gps_full_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(file_chooser_dialog));

		g_gps_basename = basename(g_gps_full_path);    

		entry = get_widget(EMULATOR_ID, MENU_GPS_FILE);
		gtk_entry_set_text(GTK_ENTRY(entry), g_gps_full_path);
	}   
	gtk_widget_destroy (file_chooser_dialog);
}


static int mkdir_lbs_dir (void)
{
	struct stat st;
	char dirpath[512] = "";
	/*
	char cmd[512] = "";
	sprintf(dirpath, "%s/opt/share/lbs-engine/res/", configuration.target_path);
	memset(cmd, '\0', 512);
	sprintf(cmd, "/bin/mkdir %s -p", dirpath);
		system(cmd);
	*/

	sprintf(dirpath, "%s/opt/share/lbs-engine", configuration.target_path);
	if (stat (dirpath, &st) != 0) { 
#ifndef _WIN32
		mkdir (dirpath, 0755);
#else
		mkdir (dirpath);
#endif
	}

	sprintf(dirpath, "%s/opt/share/lbs-engine/res/", configuration.target_path);
	if (stat (dirpath, &st) != 0) { 
#ifndef _WIN32
		mkdir (dirpath, 0755);
#else
		mkdir (dirpath);
#endif
	}

	return 0;

}

static void menu_gps_apply(GtkWidget* widget, gpointer user_data)
{

	GtkWidget * entry;
	const char *l_gps_full_path;
	GtkWidget * button;
	GtkWidget * status_entry;
	gchar temp_args[MIDBUF] = { 0, };

	entry = get_widget(EMULATOR_ID, MENU_GPS_FILE);
	l_gps_full_path = gtk_entry_get_text(GTK_ENTRY(entry));

	if (g_file_test(l_gps_full_path, G_FILE_TEST_EXISTS) == TRUE) {

		/* 1. when nfs connect*/
		
		if (configuration.qemu_configuration.diskimg_type == 0) {		
			mkdir_lbs_dir();
			safe_system("/bin/rm", "-f", tmp_file);
			safe_system("/bin/rm", "-f", gps_target_file);
			safe_system("/bin/cp", l_gps_full_path, gps_target_file);
		}

#ifndef _WIN32
		/* 2. when nfs connect*/		
		
		else {
			load_ssh("/bin/rm -f", tmp_file);
			load_ssh("/bin/rm -f", gps_target_file);
			snprintf(temp_args, sizeof(temp_args), "%s %s", l_gps_full_path, gps_target_file);
			load_ssh("/bin/rm -f", temp_args);
		}
#else
		/* 2. image */
		
		else {
			char cmdbuf[512] = "";
			sprintf(cmdbuf, "/bin/rm -f %s", tmp_file );
			load_ssh(cmdbuf, NULL);

			sprintf(cmdbuf, "%s %s %s", "/bin/mv", gps_target_file, tmp_file );
			load_ssh(cmdbuf, NULL);
			snprintf(temp_args, sizeof(temp_args), "%s %s", l_gps_full_path, gps_target_file);
			secure_copy(l_gps_full_path, gps_target_file);
			
		}
#endif
		button = get_widget(EMULATOR_ID, MENU_GPS_ACTIVE);
		gtk_widget_set_sensitive(button, FALSE);

		button = get_widget(EMULATOR_ID, MENU_GPS_INACTIVE);
		gtk_widget_set_sensitive(button, TRUE);

		status_entry = get_widget(EMULATOR_ID, MENU_GPS_STATUS);
		gtk_label_set_text(GTK_LABEL(status_entry), _("Active"));


	}
	else {
		show_message(_("Warning"), _("Please, Input GPS File "));
	}

}

static void menu_gps_copy_set(GtkWidget* widget, gpointer user_data)
{

	GtkWidget * status_entry;
	GtkWidget * button;
	gchar temp_args[MIDBUF] = { 0, };

	if (g_file_test(tmp_file, G_FILE_TEST_EXISTS) == TRUE) {

		if (g_file_test(gps_target_file, G_FILE_TEST_EXISTS) == TRUE) {

			/* 1.1 when nfs connected */
			
			if (configuration.qemu_configuration.diskimg_type == 0) 		
				safe_system("/bin/rm", "-f", gps_target_file);

			/* 1.2 when Image connected */
			
			else
				load_ssh("/bin/rm -f", gps_target_file);
		}

		/* 2.1 when nfs connected */
			
		if (configuration.qemu_configuration.diskimg_type == 0) {					
			//printf("dbi file copy (from %s to %s)\n", dbi_name, configuration.target_path);
			safe_system("/bin/cp", tmp_file, gps_target_file);
		}

		/* 2.3 when Image connected */
		
		else {
			snprintf(temp_args, sizeof(temp_args), "%s %s", tmp_file, gps_target_file);
			load_ssh("/bin/cp", temp_args);
		}

		button = get_widget(EMULATOR_ID, MENU_GPS_ACTIVE);
		gtk_widget_set_sensitive(button, FALSE);
		button = get_widget(EMULATOR_ID, MENU_GPS_INACTIVE);
		gtk_widget_set_sensitive(button, TRUE);

		status_entry = get_widget(EMULATOR_ID, MENU_GPS_STATUS);
		gtk_label_set_text(GTK_LABEL(status_entry), "Active");
	}
	
	else {
		show_message(_("Warning"), _("No GPS File exists\nPlease, Input GPS File "));
	}

	return;
}

static void make_backup_file(void)
{
	gchar temp_args[MIDBUF] = { 0, };

	if (g_file_test(gps_target_file, G_FILE_TEST_EXISTS) == TRUE) {

		if (g_file_test(tmp_file, G_FILE_TEST_EXISTS) != TRUE) {

			/* 1 when NFS connected */

			if (configuration.qemu_configuration.diskimg_type == 0) {
				safe_system("/bin/cp", gps_target_file, tmp_file);
			}

			/* 1.2 when Image connected */

			else {
				snprintf(temp_args, sizeof(temp_args), "%s %s", gps_target_file, tmp_file);
				load_ssh("/bin/cp", temp_args);
			}
		}
	}

	return;
}


static void menu_gps_remove_set(GtkWidget* widget, gpointer user_data)
{
	GtkWidget* entry;
	GtkWidget * button;
	char cmdbuf[512] = "";

	if (g_file_test(gps_target_file, G_FILE_TEST_EXISTS) == TRUE) {

		if (g_file_test(tmp_file, G_FILE_TEST_EXISTS) == TRUE) {
			/* 1.1 when NFS connected */
			if (configuration.qemu_configuration.diskimg_type == 0) 		
				safe_system("/bin/rm", "-f", tmp_file);
			/* 1.2 when Image connected */	
			else {

				sprintf(cmdbuf, "/bin/rm -f %s", tmp_file );
				load_ssh(cmdbuf, NULL);
			}
		}

		/* 2.1 when NFS connected */
		if (configuration.qemu_configuration.diskimg_type == 0) {		
			safe_system("/bin/mv", gps_target_file, tmp_file);
		}
		/* 2.2 when Image connected */
		else {
			sprintf(cmdbuf, "%s %s %s", "/bin/mv", gps_target_file, tmp_file );
			load_ssh(cmdbuf, NULL);
		}
	}
		
	else {
		show_message(_("Warning"), _("No exist GPS File, Already.\n"));
	}

	entry = get_widget(EMULATOR_ID, MENU_GPS_STATUS);
	gtk_label_set_text(GTK_LABEL(entry), _("Inactive"));

	button = get_widget(EMULATOR_ID, MENU_GPS_ACTIVE);
	gtk_widget_set_sensitive(button, TRUE);

	button = get_widget(EMULATOR_ID, MENU_GPS_INACTIVE);
	gtk_widget_set_sensitive(button, FALSE);

	return;

}

/**
  * @brief 	close gps window.
  * @param 	widget	dummy
  * @param	data	gps menu widget
  */
static void close_gps(GtkWidget* widget, gpointer data)
{
	UISTATE.is_gps_run = FALSE;
	g_object_set(data, "sensitive", TRUE, NULL);
}


void menu_create_gps(GtkWidget* parent)
{
    GtkWidget* gps_win;
    GtkWidget* label;
    GtkWidget* hbox;
    GtkWidget* vbox;

    GtkWidget* entry_gps_file;
    GtkWidget* button;
    GtkWidget* separator;
    GtkWidget* button2;		

	gps_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/*
	sprintf(gps_target_file, "/opt/share/lbs-engine/res/lbs_nmea_replay.log");
	sprintf(tmp_file, "/opt/share/lbs-engine/res/lbs_nmea_replay.bak");
	*/

	/* 1. when NFS connected */
	if (configuration.qemu_configuration.diskimg_type == 0) {		
		sprintf(gps_target_file, "%s/opt/share/lbs-engine/res/lbs_nmea_replay.log", configuration.target_path);
		sprintf(tmp_file, "%s/opt/share/lbs-engine/res/lbs_nmea_replay.bak", configuration.target_path);
	}

	/* 2. when Image connected */	
	else {
		sprintf(gps_target_file, "/opt/share/lbs-engine/res/lbs_nmea_replay.log");
		sprintf(tmp_file, "/opt/share/lbs-engine/res/lbs_nmea_replay.bak");
	}

	make_backup_file();

	gchar icon_image[128] = {0, };
	const gchar *skin = get_skin_path();	
	if (skin == NULL) 
		log_msg (MSGL_WARN, "getting icon image path is failed!!\n");

	sprintf(icon_image, "%s/icons/05_GPS.png", skin);	
	
	gtk_window_set_default_icon_from_file (icon_image, NULL);
	gtk_window_set_resizable(GTK_WINDOW(gps_win), FALSE);
	gtk_window_set_position (GTK_WINDOW (gps_win), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (gps_win), _("GPS Emulator"));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_container_add(GTK_CONTAINER(gps_win), vbox);


	/* hbox - set, unset */

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(_("GPS State   :"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	if (g_file_test(gps_target_file, G_FILE_TEST_EXISTS) == TRUE) {
		label = gtk_label_new(_("Active"));
	}
	else {
		label = gtk_label_new(_("Inactive"));
	}
	gtk_widget_set_size_request(GTK_WIDGET(label), 90, -1);

    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    add_widget(EMULATOR_ID, MENU_GPS_STATUS, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 4);

    button = gtk_button_new_with_label(_("Inactive"));
    add_widget(EMULATOR_ID, MENU_GPS_INACTIVE, button);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_set_size_request(GTK_WIDGET(button), 120, -1);
	if (g_file_test(gps_target_file, G_FILE_TEST_EXISTS) != TRUE) {
		gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	}
    g_signal_connect((gpointer)button, "clicked",
                     G_CALLBACK(menu_gps_remove_set),
                     (gpointer)button);

    button2 =  gtk_button_new_with_label(_("Active"));
    add_widget(EMULATOR_ID, MENU_GPS_ACTIVE, button2);
    gtk_box_pack_end(GTK_BOX(hbox), button2, FALSE, FALSE, 10);
    gtk_widget_set_size_request(GTK_WIDGET(button2), 120, -1);
	if (g_file_test(gps_target_file, G_FILE_TEST_EXISTS) == TRUE) {
		gtk_widget_set_sensitive(GTK_WIDGET(button2), FALSE);
	}
    g_signal_connect((gpointer)button2, "clicked",
                     G_CALLBACK(menu_gps_copy_set),
                     (gpointer)button2);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 5);

	/* GPS File */

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(_("GPS File : "));

    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	entry_gps_file = gtk_entry_new (); 
	
	gtk_entry_set_text (GTK_ENTRY (entry_gps_file), "");

	gtk_box_pack_start (GTK_BOX (hbox), entry_gps_file, TRUE, TRUE, 2);
    add_widget(EMULATOR_ID, MENU_GPS_FILE, entry_gps_file);
    gtk_widget_set_size_request(GTK_WIDGET(entry_gps_file), 220, -1);


    button = gtk_button_new_with_label(_("..."));
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_set_size_request(GTK_WIDGET(button), 40, -1);
    g_signal_connect((gpointer)button, "clicked",
                     G_CALLBACK(menu_gps_file_search),
                     (gpointer)button);

    button2 =  gtk_button_new_with_label(_("Apply"));
    gtk_box_pack_end(GTK_BOX(vbox), button2, FALSE, FALSE, 10);
    gtk_widget_set_size_request(GTK_WIDGET(button2), 120, -1);
    g_signal_connect((gpointer)button2, "clicked",
                     G_CALLBACK(menu_gps_apply),
                     (gpointer)button2);

	GtkWidget* emulator_widget;
	emulator_widget = get_widget(EMULATOR_ID, MENU_GPS);
	g_object_set(emulator_widget, "sensitive", FALSE, NULL);
	g_signal_connect (gps_win, "destroy", G_CALLBACK(close_gps), emulator_widget);
	gtk_widget_show_all(gps_win);
	
	return;
}


