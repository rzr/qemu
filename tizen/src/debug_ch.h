/*
 * New debug channel
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * Munkyu Im <munkyu.im@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 * refer to debug_ch.h
 * Copyright 1999 Patrik Stridvall
 *
 */

// header file for legacy debug_ch compatibility

#ifndef __DEBUG_CH_H
#define __DEBUG_CH_H

#include "util/new_debug_ch.h"

#undef  ERR
#define ERR     LOG_SEVERE

#ifndef WARN
#define WARN    LOG_WARNING
#endif

#ifndef INFO
#define INFO    LOG_INFO
#endif

#ifndef FIXME
#define FIXME   LOG_FINE
#endif

#ifndef TRACE
#define TRACE   LOG_TRACE
#endif

#define MULTI_DEBUG_CHANNEL(ch, chm)    DECLARE_DEBUG_CHANNEL(chm)

#endif // __DEBUG_CH_H
