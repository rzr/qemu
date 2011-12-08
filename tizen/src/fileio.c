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

#include <assert.h>
#include "fileio.h"

#include "debug_ch.h"

//DEFAULT_DEBUG_CHANNEL(tizen_sdk);
MULTI_DEBUG_CHANNEL(tizen_sdk, fileio);

extern STARTUP_OPTION startup_option;
#ifdef _WIN32
static void dos_path_to_unix_path(char *path)
{
	int i;

	for (i = 0; path[i]; i++)
		if (path[i] == '\\')
			path[i] = '/';
}

static void unix_path_to_dos_path(char *path)
{
	int i;

	for (i = 0; path[i]; i++)
		if (path[i] == '/')
			path[i] = '\\';
}
#endif

/**
 * @brief 	check about file
 * @param	filename : file path (ex: /home/$(USER_ID)/.tizen_sdk/simulator/1/emulator.conf)
 * @return	exist normal(1), not exist(0), error case(-1))
 * @date    Oct. 22. 2009
 * */

int is_exist_file(gchar *filepath)
{
	if ((filepath == NULL) || (strlen(filepath) <= 0))	{
		ERR( "filepath is incorrect.\n");
		return -1;
	}

	if (strlen(filepath) >= MAXBUF) {
		ERR( "file path is too long. (%s)\n", filepath);
		return -1;
	}

	if (g_file_test(filepath, G_FILE_TEST_IS_DIR) == TRUE) {
		INFO( "%s: is not a file, is a directory!!\n", filepath);
		return -1;
	}

	if (g_file_test(filepath, G_FILE_TEST_EXISTS) == TRUE) {
		TRACE( "%s: exists normally!!\n", filepath);
		return FILE_EXISTS;
	}

	INFO( "%s: not exists!!\n", filepath);
	return FILE_NOT_EXISTS;
}


/* exec_path = "~/tizen_sdk/Emulator/bin/emulator-manager" */
/* 			 = "~/tizen_sdk/Emulator/bin/emulator-x86" */
const gchar *get_exec_path(void)
{
	static gchar *exec_path = NULL;
	int len = 10;
	int r;

	/* allocate just once */
	if (exec_path)
		return exec_path;

	const char *env_path = getenv("EMULATOR_PATH");
	if (env_path) {
		exec_path = strdup(env_path);
		return exec_path;
	}

	while (1)
	{
		exec_path = malloc(len);
		if (!exec_path) {
			fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
		}

#ifndef _WIN32
		r = readlink("/proc/self/exe", exec_path, len);
		if (r < len) {
			exec_path[r] = 0;
			break;
		}
#else
		r = GetModuleFileName(NULL, exec_path, len);
		if (r < len) {
			exec_path[r] = 0;
			dos_path_to_unix_path(exec_path);
			break;
		}
#endif
		free(exec_path);
		len *= 2;
	}

	return exec_path;
}

/* get_root_path = "~/tizen_sdk/Emulator" */
const gchar *get_root_path(void)
{
	static gchar *root_path; 
	static gchar *root_path_buf;

	if (!root_path)
	{
		const gchar *exec_path = get_exec_path();
		root_path_buf = g_path_get_dirname(exec_path);
		root_path = g_path_get_dirname(root_path_buf);
		g_free(root_path_buf);
	}

	return root_path;
}

/* get_bin_path = "~/tizen_sdk/Emulator/bin" */
const gchar *get_bin_path(void)
{
	static gchar *bin_path; 

	if (!bin_path)
	{
		const gchar *exec_path = get_exec_path();
		bin_path = g_path_get_dirname(exec_path);
	}

	return bin_path;
}

/* get_arch_path = "x86" 
* 	             = "arm" */
const gchar *get_arch_path(void)
{
	static gchar *path_buf;
	static gchar *path;
	char *arch = (char *)g_getenv("EMULATOR_ARCH");

	const gchar *exec_path = get_exec_path();
	path_buf = g_path_get_dirname(exec_path);
	if(!arch) /* for stand alone */
	{
		char *binary = g_path_get_basename(exec_path);
		if(strstr(binary, "emulator-x86"))
			arch = g_strdup_printf("x86");
		else if(strstr(binary, "emulator-arm"))
			arch = g_strdup_printf("arm");
		else 
		{
			ERR( "binary setting failed\n");
			exit(1);
		}
		free(binary);
	}
	path = malloc(3);
	strcpy(path, arch);

	return path;
}
/* get_baseimg_abs_path = "~/tizen_sdk/Emulator/x86/emulimg.x86"
*						= "~/tizen_sdk/Emulator/arm/emulimg.arm"	*/
const gchar *get_baseimg_abs_path(void)
{
	const gchar *arch_path;
	static gchar *path;
	char *arch = (char *)g_getenv("EMULATOR_ARCH");
	const gchar *exec_path = get_exec_path();
	if(!arch) /* for stand alone */
	{
		char *binary = g_path_get_basename(exec_path);
		if(strstr(binary, "emulator-x86"))
			arch = g_strdup_printf("x86");
		else if(strstr(binary, "emulator-arm"))
			arch = g_strdup_printf("arm");
		else 
		{
			ERR( "binary setting failed\n");
			exit(1);
		}
		free(binary);
	}
	arch_path = get_arch_abs_path();
	path = malloc(strlen(arch_path) + 13);
	strcpy(path, arch_path);
	strcat(path, "/");	
	strcat(path, "emulimg.");
	strcat(path, arch);

	return path;
}

