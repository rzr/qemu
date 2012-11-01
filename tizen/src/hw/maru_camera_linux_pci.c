/*
 * Implementation of MARU Virtual Camera device by PCI bus on Linux.
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "qemu-common.h"
#include "maru_camera_common.h"
#include "pci.h"
#include "kvm.h"
#include "tizen/src/debug_ch.h"

#include <linux/videodev2.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <libv4l2.h>
#include <libv4lconvert.h>

MULTI_DEBUG_CHANNEL(tizen, camera_linux);

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define MARUCAM_DEFAULT_BUFFER_COUNT    4

#define MARUCAM_CTRL_VALUE_MAX      20
#define MARUCAM_CTRL_VALUE_MIN      1
#define MARUCAM_CTRL_VALUE_MID      10
#define MARUCAM_CTRL_VALUE_STEP     1

enum {
    _MC_THREAD_PAUSED,
    _MC_THREAD_STREAMON,
    _MC_THREAD_STREAMOFF,
};

typedef struct marucam_framebuffer {
    void *data;
    size_t size;
} marucam_framebuffer;

static int n_framebuffer;
static struct marucam_framebuffer *framebuffer;

static const char *dev_name = "/dev/video0";
static int v4l2_fd;
static int convert_trial;
static int ready_count;

static struct v4l2_format dst_fmt;

static int yioctl(int fd, int req, void *arg)
{
    int r;

    do {
        r = ioctl(fd, req, arg);
    } while (r < 0 && errno == EINTR);

    return r;
}

static int xioctl(int fd, int req, void *arg)
{
    int r;

    do {
        r = v4l2_ioctl(fd, req, arg);
    } while (r < 0 && errno == EINTR);

    return r;
}

typedef struct tagMaruCamConvertPixfmt {
    uint32_t fmt;   /* fourcc */
} MaruCamConvertPixfmt;

static MaruCamConvertPixfmt supported_dst_pixfmts[] = {
        { V4L2_PIX_FMT_YUYV },
        { V4L2_PIX_FMT_YUV420 },
        { V4L2_PIX_FMT_YVU420 },
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

struct marucam_qctrl {
    uint32_t id;
    uint32_t hit;
    int32_t min;
    int32_t max;
    int32_t step;
    int32_t init_val;
};

static struct marucam_qctrl qctrl_tbl[] = {
    { V4L2_CID_BRIGHTNESS, 0, },
    { V4L2_CID_CONTRAST, 0, },
    { V4L2_CID_SATURATION, 0, },
    { V4L2_CID_SHARPNESS, 0, },
};

static void marucam_reset_controls(void)
{
    uint32_t i;
    for (i = 0; i < ARRAY_SIZE(qctrl_tbl); i++) {
        if (qctrl_tbl[i].hit) {
            struct v4l2_control ctrl = {0,};
            qctrl_tbl[i].hit = 0;
            ctrl.id = qctrl_tbl[i].id;
            ctrl.value = qctrl_tbl[i].init_val;
            if (xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
                ERR("Failed to reset control value: id(0x%x), errstr(%s)\n",
                    ctrl.id, strerror(errno));
            }
        }
    }
}

static int32_t value_convert_from_guest(int32_t min, int32_t max, int32_t value)
{
    double rate = 0.0;
    int32_t dist = 0, ret = 0;

    dist = max - min;

    if (dist < MARUCAM_CTRL_VALUE_MAX) {
        rate = (double)MARUCAM_CTRL_VALUE_MAX / (double)dist;
        ret = min + (int32_t)(value / rate);
    } else {
        rate = (double)dist / (double)MARUCAM_CTRL_VALUE_MAX;
        ret = min + (int32_t)(rate * value);
    }
    return ret;
}

static int32_t value_convert_to_guest(int32_t min, int32_t max, int32_t value)
{
    double rate  = 0.0;
    int32_t dist = 0, ret = 0;

    dist = max - min;

    if (dist < MARUCAM_CTRL_VALUE_MAX) {
        rate = (double)MARUCAM_CTRL_VALUE_MAX / (double)dist;
        ret = (int32_t)((double)(value - min) * rate);
    } else {
        rate = (double)dist / (double)MARUCAM_CTRL_VALUE_MAX;
        ret = (int32_t)((double)(value - min) / rate);
    }

    return ret;
}

static void set_maxframeinterval(MaruCamState *state, uint32_t pixel_format,
                        uint32_t width, uint32_t height)
{
    struct v4l2_frmivalenum fival;
    struct v4l2_streamparm sp;
    uint32_t min_num = 0, min_denom = 0;

    CLEAR(fival);
    fival.pixel_format = pixel_format;
    fival.width = width;
    fival.height = height;

    if (xioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival) < 0) {
        ERR("Unable to enumerate intervals for pixelformat(0x%x), (%d:%d)\n",
            pixel_format, width, height);
        return;
    }

    if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
        float max_ival = -1.0;
        do {
            float cur_ival = (float)fival.discrete.numerator
                        / (float)fival.discrete.denominator;
            if (cur_ival > max_ival) {
                max_ival = cur_ival;
                min_num = fival.discrete.numerator;
                min_denom = fival.discrete.denominator;
            }
            TRACE("Discrete frame interval %u/%u supported\n",
                 fival.discrete.numerator, fival.discrete.denominator);
            fival.index++;
        } while (xioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival) >= 0);
    } else if ((fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) ||
                (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS)) {
        TRACE("Frame intervals from %u/%u to %u/%u supported",
            fival.stepwise.min.numerator, fival.stepwise.min.denominator,
            fival.stepwise.max.numerator, fival.stepwise.max.denominator);
        if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
            TRACE("with %u/%u step", fival.stepwise.step.numerator,
                  fival.stepwise.step.denominator);
        }
        if (((float)fival.stepwise.max.denominator /
             (float)fival.stepwise.max.numerator) >
            ((float)fival.stepwise.min.denominator /
             (float)fival.stepwise.min.numerator)) {
            min_num = fival.stepwise.max.numerator;
            min_denom = fival.stepwise.max.denominator;
        } else {
            min_num = fival.stepwise.min.numerator;
            min_denom = fival.stepwise.min.denominator;
        }
    }
    TRACE("The actual min values: %u/%u\n", min_num, min_denom);

    CLEAR(sp);
    sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sp.parm.capture.timeperframe.numerator = min_num;
    sp.parm.capture.timeperframe.denominator = min_denom;

    if (xioctl(v4l2_fd, VIDIOC_S_PARM, &sp) < 0) {
        ERR("Failed to set to minimum FPS(%u/%u)\n", min_num, min_denom);
    }
}

