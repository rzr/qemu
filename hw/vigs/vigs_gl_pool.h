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

#ifndef _QEMU_VIGS_GL_POOL_H
#define _QEMU_VIGS_GL_POOL_H

#include "vigs_types.h"
#include <glib.h>
#include <GL/gl.h>
#include <GL/glext.h>

typedef GLuint (*vigs_gl_pool_alloc_func)(void */*user_data*/,
                                          uint32_t /*width*/,
                                          uint32_t /*height*/,
                                          vigsp_surface_format /*format*/);

typedef void (*vigs_gl_pool_release_func)(void */*user_data*/,
                                          GLuint /*id*/);

struct vigs_gl_pool
{
    char name[255];

    GHashTable *entries;

    vigs_gl_pool_alloc_func alloc_func;

    vigs_gl_pool_release_func release_func;

    void *user_data;
};

struct vigs_gl_pool
    *vigs_gl_pool_create(const char *name,
                         vigs_gl_pool_alloc_func alloc_func,
                         vigs_gl_pool_release_func release_func,
                         void *user_data);

void vigs_gl_pool_destroy(struct vigs_gl_pool *pool);

GLuint vigs_gl_pool_alloc(struct vigs_gl_pool *pool,
                          uint32_t width,
                          uint32_t height,
                          vigsp_surface_format format);

void vigs_gl_pool_release(struct vigs_gl_pool *pool,
                          uint32_t width,
                          uint32_t height,
                          vigsp_surface_format format);

#endif
