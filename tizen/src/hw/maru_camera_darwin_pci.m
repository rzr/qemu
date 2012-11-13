/*
 * Implementation of MARU Virtual Camera device by PCI bus on Linux.
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * Jun Tian <jun.j.tian@intel.com>
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

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>
#import <CoreAudio/CoreAudio.h>

#include <pthread.h>
#include "qemu-common.h"
#include "maru_camera_common.h"
#include "maru_camera_darwin.h"
#include "pci.h"
#include "tizen/src/debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, camera_darwin);

/* V4L2 defines copy from videodev2.h */
#define V4L2_CTRL_FLAG_SLIDER       0x0020

#define V4L2_CTRL_CLASS_USER        0x00980000
#define V4L2_CID_BASE               (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_BRIGHTNESS         (V4L2_CID_BASE+0)
#define V4L2_CID_CONTRAST           (V4L2_CID_BASE+1)
#define V4L2_CID_SATURATION         (V4L2_CID_BASE+2)
#define V4L2_CID_SHARPNESS          (V4L2_CID_BASE+27)

typedef struct tagMaruCamConvertPixfmt {
    uint32_t fmt;   /* fourcc */
    uint32_t bpp;   /* bits per pixel, 0 for compressed formats */
    uint32_t needs_conversion;
} MaruCamConvertPixfmt;


static MaruCamConvertPixfmt supported_dst_pixfmts[] = {
    { V4L2_PIX_FMT_YUYV, 16, 0 },
    { V4L2_PIX_FMT_UYVY, 16, 0 },
    { V4L2_PIX_FMT_YUV420, 12, 0 },
    { V4L2_PIX_FMT_YVU420, 12, 0 },
};

typedef struct tagMaruCamConvertFrameInfo {
    uint32_t width;
    uint32_t height;
} MaruCamConvertFrameInfo;

static MaruCamConvertFrameInfo supported_dst_frames[] = {
    { 640, 480 },
    { 352, 288 },
    { 320, 240 },
    { 176, 144 },
    { 160, 120 },
};

#define MARUCAM_CTRL_VALUE_MAX      20
#define MARUCAM_CTRL_VALUE_MIN      1
#define MARUCAM_CTRL_VALUE_MID      10
#define MARUCAM_CTRL_VALUE_STEP     1

#if 0
struct marucam_qctrl {
    uint32_t id;
    uint32_t hit;
    long min;
    long max;
    long step;
    long init_val;
};

static struct marucam_qctrl qctrl_tbl[] = {
    { V4L2_CID_BRIGHTNESS, 0, },
    { V4L2_CID_CONTRAST, 0, },
    { V4L2_CID_SATURATION,0, },
    { V4L2_CID_SHARPNESS, 0, },
};
#endif

static MaruCamState *g_state = NULL;

static uint32_t ready_count = 0;
static uint32_t cur_fmt_idx = 0;
static uint32_t cur_frame_idx = 0;

/***********************************
 * Mac camera helper functions
 ***********************************/

/* Returns current time in microseconds. */
static uint64_t camera_get_timestamp(void)
{
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return (uint64_t)t.tv_sec * 1000000LL + t.tv_usec;
}

/* Sleeps for the given amount of milliseconds */
static void camera_sleep(int millisec)
{
    struct timeval t;
    const uint64_t wake_at = camera_get_timestamp() + (uint64_t)millisec * 1000;
    do {
        const uint64_t stamp = camera_get_timestamp();
        if ((stamp / 1000) >= (wake_at / 1000)) {
            break;
        }
        t.tv_sec = (wake_at - stamp) / 1000000;
        t.tv_usec = (wake_at - stamp) - (uint64_t)t.tv_sec * 1000000;
    } while (select(0, NULL, NULL, NULL, &t) < 0 && errno == EINTR);
}