static uint32_t stop_capturing(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(v4l2_fd, VIDIOC_STREAMOFF, &type) < 0) {
        ERR("Failed to ioctl() with VIDIOC_STREAMOFF: %s\n", strerror(errno));
        return errno;
    }
    return 0;
}

static uint32_t start_capturing(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
        ERR("Failed to ioctl() with VIDIOC_STREAMON: %s\n", strerror(errno));
        return errno;
    }
    return 0;
}

static void free_framebuffers(marucam_framebuffer *fb, int buf_num)
{
    int i;

    if (fb == NULL) {
        ERR("The framebuffer is NULL. Failed to release the framebuffer\n");
        return;
    } else if (buf_num == 0) {
        ERR("The buffer count is 0. Failed to release the framebuffer\n");
        return;
    } else {
        TRACE("[%s]:fb(0x%p), buf_num(%d)\n", __func__, fb, buf_num);
    }

    /* Unmap framebuffers. */
    for (i = 0; i < buf_num; i++) {
        if (fb[i].data != NULL) {
            v4l2_munmap(fb[i].data, fb[i].size);
            fb[i].data = NULL;
            fb[i].size = 0;
        } else {
            ERR("framebuffer[%d].data is NULL.\n", i);
        }
    }
}

static uint32_t
mmap_framebuffers(marucam_framebuffer **fb, int *buf_num)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);
    req.count   = MARUCAM_DEFAULT_BUFFER_COUNT;
    req.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory  = V4L2_MEMORY_MMAP;
    if (xioctl(v4l2_fd, VIDIOC_REQBUFS, &req) < 0) {
        if (errno == EINVAL) {
            ERR("%s does not support memory mapping: %s\n",
                dev_name, strerror(errno));
        } else {
            ERR("Failed to request bufs: %s\n", strerror(errno));
        }
        return errno;
    }
    if (req.count == 0) {
        ERR("Insufficient buffer memory on %s\n", dev_name);
        return EINVAL;
    }

    *fb = g_new0(marucam_framebuffer, req.count);
    if (*fb == NULL) {
        ERR("Not enough memory to allocate framebuffers\n");
        return ENOMEM;
    }

    for (*buf_num = 0; *buf_num < req.count; ++*buf_num) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = *buf_num;
        if (xioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            ERR("Failed to ioctl() with VIDIOC_QUERYBUF: %s\n",
                strerror(errno));
            return errno;
        }

        (*fb)[*buf_num].size = buf.length;
        (*fb)[*buf_num].data = v4l2_mmap(NULL,
                     buf.length,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     v4l2_fd, buf.m.offset);
        if (MAP_FAILED == (*fb)[*buf_num].data) {
            ERR("Failed to mmap: %s\n", strerror(errno));
            return errno;
        }

        /* Queue the mapped buffer. */
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = *buf_num;
        if (xioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
            ERR("Failed to ioctl() with VIDIOC_QBUF: %s\n", strerror(errno));
            return errno;
        }
    }
    return 0;
}

