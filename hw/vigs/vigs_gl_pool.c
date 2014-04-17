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

#include "vigs_gl_pool.h"
#include "vigs_log.h"

struct vigs_gl_pool_key
{
    uint32_t width;
    uint32_t height;
    vigsp_surface_format format;
};

struct vigs_gl_pool_value
{
    struct vigs_gl_pool *pool;

    GLuint id;

    /*
     * We don't want to use struct vigs_ref here since
     * these objects are owned by hash table, i.e. destruction
     * must be performed from outside when needed.
     */
    uint32_t user_count;
};

static guint vigs_gl_pool_hash(gconstpointer a)
{
    const struct vigs_gl_pool_key *key = a;

    return (key->width |
            (key->height << 13) |
            ((uint32_t)key->format << 26));
}

static gboolean vigs_gl_pool_equal(gconstpointer a, gconstpointer b)
{
    const struct vigs_gl_pool_key *left = a;
    const struct vigs_gl_pool_key *right = b;

    if (left->width != right->width) {
        return FALSE;
    }

    if (left->height != right->height) {
        return FALSE;
    }

    if (left->format != right->format) {
        return FALSE;
    }

    return TRUE;
}

static void vigs_gl_pool_key_destroy(gpointer data)
{
    struct vigs_gl_pool_key *key = data;

    g_free(key);
}

static void vigs_gl_pool_value_destroy(gpointer data)
{
    struct vigs_gl_pool_value *value = data;

    assert(value->user_count == 0);

    value->pool->release_func(value->pool->user_data, value->id);

    g_free(value);
}

struct vigs_gl_pool
    *vigs_gl_pool_create(const char *name,
                         vigs_gl_pool_alloc_func alloc_func,
                         vigs_gl_pool_release_func release_func,
                         void *user_data)
{
    struct vigs_gl_pool *pool;

    pool = g_malloc0(sizeof(*pool));

    strncpy(pool->name, name, sizeof(pool->name)/sizeof(pool->name[0]));
    pool->name[sizeof(pool->name)/sizeof(pool->name[0]) - 1] = '\0';

    pool->entries = g_hash_table_new_full(vigs_gl_pool_hash,
                                          vigs_gl_pool_equal,
                                          vigs_gl_pool_key_destroy,
                                          vigs_gl_pool_value_destroy);

    pool->alloc_func = alloc_func;
    pool->release_func = release_func;
    pool->user_data = user_data;

    return pool;
}

void vigs_gl_pool_destroy(struct vigs_gl_pool *pool)
{
    g_hash_table_destroy(pool->entries);

    g_free(pool);
}

GLuint vigs_gl_pool_alloc(struct vigs_gl_pool *pool,
                          uint32_t width,
                          uint32_t height,
                          vigsp_surface_format format)
{
    struct vigs_gl_pool_key tmp_key;
    GLuint id;
    struct vigs_gl_pool_key *key;
    struct vigs_gl_pool_value *value;

    tmp_key.width = width;
    tmp_key.height = height;
    tmp_key.format = format;

    value = g_hash_table_lookup(pool->entries, &tmp_key);

    if (value) {
        ++value->user_count;

        return value->id;
    }

    id = pool->alloc_func(pool->user_data, width, height, format);

    if (!id) {
        return id;
    }

    key = g_malloc0(sizeof(*key));

    key->width = width;
    key->height = height;
    key->format = format;

    value = g_malloc0(sizeof(*value));

    value->pool = pool;
    value->id = id;
    value->user_count = 1;

    g_hash_table_insert(pool->entries, key, value);

    VIGS_LOG_TRACE("pool %s: num_entries = %u",
                   pool->name,
                   g_hash_table_size(pool->entries));

    return value->id;
}

void vigs_gl_pool_release(struct vigs_gl_pool *pool,
                          uint32_t width,
                          uint32_t height,
                          vigsp_surface_format format)
{
    struct vigs_gl_pool_key tmp_key;
    struct vigs_gl_pool_value *value;

    tmp_key.width = width;
    tmp_key.height = height;
    tmp_key.format = format;

    value = g_hash_table_lookup(pool->entries, &tmp_key);

    assert(value);

    if (!value) {
        return;
    }

    assert(value->user_count > 0);

    if (--value->user_count == 0) {
        g_hash_table_remove(pool->entries, &tmp_key);

        VIGS_LOG_TRACE("pool %s: num_entries = %u",
                       pool->name,
                       g_hash_table_size(pool->entries));
    }
}
