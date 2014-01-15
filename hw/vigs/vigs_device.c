/*
 * vigs
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
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

#include "vigs_device.h"
#include "vigs_log.h"
#include "vigs_server.h"
#include "vigs_backend.h"
#include "vigs_regs.h"
#include "vigs_fenceman.h"
#include "hw/hw.h"
#include "ui/console.h"
#include "qemu/main-loop.h"

#define PCI_VENDOR_ID_VIGS 0x19B2
#define PCI_DEVICE_ID_VIGS 0x1011

#define VIGS_IO_SIZE 0x1000

struct work_queue;

typedef struct VIGSState
{
    VIGSDevice dev;

    void *display;

    struct work_queue *render_queue;

    MemoryRegion vram_bar;
    uint32_t vram_size;

    MemoryRegion ram_bar;
    uint32_t ram_size;

    MemoryRegion io_bar;

    struct vigs_fenceman *fenceman;

    QEMUBH *fence_ack_bh;

    struct vigs_server *server;

    /*
     * Our console.
     */
    QemuConsole *con;

    uint32_t reg_int;
} VIGSState;

#define TYPE_VIGS_DEVICE "vigs"

extern const char *vigs_backend;

static void vigs_update_irq(VIGSState *s)
{
    bool raise = false;

    if ((s->reg_int & VIGS_REG_INT_VBLANK_ENABLE) &&
        (s->reg_int & VIGS_REG_INT_VBLANK_PENDING)) {
        raise = true;
    }

    if (s->reg_int & VIGS_REG_INT_FENCE_ACK_PENDING) {
        raise = true;
    }

    if (raise) {
        pci_set_irq(&s->dev.pci_dev, 1);
    } else {
        pci_set_irq(&s->dev.pci_dev, 0);
    }
}

static void vigs_fence_ack_bh(void *opaque)
{
    VIGSState *s = opaque;

    if (vigs_fenceman_pending(s->fenceman)) {
        s->reg_int |= VIGS_REG_INT_FENCE_ACK_PENDING;
    }

    vigs_update_irq(s);
}

static void vigs_hw_update(void *opaque)
{
    VIGSState *s = opaque;
    DisplaySurface *ds = qemu_console_surface(s->con);

    if (!surface_data(ds)) {
        return;
    }

    vigs_server_update_display(s->server);

    dpy_gfx_update(s->con, 0, 0, surface_width(ds), surface_height(ds));

    if (s->reg_int & VIGS_REG_INT_VBLANK_ENABLE) {
        s->reg_int |= VIGS_REG_INT_VBLANK_PENDING;
        vigs_update_irq(s);
    }
}

static void vigs_hw_invalidate(void *opaque)
{
}

static void vigs_dpy_resize(void *user_data,
                            uint32_t width,
                            uint32_t height)
{
    VIGSState *s = user_data;
    DisplaySurface *ds = qemu_console_surface(s->con);

    if ((width != surface_width(ds)) ||
        (height != surface_height(ds)))
    {
        qemu_console_resize(s->con, width, height);
    }
}

static uint32_t vigs_dpy_get_stride(void *user_data)
{
    VIGSState *s = user_data;
    DisplaySurface *ds = qemu_console_surface(s->con);

    return surface_stride(ds);
}

static uint32_t vigs_dpy_get_bpp(void *user_data)
{
    VIGSState *s = user_data;
    DisplaySurface *ds = qemu_console_surface(s->con);

    return surface_bytes_per_pixel(ds);
}

static uint8_t *vigs_dpy_get_data(void *user_data)
{
    VIGSState *s = user_data;
    DisplaySurface *ds = qemu_console_surface(s->con);

    return surface_data(ds);
}

static void vigs_fence_ack(void *user_data,
                           uint32_t fence_seq)
{
    VIGSState *s = user_data;

    vigs_fenceman_ack(s->fenceman, fence_seq);

    qemu_bh_schedule(s->fence_ack_bh);
}

