/*
 *  TCP/IP OpenGL server
 *
 *  Copyright (c) 2007 Even Rouault
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* gcc -Wall -O2 -g opengl_server.c opengl_exec.c -o opengl_server -I../i386-softmmu -I. -I.. -lGL */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define PORT    5555

#define ENABLE_GL_LOG

#include "opengl_func.h"
#include "opengl_utils.h"

static int refresh_rate = 1000;
static int must_save = 0;
static int timestamp = 1; /* only valid if must_save == 1. include timestamps in the save file to enable real-time playback */

extern int display_function_call;
extern void init_process_tab(void);
extern int do_function_call(Display*, int, int, long*, char*);
extern void opengl_exec_set_local_connection(void);
extern void opengl_exec_set_parent_window(Display* _dpy, Window _parent_window);

#ifdef ENABLE_GL_LOG
static FILE* f = NULL;

static char* filename = "/tmp/debug_gl.bin";

#define write_gl_debug_init() do { if (f == NULL) f = fopen(filename, "wb"); } while(0)

static void inline  write_gl_debug_cmd_char(char my_int)
{
	write_gl_debug_init();
	fwrite(&my_int, sizeof(my_int), 1, f);
}

static void inline  write_gl_debug_cmd_short(short my_int)
{
	write_gl_debug_init();
	fwrite(&my_int, sizeof(my_int), 1, f);
}

static void inline write_gl_debug_cmd_int(int my_int)
{
	write_gl_debug_init();
	fwrite(&my_int, sizeof(my_int), 1, f);
}

static void inline  write_gl_debug_cmd_longlong(long long my_longlong)
{
	write_gl_debug_init();
	fwrite(&my_longlong, sizeof(my_longlong), 1, f);
}

static void inline  write_gl_debug_cmd_buffer_with_size(int size, void* buffer)
{
	write_gl_debug_init();
	fwrite(&size, sizeof(int), 1, f);
	if (size)
		fwrite(buffer, size, 1, f);
}

static void inline  write_gl_debug_cmd_buffer_without_size(int size, void* buffer)
{
	write_gl_debug_init();
	if (size)
		fwrite(buffer, size, 1, f);
}

static void inline  write_gl_debug_end()
{
	write_gl_debug_init();
	fclose(f);
	f = NULL;
}

#endif

static void write_sock_data(int sock, void* data, int len)
{
	if (len && data)
	{
		int offset = 0;
		while(offset < len)
		{
			int nwritten = write(sock, data + offset, len - offset);
			if (nwritten == -1)
			{
				if (errno == EINTR)
					continue;
				perror("write");
				assert(nwritten != -1);
			}
			offset += nwritten;
		}
	}
}

static void inline write_sock_int(int sock, int my_int)
{
	write_sock_data(sock, &my_int, sizeof(int));
}

static int total_read = 0;
static void read_sock_data(int sock, void* data, int len)
{
	if (len)
	{
		int offset = 0;
		while(offset < len)
		{
			int nread = read(sock, data + offset, len - offset);
			if (nread == -1)
			{
				if (errno == EINTR)
					continue;
				perror("read");
				assert(nread != -1);
			}
			if (nread == 0)
			{
				fprintf(stderr, "nread = 0\n");
			}
			assert(nread > 0);
			offset += nread;
			total_read += nread;
		}
	}
}

static int inline read_sock_int(int sock)
{
	int ret;
	read_sock_data(sock, &ret, sizeof(int));
	return ret;
}

static short inline read_sock_short(int sock)
{
	short ret;
	read_sock_data(sock, &ret, sizeof(short));
	return ret;
}


static Display* dpy = NULL;
static int parent_xid = -1;


static struct timeval last_time, current_time, time_stamp_start;
static int count_last_time = 0, count_current = 0;

static struct timeval last_read_time, current_read_time;

int has_x_error = 0;

	int
