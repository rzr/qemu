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

#include "yagl_egl_native_config.h"
#include "yagl_process.h"

void yagl_egl_native_config_init(struct yagl_egl_native_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->surface_type = EGL_WINDOW_BIT;
    cfg->caveat = EGL_DONT_CARE;
    cfg->config_id = EGL_DONT_CARE;
    cfg->max_swap_interval = EGL_DONT_CARE;
    cfg->min_swap_interval = EGL_DONT_CARE;
    cfg->native_visual_type = EGL_DONT_CARE;
    cfg->transparent_type = EGL_NONE;
    cfg->trans_red_val = EGL_DONT_CARE;
    cfg->trans_green_val = EGL_DONT_CARE;
    cfg->trans_blue_val = EGL_DONT_CARE;
    cfg->match_format_khr = EGL_DONT_CARE;
    cfg->bind_to_texture_rgb = EGL_DONT_CARE;
    cfg->bind_to_texture_rgba = EGL_DONT_CARE;
}