static uint64_t vigs_io_read(void *opaque, hwaddr offset,
                             unsigned size)
{
    VIGSState *s = opaque;

    switch (offset) {
    case VIGS_REG_INT:
        return s->reg_int;
    case VIGS_REG_FENCE_LOWER:
        return vigs_fenceman_get_lower(s->fenceman);
    case VIGS_REG_FENCE_UPPER:
        return vigs_fenceman_get_upper(s->fenceman);
    default:
        VIGS_LOG_CRITICAL("Bad register 0x%X read", (uint32_t)offset);
        break;
    }

    return 0;
}

static void vigs_io_write(void *opaque, hwaddr offset,
                          uint64_t value, unsigned size)
{
    VIGSState *s = opaque;

    switch (offset) {
    case VIGS_REG_EXEC:
        vigs_server_dispatch(s->server, value);
        break;
    case VIGS_REG_INT:
        if (value & VIGS_REG_INT_VBLANK_PENDING) {
            value &= ~VIGS_REG_INT_VBLANK_PENDING;
        } else {
            value |= (s->reg_int & VIGS_REG_INT_VBLANK_PENDING);
        }

        if (value & VIGS_REG_INT_FENCE_ACK_PENDING) {
            value &= ~VIGS_REG_INT_FENCE_ACK_PENDING;
        } else {
            value |= (s->reg_int & VIGS_REG_INT_FENCE_ACK_PENDING);
        }

        if (((s->reg_int & VIGS_REG_INT_VBLANK_ENABLE) == 0) &&
            (value & VIGS_REG_INT_VBLANK_ENABLE)) {
            VIGS_LOG_DEBUG("VBLANK On");
        } else if (((value & VIGS_REG_INT_VBLANK_ENABLE) == 0) &&
                   (s->reg_int & VIGS_REG_INT_VBLANK_ENABLE)) {
            VIGS_LOG_DEBUG("VBLANK Off");
        }

        s->reg_int = value & VIGS_REG_INT_MASK;

        vigs_update_irq(s);
        break;
    default:
        VIGS_LOG_CRITICAL("Bad register 0x%X write", (uint32_t)offset);
        break;
    }
}

static struct GraphicHwOps vigs_hw_ops =
{
    .invalidate = vigs_hw_invalidate,
    .gfx_update = vigs_hw_update
};