static int is_streamon(MaruCamState *state)
{
    int st;
    qemu_mutex_lock(&state->thread_mutex);
    st = state->streamon;
    qemu_mutex_unlock(&state->thread_mutex);
    return (st == _MC_THREAD_STREAMON);
}

static int is_stream_paused(MaruCamState *state)
{
    int st;
    qemu_mutex_lock(&state->thread_mutex);
    st = state->streamon;
    qemu_mutex_unlock(&state->thread_mutex);
    return (st == _MC_THREAD_PAUSED);
}

static void __raise_err_intr(MaruCamState *state)
{
    qemu_mutex_lock(&state->thread_mutex);
    if (state->streamon == _MC_THREAD_STREAMON) {
        state->req_frame = 0; /* clear request */
        state->isr = 0x08;   /* set a error flag of rasing a interrupt */
        qemu_bh_schedule(state->tx_bh);
    }
    qemu_mutex_unlock(&state->thread_mutex);
}

static void
notify_buffer_ready(MaruCamState *state, void *ptr, size_t size)
{
    void *buf = NULL;

    qemu_mutex_lock(&state->thread_mutex);
    if (state->streamon == _MC_THREAD_STREAMON) {
        if (ready_count < MARUCAM_SKIPFRAMES) {
            /* skip a frame cause first some frame are distorted */
            ++ready_count;
            TRACE("Skip %d frame\n", ready_count);
            qemu_mutex_unlock(&state->thread_mutex);
            return;
        }
        if (state->req_frame == 0) {
            TRACE("There is no request\n");
            qemu_mutex_unlock(&state->thread_mutex);
            return;
        }
        buf = state->vaddr + state->buf_size * (state->req_frame - 1);
        memcpy(buf, ptr, state->buf_size);
        state->req_frame = 0; /* clear request */
        state->isr |= 0x01;   /* set a flag of rasing a interrupt */
        qemu_bh_schedule(state->tx_bh);
    }
    qemu_mutex_unlock(&state->thread_mutex);
}

static int read_frame(MaruCamState *state)
{
    struct v4l2_buffer buf;

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(v4l2_fd, VIDIOC_DQBUF, &buf) < 0) {
        switch (errno) {
        case EAGAIN:
        case EINTR:
            ERR("DQBUF error, try again: %s\n", strerror(errno));
            return 0;
        case EIO:
            ERR("The v4l2_read() met the EIO\n");
            if (convert_trial-- == -1) {
                ERR("Try count for v4l2_read is exceeded: %s\n",
                    strerror(errno));
                return -1;
            }
            return 0;
        default:
            ERR("DQBUF error: %s\n", strerror(errno));
            return -1;
        }
    }

    notify_buffer_ready(state, framebuffer[buf.index].data, buf.bytesused);

    if (xioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
        ERR("QBUF error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int __v4l2_streaming(MaruCamState *state)
{
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds);
    FD_SET(v4l2_fd, &fds);

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    ret = select(v4l2_fd + 1, &fds, NULL, NULL, &tv);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EINTR) {
            ERR("Select again: %s\n", strerror(errno));
            return 0;
        }
        ERR("Failed to select: %s\n", strerror(errno));
        __raise_err_intr(state);
        return -1;
    } else if (!ret) {
        ERR("Select timed out\n");
        return 0;
    }

    if (!v4l2_fd || (v4l2_fd == -1)) {
        ERR("The file descriptor is closed or not opened\n");
        __raise_err_intr(state);
        return -1;
    }

    ret = read_frame(state);
    if (ret < 0) {
        ERR("Failed to operate the read_frame()\n");
        __raise_err_intr(state);
        return -1;
    }
    return 0;
}

