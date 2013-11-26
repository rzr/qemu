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

#ifndef _QEMU_YAGL_GLES_API_PS_H
#define _QEMU_YAGL_GLES_API_PS_H

#include "yagl_api.h"
#include <glib.h>

struct yagl_gles_driver;

struct yagl_gles_api_ps
{
    struct yagl_api_ps base;

    struct yagl_gles_driver *driver;

    GHashTable *locations;
};

void yagl_gles_api_ps_init(struct yagl_gles_api_ps *gles_api_ps,
                           struct yagl_gles_driver *driver);

void yagl_gles_api_ps_cleanup(struct yagl_gles_api_ps *gles_api_ps);

void yagl_gles_api_ps_add_location(struct yagl_gles_api_ps *gles_api_ps,
                                   uint32_t location,
                                   GLint actual_location);

GLint yagl_gles_api_ps_translate_location(struct yagl_gles_api_ps *gles_api_ps,
                                          GLboolean tl,
                                          uint32_t location);

void yagl_gles_api_ps_remove_location(struct yagl_gles_api_ps *gles_api_ps,
                                      uint32_t location);

#endif
