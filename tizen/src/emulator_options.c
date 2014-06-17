/*
 * Emulator
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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
 */

#include <string.h>
#include "qemu/queue.h"

#include "maru_common.h"
#include "emulator_options.h"
#include "emulator.h"

#define LINE_LIMIT 1024
#define TOKEN_LIMIT 128
#define OPTION_LIMIT 256

struct variable {
    gchar *name;
    gchar *value;
    QTAILQ_ENTRY(variable) entry;
};

static QTAILQ_HEAD(, variable) variables =
    QTAILQ_HEAD_INITIALIZER(variables);

struct emulator_opts {
    int num;
    char *options[OPTION_LIMIT];
};

static struct emulator_opts default_qemu_opts;
static struct emulator_opts default_skin_opts;

void set_variable(const char * const arg1, const char * const arg2,
                bool override)
{
    char *name = NULL;
    char *value = NULL;
    struct variable *var = NULL;
    int i;

    // strip '-'
    for (i = 0; i < strlen(arg1); ++i) {
        if (arg1[i] != '-')
            break;
    }

    name = g_strdup(arg1 + i);
    if(!arg2) {
        value = g_strdup("");
    } else {
        value = g_strdup(arg2);
    }

    QTAILQ_FOREACH(var, &variables, entry) {
        if (!g_strcmp0(name, var->name)) {
            if(!override)
                return;

            g_free(name);
            g_free(var->value);

            var->value = value;

            return;
        }
    }

    var = g_malloc(sizeof(*var));


    var->name = name;
    var->value = value;
    QTAILQ_INSERT_TAIL(&variables, var, entry);
}

char *get_variable(const char * const name)
{
    struct variable *var = NULL;

    QTAILQ_FOREACH(var, &variables, entry) {
        if (!g_strcmp0(name, var->name)) {
            return var->value;
        }
    }

    return NULL;
}

void reset_variables(void)
{
    struct variable *var = NULL;

    QTAILQ_FOREACH(var, &variables, entry) {
        QTAILQ_REMOVE(&variables, var, entry);
        g_free(var->name);
        g_free(var->value);
    }
}

static void reset_default_opts(void)
{
    int i = 0;

    for (i = 0; i < default_qemu_opts.num; ++i) {
        g_free(default_qemu_opts.options[i]);
    }
    for (i = 0; i < default_skin_opts.num; ++i) {
        g_free(default_skin_opts.options[i]);
    }
}

static char *substitute_variables(char *src)
{
    int i = 0;
    int start_index = -1;
    int end_index = -1;
    char *str = g_strdup(src);

    for (i = 0; str[i]; ++i) {
        if(str[i] == '$' && str[i + 1] && str[i + 1] == '{') {
            start_index = i++;

            continue;
        }
        else if(str[i] == '}') {
            end_index = i;

            if (start_index == -1 || end_index == -1) {
                // must not enter here
                continue;
            }

            char name[TOKEN_LIMIT];
            char *value = NULL;
            char *buf = NULL;
            int length;

            g_strlcpy(name, str + start_index + 2, end_index - start_index - 1);

            // search stored variables
            value = get_variable(name);

            // if there is no name in stored variables,
            // try to search environment variables
            if(!value) {
                value = getenv(name);
            }

            if(!value) {
                fprintf(stderr, "[%s] is not set."
                        " Please input value using commandline argument"
                        " \"--%s\" or profile default file or envirionment"
                        " variable.\n", name, name);
                value = (char *)"";
            }

            length = start_index + strlen(value) + (strlen(str) - end_index);
            buf = g_malloc(length);

            g_strlcpy(buf, str, start_index + 1);
            g_strlcat(buf, value, length);
            g_strlcat(buf, str + end_index + 1, length);

            g_free(str);
            str = buf;

            i = start_index + strlen(value);

            start_index = end_index = -1;
        }
    }

    return str;
}

