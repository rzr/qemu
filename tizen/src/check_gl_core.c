/*
 * Copyright (c) 2011 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "check_gl.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static const char *log_level_str[gl_error + 1] =
{
    "INFO",
    "WARN",
    "ERROR"
};

static const char *gl_version_str[gl_3_2 + 1] =
{
    "2.1",
    "3.1",
    "3.2"
};

void check_gl_log(gl_log_level level, const char *format, ...)
{
    va_list args;

    fprintf(stderr, "%-5s", log_level_str[level]);

    va_start(args, format);
    fprintf(stderr, " - ");
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

static const GLubyte *(GLAPIENTRY *get_string)(GLenum);
static const GLubyte *(GLAPIENTRY *get_stringi)(GLenum, GLuint);
static void (GLAPIENTRY *get_integerv)(GLenum, GLint*);

static struct gl_context *check_gl_version(gl_version version)
{
    struct gl_context *ctx = NULL;
    int hw = 1;

    ctx = check_gl_context_create(NULL, version);

    if (!ctx) {
        goto fail;
    }

    if (!check_gl_make_current(ctx)) {
        goto fail;
    }

    if ((strstr((const char*)get_string(GL_RENDERER), "Software") != NULL)) {
        check_gl_log(gl_warn,
                     "Host OpenGL %s - software",
                     gl_version_str[version]);
        hw = 0;
    } else if (strncmp((const char*)get_string(GL_VERSION),
                       "1.4", strlen("1.4")) == 0) {
        /*
         * Handle LIBGL_ALWAYS_INDIRECT=1 case.
         */
        check_gl_log(gl_warn,
                     "Host OpenGL %s - NOT supported",
                     gl_version_str[version]);
        hw = 0;
    } else {
        check_gl_log(gl_info,
                     "Host OpenGL %s - supported",
                     gl_version_str[version]);
    }

    check_gl_log(gl_info, "+ GL_VENDOR = %s", (const char*)get_string(GL_VENDOR));
    check_gl_log(gl_info, "+ GL_RENDERER = %s", (const char*)get_string(GL_RENDERER));
    check_gl_log(gl_info, "+ GL_VERSION = %s", (const char*)get_string(GL_VERSION));

    if (!hw) {
        check_gl_context_destroy(ctx);
        ctx = NULL;
    }

    check_gl_make_current(NULL);

    return ctx;

fail:
    if (ctx) {
        check_gl_context_destroy(ctx);
    }

    check_gl_log(gl_info,
                 "Host OpenGL %s - NOT supported",
                 gl_version_str[version]);

    return NULL;
}

int check_gl(void)
{
    struct gl_context *ctx_2;
    struct gl_context *ctx_3_1;
    struct gl_context *ctx_3_2;
    int have_es3 = 0;
    int have_es1 = 0;

    if (!check_gl_init()) {
        return 1;
    }

    if (!check_gl_procaddr((void**)&get_string, "glGetString", 0) ||
        !check_gl_procaddr((void**)&get_stringi, "glGetStringi", 1) ||
        !check_gl_procaddr((void**)&get_integerv, "glGetIntegerv", 0)) {
        return 1;
    }

    ctx_2 = check_gl_version(gl_2);
    ctx_3_1 = check_gl_version(gl_3_1);
    ctx_3_2 = check_gl_version(gl_3_2);

    if (!ctx_2 && !ctx_3_1 && !ctx_3_2) {
        check_gl_log(gl_info, "Host does not have hardware GL acceleration!");
        return 1;
    }

    have_es1 = (ctx_2 != NULL);
    have_es3 = (ctx_3_2 != NULL);

    /*
     * Check if GL_ARB_ES3_compatibility is supported within
     * OpenGL 3.1 context.
     */

    if (ctx_3_1 && get_stringi && check_gl_make_current(ctx_3_1)) {
        GLint i, num_extensions = 0;

        get_integerv(GL_NUM_EXTENSIONS, &num_extensions);

        for (i = 0; i < num_extensions; ++i) {
            const char *tmp;

            tmp = (const char*)get_stringi(GL_EXTENSIONS, i);

            if (strcmp(tmp, "GL_ARB_ES3_compatibility") == 0) {
                have_es3 = 1;
                break;
            }
        }

        check_gl_make_current(NULL);
    }

    /*
     * Check if OpenGL 2.1 contexts can be shared with OpenGL 3.x contexts
     */

    if (ctx_2 && ((ctx_3_1 && have_es3) || ctx_3_2)) {
        struct gl_context *ctx = NULL;

        ctx = check_gl_context_create(((ctx_3_1 && have_es3) ? ctx_3_1
                                                             : ctx_3_2),
                                      gl_2);

        if (ctx) {
            if (check_gl_make_current(ctx)) {
                check_gl_make_current(NULL);
            } else {
                have_es1 = 0;
            }
            check_gl_context_destroy(ctx);
        } else {
            have_es1 = 0;
        }
    }

    if (have_es1) {
        check_gl_log(gl_info, "Guest OpenGL ES v1_CM - supported");
    } else {
        check_gl_log(gl_warn, "Guest OpenGL ES v1_CM - NOT supported");
    }
    check_gl_log(gl_info, "Guest OpenGL ES 2.0 - supported");
    if (have_es3) {
        check_gl_log(gl_info, "Guest OpenGL ES 3.0 - supported");
    } else {
        check_gl_log(gl_warn, "Guest OpenGL ES 3.0 - NOT supported");
    }

    check_gl_log(gl_info, "Host has hardware GL acceleration!");

    return 0;
}
