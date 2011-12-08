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
#include "debug_ch.h"
#ifndef _WIN32
#include <sys/ipc.h>  
#include <sys/shm.h> 
#endif
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else /* !_WIN32 */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif /* !_WIN32 */


//DEFAULT_DEBUG_CHANNEL(tizen);
MULTI_DEBUG_CHANNEL(tizen, emulmgr);

#define SDCARD_SIZE_256		"256"
#define SDCARD_SIZE_512		"512"
#define SDCARD_SIZE_1024	"1024"
#define SDCARD_SIZE_1536	"1536"
#define SDCARD_DEFAULT_SIZE		1
# define VT_NAME_MAXBUF		21
#define RAM_SIZE_512	"512"
#define RAM_SIZE_768	"768"
#define RAM_SIZE_1024	"1024"
#define RAM_DEFAULT_SIZE	0
#define RAM_768_SIZE	1
#define RAM_1024_SIZE	2
#define CREATE_MODE	1
#define DELETE_MODE	2
#define MODIFY_MODE 3

GtkBuilder *g_builder;
GtkBuilder *g_create_builder;
enum {
	TARGET_NAME,
	RAM_SIZE,
	RESOLUTION,
	N_COL
};
GtkWidget *g_main_window;
VIRTUALTARGETINFO virtual_target_info;
gchar *target_list_filepath;
gchar *g_info_file;
GtkWidget *list;
int sdcard_create_size;
GtkWidget *f_entry;
gchar *g_arch;
gchar icon_image[128] = {0, };

#ifdef _WIN32
void socket_cleanup(void)
{
	WSACleanup();
}
#endif

int socket_init(void)
{
#ifdef _WIN32
	WSADATA Data;
	int ret, err;

	ret = WSAStartup(MAKEWORD(2,0), &Data);
	if (ret != 0) {
		err = WSAGetLastError();
		fprintf(stderr, "WSAStartup: %d\n", err);
		return -1;
	}
	atexit(socket_cleanup);
#endif
	return 0;
}


static int check_port_bind_listen(u_int port)
{
	struct sockaddr_in addr;
	int s, opt = 1;
	int ret = -1;
	socklen_t addrlen = sizeof(addr);
	memset(&addr, 0, addrlen);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (((s = socket(AF_INET,SOCK_STREAM,0)) < 0) ||
			(bind(s,(struct sockaddr *)&addr, sizeof(addr)) < 0) ||
			(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int)) < 0) ||
			(listen(s,1) < 0)) {

		/* fail */
		ret = -1;
		ERR( "port(%d) listen  fail \n", port);
	}else{
		/*fsucess*/
		ret = 1;
		INFO( "port(%d) listen  ok \n", port);
	}

#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif

	return ret;
}


void activate_target(char *target_name)
{
	char *cmd = NULL;
	GError *error = NULL;
	char *enable_kvm = NULL;
	char *kvm = NULL;
	gchar *path;
	char *info_file;
	char *virtual_target_path;
	char *binary = NULL;
	char *emul_add_opt = NULL;
	char *qemu_add_opt = NULL;
	int info_file_status;	

	if(check_shdmem(target_name, CREATE_MODE) == -1)
		return ;

	path = (char*)get_arch_abs_path();
	virtual_target_path = get_virtual_target_abs_path(target_name);
	info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);
	info_file_status = is_exist_file(info_file);
	//if targetlist exist but config file not exists
	if(info_file_status == -1 || info_file_status == FILE_NOT_EXISTS)
	{
		ERR( "target info file not exists : %s\n", target_name);
		return ;
	}

#ifndef _WIN32
	kvm = get_config_value(info_file, QEMU_GROUP, KVM_KEY);
	if(g_file_test("/dev/kvm", G_FILE_TEST_EXISTS) && strcmp(kvm,"1") == 0)
	{
		enable_kvm = g_strdup_printf("-enable-kvm");
	}
	else
		enable_kvm = g_strdup_printf(" ");
#else /* _WIN32 */
	enable_kvm = g_strdup_printf(" ");
#endif

	binary = get_config_value(info_file, QEMU_GROUP, BINARY_KEY);
	emul_add_opt = get_config_value(info_file, ADDITIONAL_OPTION_GROUP, EMULATOR_OPTION_KEY);
	qemu_add_opt = get_config_value(info_file, ADDITIONAL_OPTION_GROUP, QEMU_OPTION_KEY);
	if(emul_add_opt == 0)
		emul_add_opt = g_strdup_printf(" ");
	if(qemu_add_opt == 0)
		qemu_add_opt = g_strdup_printf(" ");
#ifndef _WIN32	
	cmd = g_strdup_printf("./%s --vtm %s --disk %semulimg-%s.x86 %s \
			-- -vga tizen -bios bios.bin -L %s/data/pc-bios -kernel %s/data/kernel-img/bzImage %s %s",
			binary, target_name, get_virtual_target_abs_path(target_name), target_name, emul_add_opt, path, path, enable_kvm, qemu_add_opt );
#else /*_WIN32 */
	cmd = g_strdup_printf("%s --vtm %s --disk %semulimg-%s.x86 %s\
			-- -vga tizen -bios bios.bin -L %s/data/pc-bios -kernel %s/data/kernel-img/bzImage %s %s",
			binary, target_name, get_virtual_target_abs_path(target_name), target_name , emul_add_opt, path, path, enable_kvm, qemu_add_opt );
#endif

	if(!g_spawn_command_line_async(cmd, &error))
	{
		TRACE( "Failed to invoke command: %s\n", error->message);
		show_message("Failed to invoke command", error->message);
		g_error_free(error);
		g_free(cmd);
		exit(1);
	}
	g_free(cmd);

	return;
}

int check_shdmem(char *target_name, int type)
{
#ifndef _WIN32
	int shm_id;
	void *shm_addr;
	u_int port;
	int val;
	struct shmid_ds shm_info;

	for(port=26100;port < 26200; port += 10)
	{
		if ( -1 != ( shm_id = shmget( (key_t)port, 0, 0)))
		{
			if((void *)-1 == (shm_addr = shmat(shm_id, (void *)0, 0)))
			{
				ERR( "%s\n", strerror(errno));
				break;
			}

			val = shmctl(shm_id, IPC_STAT, &shm_info);
			if(val != -1)
			{
				INFO( "count of process that use shared memory : %d\n", shm_info.shm_nattch);
				if(shm_info.shm_nattch > 0 && strcmp(target_name, (char*)shm_addr) == 0)
				{
					if(check_port_bind_listen(port+1) > 0){
						shmdt(shm_addr);
						continue;
					}
					if(type == CREATE_MODE)
						show_message("Warning", "Can not activate this target!\nVirtual target with the same name is running now!");
					else if(type == DELETE_MODE)
						show_message("Warning", "Can not delete this target!\nVirtual target with the same name is running now!");
					else if(type == MODIFY_MODE)
						show_message("Warning", "Can not modify this target!\nVirtual target with the same name is running now!");
					else
						ERR("wrong type passed\n");

					shmdt(shm_addr);
					return -1;
				}
				else
					shmdt(shm_addr);
			}
		}
	}

#endif
	return 0;

}

