/* 
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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


#include "process.h"
#ifndef _WIN32
#include <wait.h>
#else
#include <stdlib.h>
#endif
#include <assert.h>

//#include "fileio.h"
#include "debug_ch.h"

//DEFAULT_DEBUG_CHANNEL(tizen);
MULTI_DEBUG_CHANNEL(tizen, process);

static char portfname[512] = { 0, };
char tizen_vms_path[512] = {0, };
extern int tizen_base_port;
#ifdef _WIN32
static char *mbstok_r (char *string, const char *delim, char **save_ptr)
{
	if (MB_CUR_MAX > 1)
	{
		if (string == NULL)
		{
			string = *save_ptr;
			if (string == NULL)
			return NULL; /* reminder that end of token sequence has been
			    reached */
		}

		/* Skip leading delimiters.  */
		string += _mbsspn (string, delim);

		/* Found a token?  */
		if (*string == '\0')
		{
			*save_ptr = NULL;
			return NULL;
		}

		/* Move past the token.  */
		{
			char *token_end = _mbspbrk (string, delim);

			if (token_end != NULL)
			{
				/* NUL-terminate the token.  */
				*token_end = '\0';
				*save_ptr = token_end + 1;
			}
			else
				*save_ptr = NULL;
		}
		return string;
	}
	else
		return NULL;
}
#endif

/**
 * @brief 	make port directory
 * @param	portfname : port file name
 * @date    Nov 25. 2008
 * */ 

static int make_port_path(const char *portfname)
{
	char dir[512] = "", buf[512] = "";
	char *ptr, *last = NULL, *lptr = NULL;
	int dirnamelen = 0;

	sprintf(buf, "%s", portfname);
#ifndef _WIN32	
	lptr = ptr = strtok_r(buf+1, "/", &last);
#else
	lptr = ptr = mbstok_r(buf, "/", &last);
#endif

	for (;;) {
#ifndef _WIN32
		if ((ptr = strtok_r(NULL, "/", &last)) == NULL) break;
#else
		if ((ptr = mbstok_r(NULL, "/", &last)) == NULL) break;
#endif
		dirnamelen = strlen(lptr) + 1;

		if (sizeof(dir) < dirnamelen) {
			return -1;
		}
#ifdef _WIN32
		if (dir != NULL && strlen (dir) > 0) 
#endif			
		strcat(dir, "/");
		strcat(dir, lptr);

#ifndef _WIN32
		if (access(dir, R_OK) != 0) 
			mkdir(dir, S_IRWXU | S_IRWXG);
#else
        if (access(g_win32_locale_filename_from_utf8(dir), R_OK) != 0) 
			mkdir(g_win32_locale_filename_from_utf8(dir));		
#endif
		lptr = ptr;
	}

	return 0;
}


/**
 * @brief 	wirte port file
 * @param	mod: emulator, vinit
 * @date    Nov 25. 2008
 * */

int write_portfile(char *path)
{
	int		fd = -1;
	char	buf[128] = "";

    if(!g_path_is_absolute(path))
        strcpy(tizen_vms_path, g_get_current_dir());
    else
        strcpy(tizen_vms_path, g_path_get_dirname(path));

    sprintf(portfname, "%s/.port", tizen_vms_path);

	if (access(tizen_vms_path, R_OK) != 0) {
		make_port_path(portfname);
	}
#ifdef _WIN32
	if ((fd = open(g_win32_locale_filename_from_utf8(portfname), O_RDWR | O_CREAT, 0666)) < 0) {
#else
	if ((fd = open(portfname, O_RDWR | O_CREAT, 0666)) < 0) {
#endif
		ERR("Failed to create .port file\n");
		ERR("%s at %s(%d)\n", strerror(errno), __FILE__, __LINE__);
	    return -1;
	}
	
	ftruncate(fd, 0);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", tizen_base_port);
	write(fd, buf, strlen(buf));

	close(fd);

	return 0;
}

/**
 * @brief 	remove port file
 * @param	mod: emulator, vinit
 * @date    Nov 25. 2008
 * */

int remove_portfile(void)
{
	if (strlen(portfname) <= 0) {
		return -1;
	}

#ifdef _WIN32
    if (remove(g_win32_locale_filename_from_utf8(portfname)) < 0) {
#else
	if (remove(portfname) < 0) {
#endif
		ERR( "Can't remove port file. (%s)\n", portfname);
	}

	return 0;
}
