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


#include <math.h>
#include <glib.h>
#include <SDL.h>
#include "maru_finger.h"
#include "emul_state.h"
#include "debug_ch.h"
#include "console.h"

MULTI_DEBUG_CHANNEL(qemu, maru_finger);


/* ===== Reference: http://content.gpwiki.org/index.php/SDL:Tutorials:Drawing_and_Filling_Circles ===== */
/*
 * This is a 32-bit pixel function created with help from this
* website: http://www.libsdl.org/intro.en/usingvideo.html
*
* You will need to make changes if you want it to work with
* 8-, 16- or 24-bit surfaces.  Consult the above website for
* more information.
*/
static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
   Uint8 *target_pixel = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
   *(Uint32 *)target_pixel = pixel;
}

/*
* This is an implementation of the Midpoint Circle Algorithm 
* found on Wikipedia at the following link:
*
*   http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
*
* The algorithm elegantly draws a circle quickly, using a
* set_pixel function for clarity.
*/
static void sdl_draw_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel) {
   int error = -radius;
   int x = radius;
   int y = 0;

   while (x >= y) {
       sdl_set_pixel(surface, cx + x, cy + y, pixel);
       sdl_set_pixel(surface, cx + y, cy + x, pixel);

       if (x != 0) {
           sdl_set_pixel(surface, cx - x, cy + y, pixel);
           sdl_set_pixel(surface, cx + y, cy - x, pixel);
       }

       if (y != 0) {
           sdl_set_pixel(surface, cx + x, cy - y, pixel);
           sdl_set_pixel(surface, cx - y, cy + x, pixel);
       }

       if (x != 0 && y != 0) {
           sdl_set_pixel(surface, cx - x, cy - y, pixel);
           sdl_set_pixel(surface, cx - y, cy - x, pixel);
       }

       error += y;
       ++y;
       error += y;

       if (error >= 0) {
           --x;
           error -= x;
           error -= x;
       }
   }
}

/*
 * SDL_Surface 32-bit circle-fill algorithm without using trig
*
* While I humbly call this "Celdecea's Method", odds are that the
* procedure has already been documented somewhere long ago.  All of
* the circle-fill examples I came across utilized trig functions or
* scanning neighbor pixels.  This algorithm identifies the width of
* a semi-circle at each pixel height and draws a scan-line covering
* that width.
*
* The code is not optimized but very fast, owing to the fact that it
* alters pixels in the provided surface directly rather than through
* function calls.
*
* WARNING:  This function does not lock surfaces before altering, so
* use SDL_LockSurface in any release situation.
*/
static void sdl_fill_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel)
{
   // Note that there is more to altering the bitrate of this
   // method than just changing this value.  See how pixels are
   // altered at the following web page for tips:
   //   http://www.libsdl.org/intro.en/usingvideo.html
   const int bpp = 4;
   double dy;

   double r = (double)radius;

   for (dy = 1; dy <= r; dy += 1.0)
   {
       // This loop is unrolled a bit, only iterating through half of the
       // height of the circle.  The result is used to draw a scan line and
       // its mirror image below it.
       // The following formula has been simplified from our original.  We
       // are using half of the width of the circle because we are provided
       // with a center and we need left/right coordinates.
       double dx = floor(sqrt((2.0 * r * dy) - (dy * dy)));
       int x = cx - dx;
       // Grab a pointer to the left-most pixel for each half of the circle
       Uint8 *target_pixel_a = (Uint8 *)surface->pixels + ((int)(cy + r - dy)) * surface->pitch + x * bpp;
       Uint8 *target_pixel_b = (Uint8 *)surface->pixels + ((int)(cy - r + dy)) * surface->pitch + x * bpp;

       for ( ; x <= cx + dx; x++)
       {
           *(Uint32 *)target_pixel_a = pixel;
           *(Uint32 *)target_pixel_b = pixel;
           target_pixel_a += bpp;
           target_pixel_b += bpp;
       }
   }
}
/* ==================================================================================================== */

