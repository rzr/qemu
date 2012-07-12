/*
 * MARU SDL display driver
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
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


#ifndef MARU_SDL_H_
#define MARU_SDL_H_

#include "console.h"

#if 0
#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#endif
#endif

#include <SDL.h>
#include <SDL_syswm.h>
#include "qemu-common.h"

#define SDL_USER_EVENT_CODE_HARDKEY 1

void maruskin_display_init(DisplayState *ds);
void maruskin_sdl_init(uint64 swt_handle, int lcd_size_width, int lcd_size_height, bool is_resize);
void maruskin_sdl_resize(void);

DisplaySurface* get_qemu_display_surface( void );

#endif /* MARU_SDL_H_ */
