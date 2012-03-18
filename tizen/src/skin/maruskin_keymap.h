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

/*#include <curses.h>

#define SCANCODE_GREY   0x80
#define SCANCODE_SHIFT  0x100
#define SCANCODE_CTRL   0x200

#define GREY                SCANCODE_GREY
#define SHIFT_CODE          0x2a
#define SHIFT               SCANCODE_SHIFT
#define CNTRL_CODE          0x1d
#define CNTRL               SCANCODE_CTRL*/

#define CURSES_KEYS 0777
//KEY_MAX     /* KEY_MAX defined in <curses.h> */

static const int curses2keycode[CURSES_KEYS] = {
    [0 ... (CURSES_KEYS - 1)] = -1,

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
    [0x07f] = 14, /* Backspace */
    //[KEY_BACKSPACE] = 14, /* Backspace */
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
    //[KEY_ENTER] = 28, /* Return */

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

    [' '] = 57,

#if 0
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

    [KEY_HOME] = 71 | GREY, /* Home */
    [KEY_UP] = 72 | GREY, /* Up Arrow */
    [KEY_PPAGE] = 73 | GREY, /* Page Up */
    [KEY_LEFT] = 75 | GREY, /* Left Arrow */
    [KEY_RIGHT] = 77 | GREY, /* Right Arrow */
    [KEY_END] = 79 | GREY, /* End */
    [KEY_DOWN] = 80 | GREY, /* Down Arrow */
    [KEY_NPAGE] = 81 | GREY, /* Page Down */
    [KEY_IC] = 82 | GREY, /* Insert */
    [KEY_DC] = 83 | GREY, /* Delete */

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

    [KEY_F(13)] = 59 | SHIFT, /* Shift + Function Key 1 */
    [KEY_F(14)] = 60 | SHIFT, /* Shift + Function Key 2 */
    [KEY_F(15)] = 61 | SHIFT, /* Shift + Function Key 3 */
    [KEY_F(16)] = 62 | SHIFT, /* Shift + Function Key 4 */
    [KEY_F(17)] = 63 | SHIFT, /* Shift + Function Key 5 */
    [KEY_F(18)] = 64 | SHIFT, /* Shift + Function Key 6 */
    [KEY_F(19)] = 65 | SHIFT, /* Shift + Function Key 7 */
    [KEY_F(20)] = 66 | SHIFT, /* Shift + Function Key 8 */
    [KEY_F(21)] = 67 | SHIFT, /* Shift + Function Key 9 */
    [KEY_F(22)] = 68 | SHIFT, /* Shift + Function Key 10 */
    [KEY_F(23)] = 69 | SHIFT, /* Shift + Function Key 11 */
    [KEY_F(24)] = 70 | SHIFT, /* Shift + Function Key 12 */

    ['Q' - '@'] = 16 | CNTRL, /* Control + q */
    ['W' - '@'] = 17 | CNTRL, /* Control + w */
    ['E' - '@'] = 18 | CNTRL, /* Control + e */
    ['R' - '@'] = 19 | CNTRL, /* Control + r */
    ['T' - '@'] = 20 | CNTRL, /* Control + t */
    ['Y' - '@'] = 21 | CNTRL, /* Control + y */
    ['U' - '@'] = 22 | CNTRL, /* Control + u */
    /* Control + i collides with Tab */
    ['O' - '@'] = 24 | CNTRL, /* Control + o */
    ['P' - '@'] = 25 | CNTRL, /* Control + p */

    ['A' - '@'] = 30 | CNTRL, /* Control + a */
    ['S' - '@'] = 31 | CNTRL, /* Control + s */
    ['D' - '@'] = 32 | CNTRL, /* Control + d */
    ['F' - '@'] = 33 | CNTRL, /* Control + f */
    ['G' - '@'] = 34 | CNTRL, /* Control + g */
    ['H' - '@'] = 35 | CNTRL, /* Control + h */
    /* Control + j collides with Return */
    ['K' - '@'] = 37 | CNTRL, /* Control + k */
    ['L' - '@'] = 38 | CNTRL, /* Control + l */

    ['Z' - '@'] = 44 | CNTRL, /* Control + z */
    ['X' - '@'] = 45 | CNTRL, /* Control + x */
    ['C' - '@'] = 46 | CNTRL, /* Control + c */
    ['V' - '@'] = 47 | CNTRL, /* Control + v */
    ['B' - '@'] = 48 | CNTRL, /* Control + b */
    ['N' - '@'] = 49 | CNTRL, /* Control + n */
    /* Control + m collides with the keycode for Enter */
#endif
};

int javakeycode_to_scancode(int java_keycode);

#endif /* MARUSKIN_KEYMAP_H_ */
