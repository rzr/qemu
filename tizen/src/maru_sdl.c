/*
 * SDL_WINDOWID hack
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Jinhyung Jo <jinhyung.jo@samsung.com>
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


#include <pthread.h>
#include <math.h>
#include "ui/console.h"
#include "maru_sdl.h"
#include "maru_display.h"
#include "emul_state.h"
#include "maru_finger.h"
#include "hw/maru_pm.h"
#include "hw/maru_brightness.h"
#include "hw/maru_overlay.h"
#include "debug_ch.h"
/* #include "SDL_opengl.h" */

MULTI_DEBUG_CHANNEL(tizen, maru_sdl);

DisplaySurface *qemu_display_surface = NULL;

static SDL_Surface *surface_screen;
static SDL_Surface *surface_qemu;
static SDL_Surface *scaled_screen;
static SDL_Surface *rotated_screen;

static double current_scale_factor = 1.0;
static double current_screen_degree;
static int current_screen_width;
static int current_screen_height;

static int sdl_initialized;
static int sdl_alteration;

static int sdl_skip_update;

#if 0
static int sdl_opengl = 0; //0 : just SDL surface, 1 : using SDL with OpenGL
GLuint texture;
#endif


#define SDL_THREAD

static pthread_mutex_t sdl_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef SDL_THREAD
static pthread_cond_t sdl_cond = PTHREAD_COND_INITIALIZER;
static int sdl_thread_initialized;
#endif

extern pthread_mutex_t mutex_screenshot;
extern pthread_cond_t cond_screenshot;

#define SDL_FLAGS (SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_NOFRAME)
#define SDL_BPP 32

/* Image processing functions using the pixman library */
static void maru_do_pixman_sdl(pixman_image_t *dst_image)
{
    /* overlay0 */
    if (overlay0_power) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               overlay0_image, NULL, dst_image,
                               0, 0, 0, 0, overlay0_left, overlay0_top,
                               overlay0_width, overlay0_height);
    }
    /* overlay1 */
    if (overlay1_power) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               overlay1_image, NULL, dst_image,
                               0, 0, 0, 0, overlay1_left, overlay1_top,
                               overlay1_width, overlay1_height);
    }
    /* apply the brightness level */
    if (brightness_level < BRIGHTNESS_MAX) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               brightness_image, NULL, dst_image,
                               0, 0, 0, 0, 0, 0,
                               pixman_image_get_width(dst_image),
                               pixman_image_get_height(dst_image));
    }
}

static SDL_Surface *maru_do_pixman_scale(SDL_Surface *rz_src,
                                         SDL_Surface *rz_dst,
                                         int angle)
{
    pixman_image_t *src = NULL;
    pixman_image_t *dst = NULL;
    double sx = 0;
    double sy = 0;
    pixman_transform_t matrix;
    struct pixman_f_transform matrix_f;

    SDL_LockSurface(rz_src);
    SDL_LockSurface(rz_dst);

    src = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_src->w, rz_src->h, rz_src->pixels, rz_src->w * 4);
    dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_dst->w, rz_dst->h, rz_dst->pixels, rz_dst->w * 4);

    sx = (double)rz_src->w / (double)rz_dst->w;
    sy = (double)rz_src->h / (double)rz_dst->h;
    pixman_f_transform_init_identity(&matrix_f);
    pixman_f_transform_scale(&matrix_f, NULL, sx, sy);
    pixman_transform_from_pixman_f_transform(&matrix, &matrix_f);
    pixman_image_set_transform(src, &matrix);
    pixman_image_set_filter(src, PIXMAN_FILTER_BILINEAR, NULL, 0);
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst,
                           0, 0, 0, 0, 0, 0,
                           rz_dst->w, rz_dst->h);

    pixman_image_unref(src);
    pixman_image_unref(dst);

    SDL_UnlockSurface(rz_src);
    SDL_UnlockSurface(rz_dst);

    return rz_dst;
}