void entry_changed(GtkEditable *entry, gpointer data)
{
	const gchar *name = gtk_entry_get_text (GTK_ENTRY (entry));
	char* target_name = (char*)data;
	GtkWidget *label4 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "label4");
	gtk_label_set_text(GTK_LABEL(label4),"Input name of the virtual target.");

	GtkWidget *ok_button = (GtkWidget *)gtk_builder_get_object(g_create_builder, "button7");
	char* dst =  malloc(VT_NAME_MAXBUF);

	if(strlen(name) == VT_NAME_MAXBUF)
	{
		WARN( "Virtual target name length can not be longer than 20 characters.\n");
		gtk_label_set_text(GTK_LABEL(label4),"Virtual target name length can not be longer\nthan 20 characters.");
		gtk_widget_set_sensitive(ok_button, FALSE);
		return ;
	}
	escapeStr(name, dst);
	if(strcmp(name, dst) != 0)
	{
		WARN( "Virtual target name is invalid! Valid characters are 0-9 a-z A-Z -_\n");
		gtk_label_set_text(GTK_LABEL(label4),"Virtual target name is invalid!\nValid characters are 0-9 a-z A-Z -_");
		gtk_widget_set_sensitive(ok_button, FALSE);
		free(dst);
		return;
	}

	if(strcmp(name, "") == 0)
	{
		WARN( "Input name of the virtual target.\n");
		gtk_label_set_text(GTK_LABEL(label4),"Input name of the virtual target.");
		gtk_widget_set_sensitive(ok_button, FALSE);
		return;
	}
	else
		snprintf(virtual_target_info.virtual_target_name, MAXBUF, "%s", name);

	if(!target_name) // means create mode
	{
		if(name_collision_check() == 1)
		{
			WARN( "Virtual target with the same name exists! \n Choose another name.\n");
			gtk_label_set_text(GTK_LABEL(label4),"Virtual target with the same name exists!\nChoose another name.");
			gtk_widget_set_sensitive(ok_button, FALSE);
			return;
		}
	}
	else
	{
		if(strcmp(name, target_name)== 0)
		{
			gtk_widget_set_sensitive(ok_button, TRUE);
			return;
		}
		else
		{
			if(name_collision_check() == 1)
			{
				WARN( "Virtual target with the same name exists! \nChoose another name.\n");
				gtk_label_set_text(GTK_LABEL(label4),"Virtual target with the same name exists!\nChoose another name.");
				gtk_widget_set_sensitive(ok_button, FALSE);
				return;
			}
			else
				snprintf(virtual_target_info.virtual_target_name, MAXBUF, "%s", name);
		}
	}

	gtk_widget_set_sensitive(ok_button, TRUE);
}

void show_modify_window(char *target_name)
{
	GtkWidget *sub_window;
	char *virtual_target_path;
	int info_file_status;
	char full_glade_path[MAX_LEN];

	g_create_builder = gtk_builder_new();
	sprintf(full_glade_path, "%s/etc/vtm.glade", get_root_path());

	gtk_builder_add_from_file(g_create_builder, full_glade_path, NULL);

	sub_window = (GtkWidget *)gtk_builder_get_object(g_create_builder, "window2");

	add_window(sub_window, VTM_CREATE_ID);

	gtk_window_set_title(GTK_WINDOW(sub_window), "Modify existing Virtual Target");

	virtual_target_path = get_virtual_target_abs_path(target_name);
	g_info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);
	info_file_status = is_exist_file(g_info_file);
	//if targetlist exist but config file not exists
	if(info_file_status == -1 || info_file_status == FILE_NOT_EXISTS)
	{
		ERR( "target info file not exists : %s\n", target_name);
		return;
	}

	//	fill_modify_target_info();
	/* setup and fill name */
	GtkWidget *name_entry = (GtkWidget *)gtk_builder_get_object(g_create_builder, "entry1");
	gtk_entry_set_text(GTK_ENTRY(name_entry), target_name);
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), VT_NAME_MAXBUF); 

	GtkWidget *label4 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "label4");
	gtk_label_set_text(GTK_LABEL(label4),"Input name of the virtual target.");
	g_signal_connect(G_OBJECT (name_entry), "changed",	G_CALLBACK (entry_changed),	(gpointer*)target_name);

	setup_modify_frame(target_name);
	setup_modify_button(target_name);

	gtk_window_set_icon_from_file(GTK_WINDOW(sub_window), icon_image, NULL);

	g_signal_connect(GTK_OBJECT(sub_window), "delete_event", G_CALLBACK(create_window_deleted_cb), NULL);

	gtk_widget_show_all(sub_window);

	gtk_main();
}


void init_setenv()
{
	char* arch;
	int target_list_status;

	if(!g_getenv("EMULATOR_ARCH"))
		arch = g_strdup_printf("x86");
	else
		return ;

	g_setenv("EMULATOR_ARCH",arch,1);
	INFO( "architecture : %s\n", arch);
	target_list_filepath = get_targetlist_abs_filepath();
	target_list_status = is_exist_file(target_list_filepath);
	if(target_list_status == -1 || target_list_status == FILE_NOT_EXISTS)
	{
		ERR( "load target list file error\n");
		//		exit(1);
	}

	refresh_clicked_cb(arch);
	make_default_image();
}
void arch_select_cb(GtkWidget *widget, gpointer data)
{
	gchar *arch;

	GtkToggleButton *toggled_button = GTK_TOGGLE_BUTTON(data);

	if(gtk_toggle_button_get_active(toggled_button) == TRUE)
	{
		arch = (char*)gtk_button_get_label(GTK_BUTTON(toggled_button));
		g_unsetenv("EMULATOR_ARCH");
		g_setenv("EMULATOR_ARCH",arch,1);
		INFO( "architecture : %s\n", arch);
		refresh_clicked_cb(arch);
		//		g_free(arch);
	}

}

void modify_clicked_cb(GtkWidget *widget, gpointer selection)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter  iter;
	char *target_name;
	char *virtual_target_path;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) 
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),
				&model, &iter)) {
		//get target name
		gtk_tree_model_get(model, &iter, TARGET_NAME, &target_name, -1);
		if(strcmp(target_name, "default") == 0){
			show_message("Warning","You can not modify default target");
			return;
		}

		if(check_shdmem(target_name, MODIFY_MODE)== -1)
			return;

		virtual_target_path = get_virtual_target_abs_path(target_name);
		show_modify_window(target_name);	
		g_free(virtual_target_path);
	}
	else
	{
		show_message("Warning", "Target is not selected. Firstly select a target and modify.");
		return;
	}

}

void activate_clicked_cb(GtkWidget *widget, gpointer selection)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter  iter;
	char *target_name;
	char *virtual_target_path;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) 
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),
				&model, &iter)) {
		//get target name
		gtk_tree_model_get(model, &iter, TARGET_NAME, &target_name, -1);
		virtual_target_path = get_virtual_target_abs_path(target_name);
		activate_target(target_name);	
		g_free(virtual_target_path);
		g_free(target_name);
	}
	else
	{
		show_message("Warning", "Target is not selected. Firstly select a target and activate.");
		return;
	}
}

void details_clicked_cb(GtkWidget *widget, gpointer selection)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter  iter;
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
	char *basedisk_path = NULL;
	char *arch = NULL;
	char *sdcard_detail = NULL;
	char *ram_size_detail = NULL;
	char *sdcard_path_detail = NULL;
	char *details = NULL;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) 
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),
				&model, &iter)) {
		//get target name
		gtk_tree_model_get(model, &iter, TARGET_NAME, &target_name, -1);

		virtual_target_path = get_virtual_target_abs_path(target_name);
		info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);
		info_file_status = is_exist_file(info_file);

		//if targetlist exist but config file not exists
		if(info_file_status == -1 || info_file_status == FILE_NOT_EXISTS)
		{
			ERR( "target info file not exists : %s\n", target_name);
			return;
		}
		//get info from config.ini
		resolution= get_config_value(info_file, HARDWARE_GROUP, RESOLUTION_KEY);
		sdcard_type= get_config_value(info_file, HARDWARE_GROUP, SDCARD_TYPE_KEY);
		sdcard_path= get_config_value(info_file, HARDWARE_GROUP, SDCARD_PATH_KEY);
		ram_size = get_config_value(info_file, HARDWARE_GROUP, RAM_SIZE_KEY);
		dpi = get_config_value(info_file, HARDWARE_GROUP, DPI_KEY);
		disk_path = get_config_value(info_file, HARDWARE_GROUP, DISK_PATH_KEY);
		basedisk_path = get_config_value(info_file, HARDWARE_GROUP, BASEDISK_PATH_KEY);

		arch = getenv("EMULATOR_ARCH");
		if(strcmp(sdcard_type, "0") == 0)
		{
			sdcard_detail = g_strdup_printf("Not supported");
			sdcard_path_detail = g_strdup_printf(" ");
		}
		else
		{
			sdcard_detail = g_strdup_printf("Supported");
			sdcard_path_detail = g_strdup_printf("%s", sdcard_path); 
		}

		ram_size_detail = g_strdup_printf("%sMB", ram_size); 

