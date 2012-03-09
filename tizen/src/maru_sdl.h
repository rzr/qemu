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
#ifndef _WIN32
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_syswm.h>
#else
#include <windows.h>
#include <winbase.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_syswm.h>
#include <SDL_getenv.h>
#endif

void maruskin_display_init(DisplayState *ds);
void maruskin_sdl_init(int swt_handle);

#endif /* MARU_SDL_H_ */
