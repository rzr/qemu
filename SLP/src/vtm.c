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


#include "vtm.h"

#define SDCARD_SIZE_256		"256"
#define SDCARD_SIZE_512		"512"
#define SDCARD_SIZE_1024	"1024"
#define SDCARD_SIZE_1536	"1536"
#define SDCARD_DEFAULT_SIZE		1

#define RAM_SIZE_512	"512"
#define RAM_SIZE_768	"768"
#define RAM_SIZE_1024	"1024"
#define RAM_DEFAULT_SIZE	0

GtkWidget *g_main_window;
GtkBuilder *g_builder;
VIRTUALTARGETINFO virtual_target_info;
gchar *target_list_filepath;

int sdcard_create_size;

gchar *remove_space(const gchar *str)
{
	int i = 0;
	const char *in_str;
	char out_str[MAXBUF];

	in_str = str;

	while(1)
	{
		if(*in_str == '\0')
		{
			out_str[i] = 0;
			break;
		}
		else if(*in_str == ' ')
		{
			in_str++;
			continue;
		}
		else
		{
			out_str[i] = *in_str;
		}
		in_str++;
		i++;
	}
	return g_strdup(out_str);
}

gboolean run_cmd(char *cmd)
{
	char *s_out = NULL;
	char *s_err = NULL;
	int exit_status;
	GError *err = NULL;

	g_return_val_if_fail(cmd != NULL, FALSE);

	log_msg(MSGL_DEBUG, "Command: %s\n", cmd);
	if (!g_spawn_command_line_sync(cmd, &s_out, &s_err, &exit_status, &err)) {
		log_msg(MSGL_DEBUG, "Failed to invoke command: %s\n", err->message);
		show_message("Failed to invoke command", err->message);
		g_error_free(err);
		g_free(s_out);
		g_free(s_err);
		return FALSE;
	}
	if (exit_status != 0) {
		log_msg(MSGL_DEBUG, "Command returns error: %s\n", s_out);
		show_message("Command returns error", s_out);
		g_free(s_out);
		g_free(s_err);
		return FALSE;
	}

	log_msg(MSGL_DEBUG, "Command success: %s\n", cmd);
//	show_message("Command success!", s_out);
	g_free(s_out);
	g_free(s_err);
	return TRUE;
}

void fill_virtual_target_info(void)
{
	snprintf(virtual_target_info.resolution, MAXBUF, "480x800");
	virtual_target_info.sdcard_type = 0;
	sdcard_create_size = 512;
	virtual_target_info.ram_size = 512;
	snprintf(virtual_target_info.dpi, MAXBUF, "2070");
}

int create_config_file(gchar* filepath)
{
	FILE *fp = g_fopen(filepath, "w+");

	if (fp != NULL) {
		g_fprintf (fp, "[%s]\n", HARDWARE_GROUP);
		g_fprintf (fp, "%s=\n", RESOLUTION_KEY);
		g_fprintf (fp, "%s=\n", SDCARD_TYPE_KEY);
		g_fprintf (fp, "%s=\n", SDCARD_PATH_KEY);
		g_fprintf (fp, "%s=\n", RAM_SIZE_KEY);
		g_fprintf (fp, "%s=\n", DPI_KEY);
		g_fprintf (fp, "%s=\n", DISK_PATH_KEY);

		fclose(fp);
	}
	else {
		log_msg(MSGL_ERROR, "Can't open file path. (%s)\n", filepath);
		return -1;
	}

#ifndef _WIN32
	chmod(filepath, S_IRWXU | S_IRWXG | S_IRWXO);
#else
	chmod(filepath, S_IRWXU);
#endif

	return 0;
}

int write_config_file(gchar *filepath)
{
    set_config_value(filepath, HARDWARE_GROUP, RESOLUTION_KEY, virtual_target_info.resolution);
	set_config_type(filepath, HARDWARE_GROUP, SDCARD_TYPE_KEY, virtual_target_info.sdcard_type);
	set_config_value(filepath, HARDWARE_GROUP, SDCARD_PATH_KEY, virtual_target_info.sdcard_path);
    set_config_type(filepath, HARDWARE_GROUP, RAM_SIZE_KEY, virtual_target_info.ram_size);
    set_config_value(filepath, HARDWARE_GROUP, DPI_KEY, virtual_target_info.dpi);
	set_config_value(filepath, HARDWARE_GROUP, DISK_PATH_KEY, virtual_target_info.diskimg_path);

	return 0;
}