void init_multi_touch_state(void)
{
    int i;
    MultiTouchState* mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;
    INFO("multi-touch state initialization\n");

    mts->multitouch_enable = 0;

    mts->finger_cnt_max = get_emul_max_touch_point();
    if (mts->finger_cnt_max > MAX_FINGER_CNT) {
        mts->finger_cnt_max = MAX_FINGER_CNT; //TODO:
        set_emul_max_touch_point(mts->finger_cnt_max);
    }
    INFO("maxTouchPoint=%d\n", get_emul_max_touch_point());

    mts->finger_cnt = 0;

    if (mts->finger_slot != NULL) {
        g_free(mts->finger_slot);
        mts->finger_slot = NULL;
    }
    mts->finger_slot = (FingerPoint *)g_malloc0(sizeof(FingerPoint) * mts->finger_cnt_max);

    for (i = 0; i < mts->finger_cnt_max; i++) {
        finger = get_finger_point_from_slot(i);
        //finger->id = 0;
        finger->origin_x = finger->origin_y = finger->x = finger->y = -1;
    }

    mts->finger_point_size = DEFAULT_FINGER_POINT_SIZE; //temp
    int finger_point_size_half = mts->finger_point_size / 2;
    mts->finger_point_color = DEFAULT_FINGER_POINT_COLOR; //temp
    mts->finger_point_outline_color = DEFAULT_FINGER_POINT_OUTLINE_COLOR; //temp

    /* create finger point surface */
    Uint32 rmask, gmask, bmask, amask;
#ifdef HOST_WORDS_BIGENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    SDL_Surface *point = SDL_CreateRGBSurface(SDL_SRCALPHA | SDL_HWSURFACE,
		mts->finger_point_size + 2, mts->finger_point_size + 2, get_emul_sdl_bpp(), rmask, gmask, bmask, amask);

    sdl_fill_circle(point, finger_point_size_half, finger_point_size_half,
        finger_point_size_half, mts->finger_point_color); //finger point circle
    sdl_draw_circle(point, finger_point_size_half, finger_point_size_half, finger_point_size_half,
        mts->finger_point_outline_color); // finger point circle outline

    mts->finger_point_surface = (void *)point;
}

void set_multi_touch_enable(int enable)
{
    get_emul_multi_touch_state()->multitouch_enable = enable;
}

int get_multi_touch_enable(void)
{
    return get_emul_multi_touch_state()->multitouch_enable;
}

static int _add_finger_point(int origin_x, int origin_y, int x, int y)
{
    MultiTouchState *mts = get_emul_multi_touch_state();

    if (mts->finger_cnt == mts->finger_cnt_max) {
        INFO("support multi-touch up to %d fingers\n", mts->finger_cnt_max);
        return -1;
    }

    mts->finger_cnt += 1;

    mts->finger_slot[mts->finger_cnt - 1].id = mts->finger_cnt;
    mts->finger_slot[mts->finger_cnt - 1].origin_x = origin_x;
    mts->finger_slot[mts->finger_cnt - 1].origin_y = origin_y;
    mts->finger_slot[mts->finger_cnt - 1].x = x;
    mts->finger_slot[mts->finger_cnt - 1].y = y;
    INFO("%d finger touching\n", mts->finger_cnt);

    return mts->finger_cnt;
}

FingerPoint *get_finger_point_from_slot(int index)
{
    MultiTouchState *mts = get_emul_multi_touch_state();

    if (index < 0 || index > mts->finger_cnt_max) {
        return NULL;
    }

    return &(mts->finger_slot[index]);
}

FingerPoint *get_finger_point_search(int x, int y)
{
    int i;
    MultiTouchState *mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;
    int finger_region = (mts->finger_point_size / 2) + 2 + (int)((1 - get_emul_win_scale()) * 4);

    for (i = mts->finger_cnt - 1; i >= 0; i--) {
        finger = get_finger_point_from_slot(i);

        if (finger != NULL) {
            if (x >= (finger->x - finger_region) &&
                x < (finger->x + finger_region) &&
                y >= (finger->y - finger_region) &&
                y < (finger->y + finger_region)) {
                    return finger;
            }
        }
    }

    return NULL;
}

static int _grab_finger_id = 0;
#define QEMU_MOUSE_PRESSED 1
#define QEMU_MOUSE_RELEASEED 0
void maru_finger_processing_A(int touch_type, int origin_x, int origin_y, int x, int y)
{
    MultiTouchState *mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;

    if (touch_type == MOUSE_DOWN || touch_type == MOUSE_DRAG) { /* pressed */
        if (_grab_finger_id > 0) {
            finger = get_finger_point_from_slot(_grab_finger_id - 1);
            if (finger != NULL) {
                finger->origin_x = origin_x;
                finger->origin_y = origin_y;
                finger->x = x;
                finger->y = y;
                if (finger->id != 0) {
                    kbd_mouse_event(x, y, _grab_finger_id - 1, QEMU_MOUSE_PRESSED);
                    TRACE("id %d finger multi-touch dragging = (%d, %d)\n", _grab_finger_id, x, y);
                }
            }
            return;
        }

        if (mts->finger_cnt == 0)
        { //first finger touch input
            if (_add_finger_point(origin_x, origin_y, x, y) == -1) {
                return;
            }
            kbd_mouse_event(x, y, 0, QEMU_MOUSE_PRESSED);
        }
        else if ((finger = get_finger_point_search(x, y)) != NULL) //check the position of previous touch event
        {
            //finger point is selected
            _grab_finger_id = finger->id;
            TRACE("id %d finger is grabbed\n", _grab_finger_id);
        }
        else if (mts->finger_cnt == mts->finger_cnt_max) //Let's assume that this event is last finger touch input
        {
            finger = get_finger_point_from_slot(mts->finger_cnt_max - 1);
            if (finger != NULL) {
#if 1 //send release event??
                kbd_mouse_event(finger->x, finger->y, mts->finger_cnt_max - 1, 0);
#endif

                finger->origin_x = origin_x;
                finger->origin_y = origin_y;
                finger->x = x;
                finger->y = y;
                if (finger->id != 0) {
                    kbd_mouse_event(x, y, mts->finger_cnt_max - 1, QEMU_MOUSE_PRESSED);
                }
            }
        }
        else //one more finger
        {
            _add_finger_point(origin_x, origin_y, x, y) ;
            kbd_mouse_event(x, y, mts->finger_cnt - 1, QEMU_MOUSE_PRESSED);
        }

    } else if (touch_type == MOUSE_UP) { /* released */
        _grab_finger_id = 0;
    }
}