// Convert Core Video format to FOURCC
static uint32_t corevideo_to_fourcc(uint32_t cv_pix_fmt)
{
    //INFO("%s, cv_pix_fmt:%d, %s\n", __FUNCTION__, cv_pix_fmt, (const char*)&cv_pix_fmt);
    switch (cv_pix_fmt) {
        case kCVPixelFormatType_420YpCbCr8Planar:
            return V4L2_PIX_FMT_YVU420;
        case kCVPixelFormatType_422YpCbCr8:
            return V4L2_PIX_FMT_UYVY;
        case kCVPixelFormatType_422YpCbCr8_yuvs:
            return V4L2_PIX_FMT_YUYV;
        case kCVPixelFormatType_32ARGB:
        case kCVPixelFormatType_32RGBA:
            return V4L2_PIX_FMT_RGB32;
        case kCVPixelFormatType_32BGRA:
        case kCVPixelFormatType_32ABGR:
            return V4L2_PIX_FMT_BGR32;
        case kCVPixelFormatType_24RGB:
            return V4L2_PIX_FMT_RGB24;
        case kCVPixelFormatType_24BGR:
            return V4L2_PIX_FMT_BGR32;
        default:
            ERR("Unknown pixel format '%.4s'", (const char*)&cv_pix_fmt);
            return 0;
    }
}

static uint32_t get_bytesperline(uint32_t pixfmt, uint32_t width)
{
    uint32_t bytesperline;

    switch (pixfmt) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
            bytesperline = (width * 12) >> 3;
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        default:
            bytesperline = width * 2;
            break;
    }

    return bytesperline;
}

static uint32_t get_sizeimage(uint32_t pixfmt, uint32_t width, uint32_t height)
{
    return (get_bytesperline(pixfmt, width) * height);
}

///////////////////////////////////////////////////////////////////
//   Maru Camera Implementation
///////////////////////////////////////////////////////////////////

@interface MaruCameraDriver : NSObject {
    QTCaptureSession               *mCaptureSession;
    //QTCaptureMovieFileOutput       *mCaptureMovieFileOutput;
    QTCaptureVideoPreviewOutput    *mCaptureVideoPreviewOutput;
    QTCaptureDeviceInput           *mCaptureVideoDeviceInput;
    QTCaptureDeviceInput           *mCaptureAudioDeviceInput;

    CVImageBufferRef      mCurrentImageBuffer;
    CVImageBufferRef imageBuffer;
    BOOL mDeviceIsOpened;
    BOOL mCaptureIsStarted;
}

- (MaruCameraDriver*)init;
- (int)startCapture:(int)width:(int)height;
- (int)readFrame:(void*)video_buf;
- (int)setCaptureFormat:(int)width:(int)height:(int)pix_format;
- (int)getCaptureFormat:(int)width:(int)height:(int)pix_format;
- (BOOL)deviceStatus;

@end

@implementation MaruCameraDriver

- (MaruCameraDriver*)init
{
	BOOL success = NO;
	NSError *error;
    mDeviceIsOpened = NO;
    mCaptureIsStarted = NO;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    // Create the capture session
    mCaptureSession = [[QTCaptureSession alloc] init];

    // Find a video device
    QTCaptureDevice *videoDevice = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo];
    success = [videoDevice open:&error];

    // If a video input device can't be found or opened, try to find and open a muxed input device
	if (!success) {
        videoDevice = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeMuxed];
		success = [videoDevice open:&error];
        [pool release];
        return nil;
    }

    if (!success) {
        videoDevice = nil;
        [pool release];
        return nil;
    }

    if (videoDevice) {
        // Add the video device to the session as a device input
		mCaptureVideoDeviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:videoDevice];
        success = [mCaptureSession addInput:mCaptureVideoDeviceInput error:&error];

        if (!success) {
            [pool release];
            return nil;
		}

        mCaptureVideoPreviewOutput = [[QTCaptureVideoPreviewOutput alloc] init];
        success = [mCaptureSession addOutput:mCaptureVideoPreviewOutput error:&error];
        if (!success) {
            [pool release];
            return nil;
        }

        mDeviceIsOpened = YES;
        [mCaptureVideoPreviewOutput setDelegate:self];
        INFO("Camera session bundling successfully!\n");
        [pool release];
        return self;
    } else {
        [pool release];
        return nil;
    }
}

- (int)startCapture:(int)width:(int)height
{
    int ret = -1;

    if (![mCaptureSession isRunning]) {
        /* Set width & height, using default pixel format to capture */
        NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                                      [NSNumber numberWithInt: width], (id)kCVPixelBufferWidthKey,
                                                      [NSNumber numberWithInt: height], (id)kCVPixelBufferHeightKey,
                                                 nil];
        [mCaptureVideoPreviewOutput setPixelBufferAttributes:attributes];
        [mCaptureSession startRunning];
    } else {
        ERR("Capture session is already running, exit\n");
        return ret;
    }

    if ([mCaptureSession isRunning]) {
        while(!mCaptureIsStarted) {
            // Wait Until Capture is started
            [[NSRunLoop currentRunLoop] runUntilDate: [NSDate dateWithTimeIntervalSinceNow: 1]];
        }
        INFO("Capture session started!\n");
        ret = 0;
    }
    return ret;
}