/* Worker thread */
static void *marucam_worker_thread(void *thread_param)
{
    MaruCamState *state = (MaruCamState *)thread_param;

wait_worker_thread:
    qemu_mutex_lock(&state->thread_mutex);
    state->streamon = _MC_THREAD_PAUSED;
    qemu_cond_wait(&state->thread_cond, &state->thread_mutex);
    qemu_mutex_unlock(&state->thread_mutex);

    convert_trial = 10;
    ready_count = 0;
    qemu_mutex_lock(&state->thread_mutex);
    state->streamon = _MC_THREAD_STREAMON;
    qemu_mutex_unlock(&state->thread_mutex);
    INFO("Streaming on ......\n");

    while (1) {
        if (is_streamon(state)) {
            if (__v4l2_streaming(state) < 0) {
                INFO("...... Streaming off\n");
                goto wait_worker_thread;
            }
        } else {
            INFO("...... Streaming off\n");
            goto wait_worker_thread;
        }
    }
    return NULL;
}

int marucam_device_check(int log_flag)
{
    int tmp_fd;
    struct timeval t1, t2;
    struct stat st;
    struct v4l2_fmtdesc format;
    struct v4l2_frmsizeenum size;
    struct v4l2_capability cap;
    int ret = 0;

    gettimeofday(&t1, NULL);
    if (stat(dev_name, &st) < 0) {
        fprintf(stdout, "[Webcam] <WARNING> Cannot identify '%s': %s\n",
                dev_name, strerror(errno));
    } else {
        if (!S_ISCHR(st.st_mode)) {
            fprintf(stdout, "[Webcam] <WARNING>%s is no character device\n",
                    dev_name);
        }
    }

    tmp_fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (tmp_fd < 0) {
        fprintf(stdout, "[Webcam] Camera device open failed: %s\n", dev_name);
        goto error;
    }
    if (ioctl(tmp_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        fprintf(stdout, "[Webcam] Could not qeury video capabilities\n");
        goto error;
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
            !(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stdout, "[Webcam] Not supported video driver\n");
        goto error;
    }
    ret = 1;

    if (log_flag) {
        fprintf(stdout, "[Webcam] Driver: %s\n", cap.driver);
        fprintf(stdout, "[Webcam] Card:  %s\n", cap.card);
        fprintf(stdout, "[Webcam] Bus info: %s\n", cap.bus_info);

        CLEAR(format);
        format.index = 0;
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (yioctl(tmp_fd, VIDIOC_ENUM_FMT, &format) < 0) {
            goto error;
        }

        do {
            CLEAR(size);
            size.index = 0;
            size.pixel_format = format.pixelformat;

            fprintf(stdout, "[Webcam] PixelFormat: %c%c%c%c\n",
                             (char)(format.pixelformat),
                             (char)(format.pixelformat >> 8),
                             (char)(format.pixelformat >> 16),
                             (char)(format.pixelformat >> 24));

            if (yioctl(tmp_fd, VIDIOC_ENUM_FRAMESIZES, &size) < 0) {
                goto error;
            }

            if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                do {
                    fprintf(stdout, "[Webcam] got discrete frame size %dx%d\n",
                                    size.discrete.width, size.discrete.height);
                    size.index++;
                } while (yioctl(tmp_fd, VIDIOC_ENUM_FRAMESIZES, &size) >= 0);
            } else if (size.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                fprintf(stdout, "[Webcam] we have stepwise frame sizes:\n");
                fprintf(stdout, "[Webcam] min width: %d, min height: %d\n",
                        size.stepwise.min_width, size.stepwise.min_height);
                fprintf(stdout, "[Webcam] max width: %d, max height: %d\n",
                        size.stepwise.max_width, size.stepwise.max_height);
                fprintf(stdout, "[Webcam] step width: %d, step height: %d\n",
                        size.stepwise.step_width, size.stepwise.step_height);
            } else if (size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
                fprintf(stdout, "[Webcam] we have continuous frame sizes:\n");
                fprintf(stdout, "[Webcam] min width: %d, min height: %d\n",
                        size.stepwise.min_width, size.stepwise.min_height);
                fprintf(stdout, "[Webcam] max width: %d, max height: %d\n",
                        size.stepwise.max_width, size.stepwise.max_height);

            }
            format.index++;
        } while (yioctl(tmp_fd, VIDIOC_ENUM_FMT, &format) >= 0);
    }
error:
    close(tmp_fd);
    gettimeofday(&t2, NULL);
    fprintf(stdout, "[Webcam] Elapsed time: %lu:%06lu\n",
                    t2.tv_sec-t1.tv_sec, t2.tv_usec-t1.tv_usec);
    return ret;
}

