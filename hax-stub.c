/*
 *  HAX stubs
 *
 *  Copyright (C) 2012 Samsung Electronics Co Ltd.
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

#include "hax.h"

int hax_sync_vcpus(void)
{
    return 0;
}

int hax_accel_init(void)
{
   return 0;
}

void hax_disable(int disable)
{
   return;
}

int hax_pre_init(ram_addr_t ram_size)
{
   return 0;
}

int hax_enabled(void)
{
   return 0;
}

void qemu_notify_hax_event(void)
{
   return;
}
