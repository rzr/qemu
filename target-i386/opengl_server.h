/*
 * opengl_server.h
 *
 *  Created on: 2011. 6. 30.
 *      Author: dhhong
 */

#ifndef OPENGL_SERVER_H_
#define OPENGL_SERVER_H_

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif	/* _WIN32 */

#ifdef _WIN32
#define socklen_t	int
typedef HDC Display;
#else
#define closesocket close
#endif

typedef struct {
	int port;
	int different_windows;
	int display_function_call;
	int must_save;
	int parent_xid;
	int refresh_rate;
	int timestamp; /* only valid if must_save == 1. include timestamps in the save file to enable real-time playback */
} OGLS_Opts;

typedef struct {
	int sock;
	int count_last_time, count_current;
	struct timeval last_time, current_time, time_stamp_start;
	struct timeval last_read_time, current_read_time;

	struct sockaddr_in clientname;
#ifdef _WIN32
	Display Display;
	HDC active_win; /* FIXME */
#else
	Display* parent_dpy;
	Window qemu_parent_window;

	Display* Display;
	Window active_win; /* FIXME */
#endif
	int active_win_x;
	int active_win_y;

	int local_connection;

	int last_assigned_internal_num;
	int last_active_internal_num;

	int  nTabAssocAttribListVisual ;
	void* tabAssocAttribListVisual ;
	void * processTab;

	OGLS_Opts *pOption;
} OGLS_Conn ;

/* opengl_server main function */
void *init_opengl_server(void *arg);

#endif /* OPENGL_SERVER_H_ */
