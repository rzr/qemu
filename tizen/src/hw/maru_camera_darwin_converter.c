/*
 * Implementation of color conversion for MARU Virtual Camera device on MacOS.
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * Jun Tian <jun.j.tian@intel.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "qemu-common.h"
#include "maru_camera_darwin.h"
#include "tizen/src/debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, camera_darwin);

static void UYVYToYUV420(unsigned char *bufsrc, unsigned char *bufdest,
                         int width, int height);
static void YVU420ToYUV420(unsigned char *bufsrc, unsigned char *bufdest,
                           int width, int height);
static void YUYVToYUV420(unsigned char *bufsrc, unsigned char *bufdest,
                         int width, int height);

/* Convert pixel format to YUV420 */
void convert_frame(uint32_t pixel_format, int frame_width, int frame_height,
                   size_t frame_size, void *frame_pixels, void *video_buf)
{
    switch (pixel_format) {
    case V4L2_PIX_FMT_YUV420:
        memcpy(video_buf, (void *)frame_pixels, (size_t)frame_size);
        break;
    case V4L2_PIX_FMT_YVU420:
        YVU420ToYUV420(frame_pixels, video_buf, frame_width, frame_height);
        break;
    case V4L2_PIX_FMT_YUYV:
        YUYVToYUV420(frame_pixels, video_buf, frame_width, frame_height);
        break;
    case V4L2_PIX_FMT_UYVY: /* Mac default format */
        UYVYToYUV420(frame_pixels, video_buf, frame_width, frame_height);
        break;
    default:
        ERR("Cannot convert the pixel format (%.4s)...\n",
               (const char *)&pixel_format);
        break;
    }
}

static void UYVYToYUV420(unsigned char *bufsrc, unsigned char *bufdest,
                         int width, int height)
{
    int i, j;

    /* Source */
    unsigned char *ptrsrcy1, *ptrsrcy2;
    unsigned char *ptrsrcy3, *ptrsrcy4;
    unsigned char *ptrsrccb1;
    unsigned char *ptrsrccb3;
    unsigned char *ptrsrccr1;
    unsigned char *ptrsrccr3;
    int srcystride, srcccstride;

    ptrsrcy1  = bufsrc + 1;
    ptrsrcy2  = bufsrc + (width << 1) + 1;
    ptrsrcy3  = bufsrc + (width << 1) * 2 + 1;
    ptrsrcy4  = bufsrc + (width << 1) * 3 + 1;

    ptrsrccb1 = bufsrc;
    ptrsrccb3 = bufsrc + (width << 1) * 2;

    ptrsrccr1 = bufsrc + 2;
    ptrsrccr3 = bufsrc + (width << 1) * 2 + 2;

    srcystride  = (width << 1) * 3;
    srcccstride = (width << 1) * 3;

    /* Destination */
    unsigned char *ptrdesty1, *ptrdesty2;
    unsigned char *ptrdesty3, *ptrdesty4;
    unsigned char *ptrdestcb1, *ptrdestcb2;
    unsigned char *ptrdestcr1, *ptrdestcr2;
    int destystride, destccstride;

    ptrdesty1 = bufdest;
    ptrdesty2 = bufdest + width;
    ptrdesty3 = bufdest + width * 2;
    ptrdesty4 = bufdest + width * 3;

    ptrdestcb1 = bufdest + width * height;
    ptrdestcb2 = bufdest + width * height + (width >> 1);

    ptrdestcr1 = bufdest + width * height + ((width*height) >> 2);
    ptrdestcr2 = bufdest + width * height + ((width*height) >> 2)
                 + (width >> 1);

    destystride  = (width)*3;
    destccstride = (width>>1);

    for (j = 0; j < (height / 4); j++) {
        for (i = 0; i < (width / 2); i++) {
            (*ptrdesty1++) = (*ptrsrcy1);
            (*ptrdesty2++) = (*ptrsrcy2);
            (*ptrdesty3++) = (*ptrsrcy3);
            (*ptrdesty4++) = (*ptrsrcy4);

            ptrsrcy1 += 2;
            ptrsrcy2 += 2;
            ptrsrcy3 += 2;
            ptrsrcy4 += 2;

            (*ptrdesty1++) = (*ptrsrcy1);
            (*ptrdesty2++) = (*ptrsrcy2);
            (*ptrdesty3++) = (*ptrsrcy3);
            (*ptrdesty4++) = (*ptrsrcy4);

            ptrsrcy1 += 2;
            ptrsrcy2 += 2;
            ptrsrcy3 += 2;
            ptrsrcy4 += 2;

            (*ptrdestcb1++) = (*ptrsrccb1);
            (*ptrdestcb2++) = (*ptrsrccb3);

            ptrsrccb1 += 4;
            ptrsrccb3 += 4;

            (*ptrdestcr1++) = (*ptrsrccr1);
            (*ptrdestcr2++) = (*ptrsrccr3);

            ptrsrccr1 += 4;
            ptrsrccr3 += 4;

        }

        /* Update src pointers */
        ptrsrcy1  += srcystride;
        ptrsrcy2  += srcystride;
        ptrsrcy3  += srcystride;
        ptrsrcy4  += srcystride;

        ptrsrccb1 += srcccstride;
        ptrsrccb3 += srcccstride;

        ptrsrccr1 += srcccstride;
        ptrsrccr3 += srcccstride;

        /* Update dest pointers */
        ptrdesty1 += destystride;
        ptrdesty2 += destystride;
        ptrdesty3 += destystride;
        ptrdesty4 += destystride;

        ptrdestcb1 += destccstride;
        ptrdestcb2 += destccstride;

        ptrdestcr1 += destccstride;
        ptrdestcr2 += destccstride;
    }
}