static SDL_Surface *maru_do_pixman_rotate(SDL_Surface *rz_src,
                                          SDL_Surface *rz_dst,
                                          int angle)
{
    pixman_image_t *src = NULL;
    pixman_image_t *dst = NULL;
    pixman_transform_t matrix;
    struct pixman_f_transform matrix_f;

    SDL_LockSurface(rz_src);
    SDL_LockSurface(rz_dst);

    src = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_src->w, rz_src->h, rz_src->pixels, rz_src->w * 4);
    dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
        rz_dst->w, rz_dst->h, rz_dst->pixels, rz_dst->w * 4);

    pixman_f_transform_init_identity(&matrix_f);
    switch(angle) {
        case 0:
            pixman_f_transform_rotate(&matrix_f, NULL, 1.0, 0.0);
            pixman_f_transform_translate(&matrix_f, NULL, 0.0, 0.0);
            break;
        case 90:
            pixman_f_transform_rotate(&matrix_f, NULL, 0.0, 1.0);
            pixman_f_transform_translate(&matrix_f, NULL,
                                         (double)rz_dst->h, 0.0);
            break;
        case 180:
            pixman_f_transform_rotate(&matrix_f, NULL, -1.0, 0.0);
            pixman_f_transform_translate(&matrix_f, NULL,
                                         (double)rz_dst->w, (double)rz_dst->h);
            break;
        case 270:
            pixman_f_transform_rotate(&matrix_f, NULL, 0.0, -1.0);
            pixman_f_transform_translate(&matrix_f, NULL,
                                         0.0, (double)rz_dst->w);
            break;
        default:
            fprintf(stdout, "not supported angle factor (angle=%d)\n", angle);
            break;
    }
    pixman_transform_from_pixman_f_transform(&matrix, &matrix_f);
    pixman_image_set_transform(src, &matrix);
    pixman_image_set_filter(src, PIXMAN_FILTER_BILINEAR, NULL, 0);
    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst,
                           0, 0, 0, 0, 0, 0,
                           rz_dst->w, rz_dst->h);

    pixman_image_unref(src);
    pixman_image_unref(dst);

    SDL_UnlockSurface(rz_src);
    SDL_UnlockSurface(rz_dst);

    return rz_dst;
}

static void qemu_ds_sdl_update(DisplayChangeListener *dcl,
                               int x, int y, int w, int h)
{
    /* call sdl update */
#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);

    pthread_cond_signal(&sdl_cond);

    pthread_mutex_unlock(&sdl_mutex);
#else
    qemu_update();
#endif

    /* for screenshot */
    pthread_mutex_lock(&mutex_screenshot);

    MaruScreenshot* maru_screenshot = get_maru_screenshot();
    if (maru_screenshot) {
        maru_screenshot->isReady = 1;

        if (maru_screenshot->request_screenshot == 1) {
            memcpy(maru_screenshot->pixel_data,
                   surface_data(qemu_display_surface),
                   surface_stride(qemu_display_surface) *
                   surface_height(qemu_display_surface));
            maru_screenshot->request_screenshot = 0;

            pthread_cond_signal(&cond_screenshot);
        }
    }

    pthread_mutex_unlock(&mutex_screenshot);
}

static void qemu_ds_sdl_switch(DisplayChangeListener *dcl,
                               struct DisplaySurface *new_surface)
{
    int w, h;

    if (!new_surface) {
        ERR("qemu_ds_sdl_switch: new_surface is NULL\n");
        return;
    }

    qemu_display_surface = new_surface;
    w = surface_width(new_surface);
    h = surface_height(new_surface);

    INFO("qemu_ds_sdl_switch = (%d, %d)\n", w, h);

#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    if (surface_qemu != NULL) {
        SDL_FreeSurface(surface_qemu);
        surface_qemu = NULL;
    }

    /* create surface_qemu */
    if (w == get_emul_lcd_width() && h == get_emul_lcd_height()) {
        INFO("create SDL screen = (%d, %d)\n",
             get_emul_lcd_width(), get_emul_lcd_height());
        surface_qemu = SDL_CreateRGBSurfaceFrom(
            surface_data(qemu_display_surface),
            w, h,
            surface_bits_per_pixel(qemu_display_surface),
            surface_stride(qemu_display_surface),
            qemu_display_surface->pf.rmask,
            qemu_display_surface->pf.gmask,
            qemu_display_surface->pf.bmask,
            qemu_display_surface->pf.amask);
    } else {
        INFO("create blank screen = (%d, %d)\n",
             get_emul_lcd_width(), get_emul_lcd_height());
        surface_qemu = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
            surface_bits_per_pixel(qemu_display_surface), 0, 0, 0, 0);
    }

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif

    if (surface_qemu == NULL) {
        ERR("Unable to set the RGBSurface: %s\n", SDL_GetError());
        return;
    }

}

