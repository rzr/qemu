/*
 * New debug channel
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * Munkyu Im <munkyu.im@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 * refer to debug_ch.c
 * Copyright 2000 Alexandre Julliard
 *
 */

#include "qemu-common.h"

#include "emul_state.h"
#include "util/new_debug_ch.h"
#include "util/osutil.h"

#define MAX_FILE_LEN    512
static char debugchfile[MAX_FILE_LEN] = {0, };
static int fd = STDOUT_FILENO;

static const char * const debug_classes[] =
        {"SEVERE", "WARNING", "INFO", "CONFIG", "FINE", "TRACE"};

#define MAX_DEBUG_OPTIONS 256

static unsigned char default_flags =
        (1 << __DBCL_SEVERE) | (1 << __DBCL_WARNING) | (1 << __DBCL_INFO);
static int nb_debug_options = -1;
static struct _debug_channel debug_options[MAX_DEBUG_OPTIONS];

static void debug_init(void);

static int cmp_name(const void *p1, const void *p2)
{
    const char *name = p1;
    const struct _debug_channel *chan = p2;

    return g_strcmp0(name, chan->name);
}

/* get the flags to use for a given channel, possibly setting them too in case of lazy init */
unsigned char _dbg_get_channel_flags(struct _debug_channel *channel)
{
    if (nb_debug_options == -1) {
        debug_init();
    }

    if (nb_debug_options) {
        struct _debug_channel *opt;

        opt = bsearch(channel->name,
                debug_options,
                nb_debug_options,
                sizeof(debug_options[0]), cmp_name);
        if (opt) {
            return opt->flags;
        }
    }

    /* no option for this channel */
    if (channel->flags & (1 << __DBCL_INIT)) {
        channel->flags = default_flags;
    }

    return default_flags;
}

/* add a new debug option at the end of the option list */
static void add_option(const char *name, unsigned char set, unsigned char clear)
{
    int min = 0, max = nb_debug_options - 1, pos, res;

    if (!name[0]) { /* "all" option */
        default_flags = (default_flags & ~clear) | set;
        return;
    }

    if (strlen(name) >= sizeof(debug_options[0].name)) {
        return;
    }

    while (min <= max) {
        pos = (min + max) / 2;
        res = g_strcmp0(name, debug_options[pos].name);
        if (!res) {
            debug_options[pos].flags = (debug_options[pos].flags & ~clear) | set;
            return;
        }

        if (res < 0) {
            max = pos - 1;
        } else {
            min = pos + 1;
        }
    }
    if (nb_debug_options >= MAX_DEBUG_OPTIONS) {
        return;
    }

    pos = min;
    if (pos < nb_debug_options) {
        memmove(&debug_options[pos + 1],
                &debug_options[pos],
                (nb_debug_options - pos) * sizeof(debug_options[0]));
    }

    g_strlcpy(debug_options[pos].name, name, MAX_NAME_LEN);
    debug_options[pos].flags = (default_flags & ~clear) | set;
    nb_debug_options++;
}

/* parse a set of debugging option specifications and add them to the option list */
static void parse_options(const char *str)
{
    char *opt, *next, *options;
    unsigned int i;

    if (!(options = g_strdup(str))) {
        return;
    }

    for (opt = options; opt; opt = next) {
        const char *p;
        unsigned char set = 0, clear = 0;

        if ((next = strchr( opt, ',' ))) {
            *next++ = 0;
        }

        p = opt + strcspn( opt, "+-" );
        if (!p[0]) {
            p = opt;  /* assume it's a debug channel name */
        }

        if (p > opt) {
            for (i = 0; i < sizeof(debug_classes) / sizeof(debug_classes[0]); i++) {
                int len = strlen(debug_classes[i]);
                if (len != (p - opt)) {
                    continue;
                }

                if (!memcmp( opt, debug_classes[i], len)) { /* found it */
                    if (*p == '+') {
                        set |= 1 << i;
                    } else {
                        clear |= 1 << i;
                    }
                    break;
                }
            }

            if (i == sizeof(debug_classes) / sizeof(debug_classes[0])) { /* bad class name, skip it */
                continue;
            }
        } else {
            if (*p == '-') {
                clear = ~0;
            } else {
                set = ~0;
            }
        }

        if (*p == '+' || *p == '-') {
            p++;
        }
        if (!p[0]) {
            continue;
        }

        if (!strcmp(p, "all")) {
            default_flags = (default_flags & ~clear) | set;
        } else {
            add_option(p, set, clear);
        }
    }

    free(options);
}

/* print the usage message */
static void debug_usage(void)
{
    static const char usage[] =
        "Syntax of the DEBUGCH variable:\n"
        "  DEBUGCH=[class]+xxx,[class]-yyy,...\n\n"
        "Example: DEBUGCH=+all,warn-heap\n"
        "    turns on all messages except warning heap messages\n"
        "Available message classes: err, warn, fixme, trace\n";
    const int ret = write(2, usage, sizeof(usage) - 1);

    assert(ret >= 0);
    exit(1);
}

/* initialize all options at startup */
static void debug_init(void)
{
    char *debug = NULL;
    FILE *fp = NULL;
    char *tmp = NULL;

    if (nb_debug_options != -1) {
        return;  /* already initialized */
    }

    nb_debug_options = 0;
    if (0 == strlen(bin_path)) {
        g_strlcpy(debugchfile, "DEBUGCH", MAX_FILE_LEN);
    } else {
        g_strlcat(debugchfile, bin_path, MAX_FILE_LEN);
        g_strlcat(debugchfile, "DEBUGCH", MAX_FILE_LEN);
    }

    fp = fopen(debugchfile, "r");
    if (fp == NULL) {
        debug = getenv("DEBUGCH");
    } else {
        if ((tmp = (char *)malloc(1024 + 1)) == NULL){
            nb_debug_options = -1;
            fclose(fp);
            return;
        }

        if(fseek(fp, 0, SEEK_SET) != 0) {
            fclose(fp);
            fprintf(stderr, "failed to fseek()\n");

            if (tmp != NULL)
                free(tmp);

            nb_debug_options = -1;
            return;
        }
        const char* str = fgets(tmp, 1024, fp);
        if (str) {
            tmp[strlen(tmp) - 1] = 0;
            debug = tmp;
        }

        fclose(fp);
    }

    if (debug != NULL) {
        if (!g_strcmp0(debug, "help")) {
            debug_usage();
        }
        parse_options(debug);
    }

    if (tmp != NULL) {
        free(tmp);
    }

    // If "log_path" is not set, we use "stdout".
    if (log_path[0] != '\0') {
        fd = qemu_open(log_path, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0666);
        if (fd < 0) {
            fprintf(stderr, "Can't open logfile: %s\n", log_path);
            exit(1);
            return;
        }
    }
    nb_debug_options = 0;
}

int dbg_log(enum _debug_class cls, struct _debug_channel *channel,
        const char *format, ...)
{
    int ret = 0;
    int ret_write = 0;
    char buf_msg[2048];
    va_list valist;

    if (!(_dbg_get_channel_flags(channel) & (1 << cls))) {
        return -1;
    }

    ret += get_timeofday(buf_msg, sizeof(buf_msg));
    ret += g_snprintf(buf_msg + ret, sizeof(buf_msg) - ret, "(%5d) [%s:%s] ",
                qemu_get_thread_id(), debug_classes[cls], channel->name);

    va_start(valist, format);
    ret += g_vsnprintf(buf_msg + ret, sizeof(buf_msg) - ret, format, valist);
    va_end(valist);

    ret_write = qemu_write_full(fd, buf_msg, ret);

    return ret_write;

}
