/*
 * Samsung Virtual Camera device for Linux on X86.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu-common.h"
#include "svcamera.h"
#include "pci.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <libv4l2.h>
#include <libv4lconvert.h>

#if defined (DEBUG_SVCAM)
#define PIX_TO_FOURCC(pix) \
	((char) ((pix) & 0xff)), \
	((char) (((pix) >> 8) & 0xff)), \
	((char) (((pix) >> 16) & 0xff)), \
	((char) (((pix) >> 24) & 0xff))
#endif

static int v4l2_fd;

static struct v4lconvert_data *v4lcvtdata;
static struct v4l2_format src_fmt;
static struct v4l2_format dst_fmt;
static void *dst_buf;

static int xioctl(int fd, int req, void *arg)
{
	int r;

	do {
		r = v4l2_ioctl(fd, req, arg);
	} while ( r < 0 && errno == EINTR);

	return r;
}

static int __v4l_convert(SVCamState *state, void *src_ptr, uint32_t src_size)
{
	int ret;

	ret = v4lconvert_convert(v4lcvtdata,
							&src_fmt, &dst_fmt,
							(unsigned char *)src_ptr, src_size, (unsigned char *)dst_buf,
							dst_fmt.fmt.pix.sizeimage);
	if (ret < 0) {
		if (errno == EAGAIN)
			return 0;
		DEBUG_PRINT("v4lconvert_convert failed!!!");
		DEBUG_PRINT("%s", v4lconvert_get_error_message(v4lcvtdata));
		return -1;
	}

	memcpy(src_ptr, dst_buf, dst_fmt.fmt.pix.sizeimage);

	qemu_set_irq(state->dev.irq[0], 1);

	return 1;
}

static int __v4l2_get_frame(SVCamState *state){
	int r;

	r = v4l2_read(v4l2_fd, state->vaddr, dst_fmt.fmt.pix.sizeimage);
	if ( r < 0) {
		if (errno == EAGAIN)
			return 0;
		DEBUG_PRINT("READ : %s, %d", strerror(errno), errno);
		return -1;
	}

	if (v4lcvtdata) {
		r = __v4l_convert(state, state->vaddr, (uint32_t)r);
		return r;
	}

	qemu_set_irq(state->dev.irq[0], 1);

	return 1;
}

static int __v4l2_grab(SVCamState *state)
{
	fd_set fds;
	struct timeval tv;
	int r;
	
	FD_ZERO(&fds);
	FD_SET(v4l2_fd, &fds);

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	r = select(v4l2_fd + 1, &fds, NULL, NULL, &tv);

	if ( r < 0) {
		if (errno == EINTR)
			return 0;
		DEBUG_PRINT("select : %s", strerror(errno));
		return -1;
	}
	if (!r) {
		DEBUG_PRINT("Timed out");
		return -1;
	}

	return __v4l2_get_frame(state);
}

int svcam_fake_grab(SVCamState *state)
{
	return 0;
}

// Worker thread
void *svcam_worker_thread(void *thread_param)
{
	int cond;
	SVCamThreadInfo* thread = (SVCamThreadInfo*)thread_param;

wait_worker_thread:
	pthread_mutex_lock(&thread->mutex_lock);
	pthread_cond_wait(&thread->thread_cond, &thread->mutex_lock);
	pthread_mutex_unlock(&thread->mutex_lock);

	if (thread->state->is_webcam) {
		__v4l2_grab(thread->state);
		__v4l2_grab(thread->state);
		while(1)
		{
			pthread_mutex_lock(&thread->mutex_lock);
			cond = thread->state->streamon;
			pthread_mutex_unlock(&thread->mutex_lock);
			if (!cond)
				goto wait_worker_thread;
			if (__v4l2_grab(thread->state) < 0)
				goto wait_worker_thread;
		}
	} else {
		while(1)
		{
			pthread_mutex_lock(&thread->mutex_lock);
			cond = thread->state->streamon;
			pthread_mutex_unlock(&thread->mutex_lock);
			if (!cond)
				goto wait_worker_thread;
			if (svcam_fake_grab(thread->state) < 0)
				goto wait_worker_thread;
		}
	}

	qemu_free(thread_param);
	pthread_exit(NULL);
}

void svcam_device_init(SVCamState* state, SVCamParam* param)
{
	int err = 0;
	SVCamThreadInfo *thread = state->thread;

	pthread_cond_init(&thread->thread_cond, NULL);
	pthread_mutex_init(&thread->mutex_lock, NULL);

	err = pthread_create(&thread->thread_id, NULL, svcam_worker_thread, (void*)thread);
	if (err != 0) {
		DEBUG_PRINT("pthread_create() is failed");
	}
}

// SVCAM_CMD_OPEN
void svcam_device_open(SVCamState* state, SVCamParam* param)
{
	int fd;
	struct v4l2_capability cap;

	DEBUG_PRINT("");
	v4l2_fd = v4l2_open("/dev/video0", O_RDWR | O_NONBLOCK);
	if (v4l2_fd < 0) {
		DEBUG_PRINT("v4l2 device open failed. fake webcam mode ON.");
		return;
	}
	if (xioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap) < 0) {
			DEBUG_PRINT("VIDIOC_QUERYCAP failed. fake webcam mode ON.");
			v4l2_close(v4l2_fd);
			return;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) || 
					!(cap.capabilities & V4L2_CAP_STREAMING)) {
		DEBUG_PRINT("Not supported video driver. fake webcam mode ON.");
		v4l2_close(v4l2_fd);
		return;
	}

	/* we will use host webcam */
	state->is_webcam = 1;
	fd = v4l2_fd_open(v4l2_fd, V4L2_ENABLE_ENUM_FMT_EMULATION);
	if (fd != -1) {
		v4l2_fd = fd;
	}
	v4lcvtdata = v4lconvert_create(v4l2_fd);
	if (!v4lcvtdata) {
		DEBUG_PRINT("v4lconvert_create failed!!");
	}

	/* this is necessary? */
	memset(&src_fmt, 0, sizeof(src_fmt));
	memset(&dst_fmt, 0, sizeof(dst_fmt));
}