static int maru_sdl_poll_event(SDL_Event *ev)
{
    int ret = 0;

    if (sdl_initialized == 1) {
        //pthread_mutex_lock(&sdl_mutex);
        ret = SDL_PollEvent(ev);
        //pthread_mutex_unlock(&sdl_mutex);
    }

    return ret;
}
static void qemu_ds_sdl_refresh(DisplayChangeListener *dcl)
{
    SDL_Event ev1, *ev = &ev1;
    static uint32_t sdl_skip_count = 0;

    while (maru_sdl_poll_event(ev)) {
        switch (ev->type) {
            case SDL_VIDEORESIZE:
            {
                pthread_mutex_lock(&sdl_mutex);

                maruskin_sdl_init(0,
                        get_emul_lcd_width(), get_emul_lcd_height(),
                        true);

                pthread_mutex_unlock(&sdl_mutex);
                sdl_skip_update = 0;
                sdl_skip_count = 0;
                break;
            }

            default:
                break;
        }
    }

    /* If the LCD is turned off,
       the screen does not update until the LCD is turned on */
    if (sdl_skip_update && brightness_off) {
        return;
    }

    /* Usually, continuously updated.
       When the LCD is turned off,
       ten more updates the screen for a black screen. */
    if (brightness_off) {
        dpy_gfx_update(dcl->con, 0, 0, 0, 0);
        if (++sdl_skip_count > 10) {
            sdl_skip_update = 1;
        } else {
            sdl_skip_update = 0;
        }
    } else {
        graphic_hw_update(dcl->con);
        sdl_skip_count = 0;
        sdl_skip_update = 0;
    }

#ifdef TARGET_ARM
#ifdef SDL_THREAD
    pthread_mutex_lock(&sdl_mutex);
#endif

    /*
    * It is necessary only for exynos4210 FIMD in connection with
    * some WM (xfwm4, for example)
    */

    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);

#ifdef SDL_THREAD
    pthread_mutex_unlock(&sdl_mutex);
#endif
#endif
}

DisplayChangeListenerOps maru_dcl_ops = {
    .dpy_name          = "maru_sdl",
    .dpy_gfx_update    = qemu_ds_sdl_update,
    .dpy_gfx_switch    = qemu_ds_sdl_switch,
    .dpy_refresh       = qemu_ds_sdl_refresh,
};

//extern int capability_check_gl;
static void _sdl_init(void)
{
    int w, h, rwidth, rheight, temp;

    INFO("Set up a video mode with the specified width, "
         "height and bits-per-pixel\n");

    //get current setting information and calculate screen size
    rwidth = get_emul_lcd_width();
    rheight = get_emul_lcd_height();
    current_scale_factor = get_emul_win_scale();
    w = current_screen_width = rwidth * current_scale_factor;
    h = current_screen_height = rheight * current_scale_factor;

    short rotaton_type = get_emul_rotation();
    if (rotaton_type == ROTATION_PORTRAIT) {
        current_screen_degree = 0.0;
    } else if (rotaton_type == ROTATION_LANDSCAPE) {
        current_screen_degree = 90.0;
        temp = w;
        w = h;
        h = temp;
        temp = rwidth;
        rwidth = rheight;
        rheight = temp;
    } else if (rotaton_type == ROTATION_REVERSE_PORTRAIT) {
        current_screen_degree = 180.0;
    } else if (rotaton_type == ROTATION_REVERSE_LANDSCAPE) {
        current_screen_degree = 270.0;
        temp = w;
        w = h;
        h = temp;
        temp = rwidth;
        rwidth = rheight;
        rheight = temp;
    }

#if 0
    if (capability_check_gl != 0) {
        ERR("GL check returned non-zero\n");
        surface_screen = NULL;
    } else {
        surface_screen = SDL_SetVideoMode(w, h, get_emul_sdl_bpp(), SDL_OPENGL);
    }

    if (surface_screen == NULL) {
        sdl_opengl = 0;
        INFO("No OpenGL support on this system!??\n");
        ERR("%s\n", SDL_GetError());
#endif

        surface_screen = SDL_SetVideoMode(w, h,
                                          get_emul_sdl_bpp(),
                                          SDL_FLAGS);
        if (surface_screen == NULL) {
            ERR("Could not open SDL display (%dx%dx%d): %s\n",
                w, h, get_emul_sdl_bpp(), SDL_GetError());
            return;
        }
#if 0
    } else {
        sdl_opengl = 1;
        INFO("OpenGL is supported on this system.\n");
    }
#endif

    /* create buffer for image processing */
    SDL_FreeSurface(scaled_screen);
    scaled_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        w, h, get_emul_sdl_bpp(),
        surface_qemu->format->Rmask, surface_qemu->format->Gmask,
        surface_qemu->format->Bmask, surface_qemu->format->Amask);
    SDL_FreeSurface(rotated_screen);
    rotated_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        rwidth, rheight, get_emul_sdl_bpp(),
        surface_qemu->format->Rmask, surface_qemu->format->Gmask,
        surface_qemu->format->Bmask, surface_qemu->format->Amask);

