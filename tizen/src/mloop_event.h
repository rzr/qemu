/*
 * mainloop_evhandle.c
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Kitae Kim <kt920.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * DoHyung Hong
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

#ifndef MLOOP_EVENT_H_
#define MLOOP_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

void mloop_ev_init(void);
void mloop_ev_stop(void);

void mloop_evcmd_usbkbd(int on);
void mloop_evcmd_usbdisk(char *img);
void mloop_evcmd_hostkbd(int on);

int mloop_evcmd_get_usbkbd_status(void);
int mloop_evcmd_get_hostkbd_status(void);

void mloop_evcmd_set_usbkbd(void *dev);
void mloop_evcmd_set_usbdisk(void *dev);

void mloop_evcmd_raise_intr(void *irq);
void mloop_evcmd_lower_intr(void *irq);

void mloop_evcmd_touch(void);
void mloop_evcmd_keyboard(void *data);
void mloop_evcmd_ramdump(void);

void mloop_evcmd_sdcard(char* img);

#ifdef __cplusplus
}
#endif

#endif /* MLOOP_EVENT_H_ */