#ifndef _WIN32		
		/* check image & base image */
		if(access(disk_path, R_OK) != 0){
			details = g_strdup_printf("The image does not exist \n\n"
					"    - [%s]", disk_path);
			show_message("Error", details);
		}
		if(access(basedisk_path, R_OK) != 0){
			details = g_strdup_printf("The base image does not exist \n\n"
					"    - [%s]", basedisk_path);
			show_message("Error", details);
		}

		details = g_strdup_printf(""
				" - Name: %s\n"
				" - CPU: %s\n"
				" - Resolution: %s\n"
				" - Ram Size: %s\n"
				" - DPI: %s\n"
				" - SD Card: %s\n"
				" - SD Path: %s\n"
				" - Image Path: %s\n"
				" - Base Image Path: %s \n"
				, target_name, arch, resolution, ram_size_detail
				, dpi, sdcard_detail, sdcard_path_detail, disk_path, basedisk_path);

		show_message("Virtual Target Details", details);

#else /* _WIN32 */
		gchar *details_win = NULL;

		details = g_strdup_printf(""
				" - Name: %s\n"
				" - CPU: %s\n"
				" - Resolution: %s\n"
				" - Ram Size: %s\n"
				" - DPI: %s\n"
				" - SD Card: %s\n"
				" - SD Path: %s\n"
				" - Image Path: %s\n"
				" - Base Image Path: %s \n"
				, target_name, arch, resolution, ram_size_detail
				, dpi, sdcard_detail, sdcard_path_detail, disk_path, basedisk_path);

		details_win = change_path_from_slash(details);

		show_message("Virtual Target Details", details_win);

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
		g_free(sdcard_path_detail);
		g_free(details);
		return;
	}

	show_message("Warning", "Target is not selected. Firstly select a target and press the button.");
}


void delete_clicked_cb(GtkWidget *widget, gpointer selection)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter  iter;
	char *target_name;
	char *cmd = NULL;
	char *virtual_target_path;
	int target_list_status;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) 
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),
				&model, &iter)) {
		//get target name
		gtk_tree_model_get(model, &iter, TARGET_NAME, &target_name, -1);
		if(strcmp(target_name,"default") == 0)
		{
			show_message("Warning","You can not delete default target");
			return;
		}

		if(check_shdmem(target_name, DELETE_MODE)== -1)
			return;

		gboolean bResult = show_ok_cancel_message("Warning", "Are you sure you delete this target?");
		if(bResult == FALSE)
			return;

		virtual_target_path = get_virtual_target_abs_path(target_name);

#ifdef _WIN32
		char *virtual_target_win_path = change_path_from_slash(virtual_target_path);
		cmd = g_strdup_printf("rmdir /Q /S %s", virtual_target_win_path);	
		if (system(cmd)	== -1)
		{
			g_free(cmd);
			g_free(virtual_target_path);
			TRACE( "Failed to delete target name: %s", target_name);
			show_message("Failed to delete target name: %s", target_name);
			return;
		}
#else
		cmd = g_strdup_printf("rm -rf %s", virtual_target_path);
		if(!run_cmd(cmd))
		{
			g_free(cmd);
			g_free(virtual_target_path);
			TRACE( "Failed to delete target name: %s", target_name);
			show_message("Failed to delete target name: %s", target_name);
			return;
		}
#endif
		target_list_filepath = get_targetlist_abs_filepath();
		target_list_status = is_exist_file(target_list_filepath);

		del_config_key(target_list_filepath, TARGET_LIST_GROUP, target_name);
		g_free(cmd);
		g_free(virtual_target_path);
#ifdef _WIN32
		g_free(virtual_target_win_path);
#endif
		gtk_list_store_remove(store, &iter);
		g_free(target_name);
		show_message("INFO","Virtual target deletion success!");
		return;
	}

	show_message("Warning", "Target is not selected. Firstly select a target and delete.");
}

void refresh_clicked_cb(char *arch)
{
	GtkListStore *store;
	GtkTreeIter iter;
	int i;
	int num = 0;
	gchar **target_list = NULL;
	char *virtual_target_path;
	char *info_file;
	char *resolution = NULL;
	char *buf;
	gchar *ram_size = NULL;
	int info_file_status;
	gchar *local_target_list_filepath;
	GtkTreePath *first_col_path = NULL;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
	local_target_list_filepath = get_targetlist_abs_filepath();
	target_list = get_virtual_target_list(local_target_list_filepath, TARGET_LIST_GROUP, &num);

	gtk_list_store_clear(store);

	for(i = 0; i < num; i++)
	{
		gtk_list_store_append(store, &iter);

		virtual_target_path = get_virtual_target_abs_path(target_list[i]);
		info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);
		info_file_status = is_exist_file(info_file);

		//if targetlist exist but config file not exists
		if(info_file_status == -1 || info_file_status == FILE_NOT_EXISTS)
		{
			ERR( "target info file not exists : %s\n", target_list[i]);
			continue;
		}

		buf = get_config_value(info_file, HARDWARE_GROUP, RAM_SIZE_KEY);
		ram_size = g_strdup_printf("%sMB", buf); 
		resolution = get_config_value(info_file, HARDWARE_GROUP, RESOLUTION_KEY);
		gtk_list_store_set(store, &iter, TARGET_NAME, target_list[i], RESOLUTION, resolution, RAM_SIZE, ram_size, -1);

		g_free(buf);
		g_free(ram_size);
		g_free(resolution);
		g_free(virtual_target_path);
		g_free(info_file);
	}
	first_col_path = gtk_tree_path_new_from_indices(0, -1);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(list), first_col_path, NULL, 0);

	g_free(local_target_list_filepath);
	g_strfreev(target_list);

}

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

void make_default_image(void)
{
	char *cmd = NULL;
	int info_file_status;
	char *virtual_target_path = get_virtual_target_abs_path("default");
	char *info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);
	char *default_img = g_strdup_printf("%semulimg-default.x86", virtual_target_path);
	info_file_status = is_exist_file(default_img);
	if(info_file_status == -1 || info_file_status == FILE_NOT_EXISTS)
	{
		INFO( "emulimg-default.x86 not exists. is making now.\n");
		// create emulator image
#ifdef _WIN32
		cmd = g_strdup_printf("%s/qemu-img.exe create -b %s/emulimg.x86 -f qcow2 %s",
				get_bin_path(), get_arch_abs_path(), default_img);
#else
		cmd = g_strdup_printf("qemu-img create -b %s/emulimg.x86 -f qcow2 %s",
				get_arch_abs_path(), default_img);
#endif
		if(!run_cmd(cmd))
		{
			g_free(cmd);
			ERR("Error", "emulator image creation failed!");
			return;
		}

		INFO( "emulimg-default.x86 creation succeeded!\n");
		g_free(cmd);

	}
	set_config_value(info_file, HARDWARE_GROUP, DISK_PATH_KEY, default_img);
	set_config_value(info_file, HARDWARE_GROUP, BASEDISK_PATH_KEY, get_baseimg_abs_path());

	free(default_img);
}	

