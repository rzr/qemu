/*
 * Samsung Virtual Camera device(PCI) for Windows host.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
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
#include "svcamera.h"
#include "pci.h"
#include "kvm.h"

void svcam_device_init(SVCamState* state)
{
	SVCamThreadInfo *thread = state->thread;

	pthread_cond_init(&thread->thread_cond, NULL);
	pthread_mutex_init(&thread->mutex_lock, NULL);
}

// SVCAM_CMD_OPEN
void svcam_device_open(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

// SVCAM_CMD_START_PREVIEW
void svcam_device_start_preview(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

// SVCAM_CMD_STOP_PREVIEW
void svcam_device_stop_preview(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_s_param(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_g_param(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_s_fmt(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_g_fmt(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_try_fmt(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_enum_fmt(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_qctrl(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_s_ctrl(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_g_ctrl(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_enum_fsizes(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

void svcam_device_enum_fintv(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}

// SVCAM_CMD_CLOSE
void svcam_device_close(SVCamState* state)
{
	SVCamParam *param = state->thread->param;

	param->top = 0;
	param->errCode = EINVAL;
}
