/*
 *  Samsung Maru ARM SoC emulation
 *
 *  Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *    Evgeny Voevodin <e.voevodin@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MARU_ARM_H_
#define MARU_ARM_H_

#include "qemu-common.h"
#include "memory.h"
#include "exynos4210.h"

void maru_arm_write_secondary(CPUARMState *env,
        const struct arm_boot_info *info);

Exynos4210State *maru_arm_soc_init(MemoryRegion *system_mem,
        unsigned long ram_size);

int codec_init(PCIBus *bus);
int maru_camera_pci_init(PCIBus *bus);

#endif /* MARU_ARM_H_ */