gboolean run_cmd(char *cmd)
{
	char *s_out = NULL;
	char *s_err = NULL;
	int exit_status;
	GError *err = NULL;

	g_return_val_if_fail(cmd != NULL, FALSE);

	TRACE( "Command: %s\n", cmd);
	if (!g_spawn_command_line_sync(cmd, &s_out, &s_err, &exit_status, &err)) {
		TRACE( "Failed to invoke command: %s\n", err->message);
		show_message("Failed to invoke command", err->message);
		g_error_free(err);
		g_free(s_out);
		g_free(s_err);
		return FALSE;
	}
	if (exit_status != 0) {
		TRACE( "Command returns error: %s\n", s_out);
		//		show_message("Command returns error", s_out);
		g_free(s_out);
		g_free(s_err);
		return FALSE;
	}

	TRACE( "Command success: %s\n", cmd);
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
		g_fprintf (fp, "[%s]\n", COMMON_GROUP);
		g_fprintf (fp, "%s=0\n",ALWAYS_ON_TOP_KEY);

		g_fprintf (fp, "\n[%s]\n", EMULATOR_GROUP);
		g_fprintf (fp, "%s=\n", ENABLE_SHELL_KEY);
		g_fprintf (fp, "%s=100\n", MAIN_X_KEY);
		g_fprintf (fp, "%s=100\n", MAIN_Y_KEY);

		g_fprintf (fp, "\n[%s]\n", QEMU_GROUP);
		g_fprintf (fp, "%s=\n", BINARY_KEY);
		g_fprintf (fp, "%s=1\n", HTTP_PROXY_KEY);
		g_fprintf (fp, "%s=1\n", DNS_SERVER_KEY);
		g_fprintf (fp, "%s=1200\n", TELNET_PORT_KEY);
		//		g_fprintf (fp, "%s=\n", SNAPSHOT_SAVED_KEY);
		//		g_fprintf (fp, "%s=\n", SNAPSHOT_SAVED_DATE_KEY);
		g_fprintf (fp, "%s=1\n", KVM_KEY);

		g_fprintf (fp, "\n[%s]\n", ADDITIONAL_OPTION_GROUP);
		g_fprintf (fp, "%s=\n", EMULATOR_OPTION_KEY);
		g_fprintf (fp, "%s=%s\n", QEMU_OPTION_KEY,"-usb -usbdevice wacom-tablet -usbdevice keyboard -net nic,model=virtio -soundhw all -rtc base=utc -net user");
		g_fprintf (fp, "[%s]\n", HARDWARE_GROUP);
		g_fprintf (fp, "%s=\n", RESOLUTION_KEY);
		g_fprintf (fp, "%s=1\n", BUTTON_TYPE_KEY);
		g_fprintf (fp, "%s=\n", SDCARD_TYPE_KEY);
		g_fprintf (fp, "%s=\n", SDCARD_PATH_KEY);
		g_fprintf (fp, "%s=\n", RAM_SIZE_KEY);
		g_fprintf (fp, "%s=\n", DPI_KEY);
		g_fprintf (fp, "%s=0\n", DISK_TYPE_KEY);
		g_fprintf (fp, "%s=\n", BASEDISK_PATH_KEY);
		g_fprintf (fp, "%s=\n", DISK_PATH_KEY);

		fclose(fp);
	}
	else {
		ERR( "Can't open file path. (%s)\n", filepath);
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
	set_config_type(filepath, EMULATOR_GROUP, ENABLE_SHELL_KEY, 0);
	/*  QEMU option (09.05.26)*/

	char *arch = (char*)g_getenv("EMULATOR_ARCH");
	if(strcmp(arch, "x86") == 0)
		set_config_value(filepath, QEMU_GROUP, BINARY_KEY, "emulator-x86");
	else if(strcmp(arch, "arm") == 0)
		set_config_value(filepath, QEMU_GROUP, BINARY_KEY, "emulator-arm");
	else
	{
		show_message("Error", "architecture setting error\n");
		ERR( "architecture setting error");
		return -1;
	}
	//	set_config_type(filepath, QEMU_GROUP, TELNET_CONSOLE_COMMAND_TYPE_KEY, 0);
	//	set_config_value(filepath, QEMU_GROUP, TELNET_CONSOLE_COMMAND_KEY, "/usr/bin/putty -telnet -P 1200 localhost");
	//	set_config_type(filepath, QEMU_GROUP, KVM_KEY, 1);
	//	set_config_type(filepath, QEMU_GROUP, SDCARD_TYPE_KEY, pconfiguration->qemu_configuration.sdcard_type);
	//	set_config_value(filepath, QEMU_GROUP, SDCARD_PATH_KEY, pconfiguration->qemu_configuration.sdcard_path);
	//	set_config_type(filepath, QEMU_GROUP, SAVEVM_KEY, pconfiguration->qemu_configuration.save_emulator_state);
	//	set_config_type(filepath, QEMU_GROUP, SNAPSHOT_SAVED_KEY, pconfiguration->qemu_configuration.snapshot_saved);
	//	set_config_value(filepath, QEMU_GROUP, SNAPSHOT_SAVED_DATE_KEY, pconfiguration->qemu_configuration.snapshot_saved_date);


	set_config_value(filepath, HARDWARE_GROUP, RESOLUTION_KEY, virtual_target_info.resolution);
	set_config_type(filepath, HARDWARE_GROUP, SDCARD_TYPE_KEY, virtual_target_info.sdcard_type);
	set_config_value(filepath, HARDWARE_GROUP, SDCARD_PATH_KEY, virtual_target_info.sdcard_path);
	set_config_type(filepath, HARDWARE_GROUP, RAM_SIZE_KEY, virtual_target_info.ram_size);
	set_config_type(filepath, HARDWARE_GROUP, DISK_TYPE_KEY, virtual_target_info.disk_type);
	set_config_value(filepath, HARDWARE_GROUP, DPI_KEY, virtual_target_info.dpi);
	//  set_config_type(filepath, HARDWARE_GROUP, BUTTON_TYPE_KEY, virtual_target_info.button_type);
	set_config_value(filepath, HARDWARE_GROUP, DISK_PATH_KEY, virtual_target_info.diskimg_path);
	set_config_value(filepath, HARDWARE_GROUP, BASEDISK_PATH_KEY, virtual_target_info.basedisk_path);

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
	INFO( "virtual target manager exit \n");
	window_hash_destroy();
	gtk_main_quit();
}

GtkWidget *setup_list(void)
{
	GtkWidget *sc_win;
	GtkListStore *store;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;

	sc_win = gtk_scrolled_window_new(NULL, NULL);
	store = gtk_list_store_new(N_COL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	cell = gtk_cell_renderer_text_new();

	//set text alignment 
	column = gtk_tree_view_column_new_with_attributes("Target name", cell, "text", TARGET_NAME, NULL);
	gtk_tree_view_column_set_alignment(column,0.0);
	gtk_tree_view_column_set_min_width(column,130);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	column = gtk_tree_view_column_new_with_attributes("Ram size", cell, "text", RAM_SIZE, NULL);
	gtk_tree_view_column_set_alignment(column,0.0);
	gtk_tree_view_column_set_min_width(column,100);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	column = gtk_tree_view_column_new_with_attributes("Resolution", cell, "text", RESOLUTION, NULL);
	gtk_tree_view_column_set_alignment(column,0.0);
	gtk_tree_view_column_set_max_width(column,60);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sc_win), list);
	g_object_unref( G_OBJECT(store));

	return sc_win;
}

void create_window_deleted_cb(void)
{
	GtkWidget *win = NULL;
	INFO( "create window exit \n");

	win = get_window(VTM_CREATE_ID);

	gtk_widget_destroy(win);

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
		INFO( "resolution : %s\n", gtk_button_get_label(GTK_BUTTON(toggled_button)));
		g_free(resolution);
	}
}

void buttontype_select_cb(void)
{
	gboolean active = FALSE;

	GtkWidget *create_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton10");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_radiobutton));
	if(active == TRUE)
		virtual_target_info.button_type = 1;
	else
		virtual_target_info.button_type = 3;

	INFO( "button_type : %d\n", virtual_target_info.button_type);
}


void sdcard_size_select_cb(void)
{
	char *size;

	GtkComboBox *sdcard_combo_box = (GtkComboBox *)get_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX);	

	size = gtk_combo_box_get_active_text(sdcard_combo_box);
	sdcard_create_size = atoi(size);
	INFO( "sdcard create size : %d\n", atoi(size));

	g_free(size);
}