// SVCAM_CMD_START_PREVIEW
void svcam_device_start_preview(SVCamState* state, SVCamParam* param)
{
	DEBUG_PRINT("");

	pthread_mutex_lock(&state->thread->mutex_lock);
	state->streamon = 1;
	pthread_cond_signal(&state->thread->thread_cond);
	pthread_mutex_unlock(&state->thread->mutex_lock);
}

// SVCAM_CMD_STOP_PREVIEW
void svcam_device_stop_preview(SVCamState* state, SVCamParam* param)
{
	DEBUG_PRINT("");
	pthread_mutex_lock(&state->thread->mutex_lock);
	state->streamon = 0;
	pthread_mutex_unlock(&state->thread->mutex_lock);
	sleep(1);
}

void svcam_device_s_param(SVCamState* state, SVCamParam* param)
{
	struct v4l2_streamparm sp;

	DEBUG_PRINT("");
	param->top = 0;
	memset(&sp, 0, sizeof(struct v4l2_streamparm));
	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	sp.parm.capture.timeperframe.numerator = param->stack[0];
	sp.parm.capture.timeperframe.denominator = param->stack[1];

	if (xioctl(v4l2_fd, VIDIOC_S_PARM, &sp) < 0) {
		param->errCode = errno;
	}
	DEBUG_PRINT("numerator = %d", param->stack[0]);
	DEBUG_PRINT("demonimator = %d", param->stack[1]);
}

void svcam_device_g_param(SVCamState* state, SVCamParam* param)
{
	struct v4l2_streamparm sp;
	
	DEBUG_PRINT("");
	param->top = 0;
	memset(&sp, 0, sizeof(struct v4l2_streamparm));
	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(v4l2_fd, VIDIOC_G_PARM, &sp) < 0) {
		param->errCode = errno;
		return;
	}
	param->stack[0] = sp.parm.capture.timeperframe.numerator;
	param->stack[1] = sp.parm.capture.timeperframe.denominator;
	DEBUG_PRINT("numerator = %d", param->stack[0]);
	DEBUG_PRINT("demonimator = %d", param->stack[1]);
}

