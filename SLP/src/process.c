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



#include "process.h"
#ifndef _WIN32
#include <wait.h>
#endif
#include <assert.h>

#define	SHELL	"/bin/sh"

struct pid_list {
	pid_t pid;
	int fd;
	struct pid_list *next;
};

static struct pid_list *pid_list_head;


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


static pid_t pid_from_fd(int fd, int delete)
{
	struct pid_list **p;

	for (p = &pid_list_head; *p; p = &((*p)->next)) {
		if ((*p)->fd == fd) {
			int pid = (*p)->pid;
			if (delete) {
				struct pid_list *tmp = *p;
				*p = (*p)->next;
				free(tmp);
			}
			return pid;
		}
	}
	return 0;
}


static void close_all_pid_fds(void)
{
	struct pid_list *p;

	for (p = pid_list_head; p; ) {
		struct pid_list *tmp = p->next;
		free(p);
		p = tmp;
	}
}


static void save_pid(int fd, pid_t pid)
{
	struct pid_list *p;
	p = malloc(sizeof *p);
	assert(p);
	p->fd = fd;
	p->pid = pid;
	p->next = pid_list_head;
	pid_list_head = p;
}


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

		if (access(dir, R_OK) != 0) 
#ifndef _WIN32
			mkdir(dir, S_IRWXU | S_IRWXG);
#else
			mkdir(dir);		
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

int write_pidfile(const char *filename)
{
	int		fd = -1;
	char	buf[128] = "";
	char	pidfname[512] = ""; 

	const gchar *conf_path = get_conf_path();
	sprintf(pidfname, "%s/emulator.pid", conf_path);

	if (access(conf_path, R_OK) != 0) {
		make_pid_path(pidfname);
	}

	if ((fd = open(pidfname, O_RDWR | O_CREAT, 0666)) < 0) {
		log_msg(MSGL_ERROR, "%s at %s(%d)\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}

	/* 기존 내용 삭제 */
	
	ftruncate(fd, 0);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", (int)getpid());
	write(fd, buf, strlen(buf));

#ifndef _WIN32
	chmod(pidfname, S_IRWXU | S_IRWXG | S_IRWXO);
#else
	chmod(pidfname, S_IRWXU);
#endif
	close(fd);

	return 0;
}

/**
 * @brief 	wirte pid file
 * @param	mod: emulator, vinit
 * @date    Nov 25. 2008
 * */

int remove_pidfile(const char *filename)
{
	char	pidfname[512] = ""; 

	if (filename == NULL) {
		return -1;
	}

	const gchar *conf_path = get_conf_path();
	sprintf(pidfname, "%s/emulator.pid", conf_path);

	if (strlen(pidfname) <= 0) {
		return -1;
	}

	if (remove (pidfname) < 0) {
		log_msg(MSGL_ERROR, "Can't remove pid file. (%s)\n", pidfname);
	}

	return 0;
}


/**
 * @brief 	popen for finding error
 * @param	cmdstring
 * @param	type
 * @return 	FILE
 * */

FILE *
popen_err(const char *cmdstring, const char *type)
{
	FILE	*fp;
#ifndef _WIN32
	int	pfd[2];
	pid_t	pid;

	/* only allow "r" or "w" */
	if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
		errno = EINVAL;		/* required by POSIX.2 */
		return(NULL);
	}

	if (pipe(pfd) < 0)
		return(NULL);	/* errno set by pipe() */

	if ( (pid = fork()) < 0)
		return(NULL);	/* errno set by fork() */
	else if (pid == 0) {							/* child */

		if (*type == 'r') {
			close(pfd[0]);
			if (pfd[1] != STDERR_FILENO) {
				dup2(pfd[1], STDERR_FILENO);
				close(pfd[1]);
			}
		} else {
			close(pfd[1]);
			if (pfd[0] != STDIN_FILENO) {
				dup2(pfd[0], STDIN_FILENO);
				close(pfd[0]);
			}
		}
		close_all_pid_fds();
		execl(SHELL, "sh", "-c", cmdstring, (char *) 0);
		_exit(127);
	}

	/* parent */
	if (*type == 'r') {
		close(pfd[1]);
		if ( (fp = fdopen(pfd[0], type)) == NULL)
			return(NULL);
	} else {
		close(pfd[0]);
		if ( (fp = fdopen(pfd[1], type)) == NULL)
			return(NULL);
	}
	save_pid(fileno(fp), pid);	/* remember child pid for this fd */
#endif
	return(fp);
}

/**
 * @brief 	pclose for finding error
 * @param 	FILE
 * */
int pclose_err(FILE *fp)
{
	int		fd, stat;
	pid_t	pid;
#ifndef _WIN32
	fd = fileno(fp);
	if ( (pid = pid_from_fd(fd, 1)) == 0)
		return(-1);		/* fp wasn't opened by popen() */

	if (fclose(fp) == EOF)
		return(-1);

	while (waitpid(pid, &stat, 0) < 0)
		if (errno != EINTR)
			return(-1);	/* error other than EINTR from waitpid() */
#else
/*
	while(WaitForSingleObject(handle, INFINITE) == WAIT_FAILED)
	{
		printf("WaitForSingleObject FAILED: err=%d\n", GetLastError());
       	return (-1);
	}
*/
#endif
	return(stat);	/* return child's termination status */
}
