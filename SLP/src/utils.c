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


/**
	@file 	utils.c
	@brief	miscellaneous functions used in ISE
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"

static GHashTable *windows_hash = NULL; /* hash table to get widow and widget of Emulator */


/**
	@brief 	hash intialization
*/
GHashTable *window_hash_init (void)
{
	windows_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
	return windows_hash;
}


/**
	@brief 	add window widget to hash
*/
void add_window (GtkWidget *win, gint window_id)
{
    g_hash_table_insert (windows_hash, GINT_TO_POINTER (window_id), win);
}


/**
	@brief 	search window widget by using window id as a key
	@return	matched window widget
*/
GtkWidget *get_window (gint window_id)
{
    GtkWidget *win = (GtkWidget *) g_hash_table_lookup (windows_hash, GINT_TO_POINTER (window_id));
    return win;
}


/**
	@brief 	destroy hash
*/
void window_hash_destroy (void)
{
    g_hash_table_destroy (windows_hash);
}


/**
	@brief 	add widget to hash 
	@param 	window_id: ID of widget
	@param	widget_name: widget name
	@param	widget: widget to add in hash
*/
void add_widget (gint window_id, gchar * widget_name, GtkWidget * widget)
{
    if (!windows_hash) {
		log_msg(MSGL_WARN,"Parent window not exist!\n");
		return;
    }
	
    GtkWidget *parent = get_window (window_id);
    if (!parent) {
		log_msg(MSGL_WARN,"Parent window not exist!\n");
		return;
    }
	
    g_object_set_data_full (G_OBJECT (parent), widget_name, g_object_ref (widget), (GDestroyNotify) g_object_unref);
}


/**
	@brief 	get widget from hash
	@param 	window_id: ID of widget
	@param	widget_name: widget name to search in hash
	@return	widget: widget to matched widget_name
*/
GtkWidget *get_widget (gint window_id, gchar *widget_name)
{
    GtkWidget *parent;
	
    parent = get_window (window_id);
    if (!parent) {
		log_msg(MSGL_WARN, "Parent window not exist!\n");
		return NULL;
    }

    GtkWidget *w = (GtkWidget *) g_object_get_data (G_OBJECT (parent), widget_name);
    if (!w) {
		log_msg(MSGL_INFO, "Widget(%s) not found!\n", widget_name);
		return NULL;
    }
	
    return w;
}


/**
 * @brief 	set config type in emulator.conf file
 * @param	filename: configuration file path, group: EMULATOR_GROUP, field: CONF_USE_KEY, type: setting type
 * @return	fail(-1), success(0)
 * @date    Nov 18. 2008
 * */
int set_config_type(gchar *filepath, const gchar *group, const gchar *field, const int type)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	gsize length;

	keyfile = g_key_file_new();
	if (!g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_KEEP_COMMENTS, &error)) {
		log_msg(MSGL_INFO, "loading key file form %s is failed.\n", filepath);
		return -1;
	}

	g_key_file_set_integer(keyfile, group, field, type);

	gchar *data = g_key_file_to_data(keyfile, &length, &error);
	if (error != NULL) {
		g_print("in set_config_type\n");
		g_print("%s", error->message);
		g_clear_error(&error);
	}

	g_strstrip(data);
	length = strlen(data);
	g_file_set_contents(filepath, data, length, &error);
	if (error != NULL) {
		g_print("in set_cofig_type after g_file_set_contetns\n");
		g_print("%s", error->message);
		g_clear_error(&error);
	}

#ifndef _WIN32
	chmod(filepath, S_IRWXU | S_IRWXG | S_IRWXO);
#else
	chmod(filepath, S_IRWXU);
#endif

	g_free(data);
	g_key_file_free(keyfile);

	return 0;

}


/**
 * @brief 	set config value in emulator.conf file
 * @param	filename: configuration file path, group: EMULATOR_GROUP, field: TARGET_PATH_KEY, value: setting value
 * @return	fail(-1), success(0)
 * @date    Nov 18. 2008
 * */
int set_config_value(gchar *filepath, const gchar *group, const gchar *field, const gchar *value)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	gsize length;

	keyfile = g_key_file_new();
	if (!g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_KEEP_COMMENTS, &error)) {
		log_msg(MSGL_WARN, "loading key file form %s is failed.\n", filepath);
		return -1;
	}

	g_key_file_set_value(keyfile, group, field, value);

	gchar *data = g_key_file_to_data(keyfile, &length, &error);
	if (error != NULL) {
		g_print("in set_config_type\n");
		g_print("%s", error->message);
		g_clear_error(&error);
	}

	g_strstrip(data);
	length = strlen(data);
	g_file_set_contents(filepath, data, length, &error);
	if (error != NULL) {
		g_print("in set_config_value after g_file_set_contents\n");
		g_print("%s", error->message);
		g_clear_error(&error);
	}

#ifndef _WIN32
	chmod(filepath, S_IRWXU | S_IRWXG | S_IRWXO);
#else
	chmod(filepath, S_IRWXU);
#endif

	g_free(data);
	g_key_file_free(keyfile);

	return 0;
}


/**
 * @brief 	get config type	from emulator.conf file
 * @param	filename: configuration file path, group: EMULATOR_GROUP, field: CONF_USE_KEY, CONFIGURATION
 * @return	fail(0), success(type)
 * @date    Nov 18. 2008
 * */
int get_config_type(gchar *filepath, const gchar *group, const gchar *field)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	gint type = 0;

	keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_KEEP_COMMENTS, &error)) {
		log_msg( MSGL_ERROR, "loading key file from %s is failed\n", filepath );
		return -1;
	}

	type = g_key_file_get_integer(keyfile, group, field, &error);

	g_key_file_free(keyfile);
	return type;
}


/**
 * @brief 	get config value from emulator.conf file
 * @param	filename: configuration file path, group: EMULATOR_GROUP, field: TARGET_PATH_KEY, CONFIGURATION
 * @return	fail(0), success(value)
 * @date    Nov 18. 2008
 * */
char *get_config_value(gchar *filepath, const gchar *group, const gchar *field)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	gchar *value = NULL;

	keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_KEEP_COMMENTS, &error)) {
		log_msg( MSGL_ERROR, "loading key file form %s is failed\n", filepath );
		return NULL;
	}

	value = g_key_file_get_value(keyfile, group, field, NULL);

	if (!value || strlen(value) == 0) {
		//*value = '\0';
		value = NULL;
		return value;
	}

	g_key_file_free(keyfile);
	return value;
}


/**
	@brief 	convert string  to lower case string
	@param 	string string to covert
*/
#ifndef _WIN32
void
strlwr (char *string)
{
    while (1)
	{

		*string = (char) tolower (*string);

		if (*string == 0)  {
			return;
		}
		string++;
	}
}
#endif