/* get_arch_abs_path = "~/tizen_sdk/Emulator/x86" 
* 				= "~/tizen_sdk/Emulator/arm" */
const gchar *get_arch_abs_path(void)
{
	static gchar *path_buf = NULL;
	static gchar *path_buf2 = NULL;
	static gchar *path;
	char *arch = (char *)g_getenv("EMULATOR_ARCH");

	const gchar *exec_path = get_exec_path();
	path_buf2 = g_path_get_dirname(exec_path);
	path_buf = g_path_get_dirname(path_buf2);
	g_free(path_buf2);
	path = malloc(strlen(path_buf) + 5);
	if(!arch) /* for stand alone */
	{
		char *binary = g_path_get_basename(exec_path);
		if(strstr(binary, "emulator-x86"))
			arch = g_strdup_printf("x86");
		else if(strstr(binary, "emulator-arm"))
			arch = g_strdup_printf("arm");
		else 
		{
			ERR( "binary setting failed\n");
			exit(1);
		}
		free(binary);
	}
	strcpy(path, path_buf);
	strcat(path, "/");	
	strcat(path, arch);
	g_free(path_buf);

	return path;
}

/* get_skin_path = "~/tizen_sdk/simulator/skins" */
const gchar *get_skin_path(void)
{
	const char *skin_path_env;
	const char skinsubdir[] = "/skins";
	const char *path;

	static char *skin_path;

	if (skin_path)
		return skin_path;

	skin_path_env = getenv("EMULATOR_SKIN_PATH");
	if (!skin_path_env)
	{
		path = get_root_path();
		skin_path = malloc(strlen(path) + sizeof skinsubdir);
		if (!skin_path) {
			fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__);
			exit(1);
		}

		strcpy(skin_path, path);
		strcat(skin_path, skinsubdir);
	}
	else
		skin_path = strdup(skin_path_env);

	if (g_file_test(skin_path, G_FILE_TEST_IS_DIR) == FALSE) {
		fprintf(stderr, "no skin directory at %s\n", skin_path);
	}

	return skin_path;
}


/* get_data_path = "~/tizen_sdk/Emulator/data" */
const gchar *get_data_path(void)
{
	static const char suffix[] = "/data";
	static gchar *data_path;

	if (!data_path)
	{
		const gchar *path = get_arch_path();

		data_path = malloc(strlen(path) + sizeof suffix);
		assert(data_path != NULL);
		strcpy(data_path, path);
		strcat(data_path, suffix);
	}

	return data_path;
}

/* get_data_path = "~/tizen_sdk/Emulator/data" */
const gchar *get_data_abs_path(void)
{
	static const char suffix[] = "/data";
	static gchar *data_path;

	if (!data_path)
	{
		const gchar *path = get_arch_abs_path();

		data_path = malloc(strlen(path) + sizeof suffix);
		assert(data_path != NULL);
		strcpy(data_path, path);
		strcat(data_path, suffix);
	}

	return data_path;
}

#ifdef _WIN32	
/**
 * @brief 	change_path_to_slash (\, \\ -> /)
 * @param	org path to change (C:\\test\\test\\test)
 * @return	changed path (C:/test/test/test)
 * @date    Nov 19. 2009
 * */ 
gchar *change_path_to_slash(gchar *org_path)
{
	gchar *path = strdup(org_path);

	dos_path_to_unix_path(path);

	return path;
}

gchar *change_path_from_slash(gchar *org_path)
{
	gchar *path = strdup(org_path);

	unix_path_to_dos_path(path);

	return path;
}
#endif


/* get_conf_path = "~/tizen_sdk/Emulator/x86/conf" */
/*				 = "~/tizen_sdk/Emulator/arm/conf" */

