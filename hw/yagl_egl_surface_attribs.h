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

#ifndef _QEMU_YAGL_EGL_SURFACE_ATTRIBS_H
#define _QEMU_YAGL_EGL_SURFACE_ATTRIBS_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_egl_pbuffer_attribs
{
    EGLint largest;
    EGLint tex_format;
    EGLint tex_target;
    EGLint tex_mipmap;
};

/*
 * Initialize to default values.
 */
void yagl_egl_pbuffer_attribs_init(struct yagl_egl_pbuffer_attribs *attribs);

struct yagl_egl_pixmap_attribs
{
};

/*
 * Initialize to default values.
 */
void yagl_egl_pixmap_attribs_init(struct yagl_egl_pixmap_attribs *attribs);

struct yagl_egl_window_attribs
{
};

/*
 * Initialize to default values.
 */
void yagl_egl_window_attribs_init(struct yagl_egl_window_attribs *attribs);

#endif
