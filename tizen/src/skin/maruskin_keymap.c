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


int javakeycode_to_scancode(int java_keycode)
{
    int state_mask = java_keycode & JAVA_KEYCODE_BIT;
    int vk = java_keycode & JAVA_KEY_MASK;

    int ctrl_mask = java_keycode & JAVA_KEYCODE_BIT_CTRL;
    int shift_mask = java_keycode & JAVA_KEYCODE_BIT_SHIFT;
    int alt_mask = java_keycode & JAVA_KEYCODE_BIT_ALT;

/*#ifdef _WIN32
    return MapVirtualKey(vk, MAPVK_VK_TO_VSC);
#endif*/

    if (vk == 0) { //meta keys
        if (java_keycode == JAVA_KEYCODE_BIT_CTRL) { //ctrl key
            return 29;
        } else if (java_keycode == JAVA_KEYCODE_BIT_SHIFT) { //shift key
            return 42;
        } else if (java_keycode == JAVA_KEYCODE_BIT_ALT) { //alt key
            return 56;
        } else {
            return -1;
        }
    }

    if (state_mask != 0)
    { //non-character keys
        if (vk >= JAVA_KEY_F1 && vk <= JAVA_KEY_F20) { //function keys
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
                case JAVA_KEY_CAPS_LOCK :
                case JAVA_KEY_NUM_LOCK :
                case JAVA_KEY_SCROLL_LOCK :
                case JAVA_KEY_PAUSE :
                case JAVA_KEY_BREAK :
                case JAVA_KEY_PRINT_SCREEN :
                    //TODO:
                    return -1;
                    break;
                default :
                    return -1;
                    break;
            }
        }

    }
    else //state_mask == 0
    { //character keys
        if (ctrl_mask == 0 && shift_mask != 0 && alt_mask == 0) { //shift + character keys
            switch(vk) {
                case '`' :
                    vk = '~';
                    break;
                case '1' :
                    vk = '!';
                    break;
                case '2' :
                    vk = '@';
                    break;
                case '3' :
                    vk = '#';
                    break;
                case '4' :
                    vk = '$';
                    break;
                case '5' :
                    vk = '%';
                    break;
                case '6' :
                    vk = '^';
                    break;
                case '7' :
                    vk = '&';
                    break;
                case '8' :
                    vk = '*';
                    break;
                case '9' :
                    vk = '(';
                    break;
                case '0' :
                    vk = ')';
                    break;
                case '-' :
                    vk = '_';
                    break;
                case '=' :
                    vk = '+';
                    break;
                case '\\' :
                    vk = '|';
                    break;
                case '[' :
                    vk = '{';
                    break;
                case ']' :
                    vk = '}';
                    break;
                case ';' :
                    vk = ':';
                    break;
                case '\'' :
                    vk = '\"';
                    break;
                case ',' :
                    vk = '<';
                    break;
                case '.' :
                    vk = '>';
                    break;
                case '/' :
                    vk = '?';
                    break;
                default :
                    if (vk > 32 && vk < KEY_MAX) { //text keys
                        vk -= 32; //case sensitive offset
                    } else {
                        return -1;
                    }
                    break;
            }
        }

    }

    return vkkey2scancode[vk];
}