bool load_profile_default(const char * const conf, const char * const profile)
{
    int classification = 0;
    char str[LINE_LIMIT];
    char *filename;
    FILE *file = NULL;

    // if "conf" is exist, ignore "profile"
    if (conf) {
        filename = g_strdup(conf);
    } else {
        filename = g_strdup_printf("%s/%s.conf", get_bin_path(), profile);
    }

    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr,
            "Profile configuration file [%s] is not found.\n", filename);
        g_free(filename);
        return false;
    }
    g_free(filename);

    struct emulator_opts *default_opts = NULL;

    while (fgets(str, LINE_LIMIT, file)) {
        int i = 0;

        while (str[i] && str[i] != '#') {
            // tokenize
            char token[TOKEN_LIMIT] = { '\0', };
            int start_index = -1;
            bool in_quote = false;

            do {
                if (!str[i] ||
                    (!in_quote &&
                        (str[i] == ' ' || str[i] == '\t' || str[i] == '#'
                        || str[i] == '\r' || str[i] == '\n')) ||
                    (in_quote && str[i] == '"')) {
                    if (start_index != -1) {
                        g_strlcpy(token, str + start_index,
                            i - start_index + 1);
                    }
                }
                else {
                    if (start_index < 0) {
                        if (str[i] == '"') {
                            in_quote = true;
                            ++i;
                        }
                        start_index = i;
                    }
                }
            } while(str[i++] && !token[0]);

            if (!token[0]) {
                break;
            }

            // detect label
            if (!g_strcmp0(token, "[[DEFAULT_VALUE]]")) {
                classification = 0;
                continue;
            }
            else if (!g_strcmp0(token, "[[SKIN_OPTIONS]]")) {
                default_opts = &default_skin_opts;
                classification = 1;
                continue;
            }
            else if (!g_strcmp0(token, "[[QEMU_OPTIONS]]")) {
                default_opts = &default_qemu_opts;
                classification = 2;
                continue;
            }

            // process line
            switch (classification) {
            case 0: // default variables
                {
                    gchar **splitted = g_strsplit(token, "=", 2);
                    if (splitted[0] && splitted[1]) {
                        char *value = substitute_variables(splitted[1]);
                        set_variable(g_strdup(splitted[0]), value,
                                false);
                    }
                    g_strfreev(splitted);

                    break;
                }
            case 1: // skin options
            case 2: // qemu options
                default_opts->options[default_opts->num++] = g_strdup(token);

                break;
            }
        }
    }

    return true;
}

static bool assemble_args(int *argc, char **argv,
                struct emulator_opts *default_opts)
{
    int i = 0;

    for (i = 0; i < default_opts->num; ++i) {
        int j = 0;
        char *str;
        int start_index = -1;
        int end_index = -1;

        str = default_opts->options[i];

        // conditional arguments
        for (j = 0; str[j]; ++j) {
            if(str[j] == '[') {
                start_index = j;
            }
            else if(str[j] == ']') {
                end_index = j;
            }
        }
        if (start_index != -1 && end_index != -1) {
            char cond[TOKEN_LIMIT];

            g_strlcpy(cond, str + start_index + 1, end_index - start_index);
            g_strstrip(cond);
            if (strlen(cond) > 0) {
                if ((cond[0] == '!' && get_variable(g_strstrip(cond + 1))) ||
                    (cond[0] != '!' && !get_variable(cond))) {
                    continue;
                }
            }
            g_strlcpy(str, str + end_index + 1, strlen(str));
            g_strstrip(str);
        }

        // substitute variables
        argv[(*argc)++] = substitute_variables(str);
    }

    return true;
}

bool assemble_profile_args(int *qemu_argc, char **qemu_argv,
        int *skin_argc, char **skin_argv)
{
    if (!assemble_args(qemu_argc, qemu_argv,
                &default_qemu_opts)) {
        reset_default_opts();
        return false;
    }
    if (!assemble_args(skin_argc, skin_argv,
                &default_skin_opts)) {
        reset_default_opts();
        return false;
    }

    reset_default_opts();
    return true;
}
