/*
 * Multi-touch processing
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
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


#ifndef __MARU_FINGER_H__
#define __MARU_FINGER_H__


/* definitions relating to multi-touch */
//TODO : from arg
#define MAX_FINGER_CNT 3
#define DEFAULT_FINGER_POINT_SIZE 32
#define DEFAULT_FINGER_POINT_COLOR 0x7E0F0F0F
#define DEFAULT_FINGER_POINT_OUTLINE_COLOR 0xDDDDDDDD

typedef struct FingerPoint {
    int id;
    int origin_x;
    int origin_y;
    int x;
    int y;
} FingerPoint;

typedef struct MultiTouchState {
    int multitouch_enable;
    int finger_cnt;
    int finger_cnt_max;
    FingerPoint *finger_slot;

    int finger_point_size;
    int finger_point_color;
    int finger_point_outline_color;
    void *finger_point_surface; //SDL_Surface
} MultiTouchState;


void init_multi_touch_state(void);
void set_multi_touch_enable(int enable);
int get_multi_touch_enable(void);
FingerPoint *get_finger_point_from_slot(int index);
FingerPoint *get_finger_point_search(int x, int y);
void maru_finger_processing_A(int touch_type, int origin_x, int origin_y, int x, int y);
void maru_finger_processing_B(int touch_type, int origin_x, int origin_y, int x, int y);
void clear_finger_slot(void);
void cleanup_multi_touch_state(void);


#endif /* __MARU_FINGER_H__ */