const gchar *get_conf_path(void)
{
	static gchar *conf_path;

	static const char suffix[] = "/conf";

	const gchar *path = get_arch_path();
	conf_path = malloc(strlen(path) + sizeof suffix);
	assert(conf_path != NULL);
	strcpy(conf_path, path);
	strcat(conf_path, suffix);

	return conf_path;
}

/* get_conf_path = "x86/VMs" */
const gchar *get_vms_path(void)
{
	static gchar *vms_path;

	static const char suffix[] = "/VMs";

	const gchar *path = get_arch_path();
	vms_path = malloc(strlen(path) + sizeof suffix);
	assert(vms_path != NULL);
	strcpy(vms_path, path);
	strcat(vms_path, suffix);

	return vms_path;
}

/* get_conf_abs_path = "~/tizen_sdk/Emulator/x86/conf" */
/* 					 = "~/tizen_sdk/Emulator/arm/conf" */
const gchar *get_conf_abs_path(void)
{
	static gchar *conf_path;

	static const char suffix[] = "/conf";

	const gchar *path = get_arch_abs_path();
	conf_path = malloc(strlen(path) + sizeof suffix);
	assert(conf_path != NULL);
	strcpy(conf_path, path);
	strcat(conf_path, suffix);

	return conf_path;
}

/* get_vms_abs_path = "~/tizen_sdk/Emulator/x86/VMs" */
/* 				    = "~/tizen_sdk/Emulator/arm/VMs" */
const gchar *get_vms_abs_path(void)
{
	static gchar *vms_path;

	static const char suffix[] = "/VMs";

	const gchar *path = get_arch_abs_path();
	vms_path = malloc(strlen(path) + sizeof suffix);
	assert(vms_path != NULL);
	strcpy(vms_path, path);
	strcat(vms_path, suffix);

	return vms_path;
}

/*  get_targetlist_abs_filepath  = " ~/tizen_sdk/Emulator/x86/conf/targetlist.ini" */
/*  						     = " ~/tizen_sdk/Emulator/arm/conf/targetlist.ini" */
gchar *get_targetlist_abs_filepath(void)
{
	gchar *targetlist_filepath = NULL;
	targetlist_filepath = calloc(1, 512);
	if(NULL == targetlist_filepath) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}
	
	const gchar *conf_path = get_conf_abs_path();
	sprintf(targetlist_filepath, "%s/targetlist.ini", conf_path);

	return targetlist_filepath;
}


/* get_virtual_target_path = "x86/VMs/virtual_target_name/" */
/* 						   = "arm/VMs/virtual_target_name/" */
gchar *get_virtual_target_path(gchar *virtual_target_name)
{
	gchar *virtual_target_path = NULL;
	virtual_target_path = calloc(1,512);

	if(NULL == virtual_target_path) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}

	const gchar *conf_path = get_vms_path();
	sprintf(virtual_target_path, "%s/%s/", conf_path, virtual_target_name);

	return virtual_target_path;
}

/* get_virtual_target_abs_path	"~/tizen_sdk/Emulator/x86/VMs/virtual_target_name/" */
/* 								"~/tizen_sdk/Emulator/arm/VMs/virtual_target_name/" */
gchar *get_virtual_target_abs_path(gchar *virtual_target_name)
{
	gchar *virtual_target_path = NULL;
	virtual_target_path = calloc(1,512);

	if(NULL == virtual_target_path) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}

	const gchar *conf_path = get_vms_abs_path();
	sprintf(virtual_target_path, "%s/%s/", conf_path, virtual_target_name);

	return virtual_target_path;
}

/* get_virtual_target_log_path	"~/tizen_sdk/Emulator/x86/VMs/virtual_target_name/logs" */
/* 								"~/tizen_sdk/Emulator/arm/VMs/virtual_target_name/logs" */
gchar *get_virtual_target_log_path(gchar *virtual_target_name)
{
	gchar *virtual_target_log_path = NULL;
	virtual_target_log_path = calloc(1,512);

	if(NULL == virtual_target_log_path) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}

	const gchar *vms_path = get_vms_abs_path();
	sprintf(virtual_target_log_path, "%s/%s/logs", vms_path, virtual_target_name);

	return virtual_target_log_path;

}

int check_port(const char *ip_address, int port)
{
	struct sockaddr_in address;
	int sockfd, connect_status;

	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Socket Open error\n");
		return -2;
	}

	memset(&address, 0 , sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(ip_address);
	
	if(address.sin_addr.s_addr == INADDR_NONE)
	{
		fprintf(stderr, "Bad Address\n");
		close(sockfd);
		return -2;
	}
	connect_status = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
	close(sockfd);
	return connect_status;
}

