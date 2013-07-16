/*
 * Maru overlay device for VGA
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * DoHyung Hong
 * Hyunjun Son
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


#ifndef MARU_OVERLAY_H_
#define MARU_OVERLAY_H_

#include "qemu-common.h"
#include "ui/qemu-pixman.h"

extern uint8_t *overlay_ptr;
extern uint8_t overlay0_power;
extern uint16_t overlay0_left;
extern uint16_t overlay0_top;
extern uint16_t overlay0_width;
extern uint16_t overlay0_height;

extern uint8_t overlay1_power;
extern uint16_t overlay1_left;
extern uint16_t overlay1_top;
extern uint16_t overlay1_width;
extern uint16_t overlay1_height;

extern pixman_image_t *overlay0_image;
extern pixman_image_t *overlay1_image;

DeviceState *pci_maru_overlay_init(PCIBus *bus);

#endif /* MARU_OVERLAY_H_ */
