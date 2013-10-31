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

#ifndef _QEMU_YAGL_TRANSPORT_GL_H
#define _QEMU_YAGL_TRANSPORT_GL_H

#include "yagl_types.h"
#include "yagl_transport.h"

static __inline GLbitfield yagl_transport_get_out_GLbitfield(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLboolean yagl_transport_get_out_GLboolean(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint8_t(t);
}

static __inline GLubyte yagl_transport_get_out_GLubyte(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint8_t(t);
}

static __inline GLclampf yagl_transport_get_out_GLclampf(struct yagl_transport *t)
{
    return yagl_transport_get_out_float(t);
}

static __inline GLenum yagl_transport_get_out_GLenum(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLfloat yagl_transport_get_out_GLfloat(struct yagl_transport *t)
{
    return yagl_transport_get_out_float(t);
}

static __inline GLint yagl_transport_get_out_GLint(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLsizei yagl_transport_get_out_GLsizei(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLuint yagl_transport_get_out_GLuint(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

#endif