static void YVU420ToYUV420(unsigned char *bufsrc, unsigned char *bufdest,
                           int width, int height)
{
    int i, j;

    /* Source*/
    unsigned char *ptrsrcy1, *ptrsrcy2;
    unsigned char *ptrsrcy3, *ptrsrcy4;
    unsigned char *ptrsrccb1, *ptrsrccb2;
    unsigned char *ptrsrccr1, *ptrsrccr2;
    int srcystride, srcccstride;

    ptrsrcy1 = bufsrc;
    ptrsrcy2 = bufsrc + width;
    ptrsrcy3 = bufsrc + width*2;
    ptrsrcy4 = bufsrc + width*3;

    ptrsrccr1 = bufsrc + width*height;
    ptrsrccr2 = bufsrc + width*height + (width>>1);

    ptrsrccb1 = bufsrc + width*height + ((width*height) >> 2);
    ptrsrccb2 = bufsrc + width*height + ((width*height) >> 2) + (width>>1);

    srcystride  = (width)*3;
    srcccstride = (width>>1);

    /* Destination */
    unsigned char *ptrdesty1, *ptrdesty2;
    unsigned char *ptrdesty3, *ptrdesty4;
    unsigned char *ptrdestcb1, *ptrdestcb2;
    unsigned char *ptrdestcr1, *ptrdestcr2;
    int destystride, destccstride;

    ptrdesty1 = bufdest;
    ptrdesty2 = bufdest + width;
    ptrdesty3 = bufdest + width * 2;
    ptrdesty4 = bufdest + width * 3;

    ptrdestcb1 = bufdest + width * height;
    ptrdestcb2 = bufdest + width * height + (width >> 1);

    ptrdestcr1 = bufdest + width * height + ((width*height) >> 2);
    ptrdestcr2 = bufdest + width * height + ((width*height) >> 2)
                 + (width >> 1);

    destystride  = (width)*3;
    destccstride = (width>>1);

    for (j = 0; j < (height / 4); j++) {
        for (i = 0; i < (width / 2); i++) {

            (*ptrdesty1++) = (*ptrsrcy1++);
            (*ptrdesty2++) = (*ptrsrcy2++);
            (*ptrdesty3++) = (*ptrsrcy3++);
            (*ptrdesty4++) = (*ptrsrcy4++);
            (*ptrdesty1++) = (*ptrsrcy1++);
            (*ptrdesty2++) = (*ptrsrcy2++);
            (*ptrdesty3++) = (*ptrsrcy3++);
            (*ptrdesty4++) = (*ptrsrcy4++);

            (*ptrdestcb1++) = (*ptrsrccb1++);
            (*ptrdestcr1++) = (*ptrsrccr1++);
            (*ptrdestcb2++) = (*ptrsrccb2++);
            (*ptrdestcr2++) = (*ptrsrccr2++);

        }

        /* Update src pointers */
        ptrsrcy1  += srcystride;
        ptrsrcy2  += srcystride;
        ptrsrcy3  += srcystride;
        ptrsrcy4  += srcystride;

        ptrsrccb1 += srcccstride;
        ptrsrccb2 += srcccstride;

        ptrsrccr1 += srcccstride;
        ptrsrccr2 += srcccstride;

        /* Update dest pointers */
        ptrdesty1 += destystride;
        ptrdesty2 += destystride;
        ptrdesty3 += destystride;
        ptrdesty4 += destystride;

        ptrdestcb1 += destccstride;
        ptrdestcb2 += destccstride;

        ptrdestcr1 += destccstride;
        ptrdestcr2 += destccstride;

    }

}