int name_collision_check(void)
{
	int i;
	int num = 0;
	gchar **target_list = NULL;

	target_list = get_virtual_target_list(target_list_filepath, TARGET_LIST_GROUP, &num);

	for(i = 0; i < num; i++)
	{
		if(strcmp(target_list[i], virtual_target_info.virtual_target_name) == 0)
			return 1;
	}

	g_strfreev(target_list);	
	return 0;
}

void exit_vtm(void)
{
	log_msg(MSGL_INFO, "virtual target manager exit \n");

	window_hash_destroy();
	gtk_main_quit();
}

void create_window_deleted_cb(void)
{
	GtkWidget *win = NULL;
	log_msg(MSGL_INFO, "create window exit \n");

	win = get_window(VTM_CREATE_ID);

	gtk_widget_destroy(win);
	window_hash_destroy(); // when main window is back, this line should be removed

	gtk_main_quit();
}

void resolution_select_cb(GtkWidget *widget, gpointer data)
{
	char *resolution;

	GtkToggleButton *toggled_button = GTK_TOGGLE_BUTTON(data);

	if(gtk_toggle_button_get_active(toggled_button) == TRUE)
	{
		resolution = remove_space(gtk_button_get_label(GTK_BUTTON(toggled_button)));
		snprintf(virtual_target_info.resolution, MAXBUF, "%s", resolution);
		log_msg(MSGL_INFO, "resolution : %s\n", gtk_button_get_label(GTK_BUTTON(toggled_button)));
		g_free(resolution);
	}
}

void sdcard_size_select_cb(void)
{
	char *size;

	GtkComboBox *sdcard_combo_box = (GtkComboBox *)get_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX);	

	size = gtk_combo_box_get_active_text(sdcard_combo_box);
	sdcard_create_size = atoi(size);
	log_msg(MSGL_INFO, "sdcard create size : %d\n", atoi(size));

	g_free(size);
}

void set_sdcard_create_active_cb(void)
{
	gboolean active = FALSE;
	
	GtkWidget *create_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton4");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_radiobutton));

	if(active == TRUE)
		virtual_target_info.sdcard_type = 1;

	GtkWidget *sdcard_combo_box = (GtkWidget *)get_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX);

	gtk_widget_set_sensitive(sdcard_combo_box, active);
}

void set_sdcard_select_active_cb(void)
{
	gboolean active = FALSE;
	
	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton5");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(select_radiobutton));
	
	if(active == TRUE)
		virtual_target_info.sdcard_type = 2;
	
	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_builder, "filechooserbutton1");

	gtk_widget_set_sensitive(sdcard_filechooser, active);
}

void set_sdcard_none_active_cb(void)
{
	gboolean active = FALSE;
	
	GtkWidget *none_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton6");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(none_radiobutton));
	
	if(active == TRUE)
		virtual_target_info.sdcard_type = 0;
}

void sdcard_file_select_cb(void)
{
	gchar *path = NULL;

	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_builder, "filechooserbutton1");
	
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sdcard_filechooser));
	snprintf(virtual_target_info.sdcard_path, MAXBUF, "%s", path);
	log_msg(MSGL_INFO, "sdcard path : %s\n", path);

	g_free(path);
}

void ram_select_cb(void)
{
	char *size;

	GtkComboBox *ram_combobox = (GtkComboBox *)get_widget(VTM_CREATE_ID, VTM_CREATE_RAM_COMBOBOX);

	size = gtk_combo_box_get_active_text(ram_combobox);
	virtual_target_info.ram_size = atoi(size);
	log_msg(MSGL_INFO, "ram size : %d\n", atoi(size));

	g_free(size);
}

