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

#ifndef __SENSOR_H_
#define __SENSOR_H_
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "sensor_server.h"
#include "emulator.h"
#include "sdb.h"
#define UDP

extern int sensor_update(uint16_t x, uint16_t y, uint16_t z);

#define SENSORD_PORT		3580

int sensord_initialized = 0;
int sent_init_value = 0;

int sensor_parser(char *buffer);
int parse_val(char *buff, unsigned char data, char *parsbuf);

void *init_sensor_server(void)
{
	int listen_s;
	uint16_t port;
	struct sockaddr_in servaddr;
	char buf[32] = {0};
	GIOChannel *channel = NULL;
	GError *error;
	GIOStatus status;

#ifdef __MINGW32__	
	WSADATA wsadata;
	if(WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
		fprintf(stderr, "Error creating socket.\n");
		return NULL;
	}
#endif

	/* ex: 26100 + 3/udp */
	port = get_sdb_base_port() + SDB_UDP_SENSOR_INDEX;

	if((listen_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Create listen socket error: ");
		goto cleanup;
	}

	memset(&servaddr, '\0', sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(port);

	fprintf(stderr, "bind port[127.0.0.1:%d/udp] for sensor server in host \n", port);
	if(bind(listen_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		fprintf(stderr, "bind fail [%d:%s] \n", errno, strerror(errno));
		perror("bind error: ");
		goto cleanup;
	}

	channel = g_io_channel_unix_new(listen_s);
	if(channel == NULL)
	{
		fprintf(stderr, "gnet_udp_socket_get_io_channel failed\n");
		goto cleanup;
	}

	//	status = g_io_channel_set_encoding(channel, NULL, NULL);
	//	if(status != G_IO_STATUS_NORMAL)
	//	{
	//		fprintf(stderr, "encoding error %d %s\n", status, error->message);
	//		goto cleanup;
	//	}
	g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL);

	guint sourceid = g_io_add_watch(channel, G_IO_IN|G_IO_ERR|G_IO_HUP, sensor_server, NULL);

	if(sourceid <= 0)
	{
		fprintf(stderr, "g_io_add_watch() failed\n");
		g_io_channel_unref(channel);
		goto cleanup;
	}

	return NULL;

cleanup:
#ifdef __MINGW32__
	if(listen_s)
		closesocket(listen_s);
	WSACleanup();
#else
	if(listen_s)
		close(listen_s);
#endif

	return NULL;
}


int send_info_to_sensor_daemon(char *send_buf, int buf_size)
{
	int   s;  

	fprintf(stderr, "[%s][%d] start \n", __FUNCTION__, __LINE__);

	s = tcp_socket_outgoing("127.0.0.1", (uint16_t)(get_sdb_base_port() + SDB_TCP_SENSOR_INDEX)); 
	if (s < 0) {
		fprintf(stderr, "can't create socket to talk to the sdb forwarding session \n");
		fprintf(stderr, "[127.0.0.1:%d/tcp] connect fail (%d:%s)\n"
				, get_sdb_base_port() + SDB_TCP_SENSOR_INDEX
				, errno, strerror(errno)
			   );
		return -1;
	}   

	socket_send(s, "sensor\n\n\n\n", 10);
	fprintf(stderr, "[%s][%d] 10 byte sedn \n", __FUNCTION__, __LINE__);

	socket_send(s, &buf_size, 4); 
	fprintf(stderr, "[%s][%d] 4 byte sedn \n", __FUNCTION__, __LINE__);

	socket_send(s, send_buf, buf_size);
	fprintf(stderr, "send(size: %d) te 127.0.0.1:%d/tcp \n"
			, buf_size, get_sdb_base_port() + 3);

#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif

	fprintf(stderr, "[%s][%d] end \n", __FUNCTION__, __LINE__);

	return 1;
}


gboolean sensor_server(GIOChannel *channel, GIOCondition condition, gpointer data)
{
	int parse_result;

	GError *error;
	GIOStatus status;
	GIOError ioerror;

	gsize len;
	char recv_buf[32] = {0};
	char send_buf[32] = {0};

	fprintf(stderr, "[%s][%d] callback start \n", __FUNCTION__, __LINE__);

#ifdef __MINGW32__
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
		fprintf(stderr, "[%s][%d] Error creating socket  \n", __FUNCTION__, __LINE__);
		g_io_channel_unref(channel);
		g_io_channel_shutdown(channel, TRUE, NULL);
		return FALSE;
	}
#endif

	if((condition == G_IO_IN))
	{
		//status = g_io_channel_read_chars(channel, recv_buf, 32, &len, &error);
		ioerror = g_io_channel_read(channel, recv_buf, 32, &len);
		//if(status != G_IO_STATUS_NORMAL)
		if(ioerror != G_IO_ERROR_NONE)
		{
			fprintf(stderr, "[%s][%d] recv() failed %d \n", __FUNCTION__, __LINE__, ioerror);
			goto clean_up;
		}
		else
		{
			parse_result = sensor_parser(recv_buf);

			if(sensord_initialized)
			{
				if(sent_init_value == 0)
				{
					sent_init_value = 1;

					sprintf(send_buf, "1\n3\n0\n-9.80665\n0\n");

#if 1
					/* new way with sdb */
					if( send_info_to_sensor_daemon(send_buf, 32) <= 0 )
					{
						fprintf(stderr, "[%s][%d] send error \n", __FUNCTION__, __LINE__);
						fprintf(stderr, "[%s][%d] not clean_up \n", __FUNCTION__, __LINE__);
						//goto clean_up;
					}
#else
					/* old way */
					if(sendto(sensord_s, send_buf, 32, 0, (struct sockaddr *)&sensordaddr, sensord_len) <= 0)
					{
						perror("Send error: "); 
						goto clean_up;
					}
#endif
				}

				switch(parse_result)
				{
					case 0:
						sprintf(send_buf, "1\n3\n0\n-9.80665\n0\n");
						break;
					case 90:
						sprintf(send_buf, "1\n3\n-9.80665\n0\n0\n");
						break;
					case 180:
						sprintf(send_buf, "1\n3\n0\n9.80665\n0\n");
						break;
					case 270:
						sprintf(send_buf, "1\n3\n9.80665\n0\n0\n");
						break;
					case 7:
						sprintf(send_buf, "%s", recv_buf); 
						break;
				}

				if(parse_result != 1 && parse_result != -1)
				{
#if 1
					/* new way with sdb */
					if( send_info_to_sensor_daemon(send_buf, 32) <= 0 )
					{
						perror("Send error: ");
						goto clean_up;
					}

#else
					/* old way */
					if(sendto(sensord_s, send_buf, 32, 0, (struct sockaddr *)&sensordaddr, sensord_len) <= 0)
					{
						perror("Send error: "); 
						goto clean_up;
					}
#endif
				}
			}
			return TRUE;
		}
	}
	else if((condition == G_IO_ERR) || (condition == G_IO_HUP))
	{
		fprintf(stderr, "[%s][%d] G_IO_ERR | G_IO_HUP received  \n", __FUNCTION__, __LINE__);
	}

clean_up:
#ifdef __MINGW32__
	WSACleanup();
#endif
	g_io_channel_unref(channel);
	g_io_channel_shutdown(channel, TRUE, NULL);

	return FALSE;
}


int sensor_parser(char *buffer)
{
	int len = 0;
	char tmpbuf[32];
	int rotation;
	int from_skin;
	int base_mode = 0;

#ifdef SENSOR_DEBUG
	fprintf(stderr, "read data: %s\n", buffer);
#endif
	/* start */
	memset(tmpbuf, '\0', sizeof(tmpbuf));
	len = parse_val(buffer, 0x0a, tmpbuf);

	// packet from skin 
	if(strcmp(tmpbuf, "1\n") == 0)
	{
		from_skin = 1;
	}
	else if(strcmp(tmpbuf, "2\n") == 0) // packet from EI -> sensor plugin
	{
		from_skin = 0;
	}
	else if(strcmp(tmpbuf, "3\n") == 0) // packet from sensord
	{
		sensord_initialized = 1;
		return 1;
	}
	else if(strcmp(tmpbuf, "7\n") == 0) // keyboard on/off
	{
		return 7;
	}
	else
	{
		perror("bad data");
		return -1;
	}

	memset(tmpbuf, '\0', sizeof(tmpbuf));
	len += parse_val(buffer+len, 0x0a, tmpbuf);

	rotation = atoi(tmpbuf);

	if(sensord_initialized == 1)
	{
		if(from_skin == 0)
		{
			if(UISTATE.current_mode > 3)
				base_mode = 4;

			switch(rotation)
			{
				case 0:
					if(UISTATE.current_mode %4 != 0)
						rotate_event_callback(&PHONE, base_mode + 0);
					break;
				case 90:
					if(UISTATE.current_mode %4 != 1)
						rotate_event_callback(&PHONE, base_mode + 1);
					break;
				case 180:
					if(UISTATE.current_mode %4 != 2)
						rotate_event_callback(&PHONE, base_mode + 2);
					break;
				case 270:
					if(UISTATE.current_mode %4 != 3)
						rotate_event_callback(&PHONE, base_mode + 3);
					break;
				default:
					assert(0);
			}
		}
	}
	else
	{
		if(from_skin)
			show_message("Warning", "Sensor server is not ready!\nYou cannot rotate the emulator until sensor server is ready.");
		return -1;
	}

	if(from_skin)
		return rotation;
	else
		return 1;
}

int parse_val(char *buff, unsigned char data, char *parsbuf)
{
	int count = 0;
	while(1)
	{
		if(count > 12)
			return -1;
		if(buff[count] == data)
		{
			count++;
			strncpy(parsbuf, buff, count);
			return count;	
		}
		count++;
	}

	return 0;
}
#if 0
int main(int argc, char *args[])
{
	init_sensor_server(0);
}
#endif
#endif