void svcam_device_s_fmt(SVCamState* state, SVCamParam* param)
{
	DEBUG_PRINT("");
	param->top = 0;
	memset(&dst_fmt, 0, sizeof(struct v4l2_format));
	dst_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_fmt.fmt.pix.width = param->stack[0];
	dst_fmt.fmt.pix.height = param->stack[1];
	dst_fmt.fmt.pix.pixelformat = param->stack[2];
	dst_fmt.fmt.pix.field = param->stack[3];
	DEBUG_PRINT("[IN] width : %d", dst_fmt.fmt.pix.width);
	DEBUG_PRINT("[IN] height : %d", dst_fmt.fmt.pix.height);
	DEBUG_PRINT("[IN] pix : %c%c%c%c", PIX_TO_FOURCC(dst_fmt.fmt.pix.pixelformat));
	DEBUG_PRINT("[IN] field : %d", dst_fmt.fmt.pix.field);

	if (v4lcvtdata) {
		if (v4lconvert_try_format(v4lcvtdata, &dst_fmt, &src_fmt) != 0)
			DEBUG_PRINT("v4lconvert_try_format failed!!");
		if (xioctl(v4l2_fd, VIDIOC_S_FMT, &src_fmt) < 0) {
			DEBUG_PRINT("VIDIOC_S_FMT failed!!");
			param->errCode = errno;
			return;
		}
		if (dst_buf) {
			qemu_free(dst_buf);
			dst_buf = NULL;
		}
		dst_buf = qemu_malloc(dst_fmt.fmt.pix.sizeimage);
	} else {
		if (xioctl(v4l2_fd, VIDIOC_S_FMT, &dst_fmt) < 0) {
			DEBUG_PRINT("VIDIOC_S_FMT failed!!");
			param->errCode = errno;
			return;
		}
	}

	DEBUG_PRINT("[OUT] width : %d", dst_fmt.fmt.pix.width);
	DEBUG_PRINT("[OUT] height : %d", dst_fmt.fmt.pix.height);
	DEBUG_PRINT("[OUT] pix : %c%c%c%c", PIX_TO_FOURCC(dst_fmt.fmt.pix.pixelformat));
	DEBUG_PRINT("[OUT] field : %d", dst_fmt.fmt.pix.field);
	DEBUG_PRINT("[OUT] bytesperline : %d", dst_fmt.fmt.pix.bytesperline);
	DEBUG_PRINT("[OUT] sizeimage : %d", dst_fmt.fmt.pix.sizeimage);
}

void svcam_device_g_fmt(SVCamState* state, SVCamParam* param)
{
	struct v4l2_format format;

	DEBUG_PRINT("");
	param->top = 0;
	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(v4l2_fd, VIDIOC_G_FMT, &format) < 0) {
		param->errCode = errno;		
	} else {
		param->stack[0] = format.fmt.pix.width;
		param->stack[1] = format.fmt.pix.height;
		param->stack[2] = format.fmt.pix.pixelformat;
		param->stack[3] = format.fmt.pix.field;
		param->stack[4] = format.fmt.pix.bytesperline;
		param->stack[5] = format.fmt.pix.sizeimage;
		memcpy(&dst_fmt, &format, sizeof(format));
		if (v4lcvtdata) {
				if (v4lconvert_try_format(v4lcvtdata, &dst_fmt, &src_fmt) != 0)
					DEBUG_PRINT("v4lconvert_try_format failed!!");
				if (dst_buf) {
					qemu_free(dst_buf);
					dst_buf = NULL;
				}
				dst_buf = qemu_malloc(dst_fmt.fmt.pix.sizeimage);
		}

		DEBUG_PRINT("[OUT] width : %d", format.fmt.pix.width);
		DEBUG_PRINT("[OUT] height : %d", format.fmt.pix.height);
		DEBUG_PRINT("[OUT] pix : %c%c%c%c", PIX_TO_FOURCC(format.fmt.pix.pixelformat));
		DEBUG_PRINT("[OUT] field : %d", format.fmt.pix.field);
		DEBUG_PRINT("[OUT] bytesperline : %d", format.fmt.pix.bytesperline);
		DEBUG_PRINT("[OUT] sizeimage : %d", format.fmt.pix.sizeimage);
	}
}

void svcam_device_try_fmt(SVCamState* state, SVCamParam* param)
{
	struct v4l2_format format;

	DEBUG_PRINT("");
	param->top = 0;
	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = param->stack[0];
	format.fmt.pix.height = param->stack[1];
	format.fmt.pix.pixelformat = param->stack[2];
	format.fmt.pix.field = param->stack[3];
	DEBUG_PRINT("[IN] width : %d", format.fmt.pix.width);
	DEBUG_PRINT("[IN] height : %d", format.fmt.pix.height);
	DEBUG_PRINT("[IN] pix : %c%c%c%c", PIX_TO_FOURCC(format.fmt.pix.pixelformat));
	DEBUG_PRINT("[IN] field : %d", format.fmt.pix.field);

	if (xioctl(v4l2_fd, VIDIOC_TRY_FMT, &format) < 0) {
		param->errCode = errno;
		return;
	}
}

