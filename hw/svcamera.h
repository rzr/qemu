/*
 * Samsung Virtual Camera device.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef _SVCAMERA_H
#define _SVCAMERA_H

#include "pci.h"
#include <pthread.h>

#define DEBUG_SVCAM

#if defined (DEBUG_SVCAM)
#  define DEBUG_PRINT(x, arg...) do { printf("[%s] " x "\n", __func__,  ##arg); } while (0)
#else
#  define DEBUG_PRINT(x)
#endif

#define SVCAM_MAX_PARAM	 20

/* must sync with GUEST camera_driver */
#define SVCAM_CMD_INIT           0x00
#define SVCAM_CMD_OPEN           0x04
#define SVCAM_CMD_CLOSE          0x08
#define SVCAM_CMD_ISSTREAM       0x0C
#define SVCAM_CMD_START_PREVIEW  0x10
#define SVCAM_CMD_STOP_PREVIEW   0x14
#define SVCAM_CMD_S_PARAM        0x18
#define SVCAM_CMD_G_PARAM        0x1C
#define SVCAM_CMD_ENUM_FMT       0x20
#define SVCAM_CMD_TRY_FMT        0x24
#define SVCAM_CMD_S_FMT          0x28
#define SVCAM_CMD_G_FMT          0x2C
#define SVCAM_CMD_QCTRL          0x30
#define SVCAM_CMD_S_CTRL         0x34
#define SVCAM_CMD_G_CTRL         0x38
#define SVCAM_CMD_ENUM_FSIZES    0x3C
#define SVCAM_CMD_ENUM_FINTV     0x40
#define SVCAM_CMD_S_DATA         0x44
#define SVCAM_CMD_G_DATA         0x48
#define SVCAM_CMD_CLRIRQ         0x4C
#define SVCAM_CMD_DATACLR        0x50

typedef struct SVCamState SVCamState;
typedef struct SVCamThreadInfo SVCamThreadInfo;
typedef struct SVCamParam SVCamParam;

struct SVCamParam {
	uint32_t	top;
	uint32_t	retVal;
	uint32_t	errCode;
	uint32_t	stack[SVCAM_MAX_PARAM];
};

struct SVCamThreadInfo {
	SVCamState* state;
	SVCamParam* param;

#if defined(_WIN32)
	HANDLE 	  tid;
	HANDLE    hReady;
	HANDLE    hFinish;
#else
	pthread_t		thread_id;
	pthread_cond_t	thread_cond;
	pthread_mutex_t	mutex_lock;
#endif
};

struct SVCamState {
	PCIDevice				dev;

	int								streamon;

	ram_addr_t				mem_offset;
	void								*vaddr;

	int								cam_mmio;

	uint32_t					mem_addr;
	uint32_t					mmio_addr;

	SVCamThreadInfo	*thread;
};

/* ----------------------------------------------------------------------------- */
/* Fucntion prototype 						                                     */
/* ----------------------------------------------------------------------------- */
void svcam_device_init(SVCamState *state);
void svcam_device_open(SVCamState *state);
void svcam_device_close(SVCamState *state);
void svcam_device_start_preview(SVCamState *state);
void svcam_device_stop_preview(SVCamState *state);
void svcam_device_s_param(SVCamState *state);
void svcam_device_g_param(SVCamState *state);
void svcam_device_s_fmt(SVCamState *state);
void svcam_device_g_fmt(SVCamState *state);
void svcam_device_try_fmt(SVCamState *state);
void svcam_device_enum_fmt(SVCamState *state);
void svcam_device_qctrl(SVCamState *state);
void svcam_device_s_ctrl(SVCamState *state);
void svcam_device_g_ctrl(SVCamState *state);
void svcam_device_enum_fsizes(SVCamState *state);
void svcam_device_enum_fintv(SVCamState *state);


#endif	/* _CAMERA_H */
