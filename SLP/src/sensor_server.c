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

#include "emulator.h"
#define UDP

extern int sensor_update(uint16_t x, uint16_t y, uint16_t z);

#define DEFAULT_SENSOR_PORT 3581
#define SENSORD_PORT		3580

int sensord_initialized = 0;
int sent_init_value = 0;

void *init_sensor_server(void *arg);
int sensor_parser(char *buffer);
int parse_val(char *buff, unsigned char data, char *parsbuf);

void *init_sensor_server(void *arg)
{
	int parse_result;
	int listen_s;			/* listening socket */
#ifdef TCP
    int client_s;			/* Client socket  */
#endif
	int sensord_s;
#ifndef _WIN32
	socklen_t client_len = (socklen_t)sizeof(struct sockaddr_in);
	socklen_t sensord_len = (socklen_t)sizeof(struct sockaddr_in);
#else
	int client_len = (int)sizeof(struct sockaddr_in);
	int sensord_len = (int)sizeof(struct sockaddr_in);
#endif
	int port_n, port = *((int *)arg);	 		/* sensor port number */
	char recv_buf[32];
	char send_buf[32];
	struct sockaddr_in servaddr;
	struct sockaddr_in clientaddr;
	struct sockaddr_in sensordaddr;

#ifdef __MINGW32__
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
        printf("Error creating socket.");
        return NULL;
    }
#endif

	if(port !=0 )
		port_n = port;
	else
		port_n = DEFAULT_SENSOR_PORT;
#ifdef TCP
	if((listen_s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
#else
	if((listen_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
#endif
	{
		perror("Create listen socket error: ");
        goto clean_up;
	}

	if((sensord_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("Create send socket error: ");
		goto clean_up;
	}
	
	memset(&servaddr, '\0', sizeof(servaddr));
	memset(&clientaddr,'\0', sizeof(clientaddr));
	memset(&sensordaddr,'\0', sizeof(sensordaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(port_n);

	sensordaddr.sin_family = AF_INET;
	sensordaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sensordaddr.sin_port = htons(SENSORD_PORT);
	
	if(bind(listen_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind error: ");
        goto clean_up;
	}
#ifdef TCP	
	if(listen(listen_s, 3) == -1)
	{
		perror("Listen error: ");
        goto clean_up;
	}
#endif
	while(1)
	{
#ifdef TCP
		client_s = accept(listen_s, (struct sockaddr *)&clientaddr, &client_len);
		if(read(client_s, recv_buf, 32) <= 0)
		{
			perror("Read error: ");
#ifndef __MINGW32__
			closesocket(client_s);
#else
			close(client_s);
#endif
            goto clean_up;
		}
#else
		if(recvfrom(listen_s, recv_buf, 32, 0, (struct sockaddr *)&clientaddr, &client_len) <= 0)
            continue;
#endif
	    parse_result = sensor_parser(recv_buf);

		if(sensord_initialized)
		{
			if(sent_init_value == 0)
			{
				sent_init_value = 1;
				sprintf(send_buf, "1\n3\n0\n-9.80665\n0\n");
				if(sendto(sensord_s, send_buf, 32, 0, (struct sockaddr *)&sensordaddr, sensord_len) <= 0)
				{
					perror("Send error: "); 
					goto clean_up;
				}
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
				if(sendto(sensord_s, send_buf, 32, 0, (struct sockaddr *)&sensordaddr, sensord_len) <= 0)
				{
					perror("Send error: "); 
					goto clean_up;
				}
			}
		}
	}

clean_up:
#ifdef __MINGW32__
    if( listen_s )
        closesocket(listen_s);
	if( sensord_s )
		closesocket(sensord_s);
    WSACleanup();
#else
    if( listen_s )
        close(listen_s);
	if( sensord_s )
		close(sensord_s);
#endif

    return NULL;
}

int sensor_parser(char *buffer)
{
	int len = 0;
	char tmpbuf[32];
	int rotation;
	int from_skin;

#ifdef SENSOR_DEBUG
	printf("read data: %s\n", buffer);
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
			switch(rotation)
			{
			case 0:
				if(UISTATE.current_mode != 0)
					rotate_event_callback(&PHONE, 0);
				break;
			case 90:
				if(UISTATE.current_mode != 1)
					rotate_event_callback(&PHONE, 1);
				break;
			case 180:
				if(UISTATE.current_mode != 2)
					rotate_event_callback(&PHONE, 2);
				break;
			case 270:
				if(UISTATE.current_mode != 3)
					rotate_event_callback(&PHONE, 3);
				break;
			default:
				assert(0);
			}
		}
	}
	else
	{
//		if(from_skin)
//			show_message("Warning", "Sensor server is not ready!\nYou cannot rotate the emulator until sensor server is ready.");
		return -1;
	}

	if(from_skin)
		return rotation;
	else
		return 1;
//	sensor_update(x,y,z);
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
