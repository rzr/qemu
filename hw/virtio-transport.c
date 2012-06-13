/*
 * Virtio transport bindings
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

#include "virtio-transport.h"

#define VIRTIO_TRANSPORT_BUS "virtio-transport"

struct BusInfo virtio_transport_bus_info = {
    .name = VIRTIO_TRANSPORT_BUS,
    .size = sizeof(VirtIOTransportBusState),
};

int virtio_init_transport(DeviceState *dev, VirtIODevice *vdev)
{
    DeviceState *transport_dev = qdev_get_parent_bus(dev)->parent;
    BusState *bus;
    VirtIOTransportBusState *virtio_transport_bus;

    /* transport device has single child bus */
    bus = QLIST_FIRST(&transport_dev->child_bus);
    virtio_transport_bus = DO_UPCAST(VirtIOTransportBusState, bus, bus);

    if (virtio_transport_bus->init_fn) {
        return virtio_transport_bus->init_fn(dev, vdev);
    }

    return 0;
}

uint32_t virtio_count_siblings(BusState *parent_bus, const char *child_bus)
{
    DeviceState *dev;
    BusState *bus;
    uint32_t i = 0;
    char *buf;
    int len;

    len = strlen(child_bus) + 1;
    buf = g_malloc(len);
    snprintf(buf, len, "%s.", child_bus);

    QTAILQ_FOREACH(dev, &parent_bus->children, sibling) {
        QLIST_FOREACH(bus, &dev->child_bus, sibling) {
            if (strncmp(buf, bus->name, strlen(buf)) == 0) {
                i++;
            }
        }
    }

    return i;
}
