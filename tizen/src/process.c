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
#endif
#include <assert.h>

//#include "fileio.h"
#include "debug_ch.h"

//DEFAULT_DEBUG_CHANNEL(tizen);
MULTI_DEBUG_CHANNEL(tizen, process);

static char pidfname[512] = { 0, };
static char tizen_vms_path[512] = {0, };

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
 * @brief 	make pid directory
 * @param	pidfname : pid file name
 * @date    Nov 25. 2008
 * */ 

static int make_pid_path(const char *pidfname)
{
	char dir[512] = "", buf[512] = "";
	char *ptr, *last = NULL, *lptr = NULL;
	int dirnamelen = 0;

	sprintf(buf, "%s", pidfname);
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
 * @brief 	wirte pid file
 * @param	mod: emulator, vinit
 * @date    Nov 25. 2008
 * */

int write_pidfile(char *path)
{
	int		fd = -1;
	char	buf[128] = "";

    if(!g_path_is_absolute(path))
        strcpy(tizen_vms_path, g_get_current_dir());
    else
        strcpy(tizen_vms_path, g_path_get_dirname(path));

    sprintf(pidfname, "%s/.pid", tizen_vms_path);

	if (access(tizen_vms_path, R_OK) != 0) {
		make_pid_path(pidfname);
	}
#ifdef _WIN32
	if ((fd = open(g_win32_locale_filename_from_utf8(pidfname), O_RDWR | O_CREAT, 0666)) < 0) {
#else
	if ((fd = open(pidfname, O_RDWR | O_CREAT, 0666)) < 0) {
#endif
		ERR("Failed to create .pid file\n");
		ERR("%s at %s(%d)\n", strerror(errno), __FILE__, __LINE__);
	    return -1;
	}
	
	ftruncate(fd, 0);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", (int)getpid());
	write(fd, buf, strlen(buf));

	close(fd);

	return 0;
}

/**
 * @brief 	remove pid file
 * @param	mod: emulator, vinit
 * @date    Nov 25. 2008
 * */

int remove_pidfile(void)
{
	if (strlen(pidfname) <= 0) {
		return -1;
	}

#ifdef _WIN32
    if (remove(g_win32_locale_filename_from_utf8(pidname)) < 0) {
#else
	if (remove(pidfname) < 0) {
#endif
		ERR( "Can't remove pid file. (%s)\n", pidfname);
	}

	return 0;
}
