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

#ifndef _QEMU_YAGL_API_H
#define _QEMU_YAGL_API_H

#include "yagl_types.h"

/*
 * YaGL API per-process state.
 * @{
 */

struct yagl_api_ps
{
    struct yagl_api *api;

    void (*thread_init)(struct yagl_api_ps */*api_ps*/);

    void (*batch_start)(struct yagl_api_ps */*api_ps*/);

    yagl_api_func (*get_func)(struct yagl_api_ps */*api_ps*/,
                              uint32_t /*func_id*/);

    void (*batch_end)(struct yagl_api_ps */*api_ps*/);

    void (*thread_fini)(struct yagl_api_ps */*api_ps*/);

    void (*destroy)(struct yagl_api_ps */*api_ps*/);
};

void yagl_api_ps_init(struct yagl_api_ps *api_ps,
                      struct yagl_api *api);
void yagl_api_ps_cleanup(struct yagl_api_ps *api_ps);

/*
 * @}
 */

/*
 * YaGL API.
 * @{
 */

struct yagl_api
{
    struct yagl_api_ps *(*process_init)(struct yagl_api */*api*/);

    void (*destroy)(struct yagl_api */*api*/);
};

void yagl_api_init(struct yagl_api *api);
void yagl_api_cleanup(struct yagl_api *api);

/*
 * @}
 */

#endif