void svcam_device_enum_fmt(SVCamState* state, SVCamParam* param)
{
	struct v4l2_fmtdesc format;

	DEBUG_PRINT("");
	param->top = 0;
	memset(&format, 0, sizeof(struct v4l2_fmtdesc));
	format.index = param->stack[0];
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(v4l2_fd, VIDIOC_ENUM_FMT, &format) < 0) {
		param->errCode = errno;
		return;
	}
	param->stack[0] = format.index;
	param->stack[1] = format.flags;
	param->stack[2] = format.pixelformat;
	/* set description */
	memcpy(&param->stack[3], format.description, sizeof(format.description));
	DEBUG_PRINT("index : %d", format.index);
	DEBUG_PRINT("flags : %d", format.flags);
	DEBUG_PRINT("pixelformat : %c%c%c%c", PIX_TO_FOURCC(format.pixelformat));
	DEBUG_PRINT("description : %s", format.description);
}

void svcam_device_qctrl(SVCamState* state, SVCamParam* param)
{
	struct v4l2_queryctrl ctrl = {0, };

	DEBUG_PRINT("");
	param->top = 0;
	ctrl.id = param->stack[0];

	if (xioctl(v4l2_fd, VIDIOC_QUERYCTRL, &ctrl) < 0) {
		param->errCode = errno;
		return;
	}
	param->stack[0] = ctrl.id;
	param->stack[1] = ctrl.minimum;
	param->stack[2] = ctrl.maximum;
	param->stack[3] = ctrl.step;
	param->stack[4] = ctrl.default_value;
	param->stack[5] = ctrl.flags;
	/* name field setting */
	memcpy(&param->stack[6], ctrl.name, sizeof(ctrl.name));
}

void svcam_device_s_ctrl(SVCamState* state, SVCamParam* param)
{
	struct v4l2_control ctrl = {0, };

	DEBUG_PRINT("");
	param->top = 0;
	ctrl.id = param->stack[0];
	ctrl.value = param->stack[1];

	if (xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
		param->errCode = errno;
		return;
	}
}

void svcam_device_g_ctrl(SVCamState* state, SVCamParam* param)
{
	struct v4l2_control ctrl = {0, };

	DEBUG_PRINT("");
	param->top = 0;
	ctrl.id = param->stack[0];

	if (xioctl(v4l2_fd, VIDIOC_G_CTRL, &ctrl) < 0) {
		param->errCode = errno;
		return;
	}

	param->stack[0] = ctrl.value;
}

void svcam_device_enum_fsizes(SVCamState* state, SVCamParam* param)
{
	struct v4l2_frmsizeenum fsize;

	DEBUG_PRINT("");
	param->top = 0;
	memset(&fsize, 0, sizeof(struct v4l2_frmsizeenum));
	fsize.index = param->stack[0];
	fsize.pixel_format = param->stack[1];

	if (xioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &fsize) < 0) {
		param->errCode = errno;
		return;
	}

	if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
		param->stack[0] = fsize.discrete.width;
		param->stack[1] = fsize.discrete.height;
		DEBUG_PRINT("width = %d", fsize.discrete.width);
		DEBUG_PRINT("height = %d", fsize.discrete.height);
	} else {
		param->errCode = EINVAL;
		DEBUG_PRINT("Not Supported mode, we only support DISCRETE");
	}
}

void svcam_device_enum_fintv(SVCamState* state, SVCamParam* param)
{
	struct v4l2_frmivalenum ival;

	DEBUG_PRINT("");
	param->top = 0;
	memset(&ival, 0, sizeof(struct v4l2_frmivalenum));
	ival.index = param->stack[0];
	ival.pixel_format = param->stack[1];
	ival.width = param->stack[2];
	ival.height = param->stack[3];

	if (xioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) < 0) {
		param->errCode = errno;
		return;
	}

	if (ival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
		param->stack[0] = ival.discrete.numerator;
		param->stack[1] = ival.discrete.denominator;
		DEBUG_PRINT("numerator = %d", ival.discrete.numerator);
		DEBUG_PRINT("denominator = %d", ival.discrete.denominator);
	} else {
		param->errCode = EINVAL;
		DEBUG_PRINT("Not Supported mode, we only support DISCRETE");
	}
}

// SVCAM_CMD_CLOSE
void svcam_device_close(SVCamState* state, SVCamParam* param)
{
	DEBUG_PRINT("");
	state->is_webcam = 0;
	if (v4lcvtdata) {
		v4lconvert_destroy(v4lcvtdata);
		v4lcvtdata = NULL;
	}

	if (dst_buf) {
		qemu_free(dst_buf);
		dst_buf = NULL;
	}
	v4l2_close(v4l2_fd);
}
