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


#ifndef MARUSKIN_KEYMAP_H_
#define MARUSKIN_KEYMAP_H_


/* keep it consistent with emulator-skin(swt) virtual keycode */
#define JAVA_KEYCODE_BIT (1 << 24)
#define JAVA_KEYCODE_NO_FOCUS (1 << 19)
#define JAVA_KEYCODE_BIT_CTRL (1 << 18)
#define JAVA_KEYCODE_BIT_SHIFT (1 << 17)
#define JAVA_KEYCODE_BIT_ALT (1 << 16)

//key location
#define JAVA_KEYLOCATION_KEYPAD (1 << 1)
#define JAVA_KEYLOCATION_LEFT (1 << 14)
#define JAVA_KEYLOCATION_RIGHT (1 << 17)

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
    JAVA_KEY_KEYPAD_MULTIPLY = 42,
    JAVA_KEY_KEYPAD_ADD = 43,
    JAVA_KEY_KEYPAD_SUBTRACT = 45,
    JAVA_KEY_KEYPAD_DECIMAL = 46,
    JAVA_KEY_KEYPAD_DIVIDE = 47,
    JAVA_KEY_KEYPAD_0 = 48,
    JAVA_KEY_KEYPAD_1 = 49,
    JAVA_KEY_KEYPAD_2 = 50,
    JAVA_KEY_KEYPAD_3 = 51,
    JAVA_KEY_KEYPAD_4 = 52,
    JAVA_KEY_KEYPAD_5 = 53,
    JAVA_KEY_KEYPAD_6 = 54,
    JAVA_KEY_KEYPAD_7 = 55,
    JAVA_KEY_KEYPAD_8 = 56,
    JAVA_KEY_KEYPAD_9 = 57,
    JAVA_KEY_KEYPAD_CR = 80,
    JAVA_KEY_CAPS_LOCK = 82,
    JAVA_KEY_NUM_LOCK,
    JAVA_KEY_SCROLL_LOCK,
    JAVA_KEY_PAUSE,
    JAVA_KEY_BREAK,
    JAVA_KEY_PRINT_SCREEN
};


#define KEY_MAX 0777

#define KEY_F0 0410
#define KEY_F(n) (KEY_F0+(n))

#define KEY_DOWN 0402
#define KEY_UP 0403
#define KEY_LEFT 0404
#define KEY_RIGHT 0405

#define KEY_BTAB 0541


#define SHIFT 0
static const int vkkey2scancode[KEY_MAX] = {
    [0 ... (KEY_MAX - 1)] = -1,

    [0x01b] = 1, /* Escape */
    ['1'] = 2,
    ['2'] = 3,
    ['3'] = 4,
    ['4'] = 5,
    ['5'] = 6,
    ['6'] = 7,
    ['7'] = 8,
    ['8'] = 9,
    ['9'] = 10,
    ['0'] = 11,
    ['-'] = 12,
    ['='] = 13,
    [0x07f] = 83, /* Delete */
    [0x008] = 14, /* Backspace */

    ['\t'] = 15, /* Tab */
    ['q'] = 16,
    ['w'] = 17,
    ['e'] = 18,
    ['r'] = 19,
    ['t'] = 20,
    ['y'] = 21,
    ['u'] = 22,
    ['i'] = 23,
    ['o'] = 24,
    ['p'] = 25,
    ['['] = 26,
    [']'] = 27,
    ['\n'] = 28, /* Return */
    ['\r'] = 28, /* Return */

    ['a'] = 30,
    ['s'] = 31,
    ['d'] = 32,
    ['f'] = 33,
    ['g'] = 34,
    ['h'] = 35,
    ['j'] = 36,
    ['k'] = 37,
    ['l'] = 38,
    [';'] = 39,
    ['\''] = 40, /* Single quote */
    ['`'] = 41,
    ['\\'] = 43, /* Backslash */

    ['z'] = 44,
    ['x'] = 45,
    ['c'] = 46,
    ['v'] = 47,
    ['b'] = 48,
    ['n'] = 49,
    ['m'] = 50,
    [','] = 51,
    ['.'] = 52,
    ['/'] = 53,

    [' '] = 57, /* Space */

    [KEY_F(1)] = 59, /* Function Key 1 */
    [KEY_F(2)] = 60, /* Function Key 2 */
    [KEY_F(3)] = 61, /* Function Key 3 */
    [KEY_F(4)] = 62, /* Function Key 4 */
    [KEY_F(5)] = 63, /* Function Key 5 */
    [KEY_F(6)] = 64, /* Function Key 6 */
    [KEY_F(7)] = 65, /* Function Key 7 */
    [KEY_F(8)] = 66, /* Function Key 8 */
    [KEY_F(9)] = 67, /* Function Key 9 */
    [KEY_F(10)] = 68, /* Function Key 10 */
    [KEY_F(11)] = 87, /* Function Key 11 */
    [KEY_F(12)] = 88, /* Function Key 12 */

    [KEY_UP] = 72, /* Up Arrow */
    [KEY_LEFT] = 75, /* Left Arrow */
    [KEY_RIGHT] = 77, /* Right Arrow */
    [KEY_DOWN] = 80, /* Down Arrow */

    ['!'] = 2 | SHIFT,
    ['@'] = 3 | SHIFT,
    ['#'] = 4 | SHIFT,
    ['$'] = 5 | SHIFT,
    ['%'] = 6 | SHIFT,
    ['^'] = 7 | SHIFT,
    ['&'] = 8 | SHIFT,
    ['*'] = 9 | SHIFT,
    ['('] = 10 | SHIFT,
    [')'] = 11 | SHIFT,
    ['_'] = 12 | SHIFT,
    ['+'] = 13 | SHIFT,

    [KEY_BTAB] = 15 | SHIFT, /* Shift + Tab */
    ['Q'] = 16 | SHIFT,
    ['W'] = 17 | SHIFT,
    ['E'] = 18 | SHIFT,
    ['R'] = 19 | SHIFT,
    ['T'] = 20 | SHIFT,
    ['Y'] = 21 | SHIFT,
    ['U'] = 22 | SHIFT,
    ['I'] = 23 | SHIFT,
    ['O'] = 24 | SHIFT,
    ['P'] = 25 | SHIFT,
    ['{'] = 26 | SHIFT,
    ['}'] = 27 | SHIFT,

    ['A'] = 30 | SHIFT,
    ['S'] = 31 | SHIFT,
    ['D'] = 32 | SHIFT,
    ['F'] = 33 | SHIFT,
    ['G'] = 34 | SHIFT,
    ['H'] = 35 | SHIFT,
    ['J'] = 36 | SHIFT,
    ['K'] = 37 | SHIFT,
    ['L'] = 38 | SHIFT,
    [':'] = 39 | SHIFT,
    ['"'] = 40 | SHIFT,
    ['~'] = 41 | SHIFT,
    ['|'] = 43 | SHIFT,

    ['Z'] = 44 | SHIFT,
    ['X'] = 45 | SHIFT,
    ['C'] = 46 | SHIFT,
    ['V'] = 47 | SHIFT,
    ['B'] = 48 | SHIFT,
    ['N'] = 49 | SHIFT,
    ['M'] = 50 | SHIFT,
    ['<'] = 51 | SHIFT,
    ['>'] = 52 | SHIFT,
    ['?'] = 53 | SHIFT,
};

int javakeycode_to_scancode(int java_keycode, int event_type, int key_location);

#endif /* MARUSKIN_KEYMAP_H_ */