void set_sdcard_create_active_cb(void)
{
	gboolean active = FALSE;

	GtkWidget *create_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton4");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_radiobutton));

	if(active == TRUE)
		virtual_target_info.sdcard_type = 1;

	GtkWidget *sdcard_combo_box = (GtkWidget *)get_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX);

	gtk_widget_set_sensitive(sdcard_combo_box, active);
}

void set_disk_select_active_cb(void)
{
	gboolean active = FALSE;

	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton13");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(select_radiobutton));

	if(active == TRUE)
		virtual_target_info.disk_type = 1;

	GtkWidget *sdcard_filechooser2 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton2");

	gtk_widget_set_sensitive(sdcard_filechooser2, active);

}

void set_sdcard_select_active_cb(void)
{
	gboolean active = FALSE;

	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton5");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(select_radiobutton));

	if(active == TRUE)
		virtual_target_info.sdcard_type = 2;

	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton1");

	gtk_widget_set_sensitive(sdcard_filechooser, active);
}

void set_disk_default_active_cb(void)
{
	gboolean active = FALSE;

	GtkWidget *default_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton12");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(default_radiobutton));

	if(active == TRUE)
	{
		virtual_target_info.disk_type = 0;
		snprintf(virtual_target_info.basedisk_path, MAXBUF, "%s", get_baseimg_abs_path());
		INFO( "default disk path : %s\n", virtual_target_info.basedisk_path);
	}
}

void set_sdcard_none_active_cb(void)
{
	gboolean active = FALSE;

	GtkWidget *none_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton6");
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(none_radiobutton));

	if(active == TRUE)
		virtual_target_info.sdcard_type = 0;
}

void disk_file_select_cb(void)
{
	gchar *path = NULL;

	GtkWidget *sdcard_filechooser2 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton2");

	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sdcard_filechooser2));
	snprintf(virtual_target_info.basedisk_path, MAXBUF, "%s", path);
	INFO( "disk path : %s\n", path);

	g_free(path);

}

void sdcard_file_select_cb(void)
{
	gchar *path = NULL;

	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton1");

	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sdcard_filechooser));
	snprintf(virtual_target_info.sdcard_path, MAXBUF, "%s", path);
	INFO( "sdcard path : %s\n", path);

	g_free(path);
}

void ram_select_cb(void)
{
	char *size;

	GtkComboBox *ram_combobox = (GtkComboBox *)get_widget(VTM_CREATE_ID, VTM_CREATE_RAM_COMBOBOX);

	size = gtk_combo_box_get_active_text(ram_combobox);
	virtual_target_info.ram_size = atoi(size);
	INFO( "ram size : %d\n", atoi(size));

	g_free(size);
}

void modify_ok_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *win = get_window(VTM_CREATE_ID);
	char *target_name = (char*)data;
	char *cmd = NULL;
	char *cmd2 = NULL;
	char *dest_path = NULL;
	char *conf_file = NULL;
	const gchar *name = NULL;
	char *sdcard_name = NULL;
	char *dst;

	GtkWidget *name_entry = (GtkWidget *)gtk_builder_get_object(g_create_builder, "entry1");
	name = gtk_entry_get_text(GTK_ENTRY(name_entry));
	char *virtual_target_path = get_virtual_target_abs_path((gchar*)name);
	char *info_file = g_strdup_printf("%sconfig.ini", virtual_target_path);

	ram_select_cb();
	buttontype_select_cb();
	set_config_type(info_file, HARDWARE_GROUP, RAM_SIZE_KEY, virtual_target_info.ram_size);

	//	name character validation check
	dst =  malloc(VT_NAME_MAXBUF);
	escapeStr(name, dst);
	if(strcmp(name, dst) != 0)
	{
		WARN( "virtual target name only allowed numbers, a-z, A-Z, -_");
		show_message("Warning", "Virtual target name is not correct! \n (only allowed numbers, a-z, A-Z, -_)");
		free(dst);
		return;
	}
	free(dst);

	// no name validation check
	if(strcmp(name, "") == 0)
	{
		WARN( "Specify name of the virtual target!");
		show_message("Warning", "Specify name of the virtual target!");
		return;
	}
	else
	{
		snprintf(virtual_target_info.virtual_target_name, MAXBUF, "%s", name);
	}

	dest_path = get_virtual_target_abs_path(virtual_target_info.virtual_target_name);
	INFO( "virtual_target_path: %s\n", dest_path);

	// if try to change the target name
	if(strcmp(name, target_name) != 0)
	{
		char *vms_path = (char*)get_vms_abs_path();
		if(name_collision_check() == 1)
		{
			WARN( "Virtual target with the same name exists! Choose another name.");
			show_message("Warning", "Virtual target with the same name exists! Choose another name.");
			return;
		}	
#ifndef _WIN32
		cmd = g_strdup_printf("mv %s/%s %s/%s", 
				vms_path, target_name, vms_path, name);
		cmd2 = g_strdup_printf("mv %s/%s/emulimg-%s.x86 %s/%s/emulimg-%s.x86", 
				vms_path, name, target_name, vms_path, name, name);

		if(!run_cmd(cmd))
		{
			g_free(cmd);
			g_free(dest_path);
			show_message("Error", "Fail to change the target name!");
			return;
		}
		g_free(cmd);

		if(!run_cmd(cmd2))
		{
			g_free(cmd);
			g_free(cmd2);
			g_free(dest_path);
			show_message("Error", "Fail to change the target name!");
			return;
		}
		g_free(cmd2);

#else /* WIN32 */
		char *src_path = g_strdup_printf("%s/%s", vms_path, target_name);
		char *dst_path = g_strdup_printf("%s/%s", vms_path, name);
		char *src_img_path = g_strdup_printf("%s/%s/emulimg-%s.x86", vms_path, name, target_name);
		char *dst_img_path = g_strdup_printf("%s/%s/emulimg-%s.x86", vms_path, name, name);

		gchar *src_path_for_win = change_path_from_slash(src_path);
		gchar *dst_path_for_win = change_path_from_slash(dst_path);
		gchar *src_img_path_for_win = change_path_from_slash(src_img_path);
		gchar *dst_img_path_for_win = change_path_from_slash(dst_img_path);

		g_free(src_path);
		g_free(dst_path);
		g_free(src_img_path);
		g_free(dst_img_path);

		if (g_rename(src_path_for_win, dst_path_for_win) != 0)
		{
			g_free(src_path_for_win);
			g_free(dst_path_for_win);
			show_message("Error", "Fail to change the target name!");
			return ;
		}
		g_free(src_path_for_win);
		g_free(dst_path_for_win);

		if (g_rename(src_img_path_for_win, dst_img_path_for_win) != 0)
		{
			g_free(src_img_path_for_win);
			g_free(dst_img_path_for_win);
			show_message("Error", "Fail to change the target name!");
			return ;
		}
		g_free(src_img_path_for_win);
		g_free(dst_img_path_for_win);
		g_free(vms_path);
#endif
	}
	memset(virtual_target_info.diskimg_path, 0x00, MAXBUF);

	snprintf(virtual_target_info.diskimg_path, MAXBUF, 
			"%s/%s/emulimg-%s.x86", get_vms_abs_path(), name, name);
	TRACE( "virtual_target_info.diskimg_path: %s\n",virtual_target_info.diskimg_path);

	if(virtual_target_info.sdcard_type == 2){
		GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton1");
		char *sdcard_uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(sdcard_filechooser));
		if(sdcard_uri == NULL){
			show_message("Error", "You didn't select an existing sdcard image");
			return;
		}
		sdcard_name = g_path_get_basename(virtual_target_info.sdcard_path);
		memset(virtual_target_info.sdcard_path, 0x00, MAXBUF);
		snprintf(virtual_target_info.sdcard_path, MAXBUF, "%s%s", dest_path, sdcard_name);
		TRACE( "[sdcard_type:2]virtual_target_info.sdcard_path: %s\n", virtual_target_info.sdcard_path);
	}
	else
	{
		// target_name not changed
		dest_path = get_virtual_target_abs_path(virtual_target_info.virtual_target_name);
		snprintf(virtual_target_info.diskimg_path, MAXBUF, "%semulimg-%s.x86", dest_path, 
				virtual_target_info.virtual_target_name);
		TRACE( "virtual_target_info.diskimg_path: %s\n",virtual_target_info.diskimg_path);
	}

	//delete original target name
	target_list_filepath = get_targetlist_abs_filepath();
	del_config_key(target_list_filepath, TARGET_LIST_GROUP, target_name);
	g_free(target_name);

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
		TRACE( "[sdcard_type:0]virtual_target_info.sdcard_path: %s\n", virtual_target_info.sdcard_path);
	}
	else if(virtual_target_info.sdcard_type == 1)
	{
		// sdcard create
		sdcard_size_select_cb();
#ifndef _WIN32
		cmd = g_strdup_printf("cp %s/sdcard_%d.img %s", get_data_abs_path(), sdcard_create_size, dest_path);

		if(!run_cmd(cmd))
		{
			g_free(cmd);
			g_free(dest_path);
			show_message("Error", "SD Card img create failed!");
			return;
		}
		g_free(cmd);
#else
		char *src_sdcard_path = g_strdup_printf("%s/sdcard_%d.img", get_data_abs_path(), sdcard_create_size);
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
		TRACE( "[sdcard_type:1]virtual_target_info.sdcard_path: %s\n", virtual_target_info.sdcard_path);
	}
	else if(virtual_target_info.sdcard_type == 2){
		if(strcmp(virtual_target_info.sdcard_path, "") == 0){
			show_message("Error", "You didn't select an existing sdcard image");
			return;
		}
		TRACE( "[sdcard_type:2]virtual_target_info.sdcard_path: %s\n", virtual_target_info.sdcard_path);
	}

	// add virtual target name to targetlist.ini
	set_config_value(target_list_filepath, TARGET_LIST_GROUP, virtual_target_info.virtual_target_name, "");
	// write config.ini
	conf_file = g_strdup_printf("%sconfig.ini", dest_path);
	//	create_config_file(conf_file);
	snprintf(virtual_target_info.dpi, MAXBUF, "2070");
	if(write_config_file(conf_file) == -1)
	{
		show_message("Error", "Virtual target modification failed!");
		return ;
	}

	show_message("INFO", "Virtual target modification success!");

	g_free(dest_path);
	g_free(conf_file);

	gtk_widget_destroy(win);
	char *arch = (char*)g_getenv("EMULATOR_ARCH");
	refresh_clicked_cb(arch);

	g_object_unref(G_OBJECT(g_create_builder));

	gtk_main_quit();
	return;

}