void marucam_device_init(MaruCamState *state)
{
    qemu_thread_create(&state->thread_id, marucam_worker_thread, (void *)state,
            QEMU_THREAD_JOINABLE);
}

void marucam_device_open(MaruCamState *state)
{
    MaruCamParam *param = state->param;

    param->top = 0;
    v4l2_fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (v4l2_fd < 0) {
        ERR("The v4l2 device open failed: %s\n", dev_name);
        param->errCode = EINVAL;
        return;
    }
    INFO("Opened\n");

    /* FIXME : Do not use fixed values */
    CLEAR(dst_fmt);
    dst_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dst_fmt.fmt.pix.width = 640;
    dst_fmt.fmt.pix.height = 480;
    dst_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    dst_fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (xioctl(v4l2_fd, VIDIOC_S_FMT, &dst_fmt) < 0) {
        ERR("Failed to set video format: format(0x%x), width:height(%d:%d), "
          "errstr(%s)\n", dst_fmt.fmt.pix.pixelformat, dst_fmt.fmt.pix.width,
          dst_fmt.fmt.pix.height, strerror(errno));
        param->errCode = errno;
        return;
    }
    TRACE("Set the default format: w:h(%dx%d), fmt(0x%x), size(%d), "
         "color(%d), field(%d)\n",
         dst_fmt.fmt.pix.width, dst_fmt.fmt.pix.height,
         dst_fmt.fmt.pix.pixelformat, dst_fmt.fmt.pix.sizeimage,
         dst_fmt.fmt.pix.colorspace, dst_fmt.fmt.pix.field);
}

void marucam_device_start_preview(MaruCamState *state)
{
    struct timespec req;
    MaruCamParam *param = state->param;
    param->top = 0;
    req.tv_sec = 0;
    req.tv_nsec = 10000000;

    INFO("Pixfmt(%c%c%c%C), W:H(%d:%d), buf size(%u)\n",
         (char)(dst_fmt.fmt.pix.pixelformat),
         (char)(dst_fmt.fmt.pix.pixelformat >> 8),
         (char)(dst_fmt.fmt.pix.pixelformat >> 16),
         (char)(dst_fmt.fmt.pix.pixelformat >> 24),
         dst_fmt.fmt.pix.width,
         dst_fmt.fmt.pix.height,
         dst_fmt.fmt.pix.sizeimage);

    param->errCode = mmap_framebuffers(&framebuffer, &n_framebuffer);
    if (param->errCode) {
        ERR("Failed to mmap framebuffers\n");
        if (framebuffer != NULL) {
            free_framebuffers(framebuffer, n_framebuffer);
            g_free(framebuffer);
            framebuffer = NULL;
            n_framebuffer = 0;
        }
        return;
    }

    param->errCode = start_capturing();
    if (param->errCode) {
        if (framebuffer != NULL) {
            free_framebuffers(framebuffer, n_framebuffer);
            g_free(framebuffer);
            framebuffer = NULL;
            n_framebuffer = 0;
        }
        return;
    }

    INFO("Starting preview\n");
    state->buf_size = dst_fmt.fmt.pix.sizeimage;
    qemu_mutex_lock(&state->thread_mutex);
    qemu_cond_signal(&state->thread_cond);
    qemu_mutex_unlock(&state->thread_mutex);

    /* nanosleep until thread is streamon  */
    while (!is_streamon(state)) {
        nanosleep(&req, NULL);
    }
}