- (int)readFrame:(void*)video_buf
{
    int ret = -1;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    @synchronized (self) {
        if (mCurrentImageBuffer != nil) {
            const uint32_t pixel_format = corevideo_to_fourcc(CVPixelBufferGetPixelFormatType(mCurrentImageBuffer));
            const int frame_width = CVPixelBufferGetWidth(mCurrentImageBuffer);
            const int frame_height = CVPixelBufferGetHeight(mCurrentImageBuffer);
            const size_t frame_size =CVPixelBufferGetBytesPerRow(mCurrentImageBuffer) * frame_height;
            INFO("buffer(%p), pixel_format(%d,%.4s), frame_width(%d), frame_height(%d0, frame_size(%d)\n",
                 mCurrentImageBuffer, (int)pixel_format, (const char*)&pixel_format, frame_width, frame_height, (int)frame_size);

            //convert frame to v4l2 format
            CVPixelBufferLockBaseAddress(mCurrentImageBuffer, 0);
            const void* frame_pixels = CVPixelBufferGetBaseAddress(mCurrentImageBuffer);
            if (frame_pixels != nil && video_buf != nil) {
                convert_frame(pixel_format, frame_width, frame_height,
                              frame_size, (void*)frame_pixels, video_buf);
            } else {
                INFO("Unable to obtain framebuffer");
                return -1;
            }
            CVPixelBufferUnlockBaseAddress(mCurrentImageBuffer, 0);
            return 0;
        } else {
            //INFO("%s, First frame not captured ?\n", __FUNCTION__);
            return 1;
        }
    }

    [pool release];
    return ret;
}

- (int)setCaptureFormat:(int)width:(int)height:(int)pix_format
{
    int ret = -1;
    NSDictionary *attributes;

    if (mCaptureSession == nil || mCaptureVideoPreviewOutput == nil) {
        ERR("Capture session is not initiated.\n");
        return ret;
    }

    /* Set the pixel buffer attributes before running the capture session */
    if (![mCaptureSession isRunning]) {
        if (pix_format) {
            attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                            [NSNumber numberWithInt: width], (id)kCVPixelBufferWidthKey,
                                            [NSNumber numberWithInt: height], (id)kCVPixelBufferHeightKey,
                                            [NSNumber numberWithInt: pix_format], (id)kCVPixelBufferPixelFormatTypeKey,
                                       nil];
        } else {
            attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                            [NSNumber numberWithInt: width], (id)kCVPixelBufferWidthKey,
                                            [NSNumber numberWithInt: height], (id)kCVPixelBufferHeightKey,
                                       nil];
        }
        [mCaptureVideoPreviewOutput setPixelBufferAttributes:attributes];
        ret = 0;
    } else {
        ERR("Cannot set pixel buffer attributes when it's running.\n");
        return ret;
    }

    return ret;
}

- (int)getCaptureFormat:(int)width:(int)height:(int)pix_format
{
    return 0;
}

/* Get the device bundling status */
- (BOOL)deviceStatus
{
    return mDeviceIsOpened;
}

/* Handle deallocation of memory for your capture objects */

- (void)dealloc
{
	[mCaptureSession release];
	[mCaptureVideoDeviceInput release];
    [mCaptureAudioDeviceInput release];
    [mCaptureVideoPreviewOutput release];
    [super dealloc];
}

/* Receive this method whenever the output decompresses and outputs a new video frame */
- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame
     withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
    CVImageBufferRef imageBufferToRelease;
    CVBufferRetain(videoFrame);

    @synchronized (self)
    {
        imageBufferToRelease = mCurrentImageBuffer;
        mCurrentImageBuffer = videoFrame;
        mCaptureIsStarted = YES;
    }
    CVBufferRelease(imageBufferToRelease);
}

@end

///////////////////////////////////////////////////////////////////
//   Maru Camera APIs
///////////////////////////////////////////////////////////////////

typedef struct MaruCameraDevice MaruCameraDevice;
struct MaruCameraDevice {
    /* Maru camera device object. */
    MaruCameraDriver *driver;
};

/* Golbal representation of the Maru camera */
MaruCameraDevice *mcd = NULL;

