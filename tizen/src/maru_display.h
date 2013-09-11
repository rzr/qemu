/*
 * MARU display driver
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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


#ifndef __MARU_DISPLAY_H__
#define __MARU_DISPLAY_H__

#include "ui/console.h"

typedef struct MaruScreenshot {
    unsigned char *pixel_data;
    int request_screenshot;
    int isReady;
} MaruScreenshot;

void maru_display_init(DisplayState *ds);
void maru_display_fini(void);
void maruskin_init(uint64 swt_handle,
    unsigned int display_width, unsigned int display_height,
    bool blank_guide);

MaruScreenshot *get_maru_screenshot(void);
void set_maru_screenshot(DisplaySurface *surface);

#endif /* __MARU_DISPLAY_H__ */
