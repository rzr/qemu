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

#include <GL/gl.h>
#include "yagl_gles_api_ps.h"
#include "yagl_process.h"
#include "yagl_thread.h"

void yagl_gles_api_ps_init(struct yagl_gles_api_ps *gles_api_ps,
                           struct yagl_gles_driver *driver)
{
    gles_api_ps->driver = driver;
    gles_api_ps->locations = g_hash_table_new_full(g_direct_hash,
                                                   g_direct_equal,
                                                   NULL,
                                                   NULL);

    assert(gles_api_ps->locations);
}

void yagl_gles_api_ps_cleanup(struct yagl_gles_api_ps *gles_api_ps)
{
    g_hash_table_destroy(gles_api_ps->locations);
}

void yagl_gles_api_ps_add_location(struct yagl_gles_api_ps *gles_api_ps,
                                   uint32_t location,
                                   GLint actual_location)
{
    guint size = g_hash_table_size(gles_api_ps->locations);

    g_hash_table_insert(gles_api_ps->locations,
                        GUINT_TO_POINTER(location),
                        GINT_TO_POINTER(actual_location));

    assert(g_hash_table_size(gles_api_ps->locations) > size);
}

GLint yagl_gles_api_ps_translate_location(struct yagl_gles_api_ps *gles_api_ps,
                                          GLboolean tl,
                                          uint32_t location)
{
    gpointer key, item;

    if (!tl) {
        return location;
    }

    if (g_hash_table_lookup_extended(gles_api_ps->locations,
                                     GUINT_TO_POINTER(location),
                                     &key,
                                     &item))
    {
        return GPOINTER_TO_INT(item);
    } else {
        assert(0);
        return -1;
    }
}

void yagl_gles_api_ps_remove_location(struct yagl_gles_api_ps *gles_api_ps,
                                      uint32_t location)
{
    guint size = g_hash_table_size(gles_api_ps->locations);

    g_hash_table_remove(gles_api_ps->locations, GUINT_TO_POINTER(location));

    assert(g_hash_table_size(gles_api_ps->locations) < size);
}
