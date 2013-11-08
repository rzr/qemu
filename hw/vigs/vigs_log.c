/*
 * vigs
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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

#include "vigs_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static const char *g_log_level_to_str[vigs_log_level_max + 1] =
{
    "OFF",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

static vigs_log_level g_log_level = vigs_log_level_off;

static void vigs_log_print_current_time(void)
{
    char buff[128];
#ifdef _WIN32
    struct tm *ptm;
#else
    struct tm tm;
#endif
    qemu_timeval tv = { 0, 0 };
    time_t ti;

    qemu_gettimeofday(&tv);

    ti = tv.tv_sec;

#ifdef _WIN32
    ptm = localtime(&ti);
    strftime(buff, sizeof(buff),
             "%H:%M:%S", ptm);
#else
    localtime_r(&ti, &tm);
    strftime(buff, sizeof(buff),
             "%H:%M:%S", &tm);
#endif
    fprintf(stderr, "%s", buff);
}

void vigs_log_init(void)
{
    char *level_str = getenv("VIGS_DEBUG");
    int level = level_str ? atoi(level_str) : vigs_log_level_off;

    if (level < 0) {
        g_log_level = vigs_log_level_off;
    } else if (level > vigs_log_level_max) {
        g_log_level = (vigs_log_level)vigs_log_level_max;
    } else {
        g_log_level = (vigs_log_level)level;
    }
}

void vigs_log_cleanup(void)
{
    g_log_level = vigs_log_level_off;
}

void vigs_log_event(vigs_log_level log_level,
                    const char* func,
                    int line,
                    const char* format, ...)
{
    va_list args;

    vigs_log_print_current_time();
    fprintf(stderr,
            " %-5s %s:%d",
            g_log_level_to_str[log_level],
            func,
            line);
    if (format) {
        va_start(args, format);
        fprintf(stderr, " - ");
        vfprintf(stderr, format, args);
        va_end(args);
    }
    fprintf(stderr, "\n");
}

bool vigs_log_is_enabled_for_level(vigs_log_level log_level)
{
    return log_level <= g_log_level;
}