void ok_clicked_cb(void)
{
	GtkWidget *win = get_window(VTM_CREATE_ID);

	char *cmd = NULL;
	char *dest_path = NULL;
	char *conf_file = NULL;
	const gchar *name = NULL;

	GtkWidget *name_entry = (GtkWidget *)gtk_builder_get_object(g_builder, "entry1");
	name = gtk_entry_get_text(GTK_ENTRY(name_entry));

// name validation
	if(strcmp(name, "") == 0)
	{
		show_message("Warning", "Please specify name of the virtual target!");
		return;
	}
	else
	{
		snprintf(virtual_target_info.virtual_target_name, MAXBUF, "%s", name);
	}

	if(name_collision_check() == 1)
	{
		show_message("Warning", "Virtual target with the same name exists! Please choose another name.");
		return;
	}

	dest_path = get_virtual_target_path(virtual_target_info.virtual_target_name);
	if(access(dest_path, R_OK) != 0)
#ifndef _WIN32
		mkdir(dest_path, S_IRWXU | S_IRWXG);
#else
		mkdir(dest_path);
#endif
// sdcard
	if(virtual_target_info.sdcard_type == 0)
	{
		memset(virtual_target_info.sdcard_path, 0x00, MAXBUF);
	}
	else if(virtual_target_info.sdcard_type == 1)
	{
		// sdcard create
#ifndef _WIN32
		cmd = g_strdup_printf("cp %s/sdcard_%d.img %s", get_data_path(), sdcard_create_size, dest_path);
		
		if(!run_cmd(cmd))
		{
			g_free(cmd);
			g_free(dest_path);
			show_message("Error", "SD Card img create failed!");
			return;
		}
		g_free(cmd);
#else
		char *src_sdcard_path = g_strdup_printf("%s/sdcard_%d.img", get_data_path(), sdcard_create_size);
		char *dst_sdcard_path = g_strdup_printf("%s/sdcard_%d.img", dest_path, sdcard_create_size);

		gchar *src_dos_path = change_path_from_slash(src_sdcard_path);
		gchar *dst_dos_path = change_path_from_slash(dst_sdcard_path);

		g_free(src_sdcard_path);
		g_free(dst_sdcard_path);

		if(!CopyFileA(src_dos_path, dst_dos_path, FALSE))
		{
			g_free(dest_path);
			g_free(src_dos_path);
			g_free(dst_dos_path);
			show_message("Error", "SD Card img create failed!");
			return;
		}
		g_free(src_dos_path);
		g_free(dst_dos_path);

#endif

		snprintf(virtual_target_info.sdcard_path, MAXBUF, "%ssdcard_%d.img", dest_path, sdcard_create_size);
	}
// create emulator image	
	cmd = g_strdup_printf("qemu-img create -b %s/emulimg.x86 -f qcow2 %semulimg-%s.x86", get_path(),
			dest_path, virtual_target_info.virtual_target_name);
	if(!run_cmd(cmd))
	{
		g_free(cmd);
		g_free(dest_path);
		show_message("Error", "emulator image create failed!");
		return;
	}
	g_free(cmd);

// diskimg_path
	snprintf(virtual_target_info.diskimg_path, MAXBUF, "%semulimg-%s.x86", dest_path, 
			virtual_target_info.virtual_target_name);

// add virtual target name to targetlist.ini
	set_config_value(target_list_filepath, TARGET_LIST_GROUP, virtual_target_info.virtual_target_name, "");
// write config.ini
	conf_file = g_strdup_printf("%sconfig.ini", dest_path);
	create_config_file(conf_file);
	write_config_file(conf_file);

	show_message("INFO", "Virtual target creation success!");

	g_free(conf_file);
	g_free(dest_path);

	gtk_widget_destroy(win);
	window_hash_destroy(); // when main window is back, this line should be removed

	gtk_main_quit();
	return;
}

void setup_create_frame(void)
{
	setup_resolution_frame();
	setup_sdcard_frame();
	setup_ram_frame();
}

void setup_create_button(void)
{
	GtkWidget *ok_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button4");
	GtkWidget *cancel_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button5");
	
	g_signal_connect(ok_button, "clicked", G_CALLBACK(ok_clicked_cb), NULL);
	g_signal_connect(cancel_button, "clicked", G_CALLBACK(create_window_deleted_cb), NULL);
}

void setup_resolution_frame(void)
{
	GtkWidget *radiobutton1 = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton1");
	GtkWidget *radiobutton2 = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton2");
	GtkWidget *radiobutton3 = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton3");
	GtkWidget *radiobutton4 = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton7");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton2), TRUE);

	g_signal_connect(GTK_RADIO_BUTTON(radiobutton1), "toggled", G_CALLBACK(resolution_select_cb), radiobutton1);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton2), "toggled", G_CALLBACK(resolution_select_cb), radiobutton2);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton3), "toggled", G_CALLBACK(resolution_select_cb), radiobutton3);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton4), "toggled", G_CALLBACK(resolution_select_cb), radiobutton4);
}