void marucam_device_stop_preview(MaruCamState *state)
{
    struct timespec req;
    MaruCamParam *param = state->param;
    param->top = 0;
    req.tv_sec = 0;
    req.tv_nsec = 50000000;

    if (is_streamon(state)) {
        qemu_mutex_lock(&state->thread_mutex);
        state->streamon = _MC_THREAD_STREAMOFF;
        qemu_mutex_unlock(&state->thread_mutex);

        /* nanosleep until thread is paused  */
        while (!is_stream_paused(state)) {
            nanosleep(&req, NULL);
        }
    }

    param->errCode = stop_capturing();
    if (framebuffer != NULL) {
        free_framebuffers(framebuffer, n_framebuffer);
        g_free(framebuffer);
        framebuffer = NULL;
        n_framebuffer = 0;
    }
    state->buf_size = 0;
    INFO("Stopping preview\n");
}

void marucam_device_s_param(MaruCamState *state)
{
    MaruCamParam *param = state->param;

    param->top = 0;

    /* If KVM enabled, We use default FPS of the webcam.
     * If KVM disabled, we use mininum FPS of the webcam */
    if (!kvm_enabled()) {
        set_maxframeinterval(state, dst_fmt.fmt.pix.pixelformat,
                     dst_fmt.fmt.pix.width,
                     dst_fmt.fmt.pix.height);
    }
}

void marucam_device_g_param(MaruCamState *state)
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

void marucam_device_s_fmt(MaruCamState *state)
{
    struct v4l2_format format;
    MaruCamParam *param = state->param;

    param->top = 0;
    CLEAR(format);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = param->stack[0];
    format.fmt.pix.height = param->stack[1];
    format.fmt.pix.pixelformat = param->stack[2];
    format.fmt.pix.field = V4L2_FIELD_ANY;

    if (xioctl(v4l2_fd, VIDIOC_S_FMT, &format) < 0) {
        ERR("Failed to set video format: format(0x%x), width:height(%d:%d), "
          "errstr(%s)\n", format.fmt.pix.pixelformat, format.fmt.pix.width,
          format.fmt.pix.height, strerror(errno));
        param->errCode = errno;
        return;
    }

    memcpy(&dst_fmt, &format, sizeof(format));
    param->stack[0] = dst_fmt.fmt.pix.width;
    param->stack[1] = dst_fmt.fmt.pix.height;
    param->stack[2] = dst_fmt.fmt.pix.field;
    param->stack[3] = dst_fmt.fmt.pix.pixelformat;
    param->stack[4] = dst_fmt.fmt.pix.bytesperline;
    param->stack[5] = dst_fmt.fmt.pix.sizeimage;
    param->stack[6] = dst_fmt.fmt.pix.colorspace;
    param->stack[7] = dst_fmt.fmt.pix.priv;
    TRACE("Set the format: w:h(%dx%d), fmt(0x%x), size(%d), "
         "color(%d), field(%d)\n",
         dst_fmt.fmt.pix.width, dst_fmt.fmt.pix.height,
         dst_fmt.fmt.pix.pixelformat, dst_fmt.fmt.pix.sizeimage,
         dst_fmt.fmt.pix.colorspace, dst_fmt.fmt.pix.field);
}

void marucam_device_g_fmt(MaruCamState *state)
{
    struct v4l2_format format;
    MaruCamParam *param = state->param;

    param->top = 0;
    CLEAR(format);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(v4l2_fd, VIDIOC_G_FMT, &format) < 0) {
        ERR("Failed to get video format: %s\n", strerror(errno));
        param->errCode = errno;
    } else {
        param->stack[0] = format.fmt.pix.width;
        param->stack[1] = format.fmt.pix.height;
        param->stack[2] = format.fmt.pix.field;
        param->stack[3] = format.fmt.pix.pixelformat;
        param->stack[4] = format.fmt.pix.bytesperline;
        param->stack[5] = format.fmt.pix.sizeimage;
        param->stack[6] = format.fmt.pix.colorspace;
        param->stack[7] = format.fmt.pix.priv;
        TRACE("Get the format: w:h(%dx%d), fmt(0x%x), size(%d), "
             "color(%d), field(%d)\n",
             format.fmt.pix.width, format.fmt.pix.height,
             format.fmt.pix.pixelformat, format.fmt.pix.sizeimage,
             format.fmt.pix.colorspace, format.fmt.pix.field);
    }
}