static int marucam_device_read_frame()
{
    int ret;
    void* tmp_buf;

    if (g_state->streamon == 0) {
        INFO("%s, %d, Streaming is off, exiting ...\n", __FUNCTION__, __LINE__);
        return -1;
    }

    qemu_mutex_lock(&g_state->thread_mutex);
    if (g_state->req_frame == 0) {
        qemu_mutex_unlock(&g_state->thread_mutex);
        camera_sleep(30);
        //INFO("%s, %d, req_frame = 0, next loop\n", __FUNCTION__, __LINE__);
        return 0;
    }
    tmp_buf = g_state->vaddr + g_state->buf_size * (g_state->req_frame - 1);
    qemu_mutex_unlock(&g_state->thread_mutex);

    ret =  [mcd->driver readFrame: tmp_buf];
    //INFO("%s, %d, ret:%d\n", __FUNCTION__, __LINE__, ret);
    if (ret == -1) {
        //INFO("%s, Capture error\n", __FUNCTION__);
        return -1;
    }
    if(ret == 0) {
        //INFO("%s, Capture a frame\n", __FUNCTION__);
    }
    if(ret == 1) {
        //INFO("%s, Capture first frame missed\n", __FUNCTION__);
    }

    qemu_mutex_lock(&g_state->thread_mutex);
    if (g_state->streamon) {
        g_state->req_frame = 0; // clear request
        g_state->isr |= 0x01;   // set a flag of rasing a interrupt
        qemu_bh_schedule(g_state->tx_bh);
        ret = 1;
    } else {
        ret = -1;
    }
    qemu_mutex_unlock(&g_state->thread_mutex);
    return ret;
}

/* Worker thread to grab frames to the preview window */
static void *marucam_worker_thread(void *thread_param)
{
  //MaruCamState *state = (MaruCamState*)thread_param;

  wait_worker_thread:
    qemu_mutex_lock(&g_state->thread_mutex);
    // Wait on the condition
    qemu_cond_wait(&g_state->thread_cond, &g_state->thread_mutex);
    qemu_mutex_unlock(&g_state->thread_mutex);

    /* Loop: capture frame -> convert format -> render to screen */
    while (1)
    {
        if (g_state->streamon) {
            if (marucam_device_read_frame() < 0) {
                INFO("Streaming is off ...\n");
                goto wait_worker_thread;
            }
        } else {
            INFO("Streaming is off ...\n");
            goto wait_worker_thread;
        }
        //camera_sleep(50);
    }
    return NULL;
}

int marucam_device_check(int log_flag)
{
    // FIXME: check the device parameters
    INFO("Checking camera device\n");
    return 1;
}

/**********************************************
 * MARU camera routines
 **********************************************/

void marucam_device_init(MaruCamState* state)
{
    g_state = state;
    qemu_thread_create(&state->thread_id, marucam_worker_thread, (void*)g_state, QEMU_THREAD_JOINABLE);
}

/* MARUCAM_CMD_OPEN */
void marucam_device_open(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    param->top = 0;

    mcd = (MaruCameraDevice*)malloc(sizeof(MaruCameraDevice));
    if (mcd != NULL) {
        memset(mcd, 0, sizeof(MaruCameraDevice));
    } else {
        ERR("%s: MaruCameraDevice allocate failed\n", __FUNCTION__);
    }
    //qemu_mutex_lock(&state->thread_mutex);
    mcd->driver = [[MaruCameraDriver alloc] init];
    //qemu_mutex_unlock(&state->thread_mutex);
    if (mcd->driver == nil) {
        INFO("Camera device open failed\n");
        return;
    }
    INFO("Camera opened!\n");
}

/* MARUCAM_CMD_CLOSE */
void marucam_device_close(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    param->top = 0;

    if (mcd != NULL) {
        [mcd->driver dealloc];
        free(mcd);
    }

    //marucam_reset_controls();
    INFO("Camera closed\n");
}