void setup_sdcard_frame(void)
{
// sdcard size combo box setup
	GtkWidget *hbox = (GtkWidget *)gtk_builder_get_object(g_builder, "hbox4");
	
	GtkComboBox *sdcard_combo_box = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(sdcard_combo_box), FALSE, FALSE, 1);
	add_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX, GTK_WIDGET(sdcard_combo_box));
	
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_256); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_512); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_1024); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_1536); 
	
	gtk_combo_box_set_active(sdcard_combo_box, SDCARD_DEFAULT_SIZE);

// radio button setup
	GtkWidget *create_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton4");
	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton5");
	GtkWidget *none_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton6");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(none_radiobutton), TRUE);

// file chooser setup
	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_builder, "filechooserbutton1");

	set_sdcard_create_active_cb();	
	set_sdcard_select_active_cb();	

	g_signal_connect(G_OBJECT(sdcard_combo_box), "changed", G_CALLBACK(sdcard_size_select_cb), NULL);
	g_signal_connect(G_OBJECT(create_radiobutton), "toggled", G_CALLBACK(set_sdcard_create_active_cb), NULL);
	g_signal_connect(G_OBJECT(select_radiobutton), "toggled", G_CALLBACK(set_sdcard_select_active_cb), NULL);
	g_signal_connect(G_OBJECT(none_radiobutton), "toggled", G_CALLBACK(set_sdcard_none_active_cb), NULL);
	g_signal_connect(G_OBJECT(sdcard_filechooser), "selection-changed", G_CALLBACK(sdcard_file_select_cb), NULL);
}

void setup_ram_frame(void)
{
	GtkWidget *hbox = (GtkWidget *)gtk_builder_get_object(g_builder, "hbox6");
	
	GtkComboBox *ram_combo_box = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ram_combo_box), FALSE, FALSE, 1);
	add_widget(VTM_CREATE_ID, VTM_CREATE_RAM_COMBOBOX, GTK_WIDGET(ram_combo_box));
	
	gtk_combo_box_append_text(ram_combo_box, RAM_SIZE_512); 
	gtk_combo_box_append_text(ram_combo_box, RAM_SIZE_768); 
	gtk_combo_box_append_text(ram_combo_box, RAM_SIZE_1024); 
	
	gtk_combo_box_set_active(ram_combo_box, RAM_DEFAULT_SIZE);

	g_signal_connect(G_OBJECT(ram_combo_box), "changed", G_CALLBACK(ram_select_cb), NULL);
}

void show_create_window(void)
{
	GtkWidget *win = (GtkWidget *)gtk_builder_get_object(g_builder, "window2");
	add_window(win, VTM_CREATE_ID);

	fill_virtual_target_info();

	setup_create_frame();
	setup_create_button();

	gtk_widget_show_all(win);
	g_signal_connect(GTK_OBJECT(win), "delete_event", G_CALLBACK(create_window_deleted_cb), NULL);

	gtk_main();
}

void construct_main_window(void)
{
	g_main_window = (GtkWidget *)gtk_builder_get_object(g_builder, "window1");

	g_signal_connect(G_OBJECT(g_main_window), "delete-event", G_CALLBACK(exit_vtm), NULL); 

	gtk_widget_show(g_main_window);
}

int main(int argc, char** argv)
{
	int target_list_status;
	gtk_init(&argc, &argv);
	log_msg_init(MSGL_DEFAULT);
	
	log_msg(MSGL_INFO, "virtual target manager start \n");

	target_list_filepath = get_target_list_filepath();
	target_list_status = is_exist_file(target_list_filepath);

	if(target_list_status == -1 || target_list_status == FILE_NOT_EXISTS)
	{
		log_msg(MSGL_ERROR, "load target list file error\n");
		exit(1);
	}

	g_builder = gtk_builder_new();
	char full_glade_path[MAX_LEN];
	sprintf(full_glade_path, "%s/vtm.glade", get_bin_path());

	gtk_builder_add_from_file(g_builder, full_glade_path, NULL);

	window_hash_init();

//	construct_main_window();
	show_create_window();

//	gtk_main();

	free(target_list_filepath);
	return 0;
}