read_from_client (int sock)
{
	long args[50];
	int args_size[50];
	char ret_string[32768];
	char command_buffer[65536*16];

	if (dpy == NULL)
	{
		init_process_tab();
		dpy = XOpenDisplay(NULL);
		if (parent_xid != -1)
		{
			opengl_exec_set_parent_window(dpy, parent_xid);
		}
	}

	int i;
	int func_number = read_sock_short(sock);

	Signature* signature = (Signature*)tab_opengl_calls[func_number];
	int ret_type = signature->ret_type;
	int nb_args = signature->nb_args;
	int* args_type = signature->args_type;
	int pid = 0;

	if (func_number == _serialized_calls_func)
	{
		int command_buffer_size = read_sock_int(sock);
		int commmand_buffer_offset = 0;
		read_sock_data(sock, command_buffer, command_buffer_size);

#ifdef ENABLE_GL_LOG
		if (must_save) write_gl_debug_cmd_short(_serialized_calls_func);
#endif

		while(commmand_buffer_offset < command_buffer_size)
		{
			func_number = *(short*)(command_buffer + commmand_buffer_offset);
			if( ! (func_number >= 0 && func_number < GL_N_CALLS) )
			{
				fprintf(stderr, "func_number >= 0 && func_number < GL_N_CALLS failed at "
						"commmand_buffer_offset=%d (command_buffer_size=%d)\n",
						commmand_buffer_offset, command_buffer_size);
				exit(-1);
			}

#ifdef ENABLE_GL_LOG
			if (must_save) write_gl_debug_cmd_short(func_number);
#endif
			commmand_buffer_offset += sizeof(short);


			signature = (Signature*)tab_opengl_calls[func_number];
			ret_type = signature->ret_type;
			assert(ret_type == TYPE_NONE);
			nb_args = signature->nb_args;
			args_type = signature->args_type;

			for(i=0;i<nb_args;i++)
			{
				switch(args_type[i])
				{
					case TYPE_UNSIGNED_CHAR:
					case TYPE_CHAR:
						{
							args[i] = *(int*)(command_buffer + commmand_buffer_offset);
#ifdef ENABLE_GL_LOG
							if (must_save) write_gl_debug_cmd_char(args[i]);
#endif
							commmand_buffer_offset += sizeof(int);
							break;
						}

					case TYPE_UNSIGNED_SHORT:
					case TYPE_SHORT:
						{
							args[i] = *(int*)(command_buffer + commmand_buffer_offset);
#ifdef ENABLE_GL_LOG
							if (must_save) write_gl_debug_cmd_short(args[i]);
#endif
							commmand_buffer_offset += sizeof(int);
							break;
						}

					case TYPE_UNSIGNED_INT:
					case TYPE_INT:
					case TYPE_FLOAT:
						{
							args[i] = *(int*)(command_buffer + commmand_buffer_offset);
#ifdef ENABLE_GL_LOG
							if (must_save) write_gl_debug_cmd_int(args[i]);
#endif
							commmand_buffer_offset += sizeof(int);
							break;
						}

					case TYPE_NULL_TERMINATED_STRING:
CASE_IN_UNKNOWN_SIZE_POINTERS:
						{
							args_size[i] = *(int*)(command_buffer + commmand_buffer_offset);
							commmand_buffer_offset += sizeof(int);

							if (args_size[i] == 0)
							{
								args[i] = 0;
							}
							else
							{
								args[i] = (long)(command_buffer + commmand_buffer_offset);
							}

							if (args[i] == 0)
							{
								if (!IS_NULL_POINTER_OK_FOR_FUNC(func_number))
								{
									fprintf(stderr, "call %s arg %d pid=%d\n", tab_opengl_calls_name[func_number], i, pid);
									return 0;
								}
							}
#ifdef ENABLE_GL_LOG
							if (must_save) write_gl_debug_cmd_buffer_with_size(args_size[i], (void*)args[i]);
#endif
							commmand_buffer_offset += args_size[i];

							break;
						}

CASE_IN_LENGTH_DEPENDING_ON_PREVIOUS_ARGS:
						{
							args_size[i] = compute_arg_length(stderr, func_number, i, args);
							args[i] = (args_size[i]) ? (long)(command_buffer + commmand_buffer_offset) : 0;
#ifdef ENABLE_GL_LOG
							if (must_save) write_gl_debug_cmd_buffer_without_size(args_size[i], (void*)args[i]);
#endif
							commmand_buffer_offset += args_size[i];
							break;
						}

CASE_OUT_POINTERS:
						{
							fprintf(stderr, "shouldn't happen TYPE_OUT_xxxx : call %s arg %d pid=%d\n", tab_opengl_calls_name[func_number], i, pid);
							return 0;
							break;
						}

					case TYPE_DOUBLE:
CASE_IN_KNOWN_SIZE_POINTERS:
						args[i] = (long)(command_buffer + commmand_buffer_offset);
						args_size[i] = tab_args_type_length[args_type[i]];
#ifdef ENABLE_GL_LOG
						if (must_save) write_gl_debug_cmd_buffer_without_size(tab_args_type_length[args_type[i]], (void*)args[i]);
#endif
						commmand_buffer_offset += tab_args_type_length[args_type[i]];
						break;

					case TYPE_IN_IGNORED_POINTER:
						args[i] = 0;
						break;

					default:
						fprintf(stderr, "shouldn't happen : call %s arg %d pid=%d\n", tab_opengl_calls_name[func_number], i, pid);
						return 0;
						break;
				}
			}

			if (display_function_call) display_gl_call(stderr, func_number, args, args_size);

			do_function_call(dpy, func_number, 1, args, ret_string);
		}
	}
	else
	{
#ifdef ENABLE_GL_LOG
		if (must_save && func_number != _synchronize_func) write_gl_debug_cmd_short(func_number);
#endif

		for(i=0;i<nb_args;i++)
		{
			switch(args_type[i])
			{
				case TYPE_UNSIGNED_CHAR:
				case TYPE_CHAR:
					args[i] = read_sock_int(sock);
#ifdef ENABLE_GL_LOG
					if (must_save) write_gl_debug_cmd_char(args[i]);
#endif
					break;

				case TYPE_UNSIGNED_SHORT:
				case TYPE_SHORT:
					args[i] = read_sock_int(sock);
#ifdef ENABLE_GL_LOG
					if (must_save) write_gl_debug_cmd_short(args[i]);
#endif
					break;

				case TYPE_UNSIGNED_INT:
				case TYPE_INT:
				case TYPE_FLOAT:
					args[i] = read_sock_int(sock);
#ifdef ENABLE_GL_LOG
					if (must_save) write_gl_debug_cmd_int(args[i]);
#endif
					break;

				case TYPE_NULL_TERMINATED_STRING:
CASE_IN_UNKNOWN_SIZE_POINTERS:
					{
						args_size[i] = read_sock_int(sock);
						if (args_size[i])
						{
							args[i] = (long)malloc(args_size[i]);
							read_sock_data(sock, (void*)args[i], args_size[i]);
						}
						else
						{
							args[i] = 0;
							if (!IS_NULL_POINTER_OK_FOR_FUNC(func_number))
							{
								fprintf(stderr, "call %s arg %d\n", tab_opengl_calls_name[func_number], i);
								return 0;
							}
						}
#ifdef ENABLE_GL_LOG
						if (must_save) write_gl_debug_cmd_buffer_with_size(args_size[i], (void*)args[i]);
#endif
						break;
					}

CASE_IN_LENGTH_DEPENDING_ON_PREVIOUS_ARGS:
					{
						args_size[i] = compute_arg_length(stderr, func_number, i, args);
						args[i] = (args_size[i]) ? (long)malloc(args_size[i]) : 0;
						read_sock_data(sock, (void*)args[i], args_size[i]);
#ifdef ENABLE_GL_LOG
						if (must_save) write_gl_debug_cmd_buffer_without_size(args_size[i], (void*)args[i]);
#endif
						break;
					}

CASE_OUT_LENGTH_DEPENDING_ON_PREVIOUS_ARGS:
					{
						args_size[i] = compute_arg_length(stderr, func_number, i, args);
						args[i] = (long)malloc(args_size[i]);
						break;
					}

CASE_OUT_UNKNOWN_SIZE_POINTERS:
					{
						args_size[i] = read_sock_int(sock);
						if (func_number == glGetProgramLocalParameterdvARB_func)
						{
							fprintf(stderr, "size = %d\n", args_size[i]);
						}
						if (args_size[i])
						{
							args[i] = (long)malloc(args_size[i]);
						}
						else
						{
							if (!IS_NULL_POINTER_OK_FOR_FUNC(func_number))
							{
								fprintf(stderr, "call %s arg %d pid=%d\n", tab_opengl_calls_name[func_number], i, pid);
								return 0;
							};
							args[i] = 0;
						}
						//fprintf(stderr, "%p %d\n", (void*)args[i], args_size[i]);
#ifdef ENABLE_GL_LOG
						if (must_save) write_gl_debug_cmd_int(args_size[i]);
#endif
						break;
					}

CASE_OUT_KNOWN_SIZE_POINTERS:
					{
						args_size[i] = tab_args_type_length[args_type[i]];
						assert(args_size[i]);
						args[i] = (long)malloc(args_size[i]);
						//fprintf(stderr, "%p %d\n", (void*)args[i], args_size[i]);
						break;
					}

				case TYPE_DOUBLE:
CASE_IN_KNOWN_SIZE_POINTERS:
					args_size[i] = tab_args_type_length[args_type[i]];
					args[i] = (long)malloc(args_size[i]);
					read_sock_data(sock, (void*)args[i], args_size[i]);
#ifdef ENABLE_GL_LOG
					if (must_save) write_gl_debug_cmd_buffer_without_size(tab_args_type_length[args_type[i]], (void*)args[i]);
#endif
					break;

				case TYPE_IN_IGNORED_POINTER:
					args[i] = 0;
					break;

				default:
					fprintf(stderr, "shouldn't happen : call %s arg %d\n", tab_opengl_calls_name[func_number], i);
					return 0;
					break;
			}
		}

		if (display_function_call) display_gl_call(stderr, func_number, args, args_size);

		if (getenv("ALWAYS_FLUSH")) fflush(f);

		int ret = do_function_call(dpy, func_number, 1, args, ret_string);
#ifdef ENABLE_GL_LOG
		if (must_save && func_number == glXGetVisualFromFBConfig_func)
		{
			write_gl_debug_cmd_int(ret);
		}
#endif

		for(i=0;i<nb_args;i++)
		{
			switch(args_type[i])
			{
				case TYPE_UNSIGNED_INT:
				case TYPE_INT:
				case TYPE_UNSIGNED_CHAR:
				case TYPE_CHAR:
				case TYPE_UNSIGNED_SHORT:
				case TYPE_SHORT:
				case TYPE_FLOAT:
					break;

				case TYPE_NULL_TERMINATED_STRING:
				case TYPE_DOUBLE:
CASE_IN_POINTERS:
					if (args[i]) free((void*)args[i]);
					break;

CASE_OUT_POINTERS:
					//fprintf(stderr, "%p %d\n", (void*)args[i], args_size[i]);
					write_sock_data(sock, (void*)args[i], args_size[i]);
					if (display_function_call)
					{
						if (args_type[i] == TYPE_OUT_1INT)
						{
							fprintf(stderr, "out[%d] : %d\n", i, *(int*)args[i]);
						}
						else if (args_type[i] == TYPE_OUT_1FLOAT)
						{
							fprintf(stderr, "out[%d] : %f\n", i, *(float*)args[i]);
						}
					}
					if (args[i]) free((void*)args[i]);
					break;

				case TYPE_IN_IGNORED_POINTER:
					args[i] = 0;
					break;

				default:
					fprintf(stderr, "shouldn't happen : call %s arg %d\n", tab_opengl_calls_name[func_number], i);
					return 0;
					break;
			}
		}

		if (signature->ret_type == TYPE_CONST_CHAR)
		{
			write_sock_int(sock, strlen(ret_string) + 1);
			write_sock_data(sock, ret_string, strlen(ret_string) + 1);
		}
		else if (signature->ret_type != TYPE_NONE)
		{
			write_sock_int(sock, ret);
		}

#ifdef ENABLE_GL_LOG
		if (must_save && func_number == _exit_process_func)
		{
			write_gl_debug_end();
		}
#endif
		if (func_number == _exit_process_func)
		{
			return -1;
		}
		else if (func_number == glXSwapBuffers_func)
		{
			int diff_time;
			count_current++;
			gettimeofday(&current_time, NULL);
#ifdef ENABLE_GL_LOG
			if (must_save && timestamp)
			{
				long long ts = (current_time.tv_sec - time_stamp_start.tv_sec) * (long long)1000000 + current_time.tv_usec - time_stamp_start.tv_usec;
				/* -1 is special code that indicates time synchro */
				write_gl_debug_cmd_short(timesynchro_func);
				write_gl_debug_cmd_longlong(ts);
			}
#endif
			diff_time = (current_time.tv_sec - last_time.tv_sec) * 1000 + (current_time.tv_usec - last_time.tv_usec) / 1000;
			if (diff_time > refresh_rate)
			{
#ifdef ENABLE_GL_LOG
				fflush(f);
#endif
				printf("%d frames in %.1f seconds = %.3f FPS\n",
						count_current - count_last_time,
						diff_time / 1000.,
						(count_current - count_last_time) * 1000. / diff_time);
				last_time.tv_sec = current_time.tv_sec;
				last_time.tv_usec = current_time.tv_usec;
				count_last_time = count_current;
			}
		}
	}
	return 0;
}

	int