static const MemoryRegionOps vigs_io_ops =
{
    .read = vigs_io_read,
    .write = vigs_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static struct vigs_display_ops vigs_dpy_ops =
{
    .resize = vigs_dpy_resize,
    .get_stride = vigs_dpy_get_stride,
    .get_bpp = vigs_dpy_get_bpp,
    .get_data = vigs_dpy_get_data,
    .fence_ack = vigs_fence_ack,
};

static int vigs_device_init(PCIDevice *dev)
{
    VIGSState *s = DO_UPCAST(VIGSState, dev.pci_dev, dev);
    struct vigs_backend *backend = NULL;

    vigs_log_init();

    if (s->vram_size < 16 * 1024 * 1024) {
        VIGS_LOG_WARN("\"vram_size\" is too small, defaulting to 16mb");
        s->vram_size = 16 * 1024 * 1024;
    }

    if (s->ram_size < 1 * 1024 * 1024) {
        VIGS_LOG_WARN("\"ram_size\" is too small, defaulting to 1mb");
        s->ram_size = 1 * 1024 * 1024;
    }

    pci_config_set_interrupt_pin(dev->config, 1);

    memory_region_init_ram(&s->vram_bar, OBJECT(s),
                           TYPE_VIGS_DEVICE ".vram",
                           s->vram_size);

    memory_region_init_ram(&s->ram_bar, OBJECT(s),
                           TYPE_VIGS_DEVICE ".ram",
                           s->ram_size);

    memory_region_init_io(&s->io_bar, OBJECT(s),
                          &vigs_io_ops,
                          s,
                          TYPE_VIGS_DEVICE ".io",
                          VIGS_IO_SIZE);

    pci_register_bar(&s->dev.pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->vram_bar);
    pci_register_bar(&s->dev.pci_dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->ram_bar);
    pci_register_bar(&s->dev.pci_dev, 2, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->io_bar);

    if (!strcmp(vigs_backend, "gl")) {
        backend = vigs_gl_backend_create(s->display);
    } else if (!strcmp(vigs_backend, "sw")) {
        backend = vigs_sw_backend_create();
    }

    if (!backend) {
        goto fail;
    }

    s->fenceman = vigs_fenceman_create();

    s->fence_ack_bh = qemu_bh_new(vigs_fence_ack_bh, s);

    s->con = graphic_console_init(DEVICE(dev), &vigs_hw_ops, s);

    if (!s->con) {
        goto fail;
    }

    s->server = vigs_server_create(memory_region_get_ram_ptr(&s->vram_bar),
                                   memory_region_get_ram_ptr(&s->ram_bar),
                                   &vigs_dpy_ops,
                                   s,
                                   backend,
                                   s->render_queue);

    if (!s->server) {
        goto fail;
    }

    s->dev.wsi = &s->server->wsi;

    VIGS_LOG_INFO("VIGS initialized");

    VIGS_LOG_DEBUG("vram_size = %u", s->vram_size);
    VIGS_LOG_DEBUG("ram_size = %u", s->ram_size);

    return 0;

fail:
    if (backend) {
        backend->destroy(backend);
    }

    if (s->fence_ack_bh) {
        qemu_bh_delete(s->fence_ack_bh);
    }

    if (s->fenceman) {
        vigs_fenceman_destroy(s->fenceman);
    }

    memory_region_destroy(&s->io_bar);
    memory_region_destroy(&s->ram_bar);
    memory_region_destroy(&s->vram_bar);

    vigs_log_cleanup();

    return -1;
}

static void vigs_device_reset(DeviceState *d)
{
    VIGSState *s = container_of(d, VIGSState, dev.pci_dev.qdev);

    vigs_server_reset(s->server);

    vigs_fenceman_reset(s->fenceman);

    pci_set_irq(&s->dev.pci_dev, 0);

    s->reg_int = 0;

    VIGS_LOG_INFO("VIGS reset");
}

static void vigs_device_exit(PCIDevice *dev)
{
    VIGSState *s = DO_UPCAST(VIGSState, dev.pci_dev, dev);

    vigs_server_destroy(s->server);

    qemu_bh_delete(s->fence_ack_bh);

    vigs_fenceman_destroy(s->fenceman);

    memory_region_destroy(&s->io_bar);
    memory_region_destroy(&s->ram_bar);
    memory_region_destroy(&s->vram_bar);

    VIGS_LOG_INFO("VIGS deinitialized");

    vigs_log_cleanup();
}

static Property vigs_properties[] = {
    {
        .name   = "display",
        .info   = &qdev_prop_ptr,
        .offset = offsetof(VIGSState, display),
    },
    {
        .name   = "render_queue",
        .info   = &qdev_prop_ptr,
        .offset = offsetof(VIGSState, render_queue),
    },
    DEFINE_PROP_UINT32("vram_size", VIGSState, vram_size,
                       32 * 1024 * 1024),
    DEFINE_PROP_UINT32("ram_size", VIGSState, ram_size,
                       1 * 1024 * 1024),
    DEFINE_PROP_END_OF_LIST(),
};

static void vigs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = vigs_device_init;
    k->exit = vigs_device_exit;
    k->vendor_id = PCI_VENDOR_ID_VIGS;
    k->device_id = PCI_DEVICE_ID_VIGS;
    k->class_id = PCI_CLASS_DISPLAY_VGA;
    dc->reset = vigs_device_reset;
    dc->props = vigs_properties;
    dc->desc = "VIGS device";
}

static TypeInfo vigs_device_info =
{
    .name          = TYPE_VIGS_DEVICE,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(VIGSState),
    .class_init    = vigs_class_init,
};

static void vigs_register_types(void)
{
    type_register_static(&vigs_device_info);
}

type_init(vigs_register_types)