void ok_clicked_cb(void)
{
	char *cmd = NULL;
	char *dest_path = NULL;
	char *log_path = NULL;
	char *conf_file = NULL;
	GtkWidget *win = get_window(VTM_CREATE_ID);

	dest_path = get_virtual_target_abs_path(virtual_target_info.virtual_target_name);
	if(access(dest_path, R_OK) != 0)
#ifndef _WIN32
		mkdir(dest_path, S_IRWXU | S_IRWXG);
#else
	mkdir(dest_path);
#endif
	log_path = get_virtual_target_log_path(virtual_target_info.virtual_target_name);
	if(access(log_path, R_OK) != 0)
#ifndef _WIN32
		mkdir(log_path, S_IRWXU | S_IRWXG);
#else
	mkdir(log_path);
#endif
	//disk type
	if(virtual_target_info.disk_type == 0)
		snprintf(virtual_target_info.basedisk_path, MAXBUF, "%s", get_baseimg_abs_path());
	else if(virtual_target_info.disk_type == 1){
		if(strcmp(virtual_target_info.basedisk_path, "") == 0){
			show_message("Error", "You didn't select an existing sdcard image");
			return;
		}
	}
	// sdcard
	if(virtual_target_info.sdcard_type == 0)
	{
		memset(virtual_target_info.sdcard_path, 0x00, MAXBUF);
	}
	else if(virtual_target_info.sdcard_type == 1)
	{
		// sdcard create
#ifndef _WIN32
		cmd = g_strdup_printf("cp %s/sdcard_%d.img %s", get_data_abs_path(), sdcard_create_size, dest_path);

		if(!run_cmd(cmd))
		{
			g_free(cmd);
			g_free(dest_path);
			show_message("Error", "SD Card img create failed!");
			return;
		}
		g_free(cmd);
#else
		char *src_sdcard_path = g_strdup_printf("%s/sdcard_%d.img", get_data_abs_path(), sdcard_create_size);
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
	else if(virtual_target_info.sdcard_type == 2){
		if(strcmp(virtual_target_info.sdcard_path, "") == 0){
			show_message("Error", "You didn't select an existing sdcard image");
			return;
		}
	}

	// create emulator image
#ifdef _WIN32
	if(virtual_target_info.disk_type == 1){
		cmd = g_strdup_printf("%s/bin/qemu-img.exe create -b %s -f qcow2 %semulimg-%s.x86", get_root_path(), virtual_target_info.basedisk_path,
				dest_path, virtual_target_info.virtual_target_name);
	}
	else
	{
		cmd = g_strdup_printf("%s/bin/qemu-img.exe create -b %s/emulimg.x86 -f qcow2 %semulimg-%s.x86", get_root_path(), get_arch_abs_path(),
				dest_path, virtual_target_info.virtual_target_name);
	}
#else
	if(virtual_target_info.disk_type == 1){
		cmd = g_strdup_printf("qemu-img create -b %s -f qcow2 %semulimg-%s.x86", virtual_target_info.basedisk_path,
				dest_path, virtual_target_info.virtual_target_name);
	}
	else
	{
		cmd = g_strdup_printf("qemu-img create -b %s -f qcow2 %semulimg-%s.x86", virtual_target_info.basedisk_path,
				dest_path, virtual_target_info.virtual_target_name);
	}
#endif
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
	snprintf(virtual_target_info.dpi, MAXBUF, "2070");
	if(write_config_file(conf_file) == -1)
	{
		show_message("Error", "Virtual target creation failed!");
		return ;
	}
	show_message("INFO", "Virtual target creation success!");

	g_free(conf_file);
	g_free(dest_path);

	gtk_widget_destroy(win);
	char *arch = (char*)g_getenv("EMULATOR_ARCH");
	refresh_clicked_cb(arch);

	g_object_unref(G_OBJECT(g_create_builder));

	gtk_main_quit();
	return;
}

void setup_create_frame(void)
{
	setup_buttontype_frame();
	setup_resolution_frame();
	setup_sdcard_frame();
	setup_disk_frame();
	setup_ram_frame();
}

void setup_modify_frame(char *target_name)
{
	setup_modify_buttontype_frame(target_name);
	setup_modify_resolution_frame(target_name);
	setup_modify_sdcard_frame(target_name);
	setup_modify_disk_frame(target_name);
	setup_modify_ram_frame(target_name);
}

void setup_modify_resolution_frame(char *target_name)
{
	char *resolution;
	GtkWidget *radiobutton1 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton1");
	GtkWidget *radiobutton2 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton2");
	GtkWidget *radiobutton3 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton3");
	GtkWidget *radiobutton4 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton7");
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton1), "toggled", G_CALLBACK(resolution_select_cb), radiobutton1);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton2), "toggled", G_CALLBACK(resolution_select_cb), radiobutton2);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton3), "toggled", G_CALLBACK(resolution_select_cb), radiobutton3);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton4), "toggled", G_CALLBACK(resolution_select_cb), radiobutton4);

	resolution= get_config_value(g_info_file, HARDWARE_GROUP, RESOLUTION_KEY);

	if(strcmp(resolution, 
				remove_space(gtk_button_get_label(GTK_BUTTON(radiobutton1)))) == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton1), TRUE);
	else if(strcmp(resolution, 
				remove_space(gtk_button_get_label(GTK_BUTTON(radiobutton2)))) == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton2), TRUE);
	else if(strcmp(resolution, 
				remove_space(gtk_button_get_label(GTK_BUTTON(radiobutton3)))) == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton3), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton4), TRUE);

	snprintf(virtual_target_info.resolution, MAXBUF, "%s", resolution);
	INFO( "resolution : %s\n", resolution);
	g_free(resolution);
}

