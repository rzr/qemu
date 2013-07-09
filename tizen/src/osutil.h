/* 
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

#ifndef __OSUTIL_H__
#define __OSUTIL_H__

#include "maru_common.h"

#define HTTP_PROTOCOL "http="
#define HTTP_PREFIX "http://"
#define HTTPS_PROTOCOL "https="
#define FTP_PROTOCOL "ftp="
#define SOCKS_PROTOCOL "socks="
#define DIRECT "DIRECT"
#define PROXY "PROXY"
#define MAXPORTLEN 6

#define GNOME_PROXY_MODE 0
#define GNOME_PROXY_AUTOCONFIG_URL 1
#define GNOME_PROXY_HTTP_HOST 2
#define GNOME_PROXY_HTTP_PORT 3
#define GNOME_PROXY_HTTPS_HOST 4
#define GNOME_PROXY_HTTPS_PORT 5
#define GNOME_PROXY_FTP_HOST 6
#define GNOME_PROXY_FTP_PORT 7
#define GNOME_PROXY_SOCKS_HOST 8
#define GNOME_PROXY_SOCKS_PORT 9
#define GCONFTOOL 0
#define GSETTINGS 1

extern const char *pac_tempfile; 

void check_vm_lock_os(void);
void make_vm_lock_os(void);

void set_bin_path_os(gchar *);

void print_system_info_os(void);

void get_host_proxy_os(char *, char *, char *, char *);

inline void download_url(char *);
inline size_t write_data(void *, size_t, size_t, FILE *);
inline void remove_string(char *, char *, const char *);

#endif // __OS_UTIL_H__

