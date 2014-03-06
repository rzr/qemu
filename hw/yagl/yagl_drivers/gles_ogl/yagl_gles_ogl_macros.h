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

#ifndef _QEMU_YAGL_GLES_OGL_MACROS_H
#define _QEMU_YAGL_GLES_OGL_MACROS_H

#include <GL/gl.h>

/*
 * GLES OpenGL function loading/assigning macros.
 * @{
 */

#define YAGL_GLES_OGL_GET_PROC(driver, func, sym) \
    do { \
        *(void**)(&driver->func) = yagl_dyn_lib_get_ogl_procaddr(dyn_lib, #sym); \
        if (!driver->func) { \
            YAGL_LOG_ERROR("Unable to get symbol \"%s\": %s", \
                           #sym, \
                           yagl_dyn_lib_get_error(dyn_lib)); \
            goto fail; \
        } \
    } while (0)

#define YAGL_GLES_OGL_GET_PROC_OPT(driver, func, sym) \
    *(void**)(&driver->func) = yagl_dyn_lib_get_ogl_procaddr(dyn_lib, #sym)

/*
 * @}
 */

#endif
