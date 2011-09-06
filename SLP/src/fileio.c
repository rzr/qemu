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
 * @param	filename : file path (ex: /home/$(USER_ID)/.samsung_sdk/simulator/1/emulator.conf)
 * @return	exist normal(1), not exist(0), error case(-1))
 * @date    Oct. 22. 2009
 * */

int is_exist_file(gchar *filepath)
{
	if ((filepath == NULL) || (strlen(filepath) <= 0))	{
		log_msg(MSGL_ERROR, "filepath is incorrect.\n");
		return -1;
	}

	if (strlen(filepath) >= MAXBUF) {
		log_msg(MSGL_ERROR, "file path is too long. (%s)\n", filepath);
		return -1;
	}

	if (g_file_test(filepath, G_FILE_TEST_IS_DIR) == TRUE) {
		log_msg(MSGL_INFO, "%s: is not a file, is a directory!!\n", filepath);
		return -1;
	}

	if (g_file_test(filepath, G_FILE_TEST_EXISTS) == TRUE) {
		log_msg(MSGL_INFO, "%s: exists normally!!\n", filepath);
		return FILE_EXISTS;
	}

	log_msg(MSGL_INFO, "%s: not exists!!\n", filepath);
	return FILE_NOT_EXISTS;
}


/* emulator_path = "/opt/samsung_sdk/simulator/bin/simulator" : get_my_exec_path() */
const gchar *get_emulator_path(void)
{
	static gchar *emulator_path = NULL;
	int len = 10;
	int r;

	/* allocate just once */
	if (emulator_path)
		return emulator_path;

	const char *env_path = getenv("EMULATOR_PATH");
	if (env_path) {
		emulator_path = strdup(env_path);
		return emulator_path;
	}

	while (1)
	{
		emulator_path = malloc(len);
		if (!emulator_path) {
			fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
		}

#ifndef _WIN32
		r = readlink("/proc/self/exe", emulator_path, len);
		if (r < len) {
			emulator_path[r] = 0;
			break;
		}
#else
		r = GetModuleFileName(NULL, emulator_path, len);
		if (r < len) {
			emulator_path[r] = 0;
			dos_path_to_unix_path(emulator_path);
			break;
		}
#endif
		free(emulator_path);
		len *= 2;
	}

	return emulator_path;
}


/* get_bin_path = "/opt/samsung_sdk/simulator/bin/" */

const gchar *get_bin_path(void)
{
	static gchar *bin_path;

	if (!bin_path)
	{
		const gchar *emulator_path = get_emulator_path();
		bin_path = g_path_get_dirname(emulator_path);
	}

	return bin_path;
}


/* get_path = "/opt/samsung_sdk/simulator/" */
const gchar *get_path(void)
{
	static gchar *path = NULL;

	if (!path)
	{
		const gchar *emulator_path = get_emulator_path();
		path = g_path_get_dirname(emulator_path);
	}

	return path;
}


/* get_skin_path = "/opt/samsung_sdk/simulator/skins" */
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
		path = get_path();
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


/* get_data_path = "/opt/samsung_sdk/simulator/data" */
const gchar *get_data_path(void)
{
	static const char suffix[] = "/data";
	static gchar *data_path;

	if (!data_path)
	{
		const gchar *path = get_path();

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


/* get_conf_path = "/opt/samsung_sdk/simulator/conf" */
/* get_conf_path = "C:\Documents and Settings\Administrator\Application Data\samsung_sdk\simulator\1" */
const gchar *get_conf_path(void)
{
	static gchar *conf_path;

	if (!conf_path)
	{
		static const char suffix[] = "/conf";

		const gchar *path = get_path();
		conf_path = malloc(strlen(path) + sizeof suffix);
		assert(conf_path != NULL);
		strcpy(conf_path, path);
		strcat(conf_path, suffix);
	}

	return conf_path;
}

/**
 * @brief 	get_emulator conf file path
 * @return	"/opt/samsung_sdk/simulator/conf/emulator.conf"
 *			=> "/root/.samsung_sdk/simulator/1/emulator.conf"
 *			=> "/home/users/.samsung_sdk/simulator/1/emulator.conf"
 * @date    July 9. 2009
 * */
gchar *get_emulator_conf_filepath(void)
{
	gchar *emulator_conf_filepath = NULL;
	emulator_conf_filepath = calloc(1, 512);
	if(NULL == emulator_conf_filepath) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}
	
	const gchar *conf_path = get_conf_path();
	sprintf(emulator_conf_filepath, "%s/emulator.conf", conf_path);

	return emulator_conf_filepath;
}

gchar *get_target_list_filepath(void)
{
	gchar *target_list_filepath = NULL;
	target_list_filepath = calloc(1,512);
	if(NULL == target_list_filepath) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}

	const gchar *conf_path = get_conf_path();
	sprintf(target_list_filepath, "%s/targetlist.ini", conf_path);

	return target_list_filepath;
}

gchar *get_virtual_target_path(gchar *virtual_target_name)
{
	gchar *virtual_target_path = NULL;
	virtual_target_path = calloc(1,512);

	if(NULL == virtual_target_path) {
		fprintf(stderr, "%s - %d: memory allocation failed!\n", __FILE__, __LINE__); exit(1);
	}

	const gchar *conf_path = get_conf_path();
	sprintf(virtual_target_path, "%s/%s/", conf_path, virtual_target_name);

	return virtual_target_path;
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

