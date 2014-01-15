/*
 * Maru power management emulator
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Seokyeon Hwang <syeon.hwang@samsung.com>
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

#include "maru_pm.h"

#include "sysemu/sysemu.h"

#include "debug_ch.h"
#include "guest_server.h"

/* define debug channel */
MULTI_DEBUG_CHANNEL(tizen, maru_pm);

ACPIREGS *maru_pm_ar;
acpi_update_sci_fn maru_pm_update_sci;

static Notifier maru_suspend;
static Notifier maru_wakeup;

void resume(void)
{
    TRACE("resume called.\n");
    qemu_system_wakeup_request(QEMU_WAKEUP_REASON_OTHER);
    maru_pm_update_sci(maru_pm_ar);
}

static void maru_notify_suspend(Notifier *notifier, void *data) {
    notify_all_sdb_clients(STATE_SUSPEND);
}

static void maru_notify_wakeup(Notifier *notifier, void *data) {
    notify_all_sdb_clients(STATE_RUNNING);
}

void acpi_maru_pm_init(ACPIREGS *ar, acpi_update_sci_fn update_sci) {
    maru_pm_ar = ar;
    qemu_system_wakeup_enable(QEMU_WAKEUP_REASON_OTHER, 1);
    maru_pm_update_sci = update_sci;

    maru_suspend.notify = maru_notify_suspend;
    maru_wakeup.notify = maru_notify_wakeup;
    qemu_register_suspend_notifier(&maru_suspend);
    qemu_register_wakeup_notifier(&maru_wakeup);
}