/* MARUCAM_CMD_START_PREVIEW */
void marucam_device_start_preview(MaruCamState* state)
{
    uint32_t width, height, pixfmt;
    MaruCamParam *param = state->param;
    param->top = 0;

    ready_count = 0;
    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;
    pixfmt = supported_dst_pixfmts[cur_fmt_idx].fmt;
    state->buf_size = get_sizeimage(pixfmt, width, height);

    INFO("Pixfmt(%c%c%c%c), W:H(%d:%d), buf size(%u), frame idx(%d), fmt idx(%d)\n",
         (char)(pixfmt), (char)(pixfmt >> 8),
         (char)(pixfmt >> 16), (char)(pixfmt >> 24),
         width, height, state->buf_size,
         cur_frame_idx, cur_fmt_idx);

    if (mcd->driver != nil) {
        [mcd->driver startCapture: width: height];
    } else {
        ERR("%s: Start capture failed: vaild device", __FUNCTION__);
        return;
    }

    // Enable the condition to capture frames now
    qemu_mutex_lock(&state->thread_mutex);
    state->streamon = 1;
    qemu_cond_signal(&state->thread_cond);
    qemu_mutex_unlock(&state->thread_mutex);

    INFO("Starting preview ...\n");
}

/* MARUCAM_CMD_STOP_PREVIEW */
void marucam_device_stop_preview(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    param->top = 0;

    INFO("Stopping preview ...\n");
    qemu_mutex_lock(&state->thread_mutex);
    state->streamon = 0;
    qemu_mutex_unlock(&state->thread_mutex);
}

/* MARUCAM_CMD_S_PARAM */
void marucam_device_s_param(MaruCamState* state)
{
    MaruCamParam *param = state->param;

    /* We use default FPS of the webcam */
    param->top = 0;
}

/* MARUCAM_CMD_G_PARAM */
void marucam_device_g_param(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    /* We use default FPS of the webcam
     * return a fixed value on guest ini file (1/30).
     */
    param->top = 0;
    param->stack[0] = 0x1000; /* V4L2_CAP_TIMEPERFRAME */
    param->stack[1] = 1; /* numerator */
    param->stack[2] = 30; /* denominator */
}

/* MARUCAM_CMD_S_FMT */
void marucam_device_s_fmt(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    uint32_t width, height, pixfmt, pidx, fidx;

    param->top = 0;
    width = param->stack[0];
    height = param->stack[1];
    pixfmt = param->stack[2];

    INFO("Set format: width(%d), height(%d), pixfmt(%d, %.4s)\n",
         width, height, pixfmt, (const char*)&pixfmt);

    for (fidx = 0; fidx < ARRAY_SIZE(supported_dst_frames); fidx++) {
        if ((supported_dst_frames[fidx].width == width) &&
            (supported_dst_frames[fidx].height == height)) {
            break;
        }
    }
    if (fidx == ARRAY_SIZE(supported_dst_frames)) {
        param->errCode = EINVAL;
        return;
    }

    for (pidx = 0; pidx < ARRAY_SIZE(supported_dst_pixfmts); pidx++) {
        if (supported_dst_pixfmts[pidx].fmt == pixfmt) {
            INFO("pixfmt index is match: %d\n", pidx);
            break;
        }
    }
    if (pidx == ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }

    if ((supported_dst_frames[cur_frame_idx].width != width) &&
        (supported_dst_frames[cur_frame_idx].height != height)) {
        if (mcd->driver == nil || [mcd->driver setCaptureFormat: width: height: 0] < 0) {
            ERR("Set pixel format failed\n");
            return;
        }

        INFO("cur_frame_idx:%d, supported_dst_frames[cur_frame_idx].width:%d\n",
             cur_frame_idx, supported_dst_frames[cur_frame_idx].width);
    }

    cur_frame_idx = fidx;
    cur_fmt_idx = pidx;

    pixfmt = supported_dst_pixfmts[cur_fmt_idx].fmt;
    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;

    param->stack[0] = width;
    param->stack[1] = height;
    param->stack[2] = 1; /* V4L2_FIELD_NONE */
    param->stack[3] = pixfmt;
    param->stack[4] = get_bytesperline(pixfmt, width);
    param->stack[5] = get_sizeimage(pixfmt, width, height);
    param->stack[6] = 0;
    param->stack[7] = 0;

    INFO("Set device pixel format ...\n");
}

/* MARUCAM_CMD_G_FMT */
void marucam_device_g_fmt(MaruCamState* state)
{
    uint32_t width, height, pixfmt;
    MaruCamParam *param = state->param;

    param->top = 0;
    pixfmt = supported_dst_pixfmts[cur_fmt_idx].fmt;
    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;

    param->stack[0] = width;
    param->stack[1] = height;
    param->stack[2] = 1; /* V4L2_FIELD_NONE */
    param->stack[3] = pixfmt;
    param->stack[4] = get_bytesperline(pixfmt, width);
    param->stack[5] = get_sizeimage(pixfmt, width, height);
    param->stack[6] = 0;
    param->stack[7] = 0;

    INFO("Get device frame format ...\n");
}

