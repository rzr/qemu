/*
 * Virtio MMIO bindings
 *
 * Copyright (c) 2011 Linaro Limited
 *
 * Authors:
 *  Peter Maydell <peter.maydell@linaro.org>
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

/* TODO:
 *  * save/load support
 *  * test net, serial, balloon
 */

#include "sysbus.h"
#include "virtio.h"
#include "virtio-transport.h"
#include "virtio-blk.h"
#include "virtio-net.h"
#include "virtio-serial.h"
#include "host-utils.h"

/* #define DEBUG_VIRTIO_MMIO */

#ifdef DEBUG_VIRTIO_MMIO

#define DPRINTF(fmt, ...) \
do { printf("virtio_mmio: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

/* Memory mapped register offsets */
#define VIRTIO_MMIO_MAGIC 0x0
#define VIRTIO_MMIO_VERSION 0x4
#define VIRTIO_MMIO_DEVICEID 0x8
#define VIRTIO_MMIO_VENDORID 0xc
#define VIRTIO_MMIO_HOSTFEATURES 0x10
#define VIRTIO_MMIO_HOSTFEATURESSEL 0x14
#define VIRTIO_MMIO_GUESTFEATURES 0x20
#define VIRTIO_MMIO_GUESTFEATURESSEL 0x24
#define VIRTIO_MMIO_GUESTPAGESIZE 0x28
#define VIRTIO_MMIO_QUEUESEL 0x30
#define VIRTIO_MMIO_QUEUENUMMAX 0x34
#define VIRTIO_MMIO_QUEUENUM 0x38
#define VIRTIO_MMIO_QUEUEALIGN 0x3c
#define VIRTIO_MMIO_QUEUEPFN 0x40
#define VIRTIO_MMIO_QUEUENOTIFY 0x50
#define VIRTIO_MMIO_INTERRUPTSTATUS 0x60
#define VIRTIO_MMIO_INTERRUPTACK 0x64
#define VIRTIO_MMIO_STATUS 0x70
/* Device specific config space starts here */
#define VIRTIO_MMIO_CONFIG 0x100

#define VIRT_MAGIC 0x74726976 /* 'virt' */
#define VIRT_VERSION 1
#define VIRT_VENDOR 0x554D4551 /* 'QEMU' */

#define VIRTIO_MMIO "virtio-mmio"
#define VIRTIO_MMIO_BUS "virtio-mmio-bus"

struct BusInfo virtio_mmio_bus_info = {
    .name = VIRTIO_MMIO_BUS,
    .size = sizeof(BusState),
};

enum VIRTIO_MMIO_MAPPINGS {
    VIRTIO_MMIO_IOMAP,
    VIRTIO_MMIO_IOMEM,
};

typedef struct {
    SysBusDevice busdev;
    VirtIODevice *vdev;
    MemoryRegion iomap; /* hold base address */
    MemoryRegion iomem; /* hold io funcs */
    MemoryRegion alias;
    qemu_irq irq;
    uint32_t int_enable;
    uint32_t host_features;
    uint32_t host_features_sel;
    uint32_t guest_features_sel;
    uint32_t guest_page_shift;
} VirtIOMMIOTransportState;

static uint64_t virtio_mmio_read(void *opaque, target_phys_addr_t offset,
                                 unsigned size)
{
    VirtIOMMIOTransportState *transport = (VirtIOMMIOTransportState *)opaque;
    VirtIODevice *vdev = transport->vdev;
    DPRINTF("virtio_mmio_read offset 0x%x\n", (int)offset);
    if (offset >= VIRTIO_MMIO_CONFIG) {
        offset -= VIRTIO_MMIO_CONFIG;
        switch (size) {
        case 1:
            return virtio_config_readb(vdev, offset);
        case 2:
            return virtio_config_readw(vdev, offset);
        case 4:
            return virtio_config_readl(vdev, offset);
        default:
            abort();
        }
    }
    if (size != 4) {
        DPRINTF("wrong size access to register!\n");
        return 0;
    }
    switch (offset) {
    case VIRTIO_MMIO_MAGIC:
        return VIRT_MAGIC;
    case VIRTIO_MMIO_VERSION:
        return VIRT_VERSION;
    case VIRTIO_MMIO_DEVICEID:
        return vdev->device_id;
    case VIRTIO_MMIO_VENDORID:
        return VIRT_VENDOR;
    case VIRTIO_MMIO_HOSTFEATURES:
        if (transport->host_features_sel) {
            return 0;
        }
        return transport->host_features;
    case VIRTIO_MMIO_QUEUENUMMAX:
        return VIRTQUEUE_MAX_SIZE;
    case VIRTIO_MMIO_QUEUEPFN:
        return virtio_queue_get_addr(vdev, vdev->queue_sel)
            >> transport->guest_page_shift;
    case VIRTIO_MMIO_INTERRUPTSTATUS:
        return vdev->isr;
    case VIRTIO_MMIO_STATUS:
        return vdev->status;
    case VIRTIO_MMIO_HOSTFEATURESSEL:
    case VIRTIO_MMIO_GUESTFEATURES:
    case VIRTIO_MMIO_GUESTFEATURESSEL:
    case VIRTIO_MMIO_GUESTPAGESIZE:
    case VIRTIO_MMIO_QUEUESEL:
    case VIRTIO_MMIO_QUEUENUM:
    case VIRTIO_MMIO_QUEUEALIGN:
    case VIRTIO_MMIO_QUEUENOTIFY:
    case VIRTIO_MMIO_INTERRUPTACK:
        DPRINTF("read of write-only register\n");
        return 0;
    default:
        DPRINTF("bad register offset\n");
        return 0;
    }
    return 0;
}

static void virtio_mmio_write(void *opaque, target_phys_addr_t offset,
                              uint64_t value, unsigned size)
{
    VirtIOMMIOTransportState *transport = (VirtIOMMIOTransportState *)opaque;
    VirtIODevice *vdev = transport->vdev;
    DPRINTF("virtio_mmio_write offset 0x%x value 0x%" PRIx64 "\n",
            (int)offset, value);
    if (offset >= VIRTIO_MMIO_CONFIG) {
        offset -= VIRTIO_MMIO_CONFIG;
        switch (size) {
        case 1:
            virtio_config_writeb(vdev, offset, value);
            break;
        case 2:
            virtio_config_writew(vdev, offset, value);
            break;
        case 4:
            virtio_config_writel(vdev, offset, value);
            break;
        default:
            abort();
        }
        return;
    }
    if (size != 4) {
        DPRINTF("wrong size access to register!\n");
        return;
    }
    switch (offset) {
    case VIRTIO_MMIO_HOSTFEATURESSEL:
        transport->host_features_sel = value;
        break;
    case VIRTIO_MMIO_GUESTFEATURES:
        if (!transport->guest_features_sel) {
            virtio_set_features(vdev, value);
        }
        break;
    case VIRTIO_MMIO_GUESTFEATURESSEL:
        transport->guest_features_sel = value;
        break;
    case VIRTIO_MMIO_GUESTPAGESIZE:
        transport->guest_page_shift = ctz32(value);
        if (transport->guest_page_shift > 31) {
            transport->guest_page_shift = 0;
        }
        DPRINTF("guest page size %" PRIx64 " shift %d\n", value,
                transport->guest_page_shift);
        break;
    case VIRTIO_MMIO_QUEUESEL:
        if (value < VIRTIO_PCI_QUEUE_MAX) {
            vdev->queue_sel = value;
        }
        break;
    case VIRTIO_MMIO_QUEUENUM:
        DPRINTF("mmio_queue write %d max %d\n", (int)value, VIRTQUEUE_MAX_SIZE);
        if (value <= VIRTQUEUE_MAX_SIZE) {
            DPRINTF("calling virtio_queue_set_num\n");
            virtio_queue_set_num(vdev, vdev->queue_sel, value);
        }
        break;
    case VIRTIO_MMIO_QUEUEALIGN:
        virtio_queue_set_align(vdev, vdev->queue_sel, value);
        break;
    case VIRTIO_MMIO_QUEUEPFN:
        if (value == 0) {
            virtio_reset(vdev);
        } else {
            virtio_queue_set_addr(vdev, vdev->queue_sel,
                                  value << transport->guest_page_shift);
        }
        break;
    case VIRTIO_MMIO_QUEUENOTIFY:
        if (value < VIRTIO_PCI_QUEUE_MAX) {
            virtio_queue_notify(vdev, value);
        }
        break;
    case VIRTIO_MMIO_INTERRUPTACK:
        vdev->isr &= ~value;
        virtio_update_irq(vdev);
        break;
    case VIRTIO_MMIO_STATUS:
        virtio_set_status(vdev, value & 0xff);
        if (vdev->status == 0) {
            virtio_reset(vdev);
        }
        break;
    case VIRTIO_MMIO_MAGIC:
    case VIRTIO_MMIO_VERSION:
    case VIRTIO_MMIO_DEVICEID:
    case VIRTIO_MMIO_VENDORID:
    case VIRTIO_MMIO_HOSTFEATURES:
    case VIRTIO_MMIO_QUEUENUMMAX:
    case VIRTIO_MMIO_INTERRUPTSTATUS:
        DPRINTF("write to readonly register\n");
        break;

    default:
        DPRINTF("bad register offset\n");
    }
}

static const MemoryRegionOps virtio_mem_ops = {
    .read = virtio_mmio_read,
    .write = virtio_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void virtio_mmio_update_irq(void *opaque, uint16_t vector)
{
    VirtIOMMIOTransportState *transport = opaque;
    int level = (transport->vdev->isr != 0);
    DPRINTF("virtio_mmio setting IRQ %d\n", level);
    qemu_set_irq(transport->irq, level);
}

static unsigned int virtio_mmio_get_features(void *opaque)
{
    VirtIOMMIOTransportState *transport = opaque;
    return transport->host_features;
}

static int virtio_mmio_load_config(void *opaque, QEMUFile *f)
{
    VirtIOMMIOTransportState *transport = opaque;
    transport->int_enable = qemu_get_be32(f);
    transport->host_features = qemu_get_be32(f);
    transport->host_features_sel = qemu_get_be32(f);
    transport->guest_features_sel = qemu_get_be32(f);
    transport->guest_page_shift = qemu_get_be32(f);
    return 0;
}

static void virtio_mmio_save_config(void *opaque, QEMUFile *f)
{
    VirtIOMMIOTransportState *transport = opaque;
    qemu_put_be32(f, transport->int_enable);
    qemu_put_be32(f, transport->host_features);
    qemu_put_be32(f, transport->host_features_sel);
    qemu_put_be32(f, transport->guest_features_sel);
    qemu_put_be32(f, transport->guest_page_shift);
}

static VirtIOBindings virtio_mmio_bindings = {
    .notify = virtio_mmio_update_irq,
    .get_features = virtio_mmio_get_features,
    .save_config = virtio_mmio_save_config,
    .load_config = virtio_mmio_load_config,
};

static void virtio_mmio_attach_to_transport(DeviceState *dev)
{
    DeviceState *transport_dev = qdev_get_parent_bus(dev)->parent;
    SysBusDevice *transport_sysbus = sysbus_from_qdev(transport_dev);
    VirtIOMMIOTransportState *transport =
            FROM_SYSBUS(VirtIOMMIOTransportState, transport_sysbus);

    /* Create alias and add it as subregion to transport iomem */
    memory_region_init_alias(&transport->alias,
                             "virtio-mmio-alias",
                             &transport->iomem,
                             0,
                             0x1000);
    /* add alias as subregion to transport iomap */
    memory_region_add_subregion(&transport->iomap,
                                0,
                                &transport->alias);
}

static int virtio_init_mmio_transport(DeviceState *dev, VirtIODevice *vdev)
{
    DeviceState *transport_dev = qdev_get_parent_bus(dev)->parent;
    SysBusDevice *transport_sysbus = sysbus_from_qdev(transport_dev);
    VirtIOMMIOTransportState *transport =
            FROM_SYSBUS(VirtIOMMIOTransportState, transport_sysbus);
    uint32_t i;

    /* Find a free transport where we will not have any siblings */
    i = virtio_count_siblings(qdev_get_parent_bus(dev), NULL);
    if (i > 1) {
        transport_dev =
                virtio_get_free_transport(qdev_get_parent_bus(transport_dev),
                                          VIRTIO_MMIO);
        assert(transport_dev != NULL);
        transport_sysbus = sysbus_from_qdev(transport_dev);
        transport = FROM_SYSBUS(VirtIOMMIOTransportState, transport_sysbus);

        /* Reconnect back-end to free transport. Access by QLIST because we
         * don't know actual suffix for virtio-pci.x */
        QTAILQ_REMOVE(&qdev_get_parent_bus(dev)->children, dev, sibling);
        qdev_set_parent_bus(dev, qdev_get_child_bus(transport_dev,
                ((BusState *)QLIST_FIRST(&transport_dev->child_bus))->name));
    }

    transport->vdev = vdev;
    transport->vdev->nvectors = 0;
    sysbus_init_irq(&transport->busdev, &transport->irq);
    memory_region_init_io(&transport->iomem, &virtio_mem_ops, transport,
                          "virtio-mmio-iomem", 0x1000);
    sysbus_init_mmio(&transport->busdev, &transport->iomem);
    virtio_bind_device(vdev, &virtio_mmio_bindings, transport);
    transport->host_features |= (0x1 << VIRTIO_F_NOTIFY_ON_EMPTY);
    transport->host_features =
            vdev->get_features(vdev, transport->host_features);

    virtio_mmio_attach_to_transport(dev);
    return 0;
}

static void virtio_mmio_transport_handler(void *opaque, int irq, int level)
{
    VirtIOMMIOTransportState *s = (VirtIOMMIOTransportState *)opaque;

    qemu_set_irq(s->irq, level);

    return;
}

static int virtio_mmio_transport_device_init(SysBusDevice *busdev)
{
    VirtIOMMIOTransportState *s =
            DO_UPCAST(VirtIOMMIOTransportState, busdev, busdev);
    BusState *parent_bus = qdev_get_parent_bus(&busdev->qdev);
    BusState *child_bus;
    VirtIOTransportBusState *virtio_transport_bus;
    char *buf;
    int len;
    uint32_t i;

    i = virtio_count_siblings(parent_bus, VIRTIO_MMIO);

    len = strlen(VIRTIO_MMIO) + 16;
    buf = g_malloc(len);
    snprintf(buf, len, "%s.%d", VIRTIO_MMIO, i);
    child_bus = qbus_create(&virtio_transport_bus_info, &busdev->qdev, buf);

    /* Populate mmio init transport function */
    virtio_transport_bus = DO_UPCAST(VirtIOTransportBusState, bus, child_bus);
    virtio_transport_bus->init_fn = virtio_init_mmio_transport;

    qdev_init_gpio_in(&s->busdev.qdev, virtio_mmio_transport_handler, 1);

    sysbus_init_irq(busdev, &s->irq);
    memory_region_init(&s->iomap, "virtio-mmio-iomap", 0x1000);
    sysbus_init_mmio(busdev, &s->iomap);
    return 0;
}

static void virtio_mmio_reset(DeviceState *d)
{
    VirtIOMMIOTransportState *transport =
            FROM_SYSBUS(VirtIOMMIOTransportState, sysbus_from_qdev(d));
    if (transport->vdev) {
        virtio_reset(transport->vdev);
    }
}

static void virtio_mmio_transport_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);
    k->init = virtio_mmio_transport_device_init;
    dc->reset = virtio_mmio_reset;
}

static TypeInfo virtio_mmio_transport_info = {
    .name = VIRTIO_MMIO_TRANSPORT,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(VirtIOMMIOTransportState),
    .class_init = virtio_mmio_transport_class_init,
};

static void virtio_mmio_register_types(void)
{
    type_register_static(&virtio_mmio_transport_info);
}

type_init(virtio_mmio_register_types)
