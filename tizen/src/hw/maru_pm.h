/*
 * Maru power management emulator
 * Based on qemu/hw/acpi_piix4.h
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#ifndef MARU_PM_H_
#define MARU_PM_H_

#include "qemu-common.h"

i2c_bus *maru_pm_init(PCIBus *bus, int devfn, uint32_t smb_io_base,
                       qemu_irq sci_irq, qemu_irq smi_irq,
                       int kvm_enabled, void *fw_cfg);

#ifdef TARGET_ARM
static int is_suspended_state( void )
{
    return 0;
}
static void resume( void )
{
    return;
}
#else
void resume( void );
int is_suspended_state( void );
#endif

#endif /* MARU_PM_H_ */