void marucam_device_try_fmt(MaruCamState* state)
{
    INFO("Try device frame format, use default setting ...\n");
}

/* Get specific pixelformat description */
void marucam_device_enum_fmt(MaruCamState* state)
{
    uint32_t index;
    MaruCamParam *param = state->param;

    param->top = 0;
    index = param->stack[0];

    if (index >= ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }
    param->stack[1] = 0; /* flags = NONE */
    param->stack[2] = supported_dst_pixfmts[index].fmt; /* pixelformat */
    switch (supported_dst_pixfmts[index].fmt) {
        case V4L2_PIX_FMT_YUYV:
            memcpy(&param->stack[3], "YUYV", 32);
            break;
        case V4L2_PIX_FMT_UYVY:
            memcpy(&param->stack[3], "UYVY", 32);
            break;
        case V4L2_PIX_FMT_YUV420:
            memcpy(&param->stack[3], "YU12", 32);
            break;
        case V4L2_PIX_FMT_YVU420:
            memcpy(&param->stack[3], "YV12", 32);
            break;
    }
}

/*
 * QTKit don't support setting brightness, contrast, saturation & sharpness
 */
void marucam_device_qctrl(MaruCamState* state)
{
    uint32_t id, i;
    //long property, min, max, step, def_val, set_val;
    char name[32] = {0,};
    MaruCamParam *param = state->param;

    param->top = 0;
    id = param->stack[0];

    switch (id) {
    case V4L2_CID_BRIGHTNESS:
        INFO("V4L2_CID_BRIGHTNESS\n");
        memcpy((void*)name, (void*)"brightness", 32);
        i = 0;
        break;
    case V4L2_CID_CONTRAST:
        INFO("V4L2_CID_CONTRAST\n");
        memcpy((void*)name, (void*)"contrast", 32);
        i = 1;
        break;
    case V4L2_CID_SATURATION:
        INFO("V4L2_CID_SATURATION\n");
        memcpy((void*)name, (void*)"saturation", 32);
        i = 2;
        break;
    case V4L2_CID_SHARPNESS:
        INFO("V4L2_CID_SHARPNESS\n");
        memcpy((void*)name, (void*)"sharpness", 32);
        i = 3;
        break;
    default:
        param->errCode = EINVAL;
        return;
    }

    param->stack[0] = id;
    param->stack[1] = MARUCAM_CTRL_VALUE_MIN;  /* minimum */
    param->stack[2] = MARUCAM_CTRL_VALUE_MAX;  /* maximum */
    param->stack[3] = MARUCAM_CTRL_VALUE_STEP; /*  step   */
    param->stack[4] = MARUCAM_CTRL_VALUE_MID;  /* default_value */
    param->stack[5] = V4L2_CTRL_FLAG_SLIDER;
    /* name field setting */
    memcpy(&param->stack[6], (void*)name, sizeof(name)/sizeof(name[0]));
}

void marucam_device_s_ctrl(MaruCamState* state)
{
    INFO("Set control\n");
}

void marucam_device_g_ctrl(MaruCamState* state)
{
    INFO("Get control\n");
}

/* Get frame width & height */
void marucam_device_enum_fsizes(MaruCamState* state)
{
    uint32_t index, pixfmt, i;
    MaruCamParam *param = state->param;

    param->top = 0;
    index = param->stack[0];
    pixfmt = param->stack[1];

    if (index >= ARRAY_SIZE(supported_dst_frames)) {
        param->errCode = EINVAL;
        return;
    }
    for (i = 0; i < ARRAY_SIZE(supported_dst_pixfmts); i++) {
        if (supported_dst_pixfmts[i].fmt == pixfmt)
            break;
    }

    if (i == ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }

    param->stack[0] = supported_dst_frames[index].width;
    param->stack[1] = supported_dst_frames[index].height;
}

void marucam_device_enum_fintv(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    param->top = 0;

    /* switch by index(param->stack[0]) */
    switch (param->stack[0]) {
        case 0:
            param->stack[1] = 30; /* denominator */
            break;
        default:
            param->errCode = EINVAL;
            return;
    }
    param->stack[0] = 1; /* numerator */
}
