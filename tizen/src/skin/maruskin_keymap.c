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
#include "emul_state.h"
#include "console.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, skin_keymap);


int javakeycode_to_scancode(int java_keycode, int event_type, int key_location)
{
    int state_mask = java_keycode & JAVA_KEYCODE_BIT;
    int vk = java_keycode & JAVA_KEY_MASK;

    int ctrl_mask = java_keycode & JAVA_KEYCODE_BIT_CTRL;
    int shift_mask = java_keycode & JAVA_KEYCODE_BIT_SHIFT;
    int alt_mask = java_keycode & JAVA_KEYCODE_BIT_ALT;


    /* CapsLock & NumLock key processing */
    if (state_mask != 0) {
        if (vk == JAVA_KEY_CAPS_LOCK) {
            if (event_type == KEY_PRESSED) {
                set_emul_caps_lock_state(get_emul_caps_lock_state() ^ 1); //toggle
            }
            return 58;
        } else if (vk == JAVA_KEY_NUM_LOCK) {
            if (event_type == KEY_PRESSED) {
                set_emul_num_lock_state(get_emul_num_lock_state() ^ 1); //toggle
            }
            return 69;
        }
    }
    /* check CapsLock & NumLock key sync */
    if (event_type == KEY_PRESSED) {
        if (get_emul_caps_lock_state() != get_host_lock_key_state(HOST_CAPSLOCK_KEY)) {
            kbd_put_keycode(58);
            kbd_put_keycode(58 | 0x80);
            set_emul_caps_lock_state(get_emul_caps_lock_state() ^ 1);
            INFO("qemu CapsLock state was synchronized with host key value (%d)\n", get_emul_caps_lock_state());
        }
        if (get_emul_num_lock_state() != get_host_lock_key_state(HOST_NUMLOCK_KEY)) {
            kbd_put_keycode(69);
            kbd_put_keycode(69 | 0x80);
            set_emul_num_lock_state(get_emul_num_lock_state() ^ 1);
            INFO("qemu NumLock state was synchronized with host key value (%d)\n", get_emul_num_lock_state());
        }
    }


/*#ifdef _WIN32
    return MapVirtualKey(vk, MAPVK_VK_TO_VSC);
#endif*/

    if (vk == 0) { //meta keys
        if (java_keycode == JAVA_KEYCODE_BIT_CTRL) { //ctrl key
            if (key_location == JAVA_KEYLOCATION_RIGHT) {
                kbd_put_keycode(224); //0xE0
            }
            return 29;
        } else if (java_keycode == JAVA_KEYCODE_BIT_SHIFT) { //shift key
#ifndef _WIN32
            //keyLocation information is not supported at swt of windows version
            if (key_location == JAVA_KEYLOCATION_RIGHT) {
                return 54; //Shift_R
            }
#endif
            return 42;
        } else if (java_keycode == JAVA_KEYCODE_BIT_ALT) { //alt key
            if (key_location == JAVA_KEYLOCATION_RIGHT) {
                kbd_put_keycode(224);
            }
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
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    vk = KEY_UP;
                    break;
                case JAVA_KEY_ARROW_DOWN :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    vk = KEY_DOWN;
                    break;
                case JAVA_KEY_ARROW_LEFT :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    vk = KEY_LEFT;
                    break;
                case JAVA_KEY_ARROW_RIGHT :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    vk = KEY_RIGHT;
                    break;

                case JAVA_KEY_PAGE_UP :
                    return 73;
                case JAVA_KEY_PAGE_DOWN :
                    return 81;
                case JAVA_KEY_HOME :
                    return 71;
                case JAVA_KEY_END :
                    return 79;
                case JAVA_KEY_INSERT :
                    return 82;

                case JAVA_KEY_KEYPAD_MULTIPLY :
                    return 55;
                case JAVA_KEY_KEYPAD_ADD :
                    return 78;
                case JAVA_KEY_KEYPAD_SUBTRACT :
                    return 74;
                case JAVA_KEY_KEYPAD_DECIMAL :
                    return 83;
                case JAVA_KEY_KEYPAD_DIVIDE :
                    return 53;
                case JAVA_KEY_KEYPAD_0 :
                    return 82;
                case JAVA_KEY_KEYPAD_1 :
                    return 79;
                case JAVA_KEY_KEYPAD_2 :
                    return 80;
                case JAVA_KEY_KEYPAD_3 :
                    return 81;
                case JAVA_KEY_KEYPAD_4 :
                    return 75;
                case JAVA_KEY_KEYPAD_5 :
                    return 76;
                case JAVA_KEY_KEYPAD_6 :
                    return 77;
                case JAVA_KEY_KEYPAD_7 :
                    return 71;
                case JAVA_KEY_KEYPAD_8 :
                    return 72;
                case JAVA_KEY_KEYPAD_9 :
                    return 73;
                case JAVA_KEY_KEYPAD_CR :
                    return 28;
 
                case JAVA_KEY_SCROLL_LOCK :
                    return 70;
                case JAVA_KEY_PAUSE :
                case JAVA_KEY_BREAK :
                    return 198;
                case JAVA_KEY_PRINT_SCREEN :
                    //not support
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
                    break;
            }
        }
    }

    return vkkey2scancode[vk];
}
