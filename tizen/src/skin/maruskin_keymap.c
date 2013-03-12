/*
 * keymap
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

#ifdef KEYMAP_DEBUG
static void trace_binary(int decimal)
{
    if (decimal != 0) {
        trace_binary(decimal / 2);
        fprintf(stdout, "%d", decimal % 2);
        fflush(stdout);
    }
}
#endif

int javakeycode_to_scancode(
    int event_type, int java_keycode, int state_mask, int key_location)
{
    bool character = true;
    int vk = 0;

#ifdef KEYMAP_DEBUG
    /* print key information */
    TRACE("keycode = %d(", java_keycode);
    trace_binary(java_keycode);
    fprintf(stdout, ")\n");
#endif

    character = !(java_keycode & JAVA_KEYCODE_BIT);
    vk = java_keycode & JAVA_KEY_MASK;

    /* CapsLock & NumLock key processing */
    if (character == false) {
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
        int caps_lock = -1;
        int num_lock = -1;

        caps_lock = get_host_lock_key_state(HOST_CAPSLOCK_KEY);
        num_lock = get_host_lock_key_state(HOST_NUMLOCK_KEY);

        if (caps_lock != -1 && get_emul_caps_lock_state() != caps_lock) {
            kbd_put_keycode(58);
            kbd_put_keycode(58 | 0x80);
            set_emul_caps_lock_state(get_emul_caps_lock_state() ^ 1);
            INFO("qemu CapsLock state was synchronized with host key value (%d)\n",
                get_emul_caps_lock_state());
        }
        if (num_lock != -1 && get_emul_num_lock_state() != num_lock) {
            kbd_put_keycode(69);
            kbd_put_keycode(69 | 0x80);
            set_emul_num_lock_state(get_emul_num_lock_state() ^ 1);
            INFO("qemu NumLock state was synchronized with host key value (%d)\n",
                get_emul_num_lock_state());
        }
    }

#if 0
#ifdef _WIN32
    return MapVirtualKey(vk, MAPVK_VK_TO_VSC);
#endif
#endif

    if (vk == 0) { //meta keys
        TRACE("meta key\n");

        if (java_keycode == JAVA_KEYCODE_BIT_CTRL) { /* ctrl key */
            if (key_location == JAVA_KEYLOCATION_RIGHT) {
                kbd_put_keycode(224); //0xE0
            }
            return 29;
        } else if (java_keycode == JAVA_KEYCODE_BIT_SHIFT) { /* shift key */
#ifndef _WIN32
            //keyLocation information is not supported at swt of windows version
            if (key_location == JAVA_KEYLOCATION_RIGHT) {
                return 54; /* Shift_R */
            }
#endif
            return 42;
        } else if (java_keycode == JAVA_KEYCODE_BIT_ALT) { /* alt key */
            if (key_location == JAVA_KEYLOCATION_RIGHT) {
                kbd_put_keycode(224);
            }
            return 56;
        } else {
            return -1;
        }
    }

    if (character == false)
    { /* non-character keys */
        if (vk >= JAVA_KEY_F1 && vk <= JAVA_KEY_F20) { /* function keys */
            TRACE("function key\n");

            vk += 255;
        } else { /* special keys */
            TRACE("special key\n");

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
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    return 73;
                case JAVA_KEY_PAGE_DOWN :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    return 81;
                case JAVA_KEY_HOME :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    return 71;
                case JAVA_KEY_END :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
                    return 79;
                case JAVA_KEY_INSERT :
                    if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                        kbd_put_keycode(224);
                    }
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
                case JAVA_KEY_KEYPAD_CR : /* KP_ENTER */
                    kbd_put_keycode(224);
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
    else
    { /* character keys */
        switch(vk) {
            case JAVA_KEY_DELETE :
                if (key_location != JAVA_KEYLOCATION_KEYPAD) {
                    kbd_put_keycode(224);
                }
                break;
            default :
                break;
        }
    }

    return vkkey2scancode[vk];
}
