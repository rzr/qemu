/*
 * Common header of MARU Virtual Camera device.
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

#ifndef _MARU_CAMERA_COMMON_H_
#define _MARU_CAMERA_COMMON_H_

#include "pci.h"
#include "qemu-thread.h"

#define MARUCAM_MAX_PARAM    20
#define MARUCAM_SKIPFRAMES    2

/* must sync with GUEST camera_driver */
#define MARUCAM_CMD_INIT           0x00
#define MARUCAM_CMD_OPEN           0x04
#define MARUCAM_CMD_CLOSE          0x08
#define MARUCAM_CMD_ISR            0x0C
#define MARUCAM_CMD_START_PREVIEW  0x10
#define MARUCAM_CMD_STOP_PREVIEW   0x14
#define MARUCAM_CMD_S_PARAM        0x18
#define MARUCAM_CMD_G_PARAM        0x1C
#define MARUCAM_CMD_ENUM_FMT       0x20
#define MARUCAM_CMD_TRY_FMT        0x24
#define MARUCAM_CMD_S_FMT          0x28
#define MARUCAM_CMD_G_FMT          0x2C
#define MARUCAM_CMD_QCTRL          0x30
#define MARUCAM_CMD_S_CTRL         0x34
#define MARUCAM_CMD_G_CTRL         0x38
#define MARUCAM_CMD_ENUM_FSIZES    0x3C
#define MARUCAM_CMD_ENUM_FINTV     0x40
#define MARUCAM_CMD_S_DATA         0x44
#define MARUCAM_CMD_G_DATA         0x48
#define MARUCAM_CMD_DATACLR        0x50
#define MARUCAM_CMD_REQFRAME       0x54

typedef struct MaruCamState MaruCamState;
typedef struct MaruCamParam MaruCamParam;

struct MaruCamParam {
    uint32_t    top;
    uint32_t    retVal;
    uint32_t    errCode;
    uint32_t    stack[MARUCAM_MAX_PARAM];
};

struct MaruCamState {
    PCIDevice           dev;
    MaruCamParam        *param;
    QemuThread          thread_id;
    QemuMutex           thread_mutex;;
    QemuCond            thread_cond;
    QEMUBH              *tx_bh;

    void                *vaddr;     /* vram ptr */
    uint32_t            isr;
    uint32_t            streamon;
    uint32_t            buf_size;
    uint32_t            req_frame;

    MemoryRegion        vram;
    MemoryRegion        mmio;
};

/* ----------------------------------------------------------------------------- */
/* Fucntion prototype                                                            */
/* ----------------------------------------------------------------------------- */
int marucam_device_check(void);
void marucam_device_init(MaruCamState *state);
void marucam_device_open(MaruCamState *state);
void marucam_device_close(MaruCamState *state);
void marucam_device_start_preview(MaruCamState *state);
void marucam_device_stop_preview(MaruCamState *state);
void marucam_device_s_param(MaruCamState *state);
void marucam_device_g_param(MaruCamState *state);
void marucam_device_s_fmt(MaruCamState *state);
void marucam_device_g_fmt(MaruCamState *state);
void marucam_device_try_fmt(MaruCamState *state);
void marucam_device_enum_fmt(MaruCamState *state);
void marucam_device_qctrl(MaruCamState *state);
void marucam_device_s_ctrl(MaruCamState *state);
void marucam_device_g_ctrl(MaruCamState *state);
void marucam_device_enum_fsizes(MaruCamState *state);
void marucam_device_enum_fintv(MaruCamState *state);

int maru_camera_pci_init(PCIBus *bus);

#endif  /* _MARU_CAMERA_COMMON_H_ */
