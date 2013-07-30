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


#include "emulator.h"
#include "net/slirp.h"
#include "qemu_socket.h"
#include "sdb.h"
#include "nbd.h"
#include "tizen/src/debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, sdb);

extern char tizen_target_path[];
extern int tizen_base_port;

#ifdef _WIN32

static void socket_close_handler( void*  _fd )
{
	int   fd = (int)_fd;
	int   ret;
	char  buff[64];

	/* we want to drain the read side of the socket before closing it */
	do {
		ret = recv( fd, buff, sizeof(buff), 0 );
	} while (ret < 0 && WSAGetLastError() == WSAEINTR);

	if (ret < 0 && WSAGetLastError() == EWOULDBLOCK)
		return;

	qemu_set_fd_handler( fd, NULL, NULL, NULL );
	closesocket( fd );
}

void socket_close( int  fd )
{
	int  old_errno = errno;

	shutdown( fd, SD_BOTH );
	/* we want to drain the socket before closing it */
	qemu_set_fd_handler( fd, socket_close_handler, NULL, (void*)fd );

	errno = old_errno;
}

#else /* !_WIN32 */

#include <unistd.h>

void socket_close( int  fd )
{
	int  old_errno = errno;

	shutdown( fd, SHUT_RDWR );
	close( fd );

	errno = old_errno;
}

#endif /* !_WIN32 */

int inet_strtoip(const char*  str, uint32_t  *ip)
{
	int  comp[4];

	if (sscanf(str, "%d.%d.%d.%d", &comp[0], &comp[1], &comp[2], &comp[3]) != 4)
		return -1;

	if ((unsigned)comp[0] >= 256 ||
			(unsigned)comp[1] >= 256 ||
			(unsigned)comp[2] >= 256 ||
			(unsigned)comp[3] >= 256)
		return -1;

	*ip = (uint32_t)((comp[0] << 24) | (comp[1] << 16) |
			(comp[2] << 8)  |  comp[3]);
	return 0;
}

int check_port_bind_listen(uint32_t port)
{
	struct sockaddr_in addr;
	int s, opt = 1;
	int ret = -1;
	socklen_t addrlen = sizeof(addr);
	memset(&addr, 0, addrlen);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (((s = qemu_socket(AF_INET, SOCK_STREAM, 0)) < 0) ||
#ifndef _WIN32
			(setsockopt(s, SOL_SOCKET,SO_REUSEADDR, (char *)&opt, sizeof(int)) < 0) ||
#endif
			(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) ||
			(listen(s, 1) < 0)) {

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

int get_sdb_base_port(void)
{
	int   tries     = 10;
	int   success   = 0;
	uint32_t port = 26100;

	if(tizen_base_port == 0){

		for ( ; tries > 0; tries--, port += 10 ) {
			if(check_port_bind_listen(port + 1) < 0 )
				continue;

			success = 1;
			break;
		}

		if (!success) {
			ERR( "it seems too many emulator instances are running on this machine. Aborting\n" );
			exit(1);
		}

		tizen_base_port = port;
		INFO( "sdb port is %d \n", tizen_base_port);
	}

	return tizen_base_port;
}

void sdb_setup(void)
{
	int   tries     = 10;
	int   success   = 0;
	uint32_t  guest_ip;
	char buf[64] = {0,};

	inet_strtoip("10.0.2.16", &guest_ip);

	for ( ; tries > 0; tries--, tizen_base_port += 10 ) {
		// redir form [tcp:26101:10.0.2.16:26101]
		sprintf(buf, "tcp:%d:10.0.2.16:26101", tizen_base_port + 1);
		if(net_slirp_redir((char*)buf) < 0)
			continue;

		INFO( "SDBD established on port %d\n", tizen_base_port + 1);
		success = 1;
		break;
	}

	INFO("redirect [%s] success\n", buf);
	if (!success) {
		ERR( "it seems too many emulator instances are running on this machine. Aborting\n" );
		exit(1);
	}

	INFO( "Port(%d/tcp) listen for SDB \n", tizen_base_port + 1);

	/* for sensort */
	sprintf(buf, "tcp:%d:10.0.2.16:3577", tizen_base_port + SDB_TCP_EMULD_INDEX );
	if(net_slirp_redir((char*)buf) < 0){
		ERR( "redirect [%s] fail \n", buf);
	}else{
		INFO("redirect [%s] success\n", buf);
	}
}

int sdb_loopback_client(int port, int type)
{
    struct sockaddr_in addr;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    s = socket(AF_INET, type, 0);
    if(s < 0) return -1;

    if(connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    return s;

}
