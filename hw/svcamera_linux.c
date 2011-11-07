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

#include <linux/videodev2.h>

#include <libv4l2.h>
#include <libv4lconvert.h>

static int v4l2_fd;
static int convert_trial;

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
		switch (errno) {
		case EINVAL:
		case ENOMEM:
			DEBUG_PRINT("fail!");
			return -1;
		case EAGAIN:
		case EIO:
		case EINTR:
		default:
			DEBUG_PRINT("try again: %d", convert_trial);
			if (convert_trial-- == -1) {
				return -1;
			}
			return 0;
		}
	}
	pthread_mutex_lock(&state->thread->mutex_lock);
	if (!state->streamon) {
		pthread_mutex_unlock(&state->thread->mutex_lock);
		return 0;
	}
	pthread_mutex_unlock(&state->thread->mutex_lock);

	memcpy(src_ptr, dst_buf, dst_fmt.fmt.pix.sizeimage);

	qemu_irq_raise(state->dev.irq[0]);

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

	return __v4l_convert(state, state->vaddr, (uint32_t)r);
}

static int __v4l2_grab(SVCamState *state)
{
	fd_set fds;
	struct timeval tv;
	int r;
	
	FD_ZERO(&fds);
	FD_SET(v4l2_fd, &fds);

	tv.tv_sec = 2;
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
	pthread_mutex_lock(&state->thread->mutex_lock);
	r = state->streamon;
	pthread_mutex_unlock(&state->thread->mutex_lock);
	if (!r)
		return 0;

	return __v4l2_get_frame(state);
}

// Worker thread
static void *svcam_worker_thread(void *thread_param)
{
	SVCamThreadInfo* thread = (SVCamThreadInfo*)thread_param;

wait_worker_thread:
	pthread_mutex_lock(&thread->mutex_lock);
	thread->state->streamon = 0;
	convert_trial = 10;
	pthread_cond_wait(&thread->thread_cond, &thread->mutex_lock);
	pthread_mutex_unlock(&thread->mutex_lock);

	pthread_mutex_lock(&thread->mutex_lock);
	while(thread->state->streamon)
	{
		pthread_mutex_unlock(&thread->mutex_lock);
		if (__v4l2_grab(thread->state) < 0) {
			DEBUG_PRINT("__v4l2_grab() error!");
			goto wait_worker_thread;
		}
		pthread_mutex_lock(&thread->mutex_lock);
	}
	pthread_mutex_unlock(&thread->mutex_lock);
	goto wait_worker_thread;

	qemu_free(thread_param);
	pthread_exit(NULL);
}

void svcam_device_init(SVCamState* state)
{
	int err = 0;
	SVCamThreadInfo *thread = state->thread;

	pthread_cond_init(&thread->thread_cond, NULL);
	pthread_mutex_init(&thread->mutex_lock, NULL);

	err = pthread_create(&thread->thread_id, NULL, svcam_worker_thread, (void*)thread);
	if (err != 0) {
		perror("svcamera pthread_create fail");
		exit(0);
	}
}

// SVCAM_CMD_OPEN
void svcam_device_open(SVCamState* state)
{
	int fd;
	struct v4l2_capability cap;
	SVCamParam *param = state->thread->param;

	DEBUG_PRINT("");

	param->top = 0;
	v4l2_fd = v4l2_open("/dev/video0", O_RDWR | O_NONBLOCK);
	if (v4l2_fd < 0) {
		DEBUG_PRINT("v4l2 device open failed.");
		param->errCode = EINVAL;
		return;
	}
	if (xioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap) < 0) {
		DEBUG_PRINT("VIDIOC_QUERYCAP failed");
		v4l2_close(v4l2_fd);
		param->errCode = EINVAL;
		return;
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
			!(cap.capabilities & V4L2_CAP_STREAMING)) {
		DEBUG_PRINT("Not supported video driver.");
		v4l2_close(v4l2_fd);
		param->errCode = EINVAL;
		return;
	}

	fd = v4l2_fd_open(v4l2_fd, V4L2_ENABLE_ENUM_FMT_EMULATION);
	if (fd != -1) {
		v4l2_fd = fd;
	}
	v4lcvtdata = v4lconvert_create(v4l2_fd);
	if (!v4lcvtdata) {
		DEBUG_PRINT("v4lconvert_create failed!!");
		v4l2_close(v4l2_fd);
		perror("v4lconvert_create fail");
		exit(0);
	}

	/* this is necessary? */
	memset(&src_fmt, 0, sizeof(src_fmt));
	memset(&dst_fmt, 0, sizeof(dst_fmt));
}

// SVCAM_CMD_START_PREVIEW
void svcam_device_start_preview(SVCamState* state)
{
	DEBUG_PRINT("");
	pthread_mutex_lock(&state->thread->mutex_lock);
	state->streamon = 1;
	pthread_cond_signal(&state->thread->thread_cond);
	pthread_mutex_unlock(&state->thread->mutex_lock);
}

// SVCAM_CMD_STOP_PREVIEW
void svcam_device_stop_preview(SVCamState* state)
{
	DEBUG_PRINT("");
	pthread_mutex_lock(&state->thread->mutex_lock);
	state->streamon = 0;
	pthread_mutex_unlock(&state->thread->mutex_lock);
	sleep(0);
	qemu_irq_lower(state->dev.irq[0]);
}