#if 0
    if (sdl_opengl == 1) {
        /* Set the OpenGL state */
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glViewport(0, 0, w, h);
        glLoadIdentity();

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glGenerateMipmapEXT(GL_TEXTURE_2D); // GL_MIPMAP_LINEAR
    }
#endif

    /* rearrange multi-touch finger points */
    if (get_emul_multi_touch_state()->multitouch_enable == 1 ||
            get_emul_multi_touch_state()->multitouch_enable == 2) {
        rearrange_finger_points(get_emul_lcd_width(), get_emul_lcd_height(),
            current_scale_factor, rotaton_type);
    }
}

#if 0
static int point_degree = 0;
static void draw_outline_circle(int cx, int cy, int r, int num_segments)
{
    int ii;
    float theta = 2 * 3.1415926 / (num_segments);
    float c = cosf(theta);//precalculate the sine and cosine
    float s = sinf(theta);
    float t;

    float x = r;//we start at angle = 0
    float y = 0;

    glEnable(GL_LINE_STIPPLE);
    glLoadIdentity();
    glColor3f(0.9, 0.9, 0.9);
    glLineStipple(1, 0xcccc);
    glLineWidth(3.f);

    glTranslatef(cx, cy, 0);
    glRotated(point_degree++ % 360, 0, 0, 1);

    glBegin(GL_LINE_LOOP);
    for(ii = 0; ii < num_segments; ii++) {
        glVertex2f(x, y); //output vertex

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    glEnd();

    glDisable(GL_LINE_STIPPLE);
}

static void draw_fill_circle(int cx, int cy, int r)
{
    glEnable(GL_POINT_SMOOTH);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.1, 0.1, 0.1, 0.6);
    glPointSize(r - 2);

    glBegin(GL_POINTS);
        glVertex2f(cx, cy);
    glEnd();

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
}
#endif