void maru_finger_processing_B(int touch_type, int origin_x, int origin_y, int x, int y)
{
    MultiTouchState *mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;

    if (touch_type == MOUSE_DOWN || touch_type == MOUSE_DRAG) { /* pressed */
        if (_grab_finger_id > 0) {
            finger = get_finger_point_from_slot(_grab_finger_id - 1);
            if (finger != NULL) {
                int origin_distance_x = origin_x - finger->origin_x;
                int origin_distance_y = origin_y - finger->origin_y;
                int distance_x = x - finger->x;
                int distance_y = y - finger->y;

                int current_screen_w = get_emul_lcd_width();
                int current_screen_h = get_emul_lcd_height();
                int temp_finger_x, temp_finger_y;

                int i;
                /* bounds checking */
                for(i = 0; i < get_emul_multi_touch_state()->finger_cnt; i++) {
                    finger = get_finger_point_from_slot(i);

                    if (finger != NULL) {
                        temp_finger_x = finger->x + distance_x;
                        temp_finger_y = finger->y + distance_y;
                        if (temp_finger_x > current_screen_w || temp_finger_x < 0
                            || temp_finger_y > current_screen_h || temp_finger_y < 0) {
                            TRACE("id %d finger is out of bounds (%d, %d)\n", i + 1, temp_finger_x, temp_finger_y);
                            return;
                        }
                    }
                }

                for(i = 0; i < get_emul_multi_touch_state()->finger_cnt; i++) {
                    finger = get_finger_point_from_slot(i);

                    if (finger != NULL) {
                        finger->origin_x += origin_distance_x;
                        finger->origin_y += origin_distance_y;
                        finger->x += distance_x;
                        finger->y += distance_y;
                        if (finger->id != 0) {
                            kbd_mouse_event(finger->x, finger->y, i, QEMU_MOUSE_PRESSED);
                            TRACE("id %d finger multi-touch dragging = (%d, %d)\n", i + 1, finger->x, finger->y);
                        }
#ifdef _WIN32
                        Sleep(2);
#else
                        usleep(2000);
#endif
                    }
                }
            }
            return;
        }

        if (mts->finger_cnt == 0)
        { //first finger touch input
            if (_add_finger_point(origin_x, origin_y, x, y) == -1) {
                return;
            }
            kbd_mouse_event(x, y, 0, QEMU_MOUSE_PRESSED);
        }
        else if ((finger = get_finger_point_search(x, y)) != NULL) //check the position of previous touch event
        {
            //finger point is selected
            _grab_finger_id = finger->id;
            TRACE("id %d finger is grabbed\n", _grab_finger_id);
        }
        else if (mts->finger_cnt == mts->finger_cnt_max) //Let's assume that this event is last finger touch input
        {
            //do nothing
        }
        else //one more finger
        {
            _add_finger_point(origin_x, origin_y, x, y) ;
            kbd_mouse_event(x, y, mts->finger_cnt - 1, QEMU_MOUSE_PRESSED);
        }

    } else if (touch_type == MOUSE_UP) { /* released */
        _grab_finger_id = 0;
    }
}

void clear_finger_slot(void)
{
    int i;
    MultiTouchState *mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;

    for (i = 0; i < mts->finger_cnt; i++) {
        finger = get_finger_point_from_slot(i);
        if (finger != NULL && finger->id != 0) {
            kbd_mouse_event(finger->x, finger->y, finger->id - 1, QEMU_MOUSE_RELEASEED);
        }

        finger->id = 0;
        finger->origin_x = finger->origin_y = finger->x = finger->y = -1;
    }

    _grab_finger_id = 0;

    mts->finger_cnt = 0;
    INFO("clear multi-touch\n");
}

void cleanup_multi_touch_state(void)
{
    MultiTouchState *mts = get_emul_multi_touch_state();
    SDL_Surface *point = (SDL_Surface *)mts->finger_point_surface;

    mts->multitouch_enable = 0;

    clear_finger_slot();
    g_free(mts->finger_slot);

    mts->finger_point_surface = NULL;
    SDL_FreeSurface(point);
}

