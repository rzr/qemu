/*
 * Emulator SDB Notification Server
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  MunKyu Im       <munkyu.im@samsung.com>
 *  Sangho Park     <sangho1206.park@samsung.com>
 *  YeongKyoon Lee  <yeongkyoon.lee@samsung.com>
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

#ifndef SDB_SERVER_H_
#define SDB_SERVER_H_

#include "maru_common.h"

#include <errno.h>

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


#define SDB_HOST_PORT 26099

#define SDB_TCP_EMULD_INDEX  2    /* emulator daemon port */
#define SDB_TCP_OPENGL_INDEX  3   /* opengl server port */
#define SDB_UDP_SENSOR_INDEX  2   /* sensor server port */

void sdb_setup(void);
void set_base_port(void);
int inet_strtoip(const char*  str, uint32_t  *ip);
int socket_send(int fd, const void*  buf, int  buflen);
void socket_close(int fd);
int check_port_bind_listen(uint32_t port);
int sdb_loopback_client(int port, int type);

void start_sdb_noti_server(int server_port);

#define STATE_RUNNING 0
#define STATE_SUSPEND 1
void notify_all_sdb_clients(int state);

#endif