static void qemu_update(void)
{
    if (sdl_alteration == 1) {
        sdl_alteration = 0;
        _sdl_init();
    } else if (sdl_alteration == -1) {
        SDL_FreeSurface(scaled_screen);
        SDL_FreeSurface(rotated_screen);
        SDL_FreeSurface(surface_qemu);
        surface_qemu = NULL;
        SDL_Quit();

        return;
    }

    if (surface_qemu != NULL) {
        int i;

#if 0
        if (sdl_opengl == 1)
        { //gl surface
            glEnable(GL_TEXTURE_2D);
            glLoadIdentity();
            glColor3f(1.0, 1.0, 1.0);
            glTexImage2D(GL_TEXTURE_2D,
                0, 3, surface_qemu->w, surface_qemu->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface_qemu->pixels);

            glClear(GL_COLOR_BUFFER_BIT);

            /* rotation */
            if (current_screen_degree == 270.0) { //reverse landscape
                glRotated(90, 0, 0, 1);
                glTranslatef(0, -current_screen_height, 0);
            } else if (current_screen_degree == 180.0) { //reverse portrait
                glRotated(180, 0, 0, 1);
                glTranslatef(-current_screen_width, -current_screen_height, 0);
            } else if (current_screen_degree == 90.0) { //landscape
                glRotated(270, 0, 0, 1);
                glTranslatef(-current_screen_width, 0, 0);
            }

            glBegin(GL_QUADS);
                glTexCoord2i(0, 0);
                glVertex3f(0, 0, 0);
                glTexCoord2i(1, 0);
                glVertex3f(current_screen_width, 0, 0);
                glTexCoord2i(1, 1);
                glVertex3f(current_screen_width, current_screen_height, 0);
                glTexCoord2i(0, 1);
                glVertex3f(0, current_screen_height, 0);
            glEnd();

            glDisable(GL_TEXTURE_2D);

            /* draw multi-touch finger points */
            MultiTouchState *mts = get_emul_multi_touch_state();
            if (mts->multitouch_enable != 0) {
                FingerPoint *finger = NULL;
                int finger_point_size_half = mts->finger_point_size / 2;

                for (i = 0; i < mts->finger_cnt; i++) {
                    finger = get_finger_point_from_slot(i);
                    if (finger != NULL && finger->id != 0) {
                        draw_outline_circle(finger->origin_x, finger->origin_y, finger_point_size_half, 10); //TODO: optimize
                        draw_fill_circle(finger->origin_x, finger->origin_y, mts->finger_point_size);
                    }
                }
            } //end  of draw multi-touch

            glFlush();

            SDL_GL_SwapBuffers();
        }
        else
        { //sdl surface
#endif
            maru_do_pixman_sdl(qemu_display_surface->image);
            if (current_scale_factor != 1.0) {
                rotated_screen = maru_do_pixman_rotate(
                    surface_qemu, rotated_screen,
                    (int)current_screen_degree);
                scaled_screen = maru_do_pixman_scale(
                    rotated_screen, scaled_screen,
                    (int)current_screen_degree);
                SDL_BlitSurface(scaled_screen, NULL, surface_screen, NULL);
            }
            else {/* current_scale_factor == 1.0 */
                if (current_screen_degree != 0.0) {
                    rotated_screen = maru_do_pixman_rotate(
                        surface_qemu, rotated_screen,
                        (int)current_screen_degree);
                    SDL_BlitSurface(rotated_screen, NULL, surface_screen, NULL);
                } else {
                    /* as-is */
                    SDL_BlitSurface(surface_qemu, NULL, surface_screen, NULL);
                }
            }

            /* draw multi-touch finger points */
            MultiTouchState *mts = get_emul_multi_touch_state();
            if (mts->multitouch_enable != 0 && mts->finger_point_surface != NULL) {
                FingerPoint *finger = NULL;
                int finger_point_size_half = mts->finger_point_size / 2;
                SDL_Rect rect;

                for (i = 0; i < mts->finger_cnt; i++) {
                    finger = get_finger_point_from_slot(i);
                    if (finger != NULL && finger->id != 0) {
                        rect.x = finger->origin_x - finger_point_size_half;
                        rect.y = finger->origin_y - finger_point_size_half;
                        rect.w = rect.h = mts->finger_point_size;

                        SDL_BlitSurface(
                                (SDL_Surface *)mts->finger_point_surface,
                                NULL, surface_screen, &rect);
                    }
                }
            } /* end of draw multi-touch */
        }

    SDL_UpdateRect(surface_screen, 0, 0, 0, 0);
}


#ifdef SDL_THREAD
static void *run_qemu_update(void* arg)
{
    while(1) {
        pthread_mutex_lock(&sdl_mutex);

        pthread_cond_wait(&sdl_cond, &sdl_mutex);

        qemu_update();

        pthread_mutex_unlock(&sdl_mutex);
    }

    return NULL;
}
#endif

void maruskin_sdl_init(uint64 swt_handle,
    int lcd_size_width, int lcd_size_height, bool is_resize)
{
    gchar SDL_windowhack[32];
    SDL_SysWMinfo info;
    long window_id = swt_handle;

    INFO("maru sdl initialization = %d\n", is_resize);

    if (is_resize == FALSE) { //once
        sprintf(SDL_windowhack, "%ld", window_id);
        g_setenv("SDL_WINDOWID", SDL_windowhack, 1);
        INFO("register SDL environment variable. (SDL_WINDOWID = %s)\n", SDL_windowhack);

        if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
            ERR("unable to init SDL: %s\n", SDL_GetError());
        }

        set_emul_lcd_size(lcd_size_width, lcd_size_height);
        set_emul_sdl_bpp(SDL_BPP);
    }

    if (sdl_initialized == 0) {
        sdl_initialized = 1;

        //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        init_multi_touch_state();

#ifndef _WIN32
        SDL_VERSION(&info.version);
        SDL_GetWMInfo(&info);
#endif
    }

    sdl_alteration = 1;

#ifdef SDL_THREAD
    if (sdl_thread_initialized == 0) {
        sdl_thread_initialized = 1;
        pthread_t thread_id;
        INFO("sdl update thread create\n");
        if (pthread_create(&thread_id, NULL, run_qemu_update, NULL) != 0) {
            ERR("pthread_create fail\n");
            return;
        }
    }
#endif
}

void maruskin_sdl_quit(void)
{
    INFO("maru sdl quit\n");

    /* remove multi-touch finger points */
    cleanup_multi_touch_state();

#if 0
    if (sdl_opengl == 1) {
        glDeleteTextures(1, &texture);
    }
#endif

    sdl_alteration = -1;

    SDL_Quit();

#ifdef SDL_THREAD
    pthread_cond_destroy(&sdl_cond);
#endif
    pthread_mutex_destroy(&sdl_mutex);
}


void maruskin_sdl_resize(void)
{
    SDL_Event ev;

    /* this fails if SDL is not initialized */
    memset(&ev, 0, sizeof(ev));
    ev.resize.type = SDL_VIDEORESIZE;

    /* This function is thread safe,
    and can be called from other threads safely. */
    SDL_PushEvent(&ev);
}

DisplaySurface *maruskin_sdl_get_display(void) {
    return qemu_display_surface;
}

