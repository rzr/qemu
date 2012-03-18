/*
 * keymap
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


#include "maruskin_keymap.h"


/* keep it consistent with java virtual keycode */
#define JAVA_KEYCODE_BIT (1 << 24)
#define JAVA_KEYCODE_BIT_CTRL (1 << 18)
#define JAVA_KEYCODE_BIT_SHIFT (1 << 17)
#define JAVA_KEYCODE_BIT_ALT (1 << 16)

#define JAVA_KEY_MASK 0xFFFF;

enum JAVA_KEYCODE {
    JAVA_KEY_ARROW_UP = 1,
    JAVA_KEY_ARROW_DOWN,
    JAVA_KEY_ARROW_LEFT,
    JAVA_KEY_ARROW_RIGHT,
    JAVA_KEY_PAGE_UP,
    JAVA_KEY_PAGE_DOWN,
    JAVA_KEY_HOME,
    JAVA_KEY_END,
    JAVA_KEY_INSERT,
    JAVA_KEY_F1 = 10,
    JAVA_KEY_F20 = 29,
};


int javakeycode_to_scancode(int java_keycode)
{
    int vk = java_keycode & JAVA_KEY_MASK;

/*#ifdef _WIN32
    return MapVirtualKey(vk, MAPVK_VK_TO_VSC);
#endif*/

    if (java_keycode & JAVA_KEYCODE_BIT) {
        if (vk >= JAVA_KEY_F1 && vk <=JAVA_KEY_F20) { //function keys
            vk += 255;
        } else { //special keys
            switch(vk) {
                case JAVA_KEY_ARROW_UP :
                    vk = KEY_UP;
                    break;
                case JAVA_KEY_ARROW_DOWN :
                    vk = KEY_DOWN;
                    break;
                case JAVA_KEY_ARROW_LEFT :
                    vk = KEY_LEFT;
                    break;
                case JAVA_KEY_ARROW_RIGHT :
                    vk = KEY_RIGHT;
                    break;
                case JAVA_KEY_PAGE_UP :
                    vk = KEY_PPAGE;
                    break;
                case JAVA_KEY_PAGE_DOWN :
                    vk = KEY_NPAGE;
                    break;
                case JAVA_KEY_HOME :
                    vk = KEY_HOME;
                    break;
                case JAVA_KEY_END :
                    vk = KEY_END;
                    break;
                case JAVA_KEY_INSERT :
                    vk = KEY_IC;
                    break;
                default :
                    break;
            }
        }
        //TODO:
        //meta keys
    }

    return vkkey2scancode[vk];
}