void setup_modify_disk_frame(char *target_name)
{
	char *disk_path;
	// radio button setup
	GtkWidget *default_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton12");
	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton13");
	// file chooser setup
	GtkWidget *sdcard_filechooser2 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton2");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Disk Files");
	gtk_file_filter_add_pattern(filter, "*.x86");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(sdcard_filechooser2), filter);
	int disk_type= get_config_type(g_info_file, HARDWARE_GROUP, DISK_TYPE_KEY);
	if(disk_type == 1)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_radiobutton), TRUE);
		disk_path= get_config_value(g_info_file, HARDWARE_GROUP, BASEDISK_PATH_KEY);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(sdcard_filechooser2), disk_path);
	}
	else if(disk_type == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(default_radiobutton), TRUE);
	//can not modify baseimg. only can create.	
	gtk_widget_set_sensitive(default_radiobutton, FALSE);
	gtk_widget_set_sensitive(select_radiobutton, FALSE);
	gtk_widget_set_sensitive(sdcard_filechooser2, FALSE);

	//	g_signal_connect(G_OBJECT(select_radiobutton), "toggled", G_CALLBACK(set_disk_select_active_cb), NULL);
	//	g_signal_connect(G_OBJECT(default_radiobutton), "toggled", G_CALLBACK(set_disk_default_active_cb), NULL);
	//	g_signal_connect(G_OBJECT(sdcard_filechooser2), "selection-changed", G_CALLBACK(disk_file_select_cb), NULL);


}

void setup_modify_sdcard_frame(char *target_name)
{
	char *sdcard_type;
	char* sdcard_path;

	GtkWidget *hbox4 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "hbox4");

	GtkComboBox *sdcard_combo_box = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_box_pack_start(GTK_BOX(hbox4), GTK_WIDGET(sdcard_combo_box), FALSE, FALSE, 1);
	add_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX, GTK_WIDGET(sdcard_combo_box));

	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_256); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_512); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_1024); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_1536); 

	gtk_combo_box_set_active(sdcard_combo_box, SDCARD_DEFAULT_SIZE);

	// radio button setup
	GtkWidget *create_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton4");
	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton5");
	GtkWidget *none_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton6");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(none_radiobutton), TRUE);

	// file chooser setup
	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton1");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "SD Card Image Files");
	gtk_file_filter_add_pattern(filter, "*.img");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(sdcard_filechooser), filter);

	sdcard_type= get_config_value(g_info_file, HARDWARE_GROUP, SDCARD_TYPE_KEY);
	if(strcmp(sdcard_type, "0") == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(none_radiobutton), TRUE);
	else{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_radiobutton), TRUE);
		gtk_widget_set_sensitive(sdcard_filechooser, TRUE);
		sdcard_path= get_config_value(g_info_file, HARDWARE_GROUP, SDCARD_PATH_KEY);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(sdcard_filechooser), sdcard_path);
	}

	set_sdcard_create_active_cb();	
	set_sdcard_select_active_cb();	

	g_signal_connect(G_OBJECT(sdcard_combo_box), "changed", G_CALLBACK(sdcard_size_select_cb), NULL);
	g_signal_connect(G_OBJECT(create_radiobutton), "toggled", G_CALLBACK(set_sdcard_create_active_cb), NULL);
	g_signal_connect(G_OBJECT(select_radiobutton), "toggled", G_CALLBACK(set_sdcard_select_active_cb), NULL);
	g_signal_connect(G_OBJECT(none_radiobutton), "toggled", G_CALLBACK(set_sdcard_none_active_cb), NULL);
	g_signal_connect(G_OBJECT(sdcard_filechooser), "selection-changed", G_CALLBACK(sdcard_file_select_cb), NULL);


}

void setup_modify_ram_frame(char *target_name)
{
	char *ram_size;

	ram_size = get_config_value(g_info_file, HARDWARE_GROUP, RAM_SIZE_KEY);

	GtkWidget *hbox6 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "hbox6");
	GtkComboBox *ram_combo_box = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_box_pack_start(GTK_BOX(hbox6), GTK_WIDGET(ram_combo_box), FALSE, FALSE, 1);
	add_widget(VTM_CREATE_ID, VTM_CREATE_RAM_COMBOBOX, GTK_WIDGET(ram_combo_box));

	gtk_combo_box_append_text(ram_combo_box, RAM_SIZE_512); 
	gtk_combo_box_append_text(ram_combo_box, RAM_SIZE_768); 
	gtk_combo_box_append_text(ram_combo_box, RAM_SIZE_1024); 

	if(strcmp(ram_size, RAM_SIZE_512) == 0)
		gtk_combo_box_set_active(ram_combo_box, RAM_DEFAULT_SIZE);
	else if(strcmp(ram_size, RAM_SIZE_768) == 0)
		gtk_combo_box_set_active(ram_combo_box, RAM_768_SIZE);
	else
		gtk_combo_box_set_active(ram_combo_box, RAM_1024_SIZE);

	g_signal_connect(G_OBJECT(ram_combo_box), "changed", G_CALLBACK(ram_select_cb), NULL);

}


void setup_create_button(void)
{
	GtkWidget *ok_button = (GtkWidget *)gtk_builder_get_object(g_create_builder, "button7");
	GtkWidget *cancel_button = (GtkWidget *)gtk_builder_get_object(g_create_builder, "button6");

	gtk_widget_set_sensitive(ok_button, FALSE);
	g_signal_connect(ok_button, "clicked", G_CALLBACK(ok_clicked_cb), NULL);
	g_signal_connect(cancel_button, "clicked", G_CALLBACK(create_window_deleted_cb), NULL);
}

void setup_modify_button(char* target_name)
{
	GtkWidget *ok_button = (GtkWidget *)gtk_builder_get_object(g_create_builder, "button7");
	GtkWidget *cancel_button = (GtkWidget *)gtk_builder_get_object(g_create_builder, "button6");

	g_signal_connect(ok_button, "clicked", G_CALLBACK(modify_ok_clicked_cb), (gpointer*)target_name);
	g_signal_connect(cancel_button, "clicked", G_CALLBACK(create_window_deleted_cb), NULL);
}

void setup_buttontype_frame(void)
{
	GtkWidget *radiobutton10 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton10");
	GtkWidget *radiobutton11 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton11");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton10), TRUE);

	g_signal_connect(GTK_RADIO_BUTTON(radiobutton10), "toggled", G_CALLBACK(buttontype_select_cb), NULL);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton11), "toggled", G_CALLBACK(buttontype_select_cb), NULL);

}

void setup_modify_buttontype_frame(char *target_name)
{
	int button_type;
	GtkWidget *radiobutton10 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton10");
	GtkWidget *radiobutton11 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton11");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton10), TRUE);

	g_signal_connect(GTK_RADIO_BUTTON(radiobutton10), "toggled", G_CALLBACK(buttontype_select_cb), NULL);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton11), "toggled", G_CALLBACK(buttontype_select_cb), NULL);

	button_type = get_config_type(g_info_file, HARDWARE_GROUP, BUTTON_TYPE_KEY);

	if(button_type == 1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton10), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton11), TRUE);

	virtual_target_info.button_type = button_type;
	INFO( "button_type : %d\n", button_type);
}

