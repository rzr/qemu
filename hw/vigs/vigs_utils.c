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

#include "vigs_utils.h"
#include "vigs_log.h"

uint32_t vigs_format_bpp(vigsp_surface_format format)
{
    switch (format) {
    case vigsp_surface_bgrx8888: return 4;
    case vigsp_surface_bgra8888: return 4;
    default:
        assert(false);
        VIGS_LOG_CRITICAL("unknown format: %d", format);
        exit(1);
        return 0;
    }
}

int vigs_format_num_buffers(vigsp_plane_format format)
{
    switch (format) {
    case vigsp_plane_bgrx8888: return 1;
    case vigsp_plane_bgra8888: return 1;
    case vigsp_plane_nv21: return 2;
    case vigsp_plane_nv42: return 2;
    case vigsp_plane_nv61: return 2;
    default:
        assert(false);
        VIGS_LOG_CRITICAL("unknown format: %d", format);
        exit(1);
        return 1;
    }
}
