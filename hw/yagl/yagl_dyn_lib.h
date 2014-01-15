/*
 * yagl
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

#ifndef _QEMU_YAGL_DYN_LIB_H
#define _QEMU_YAGL_DYN_LIB_H

#include "yagl_types.h"

struct yagl_dyn_lib;

struct yagl_dyn_lib *yagl_dyn_lib_create(void);

void yagl_dyn_lib_destroy(struct yagl_dyn_lib *dyn_lib);

bool yagl_dyn_lib_load(struct yagl_dyn_lib *dyn_lib, const char *name);

void yagl_dyn_lib_unload(struct yagl_dyn_lib *dyn_lib);

void *yagl_dyn_lib_get_sym(struct yagl_dyn_lib *dyn_lib, const char* sym_name);

const char *yagl_dyn_lib_get_error(struct yagl_dyn_lib *dyn_lib);

void *yagl_dyn_lib_get_ogl_procaddr(struct yagl_dyn_lib *dyn_lib,
                                    const char *sym_name);

#endif