void setup_resolution_frame(void)
{
	GtkWidget *radiobutton1 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton1");
	GtkWidget *radiobutton2 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton2");
	GtkWidget *radiobutton3 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton3");
	GtkWidget *radiobutton4 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton7");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton2), TRUE);

	g_signal_connect(GTK_RADIO_BUTTON(radiobutton1), "toggled", G_CALLBACK(resolution_select_cb), radiobutton1);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton2), "toggled", G_CALLBACK(resolution_select_cb), radiobutton2);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton3), "toggled", G_CALLBACK(resolution_select_cb), radiobutton3);
	g_signal_connect(GTK_RADIO_BUTTON(radiobutton4), "toggled", G_CALLBACK(resolution_select_cb), radiobutton4);
}


void setup_disk_frame(void)
{
	// radio button setup
	GtkWidget *default_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton12");
	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton13");
	// file chooser setup
	GtkWidget *sdcard_filechooser2 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton2");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Disk Files");
	gtk_file_filter_add_pattern(filter, "*.x86");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(sdcard_filechooser2), filter);
	set_disk_select_active_cb();	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(default_radiobutton), TRUE);

	g_signal_connect(G_OBJECT(select_radiobutton), "toggled", G_CALLBACK(set_disk_select_active_cb), NULL);
	g_signal_connect(G_OBJECT(default_radiobutton), "toggled", G_CALLBACK(set_disk_default_active_cb), NULL);
	g_signal_connect(G_OBJECT(sdcard_filechooser2), "selection-changed", G_CALLBACK(disk_file_select_cb), NULL);

}

void setup_sdcard_frame(void)
{
	// sdcard size combo box setup
	GtkWidget *hbox = (GtkWidget *)gtk_builder_get_object(g_create_builder, "hbox4");

	GtkComboBox *sdcard_combo_box = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(sdcard_combo_box), FALSE, FALSE, 1);
	add_widget(VTM_CREATE_ID, VTM_CREATE_SDCARD_COMBOBOX, GTK_WIDGET(sdcard_combo_box));

	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_256); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_512); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_1024); 
	gtk_combo_box_append_text(sdcard_combo_box, SDCARD_SIZE_1536); 

	gtk_combo_box_set_active(sdcard_combo_box, SDCARD_DEFAULT_SIZE);

	// radio button setup
	GtkWidget *create_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton4");
	GtkWidget *select_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton5");
	GtkWidget *none_radiobutton = (GtkWidget *)gtk_builder_get_object(g_create_builder, "radiobutton6");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(none_radiobutton), TRUE);

	// file chooser setup
	GtkWidget *sdcard_filechooser = (GtkWidget *)gtk_builder_get_object(g_create_builder, "filechooserbutton1");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "SD Card Image Files");
	gtk_file_filter_add_pattern(filter, "*.img");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(sdcard_filechooser), filter);

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
	GtkWidget *hbox = (GtkWidget *)gtk_builder_get_object(g_create_builder, "hbox6");

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
	const gchar *skin = NULL;
	GtkWidget *sub_window;

	g_create_builder = gtk_builder_new();
	char full_glade_path[MAX_LEN];
	sprintf(full_glade_path, "%s/etc/vtm.glade", get_root_path());

	gtk_builder_add_from_file(g_create_builder, full_glade_path, NULL);

	sub_window = (GtkWidget *)gtk_builder_get_object(g_create_builder, "window2");

	add_window(sub_window, VTM_CREATE_ID);

	fill_virtual_target_info();

	GtkWidget *label4 = (GtkWidget *)gtk_builder_get_object(g_create_builder, "label4");
	gtk_label_set_text(GTK_LABEL(label4),"Input name of the virtual target.");
	GtkWidget *name_entry = (GtkWidget *)gtk_builder_get_object(g_create_builder, "entry1");
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), VT_NAME_MAXBUF); 

	g_signal_connect(G_OBJECT (name_entry), "changed",	G_CALLBACK (entry_changed),	NULL);

	setup_create_frame();
	setup_create_button();

	skin = get_skin_path();
	if(skin == NULL)
		WARN( "getting icon image path is failed!!\n");
	sprintf(icon_image, "%s/icons/Emulator_20x20.png", skin);
	gtk_window_set_icon_from_file(GTK_WINDOW(sub_window), icon_image, NULL);

	g_signal_connect(GTK_OBJECT(sub_window), "delete_event", G_CALLBACK(create_window_deleted_cb), NULL);

	gtk_widget_show_all(sub_window);

	gtk_main();
}

void construct_main_window(void)
{
	GtkWidget *vbox;
	GtkWidget *list_view;
	GtkTreeSelection *selection;
	GtkWidget *create_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button1");
	GtkWidget *delete_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button2");
	GtkWidget *modify_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button3");
	GtkWidget *activate_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button4");
	GtkWidget *details_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button5");
	GtkWidget *refresh_button = (GtkWidget *)gtk_builder_get_object(g_builder, "button8");
	g_main_window = (GtkWidget *)gtk_builder_get_object(g_builder, "window1");
	gtk_window_set_icon_from_file(GTK_WINDOW(g_main_window), icon_image, NULL);
	GtkWidget *x86_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton8");
	GtkWidget *arm_radiobutton = (GtkWidget *)gtk_builder_get_object(g_builder, "radiobutton9");

	vbox = GTK_WIDGET( gtk_builder_get_object( g_builder, "vbox3" ) );

	list_view = setup_list();
	gtk_widget_grab_focus(activate_button);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x86_radiobutton), TRUE);

	gtk_widget_set_sensitive(arm_radiobutton, FALSE);

	g_signal_connect(GTK_RADIO_BUTTON(x86_radiobutton), "toggled", G_CALLBACK(arch_select_cb), x86_radiobutton);
	g_signal_connect(GTK_RADIO_BUTTON(arm_radiobutton), "toggled", G_CALLBACK(arch_select_cb), arm_radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), list_view, TRUE, TRUE, 0);
	selection  = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	g_signal_connect(create_button, "clicked", G_CALLBACK(show_create_window), NULL); 
	g_signal_connect(delete_button, "clicked", G_CALLBACK(delete_clicked_cb), selection);
	g_signal_connect(details_button, "clicked", G_CALLBACK(details_clicked_cb), selection);
	g_signal_connect(modify_button, "clicked", G_CALLBACK(modify_clicked_cb), selection);
	g_signal_connect(activate_button, "clicked", G_CALLBACK(activate_clicked_cb), selection);
	g_signal_connect(refresh_button, "clicked", G_CALLBACK(refresh_clicked_cb), selection);
	g_signal_connect(G_OBJECT(g_main_window), "delete-event", G_CALLBACK(exit_vtm), NULL); 

	g_object_unref(G_OBJECT(g_builder));

	/* setup emulator architecture and path */
	gtk_widget_show_all(g_main_window);


}

int main(int argc, char** argv)
{
	char* working_dir;
	char *buf = argv[0];
	int status;
	char *skin = NULL;
	char full_glade_path[MAX_LEN];
	working_dir = g_path_get_dirname(buf);
	status = g_chdir(working_dir);
	if(status == -1)
	{
		ERR( "fail to change working directory\n");
		exit(1);
	}

	gtk_init(&argc, &argv);
	INFO( "virtual target manager start \n");

	socket_init();

	g_builder = gtk_builder_new();
	skin = (char*)get_skin_path();
	if(skin == NULL)
		WARN( "getting icon image path is failed!!\n");
	sprintf(icon_image, "%s/icons/Emulator_20x20.png", skin);

	sprintf(full_glade_path, "%s/etc/vtm.glade", get_root_path());

	gtk_builder_add_from_file(g_builder, full_glade_path, NULL);

	window_hash_init();

	construct_main_window();

	init_setenv();

	gtk_main();

	free(target_list_filepath);
	return 0;
}