make_socket (uint16_t port)
{
	int sock;
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	/* Give the socket a name. */
	name.sin_family = AF_INET;
	name.sin_port = htons (port);
	name.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
	{
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	return sock;
}

static int x_error_handler(Display     *display,
		XErrorEvent *error)
{
	char buf[64];
	XGetErrorText(display, error->error_code, buf, 63);
	fprintf (stderr, "The program received an X Window System error.\n"
			"This probably reflects a bug in the program.\n"
			"The error was '%s'.\n"
			"  (Details: serial %ld error_code %d request_code %d minor_code %d)\n",
			buf,
			error->serial,
			error->error_code,
			error->request_code,
			error->minor_code);
	has_x_error = 1;
	return 0;
}

void usage()
{
	printf("Usage : opengl_server [OPTION]\n\n");
	printf("The following options are available :\n");
	printf("--port=XXXX         : set XXX as the port number for the TCP/IP server (default : 5555)\n");
	printf("--debug             : output debugging trace on stderr\n");
	printf("--save              : dump the serialialized OpenGL flow in a file (default : /tmp/debug_gl.bin)\n");
	printf("--filename=X        : the file where to write the serailized OpenGL flow\n");
	printf("--different-windows : even if the client is on 127.0.0.1, display OpenGL on a new X window\n");
	printf("--parent-xid=XXX    : use XXX as the XID of the parent X window where to display the OpenGL flow\n");
	printf("                     This is useful if you want to run accelerated OpenGL inside a non-patched QEMU\n");
	printf("                     or from another emulator, through TCP/IP\n");
	printf("--h or --help       : display this help\n");
}

	int
main (int argc, char* argv[])
{
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	socklen_t size;
	int port = PORT;
	int different_windows = 0;

	for(i=1;i<argc;i++)
	{
		if (argv[i][0] == '-' && argv[i][1] == '-')
			argv[i] = argv[i]+1;

		if (strcmp(argv[i], "-debug") == 0)
		{
			display_function_call = 1;
		}
		else if (strcmp(argv[i], "-save") == 0)
		{
			must_save = 1;
		}
		else if (strncmp(argv[i], "-port=",6) == 0)
		{
			port = atoi(argv[i] + 6);
		}
		else if (strncmp(argv[i], "-filename=",strlen("-filename=")) == 0)
		{
			filename = argv[i] + strlen("-filename=");
		}
		else if (strncmp(argv[i], "-parent-xid=",strlen("-parent-xid=")) == 0)
		{
			char* c = argv[i] + strlen("-parent-xid=");
			parent_xid = strtol(c, NULL, 0);
			different_windows = 1;
		}
		else if (strcmp(argv[i], "-different-windows") == 0)
		{
			different_windows = 1;
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0)
		{
			usage();
			return 0;
		}
		else
		{
			fprintf(stderr, "unknown parameter : %s\n", argv[i]);
			usage();
			return -1;
		}
	}

	/* Create the socket and set it up to accept connections. */
	sock = make_socket (port);

	int flag = 1;
	if (setsockopt(sock, IPPROTO_IP, SO_REUSEADDR,(char *)&flag, sizeof(int)) != 0)
	{
		perror("setsockopt SO_REUSEADDR");
	}
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,(char *)&flag, sizeof(int)) != 0)
	{
		perror("setsockopt TCP_NODELAY");
	}

	if (listen (sock, 1) < 0)
	{
		perror ("listen");
		exit (EXIT_FAILURE);
	}

	struct sigaction action;
	action.sa_handler = SIG_IGN;
	action.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD,&action,NULL);

	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);

	while(1)
	{
		int new, pid;

		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
		{
			perror ("select");
			exit (EXIT_FAILURE);
		}

		size = sizeof (clientname);
		new = accept (sock, (struct sockaddr *) &clientname, &size);
		if (new < 0)
		{
			perror ("accept");
			exit (EXIT_FAILURE);
		}
		pid = fork();
		if (pid == -1)
		{
			perror ("fork");
			exit(EXIT_FAILURE);
		}
		if (pid == 0)
		{
			close(sock);

			fprintf (stderr, "Server: connect from host %s, port %hd.\n",
					inet_ntoa (clientname.sin_addr),
					ntohs (clientname.sin_port));

			gettimeofday(&last_time, NULL);
			gettimeofday(&last_read_time, NULL);

			if (strcmp(inet_ntoa(clientname.sin_addr), "127.0.0.1") == 0 &&
					different_windows == 0)
			{
				opengl_exec_set_local_connection();
			}

			if (timestamp)
			{
				gettimeofday(&time_stamp_start, NULL);
			}

			XSetErrorHandler(x_error_handler);

			while(1)
			{
				if (read_from_client (new) < 0)
				{
					do_function_call(dpy, _exit_process_func, 1, NULL, NULL);

					fprintf (stderr, "Server: disconnect from host %s, port %hd.\n",
							inet_ntoa (clientname.sin_addr),
							ntohs (clientname.sin_port));

					return 0;
				}
			}
		}
		else
		{
			close(new);
		}
	}

	return 0;
}
