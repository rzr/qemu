/*
 * Emulator
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Kitae Kim <kt920.kim@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
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

#import <Cocoa/Cocoa.h>
#import <AppKit/NSEvent.h>

#include "emul_state.h"

/* retrieves the status of the host lock key */
int get_host_lock_key_state_darwin(int key)
{
    /* support only capslock */
    if (key == HOST_CAPSLOCK_KEY) {
        return ((NSAlphaShiftKeyMask & [NSEvent modifierFlags]) ? 1 : 0);
    } else if (key == HOST_NUMLOCK_KEY) {
        return 0;
    }

    return -1;
}
