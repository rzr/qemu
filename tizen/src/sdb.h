/* Copyright (C) 2006-2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

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

