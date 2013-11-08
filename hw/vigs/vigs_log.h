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

#ifndef _QEMU_VIGS_LOG_H
#define _QEMU_VIGS_LOG_H

#include "vigs_types.h"

typedef enum
{
    vigs_log_level_off = 0,
    vigs_log_level_error = 1,
    vigs_log_level_warn = 2,
    vigs_log_level_info = 3,
    vigs_log_level_debug = 4,
    vigs_log_level_trace = 5
} vigs_log_level;

#define vigs_log_level_max vigs_log_level_trace

void vigs_log_init(void);

void vigs_log_cleanup(void);

void vigs_log_event(vigs_log_level log_level,
                    const char *func,
                    int line,
                    const char *format, ...);

bool vigs_log_is_enabled_for_level(vigs_log_level log_level);

#define VIGS_LOG_EVENT(log_level, format, ...) \
    do { \
        if (vigs_log_is_enabled_for_level(vigs_log_level_##log_level)) { \
            vigs_log_event(vigs_log_level_##log_level, __FUNCTION__, __LINE__, format,##__VA_ARGS__); \
        } \
    } while(0)

#define VIGS_LOG_TRACE(format, ...) VIGS_LOG_EVENT(trace, format,##__VA_ARGS__)
#define VIGS_LOG_DEBUG(format, ...) VIGS_LOG_EVENT(debug, format,##__VA_ARGS__)
#define VIGS_LOG_INFO(format, ...) VIGS_LOG_EVENT(info, format,##__VA_ARGS__)
#define VIGS_LOG_WARN(format, ...) VIGS_LOG_EVENT(warn, format,##__VA_ARGS__)
#define VIGS_LOG_ERROR(format, ...) VIGS_LOG_EVENT(error, format,##__VA_ARGS__)
#define VIGS_LOG_CRITICAL(format, ...) vigs_log_event(vigs_log_level_error, __FUNCTION__, __LINE__, format,##__VA_ARGS__)

#endif