void svcam_device_s_param(SVCamState* state)
{
	struct v4l2_streamparm sp;
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&sp, 0, sizeof(struct v4l2_streamparm));
	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	sp.parm.capture.timeperframe.numerator = param->stack[0];
	sp.parm.capture.timeperframe.denominator = param->stack[1];

	if (xioctl(v4l2_fd, VIDIOC_S_PARM, &sp) < 0) {
		param->errCode = errno;
	}
}

void svcam_device_g_param(SVCamState* state)
{
	struct v4l2_streamparm sp;
	SVCamParam *param = state->thread->param;
	
	param->top = 0;
	memset(&sp, 0, sizeof(struct v4l2_streamparm));
	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(v4l2_fd, VIDIOC_G_PARM, &sp) < 0) {
		param->errCode = errno;
		return;
	}
	param->stack[0] = sp.parm.capture.timeperframe.numerator;
	param->stack[1] = sp.parm.capture.timeperframe.denominator;
}

void svcam_device_s_fmt(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&dst_fmt, 0, sizeof(struct v4l2_format));
	dst_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_fmt.fmt.pix.width = param->stack[0];
	dst_fmt.fmt.pix.height = param->stack[1];
	dst_fmt.fmt.pix.pixelformat = param->stack[2];
	dst_fmt.fmt.pix.field = param->stack[3];

	if (v4lconvert_try_format(v4lcvtdata, &dst_fmt, &src_fmt) != 0) {
		DEBUG_PRINT("v4lconvert_try_format failed!!");
		param->errCode = EINVAL;
		return;
	}
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

	param->stack[0] = dst_fmt.fmt.pix.width;
	param->stack[1] = dst_fmt.fmt.pix.height;
	param->stack[2] = dst_fmt.fmt.pix.field;
	param->stack[3] = dst_fmt.fmt.pix.pixelformat;
	param->stack[4] = dst_fmt.fmt.pix.bytesperline;
	param->stack[5] = dst_fmt.fmt.pix.sizeimage;
	param->stack[6] = dst_fmt.fmt.pix.colorspace;
	param->stack[7] = dst_fmt.fmt.pix.priv;
}

void svcam_device_g_fmt(SVCamState* state)
{
	struct v4l2_format format;
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(v4l2_fd, VIDIOC_G_FMT, &format) < 0) {
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
		memcpy(&dst_fmt, &format, sizeof(format));

		if (v4lconvert_try_format(v4lcvtdata, &dst_fmt, &src_fmt) != 0) {
			DEBUG_PRINT("v4lconvert_try_format failed!!");
			param->errCode = EINVAL;
			return;
		}
		if (dst_buf) {
			qemu_free(dst_buf);
			dst_buf = NULL;
		}
		dst_buf = qemu_malloc(dst_fmt.fmt.pix.sizeimage);
	}
}

void svcam_device_try_fmt(SVCamState* state)
{
	struct v4l2_format format;
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = param->stack[0];
	format.fmt.pix.height = param->stack[1];
	format.fmt.pix.pixelformat = param->stack[2];
	format.fmt.pix.field = param->stack[3];

	if (xioctl(v4l2_fd, VIDIOC_TRY_FMT, &format) < 0) {
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
}

void svcam_device_enum_fmt(SVCamState* state)
{
	struct v4l2_fmtdesc format;
	SVCamParam *param = state->thread->param;

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
}

void svcam_device_qctrl(SVCamState* state)
{
	struct v4l2_queryctrl ctrl;
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&ctrl, 0, sizeof(struct v4l2_queryctrl));
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

void svcam_device_s_ctrl(SVCamState* state)
{
	struct v4l2_control ctrl;
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&ctrl, 0, sizeof(struct v4l2_control));
	ctrl.id = param->stack[0];
	ctrl.value = param->stack[1];

	if (xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
		param->errCode = errno;
		return;
	}
}

void svcam_device_g_ctrl(SVCamState* state)
{
	struct v4l2_control ctrl;
	SVCamParam *param = state->thread->param;

	param->top = 0;
	memset(&ctrl, 0, sizeof(struct v4l2_control));
	ctrl.id = param->stack[0];

	if (xioctl(v4l2_fd, VIDIOC_G_CTRL, &ctrl) < 0) {
		param->errCode = errno;
		return;
	}

	param->stack[0] = ctrl.value;
}

void svcam_device_enum_fsizes(SVCamState* state)
{
	struct v4l2_frmsizeenum fsize;
	SVCamParam *param = state->thread->param;

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
	} else {
		param->errCode = EINVAL;
		DEBUG_PRINT("Not Supported mode, we only support DISCRETE");
	}
}

void svcam_device_enum_fintv(SVCamState* state)
{
	struct v4l2_frmivalenum ival;
	SVCamParam *param = state->thread->param;

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
	} else {
		param->errCode = EINVAL;
		DEBUG_PRINT("Not Supported mode, we only support DISCRETE");
	}
}

// SVCAM_CMD_CLOSE
void svcam_device_close(SVCamState* state)
{
	DEBUG_PRINT(" ");
	if (v4lcvtdata) {
		v4lconvert_destroy(v4lcvtdata);
		v4lcvtdata = NULL;
	}

	v4l2_close(v4l2_fd);

	if (dst_buf) {
		qemu_free(dst_buf);
		dst_buf = NULL;
	}
}
