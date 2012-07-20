/*
 * Virtio transport header
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd.
 *
 * Author:
 *  Evgeny Voevodin <e.voevodin@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIRTIO_TRANSPORT_H_
#define VIRTIO_TRANSPORT_H_

#include "sysbus.h"
#include "virtio.h"

#define VIRTIO_MMIO_TRANSPORT "virtio-mmio-transport"

extern struct BusInfo virtio_transport_bus_info;

typedef int (*virtio_init_transport_fn)(DeviceState *dev, VirtIODevice *vdev);

typedef struct VirtIOTransportBusState {
    BusState bus;
    virtio_init_transport_fn init_fn;
} VirtIOTransportBusState;

int virtio_init_transport(DeviceState *dev, VirtIODevice *vdev);
uint32_t virtio_count_siblings(BusState *parent_bus, const char *child_bus);
/*
 * Get transport device which does not have a child.
 */
DeviceState* virtio_get_free_transport(BusState *parent_bus,
                                       const char *child_bus);

#endif /* VIRTIO_TRANSPORT_H_ */