static void YUYVToYUV420(unsigned char *bufsrc, unsigned char *bufdest,
                         int width, int height)
{
    int i, j;

    /* Source*/
    unsigned char *ptrsrcy1, *ptrsrcy2;
    unsigned char *ptrsrcy3, *ptrsrcy4;
    unsigned char *ptrsrccb1;
    unsigned char *ptrsrccb3;
    unsigned char *ptrsrccr1;
    unsigned char *ptrsrccr3;
    int srcystride, srcccstride;

    ptrsrcy1  = bufsrc ;
    ptrsrcy2  = bufsrc + (width << 1);
    ptrsrcy3  = bufsrc + (width << 1) * 2;
    ptrsrcy4  = bufsrc + (width << 1) * 3;

    ptrsrccb1 = bufsrc + 1;
    ptrsrccb3 = bufsrc + (width << 1) * 2 + 1;

    ptrsrccr1 = bufsrc + 3;
    ptrsrccr3 = bufsrc + (width << 1) * 2 + 3;

    srcystride  = (width << 1) * 3;
    srcccstride = (width << 1) * 3;

    /* Destination */
    unsigned char *ptrdesty1, *ptrdesty2;
    unsigned char *ptrdesty3, *ptrdesty4;
    unsigned char *ptrdestcb1, *ptrdestcb2;
    unsigned char *ptrdestcr1, *ptrdestcr2;
    int destystride, destccstride;

    ptrdesty1 = bufdest;
    ptrdesty2 = bufdest + width;
    ptrdesty3 = bufdest + width * 2;
    ptrdesty4 = bufdest + width * 3;

    ptrdestcb1 = bufdest + width * height;
    ptrdestcb2 = bufdest + width * height + (width >> 1);

    ptrdestcr1 = bufdest + width * height + ((width * height) >> 2);
    ptrdestcr2 = bufdest + width * height + ((width * height) >> 2)
                 + (width >> 1);

    destystride  = width * 3;
    destccstride = (width >> 1);

    for (j = 0; j < (height / 4); j++) {
        for (i = 0; i < (width / 2); i++) {
            (*ptrdesty1++) = (*ptrsrcy1);
            (*ptrdesty2++) = (*ptrsrcy2);
            (*ptrdesty3++) = (*ptrsrcy3);
            (*ptrdesty4++) = (*ptrsrcy4);

            ptrsrcy1 += 2;
            ptrsrcy2 += 2;
            ptrsrcy3 += 2;
            ptrsrcy4 += 2;

            (*ptrdesty1++) = (*ptrsrcy1);
            (*ptrdesty2++) = (*ptrsrcy2);
            (*ptrdesty3++) = (*ptrsrcy3);
            (*ptrdesty4++) = (*ptrsrcy4);

            ptrsrcy1 += 2;
            ptrsrcy2 += 2;
            ptrsrcy3 += 2;
            ptrsrcy4 += 2;

            (*ptrdestcb1++) = (*ptrsrccb1);
            (*ptrdestcb2++) = (*ptrsrccb3);

            ptrsrccb1 += 4;
            ptrsrccb3 += 4;

            (*ptrdestcr1++) = (*ptrsrccr1);
            (*ptrdestcr2++) = (*ptrsrccr3);

            ptrsrccr1 += 4;
            ptrsrccr3 += 4;

        }

        /* Update src pointers */
        ptrsrcy1  += srcystride;
        ptrsrcy2  += srcystride;
        ptrsrcy3  += srcystride;
        ptrsrcy4  += srcystride;

        ptrsrccb1 += srcccstride;
        ptrsrccb3 += srcccstride;

        ptrsrccr1 += srcccstride;
        ptrsrccr3 += srcccstride;

        /* Update dest pointers */
        ptrdesty1 += destystride;
        ptrdesty2 += destystride;
        ptrdesty3 += destystride;
        ptrdesty4 += destystride;

        ptrdestcb1 += destccstride;
        ptrdestcb2 += destccstride;

        ptrdestcr1 += destccstride;
        ptrdestcr2 += destccstride;
    }
}
