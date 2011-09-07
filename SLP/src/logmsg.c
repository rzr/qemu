/*
 * Copyright (C) 2010 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
 */


/**
 * @file logmsg.c
 * @brief - implementation file
 * @defgroup
 */

#include "logmsg.h"
#include <assert.h>

static const char *msglevel[7] = {
	"[ fatal ]",
	"[ error ]",
	"[warning]",
	"[ info  ]",
	"[ debug ]",
	"[**dbg1**]",
	"[**dbg2**]"
};
static int msg_level = MSGL_DEFAULT;
static char *daemon_name = NULL;

static char logfile[256] = { 0, };

static const char *log_basename(const char *file)
{
	int i = strlen(file);
	while (i > 0) {
		i--;
		if (file[i] == '\\' || file[i] == '/')
			return &file[i+1];
	}
	return file;
}


void real_log_msg(int level, const char *file, unsigned int line, const char *func, const char *format, ...)
{
	char tmp[MSGSIZE_MAX] = { 0, };
	char txt[MSGSIZE_MAX] = { 0, };
	char timeinfo[256] = { 0, };
	FILE *fp;
	struct tm *tm_time;
	struct timeval tval;
	va_list va;
	int len;

	if (level > msg_level)
		return;

	snprintf(tmp, MSGSIZE_MAX, "%s:%d %s -> ", log_basename(file), line, func);
	len = strlen(tmp);

	va_start(va, format);
	vsnprintf(tmp+len, MSGSIZE_MAX - len, format, va);
	va_end(va);

	/* when logfile no exit, just print */

	if (strlen(logfile) <= 0) {
		fprintf(stdout, "%s / %s", msglevel[level], tmp);
		return;
	}

	tmp[MSGSIZE_MAX - 2] = '\n';
	tmp[MSGSIZE_MAX - 1] = 0;

	if ((msg_level == MSGL_ALL) || (msg_level != MSGL_FILE))
		fprintf(stdout, "%s / %s", msglevel[level], tmp);

	if ((msg_level == MSGL_ALL) || (msg_level == MSGL_FILE)) {
		gettimeofday(&tval, NULL);
		tm_time = gmtime(&(tval.tv_sec));
		strftime(timeinfo, sizeof(timeinfo), "%Y/%m/%d %H:%M:%S", tm_time);

		sprintf(txt, "%s %s / %s", timeinfo, msglevel[level], tmp);
		if ((fp = fopen(logfile, "a+")) == NULL) {
			fprintf(stdout, "log file can't open. (%s)\n", logfile);
			return;
		}

		fputs(txt, fp);
		fclose(fp);

		return;
	}

	return;
}

/**
 * @brief 	log msg initialize
 * @param	mod: emulator, vinit
 * @date    Nov 18. 2008
 * */

void log_msg_init(int level)
{
	char *env = getenv("EMUL_VERBOSE");

	/* 1.1 when getting EMUL_VERBOSE env */

	if (env) {
		if (strstr(env, ":")) {
			daemon_name=strtok(env, ":");
			msg_level = atoi(strtok(NULL, ":"));
		}
		else {
			msg_level = atoi(env);
		}

		/* 1.1.1 check message level range */

		if (!(0 < msg_level && msg_level < 7) && msg_level != MSGL_FILE) {
			fprintf(stdout, "\n>>> debug_level must be 0,1,2,3,4,9 <otherwise: MSGL_INFO<3>\n");
			msg_level = MSGL_DEFAULT;
		}
		fprintf(stderr, "msglevel = %d\n", msg_level);
	}

	if (level) {
		msg_level = level;

		/* 1.1.1 check message level range */

		if (!(0 < msg_level && msg_level < 7) && msg_level != MSGL_FILE) {
			fprintf(stdout, "\n>>> debug_level must be 0,1,2,3,4,9 <otherwise: MSGL_INFO<3>\n");
			msg_level = MSGL_DEFAULT;
		}
	}

	if ((msg_level == MSGL_ALL) || (msg_level == MSGL_FILE)) {
		strcpy(logfile, get_path());
		strcat(logfile, "/emulator.log");
	}

	log_msg(MSGL_DEBUG, "log level init complete\n");

	return;
}
/**
 * vim:set tabstop=4 shiftwidth=4 foldmethod=marker wrap:
 *
 */


/**
 * @brief 	return log level
 * @date    Feb 23. 2009
 * */

int get_log_level(void)
{
	return msg_level;
}

