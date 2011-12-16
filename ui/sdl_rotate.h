/*
 * Emulator
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * Hyunjun Son <hj79.son@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
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
 *
 * Derived from: SDL_rotozoom,  LGPL (c) A. Schiffler from the SDL_gfx library.
 * Modifications by Hyunjun Son(hj79.son@samsung.com)
 *
 * This work is licensed under the terms of the GNU GPL version 2.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef _SDL_rotate_h
#define _SDL_rotate_h

#include <SDL.h>
#include <math.h>

/* ---- Prototypes */

#ifdef WIN32
#  ifdef DLL_EXPORT
#    define SDL_ROTOZOOM_SCOPE __declspec(dllexport)
#  else
#    ifdef LIBSDL_GFX_DLL_IMPORT
#      define SDL_ROTOZOOM_SCOPE __declspec(dllimport)
#    endif
#  endif
#endif
#ifndef SDL_ROTOZOOM_SCOPE
#  define SDL_ROTOZOOM_SCOPE extern
#endif

/*

 rotozoomSurface()

 Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'angle' is the rotation in degrees. 'zoom' a scaling factor. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

 */

SDL_ROTOZOOM_SCOPE SDL_Surface *rotozoomSurface(SDL_Surface * src, double angle, double zoom, int smooth);

SDL_ROTOZOOM_SCOPE SDL_Surface *rotozoomSurfaceXY
(SDL_Surface * src, double angle, double zoomx, double zoomy, int smooth);

/* Returns the size of the target surface for a rotozoomSurface() call */

SDL_ROTOZOOM_SCOPE void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth,
		int *dstheight);

SDL_ROTOZOOM_SCOPE void rotozoomSurfaceSizeXY
(int width, int height, double angle, double zoomx, double zoomy,
		int *dstwidth, int *dstheight);

/*

 zoomSurface()

 Zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

 */

SDL_ROTOZOOM_SCOPE SDL_Surface *zoomSurface(SDL_Surface * src, double zoomx, double zoomy, int smooth);

/* Returns the size of the target surface for a zoomSurface() call */

SDL_ROTOZOOM_SCOPE void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight);


/*
    shrinkSurface()

    Shrinks a 32bit or 8bit 'src' surface ti a newly created 'dst' surface.
    'factorx' and 'factory' are the shrinking ratios (i.e. 2=1/2 the size,
    3=1/3 the size, etc.) The destination surface is antialiased by averaging
    the source box RGBA or Y information. If the surface is not 8bit
    or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.
 */

SDL_ROTOZOOM_SCOPE SDL_Surface *shrinkSurface(SDL_Surface * src, int factorx, int factory);

/*

    Other functions

 */

SDL_ROTOZOOM_SCOPE SDL_Surface* rotateSurface90Degrees(SDL_Surface* pSurf, int numClockwiseTurns);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif				/* _SDL_rotate_h */
