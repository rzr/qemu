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

#ifndef __CHECK_GL_H__
#define __CHECK_GL_H__

struct gl_context;

typedef enum
{
    gl_info = 0,
    gl_warn = 1,
    gl_error = 2
} gl_log_level;

typedef enum
{
    gl_2 = 0,
    gl_3_1 = 1,
    gl_3_2 = 2
} gl_version;

void check_gl_log(gl_log_level level, const char *format, ...);

int check_gl_init(void);

int check_gl(void);

struct gl_context *check_gl_context_create(struct gl_context *share_ctx,
                                           gl_version version);

int check_gl_make_current(struct gl_context *ctx);

void check_gl_context_destroy(struct gl_context *ctx);

int check_gl_procaddr(void **func, const char *sym, int opt);

#endif