void marucam_device_try_fmt(MaruCamState *state)
{
    struct v4l2_format format;
    MaruCamParam *param = state->param;

    param->top = 0;
    CLEAR(format);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = param->stack[0];
    format.fmt.pix.height = param->stack[1];
    format.fmt.pix.pixelformat = param->stack[2];
    format.fmt.pix.field = V4L2_FIELD_ANY;

    if (xioctl(v4l2_fd, VIDIOC_TRY_FMT, &format) < 0) {
        ERR("Failed to check video format: format(0x%x), width:height(%d:%d),"
            " errstr(%s)\n", format.fmt.pix.pixelformat, format.fmt.pix.width,
            format.fmt.pix.height, strerror(errno));
        param->errCode = errno;
        return;
    }
    param->stack[0] = format.fmt.pix.width;
    param->stack[1] = format.fmt.pix.height;
    param->stack[2] = format.fmt.pix.field;
    param->stack[3] = format.fmt.pix.pixelformat;
    param->stack[4] = format.fmt.pix.bytesperline;
    param->stack[5] = format.fmt.pix.sizeimage;
    param->stack[6] = format.fmt.pix.colorspace;
    param->stack[7] = format.fmt.pix.priv;
    TRACE("Check the format: w:h(%dx%d), fmt(0x%x), size(%d), "
         "color(%d), field(%d)\n",
         format.fmt.pix.width, format.fmt.pix.height,
         format.fmt.pix.pixelformat, format.fmt.pix.sizeimage,
         format.fmt.pix.colorspace, format.fmt.pix.field);
}

void marucam_device_enum_fmt(MaruCamState *state)
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
    /* set description */
    switch (supported_dst_pixfmts[index].fmt) {
    case V4L2_PIX_FMT_YUYV:
        memcpy(&param->stack[3], "YUYV", 32);
        break;
    case V4L2_PIX_FMT_YUV420:
        memcpy(&param->stack[3], "YU12", 32);
        break;
    case V4L2_PIX_FMT_YVU420:
        memcpy(&param->stack[3], "YV12", 32);
        break;
    }
}

void marucam_device_qctrl(MaruCamState *state)
{
    uint32_t i;
    char name[32] = {0,};
    struct v4l2_queryctrl ctrl;
    MaruCamParam *param = state->param;

    param->top = 0;
    CLEAR(ctrl);
    ctrl.id = param->stack[0];

    switch (ctrl.id) {
    case V4L2_CID_BRIGHTNESS:
        TRACE("Query : BRIGHTNESS\n");
        memcpy((void *)name, (void *)"brightness", 32);
        i = 0;
        break;
    case V4L2_CID_CONTRAST:
        TRACE("Query : CONTRAST\n");
        memcpy((void *)name, (void *)"contrast", 32);
        i = 1;
        break;
    case V4L2_CID_SATURATION:
        TRACE("Query : SATURATION\n");
        memcpy((void *)name, (void *)"saturation", 32);
        i = 2;
        break;
    case V4L2_CID_SHARPNESS:
        TRACE("Query : SHARPNESS\n");
        memcpy((void *)name, (void *)"sharpness", 32);
        i = 3;
        break;
    default:
        param->errCode = EINVAL;
        return;
    }

    if (xioctl(v4l2_fd, VIDIOC_QUERYCTRL, &ctrl) < 0) {
        if (errno != EINVAL) {
            ERR("Failed to query video controls: %s\n", strerror(errno));
        }
        param->errCode = errno;
        return;
    } else {
        struct v4l2_control sctrl;
        CLEAR(sctrl);
        sctrl.id = ctrl.id;
        if ((ctrl.maximum + ctrl.minimum) == 0) {
            sctrl.value = 0;
        } else {
            sctrl.value = (ctrl.maximum + ctrl.minimum) / 2;
        }
        if (xioctl(v4l2_fd, VIDIOC_S_CTRL, &sctrl) < 0) {
            ERR("Failed to set control value: id(0x%x), value(%d), "
                "errstr(%s)\n", sctrl.id, sctrl.value, strerror(errno));
            param->errCode = errno;
            return;
        }
        qctrl_tbl[i].hit = 1;
        qctrl_tbl[i].min = ctrl.minimum;
        qctrl_tbl[i].max = ctrl.maximum;
        qctrl_tbl[i].step = ctrl.step;
        qctrl_tbl[i].init_val = ctrl.default_value;
    }

    /* set fixed values by FW configuration file */
    param->stack[0] = ctrl.id;
    param->stack[1] = MARUCAM_CTRL_VALUE_MIN;    /* minimum */
    param->stack[2] = MARUCAM_CTRL_VALUE_MAX;    /* maximum */
    param->stack[3] = MARUCAM_CTRL_VALUE_STEP;   /* step */
    param->stack[4] = MARUCAM_CTRL_VALUE_MID;    /* default_value */
    param->stack[5] = ctrl.flags;
    /* name field setting */
    memcpy(&param->stack[6], (void *)name, sizeof(ctrl.name));
}

void marucam_device_s_ctrl(MaruCamState *state)
{
    uint32_t i;
    struct v4l2_control ctrl;
    MaruCamParam *param = state->param;

    param->top = 0;
    CLEAR(ctrl);
    ctrl.id = param->stack[0];

    switch (ctrl.id) {
    case V4L2_CID_BRIGHTNESS:
        i = 0;
        TRACE("%d is set to the value of the BRIGHTNESS\n", param->stack[1]);
        break;
    case V4L2_CID_CONTRAST:
        i = 1;
        TRACE("%d is set to the value of the CONTRAST\n", param->stack[1]);
        break;
    case V4L2_CID_SATURATION:
        i = 2;
        TRACE("%d is set to the value of the SATURATION\n", param->stack[1]);
        break;
    case V4L2_CID_SHARPNESS:
        i = 3;
        TRACE("%d is set to the value of the SHARPNESS\n", param->stack[1]);
        break;
    default:
        ERR("Our emulator does not support this control: 0x%x\n", ctrl.id);
        param->errCode = EINVAL;
        return;
    }

    ctrl.value = value_convert_from_guest(qctrl_tbl[i].min,
            qctrl_tbl[i].max, param->stack[1]);
    if (xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        ERR("Failed to set control value: id(0x%x), value(r:%d, c:%d), "
            "errstr(%s)\n", ctrl.id, param->stack[1], ctrl.value,
            strerror(errno));
        param->errCode = errno;
        return;
    }
}

void marucam_device_g_ctrl(MaruCamState *state)
{
    uint32_t i;
    struct v4l2_control ctrl;
    MaruCamParam *param = state->param;

    param->top = 0;
    CLEAR(ctrl);
    ctrl.id = param->stack[0];

    switch (ctrl.id) {
    case V4L2_CID_BRIGHTNESS:
        TRACE("Gets the value of the BRIGHTNESS\n");
        i = 0;
        break;
    case V4L2_CID_CONTRAST:
        TRACE("Gets the value of the CONTRAST\n");
        i = 1;
        break;
    case V4L2_CID_SATURATION:
        TRACE("Gets the value of the SATURATION\n");
        i = 2;
        break;
    case V4L2_CID_SHARPNESS:
        TRACE("Gets the value of the SHARPNESS\n");
        i = 3;
        break;
    default:
        ERR("Our emulator does not support this control: 0x%x\n", ctrl.id);
        param->errCode = EINVAL;
        return;
    }

    if (xioctl(v4l2_fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        ERR("Failed to get video control value: %s\n", strerror(errno));
        param->errCode = errno;
        return;
    }
    param->stack[0] = value_convert_to_guest(qctrl_tbl[i].min,
            qctrl_tbl[i].max, ctrl.value);
    TRACE("Value: %d\n", param->stack[0]);
}

void marucam_device_enum_fsizes(MaruCamState *state)
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
        if (supported_dst_pixfmts[i].fmt == pixfmt) {
            break;
        }
    }

    if (i == ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }

    param->stack[0] = supported_dst_frames[index].width;
    param->stack[1] = supported_dst_frames[index].height;
}

void marucam_device_enum_fintv(MaruCamState *state)
{
    MaruCamParam *param = state->param;

    param->top = 0;

    /* switch by index(param->stack[0]) */
    switch (param->stack[0]) {
    case 0:
        /* we only use 1/30 frame interval */
        param->stack[1] = 30;   /* denominator */
        break;
    default:
        param->errCode = EINVAL;
        return;
    }
    param->stack[0] = 1;    /* numerator */
}

void marucam_device_close(MaruCamState *state)
{
    if (!is_stream_paused(state)) {
        marucam_device_stop_preview(state);
    }

    marucam_reset_controls();

    v4l2_close(v4l2_fd);
    v4l2_fd = 0;
    INFO("Closed\n");
}
