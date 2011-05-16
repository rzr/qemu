/*
 * Samsung Virtual 3D Accelerator
 *
 * Copyright (c) 2009 Samsung Electronics.
 * Contributed by Junsik.Park <okdear.park@samsung.com>
 */

#include "hw.h"
#include "pc.h"
#include "pci.h"

#define PCI_VENDOR_ID_SAMSUNG 0x144d
#define PCI_DEVICE_ID_VIRTIO_OPENGL 0x1004

#define FUNC 0x00
#define PID 0x01
#define TRS 0x02
#define IA  0x03
#define IAS 0x04

#define RUN_OPENGL 0x10


typedef struct AccelState {
    PCIDevice dev;
    int ret;
    int Accel_mmio_io_addr;
    uint32_t function_number;
    uint32_t pid;
    uint32_t target_ret_string;
    uint32_t in_args;
    uint32_t in_args_size;
} AccelState;

#if 0
uint32_t fn;
uint32_t p;
uint32_t trs;
uint32_t ia;
uint32_t ias;
#endif

#if defined (DEBUG_ACCEL)
#  define DEBUG_PRINT(x) do { printf x ; } while (0)
#else
#  define DEBUG_PRINT(x)
#endif

static int count = 0;

static uint32_t Accel_io_readb(void *opaque, target_phys_addr_t offset)
{
    AccelState *s = (AccelState *)opaque;

    offset &= 0xFF;
    switch (offset) {
        case RUN_OPENGL:
            s->ret = decode_call(NULL, s->function_number, 
               s->pid, s->target_ret_string, s->in_args, s->in_args_size);
            break;
        default:
            /* FIXME: update s->ret? */
            DEBUG_PRINT(("3d: not implemented read(b) addr=0x%x\n", offset));
            break;
    }
    count = 0;
    return s->ret;
}

static void Accel_io_writeb(void *opaque, target_phys_addr_t offset, uint32_t val)
{
/*
    AccelState *s = (AccelState *)opaque;

    offset &= 0xFF;

    switch(offset)
    {
	case RUN_OPENGL :
	    s->ret = decode_call(s->function_number, s->pid, s->target_ret_string, s->in_args, s->in_args_size);
	    break;
	default:
	    DEBUG_PRINT(("3d: not implemented write(b) addr=0x%x val=0x%02x\n", addr, val));
	    break;
    }
*/
}

static uint32_t Accel_io_readw(void *opaque, target_phys_addr_t offset)
{
    DEBUG_PRINT(("%s\n", __FUNCTION__));
    return 0;
}

static void Accel_io_writew(void *opaque, target_phys_addr_t offset, uint32_t val)
{
    AccelState *s = (AccelState *)opaque;

    offset &= 0xFF;
    switch (offset + count) {
        case FUNC:
            s->function_number = val;
            break;
        case PID:
            s->pid = val;
            break;
        case TRS:
            s->target_ret_string = val;
            break;
        case IA:
            s->in_args = val;
            break;
        case IAS:
            s->in_args_size = val;
            break;
        default:
            DEBUG_PRINT(("3d: ioport write(w) addr=0x%x val=0x%04x via write(b)\n", offset, val));
//	    Accel_io_writeb(opaque, offset, val & 0xff);
//	    Accel_io_writeb(opaque, offset + 1, (val >> 8) & 0xff);
            break;
    }
    count++;
}

static uint32_t Accel_io_readl(void *opaque, target_phys_addr_t offset)
{
    DEBUG_PRINT(("\n"));
    return 0;
}

static void Accel_io_writel(void *opaque, target_phys_addr_t offset, uint32_t val)
{
    AccelState *s = (AccelState *)opaque;
    
    offset &= 0xFF;
    switch (offset + count) {
    case FUNC:
        s->function_number = val;
        break;
    case PID:
        s->pid = val;
        break;
    case TRS:
        s->target_ret_string = val;
        break;
    case IA:
        s->in_args = val;
        break;
    case IAS:
        s->in_args_size = val;
        break;
    default:
        DEBUG_PRINT(("3d: ioport write(w) addr=0x%x val=0x%04x via write(b)\n", offset, val));
//	    Accel_io_writeb(opaque, offset, val & 0xff);
//	    Accel_io_writeb(opaque, offset + 1, (val >> 8) & 0xff);
//	    Accel_io_writeb(opaque, offset + 2, (val >> 16) & 0xff);
//	    Accel_io_writeb(opaque, offset + 3, (val >> 24) & 0xff);
        break;
    }
    count++;
}

static void Accel_ioport_writeb(void *opaque, uint32_t addr, uint32_t val)
{
    Accel_io_writeb(opaque, addr & 0xFF, val);
}

static void Accel_ioport_writew(void *opaque, uint32_t addr, uint32_t val)
{
    Accel_io_writew(opaque, addr & 0xFF, val);
}

static void Accel_ioport_writel(void *opaque, uint32_t addr, uint32_t val)
{
    Accel_io_writel(opaque, addr & 0xFF, val);
}

static uint32_t Accel_ioport_readb(void *opaque, uint32_t addr)
{
    return Accel_io_readb(opaque, addr & 0xFF);
}

static uint32_t Accel_ioport_readw(void *opaque, uint32_t addr)
{
    return Accel_io_readw(opaque, addr & 0xFF);
}

static uint32_t Accel_ioport_readl(void *opaque, uint32_t addr)
{
    return Accel_io_readl(opaque, addr & 0xFF);
}

static void Accel_mmio_writeb(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    Accel_io_writeb(opaque, addr & 0xFF, val);
}

static void Accel_mmio_writew(void *opaque, target_phys_addr_t addr, uint32_t val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    val = bswap16(val);
#endif
    Accel_io_writew(opaque, addr & 0xFF, val);
}

static void Accel_mmio_writel(void *opaque, target_phys_addr_t addr, uint32_t val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    val = bswap32(val);
#endif
    Accel_io_writel(opaque, addr & 0xFF, val);
}

static uint32_t Accel_mmio_readb(void *opaque, target_phys_addr_t addr)
{
    return Accel_io_readb(opaque, addr & 0xFF);
}

static uint32_t Accel_mmio_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t val = Accel_io_readw(opaque, addr & 0xFF);
#ifdef TARGET_WORDS_BIGENDIAN
    val = bswap16(val);
#endif
    return val;
}

static uint32_t Accel_mmio_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t val = Accel_io_readl(opaque, addr & 0xFF);
#ifdef TARGET_WORDS_BIGENDIAN
    val = bswap32(val);
#endif
    return val;
}
static const VMStateDescription vmstate_Accel = {
    .name = "Accel",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField []) {
        VMSTATE_PCI_DEVICE(dev, AccelState),
        VMSTATE_INT32(ret, AccelState),
        VMSTATE_INT32(Accel_mmio_io_addr, AccelState),
        VMSTATE_UINT32(function_number, AccelState),
        VMSTATE_UINT32(pid, AccelState),
        VMSTATE_UINT32(target_ret_string, AccelState),
        VMSTATE_UINT32(in_args, AccelState),
        VMSTATE_UINT32(in_args_size, AccelState),
        VMSTATE_END_OF_LIST()
    }
};

static void Accel_mmio_map(PCIDevice *dev, int region_num, pcibus_t addr,
                           pcibus_t size, int type)
{
    AccelState *s = DO_UPCAST(AccelState, dev, dev);
    cpu_register_physical_memory(addr + 0, 0x100, s->Accel_mmio_io_addr);
}

static void Accel_ioport_map(PCIDevice *pci_dev, int region_num, pcibus_t addr,
                             pcibus_t size, int type)
{
    AccelState *s = DO_UPCAST(AccelState, dev,pci_dev);

    register_ioport_write(addr, 0x100, 1, Accel_ioport_writeb ,s);
    register_ioport_read(addr, 0x100, 1, Accel_ioport_readb,s);
    DEBUG_PRINT(("\n"));

    register_ioport_write(addr, 0x100, 2, Accel_ioport_writew, s);
    register_ioport_read(addr, 0x100, 2, Accel_ioport_readw, s);

    register_ioport_write(addr, 0x100, 4, Accel_ioport_writel, s);
    register_ioport_read(addr, 0x100, 4, Accel_ioport_readl, s);

}

static CPUReadMemoryFunc * const Accel_mmio_read[3] = {
    Accel_mmio_readb,
    Accel_mmio_readw,
    Accel_mmio_readl,
};
 
static CPUWriteMemoryFunc * const Accel_mmio_write[3] = {
    Accel_mmio_writeb,
    Accel_mmio_writew,
    Accel_mmio_writel,
};

static int pci_Accel_initfn(PCIDevice *dev)
{
    AccelState *s = DO_UPCAST(AccelState, dev, dev);
    uint8_t *pci_conf;

    DEBUG_PRINT(("\n"));

    pci_conf = s->dev.config;
    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_SAMSUNG);
    pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTIO_OPENGL);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_OTHER);
    pci_conf[PCI_HEADER_TYPE] = PCI_HEADER_TYPE_NORMAL;
    pci_conf[0x08] = 0x20;
    pci_conf[0x3d] = 1; 

    s->Accel_mmio_io_addr =
        cpu_register_io_memory(Accel_mmio_read, Accel_mmio_write, s, DEVICE_NATIVE_ENDIAN);

    pci_register_bar(&s->dev, 0, 256, PCI_BASE_ADDRESS_SPACE_IO,
                     Accel_ioport_map);
    pci_register_bar(&s->dev, 1, 256, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     Accel_mmio_map);
    
    s->function_number = 0;
    s->pid = 0;
    s->target_ret_string = 0;
    s->in_args = 0;
    s->in_args_size = 0;

    return 0;
}
/*
int pci_Accel_init(PCIBus *bus)
{
    PCIDevice *r;
    r = pci_create(bus, -1, "Acceleator");
    fprintf(stderr, "%s %p\n", __FUNCTION__, r);
    return 0;
}
*/
static PCIDeviceInfo Accel_info = {
    .qdev.name = "Accelerator",
    .qdev.size = sizeof(AccelState),
    .qdev.vmsd = &vmstate_Accel,
    .init = pci_Accel_initfn,
};

static void Accel_register_device(void)
{
    pci_qdev_register(&Accel_info);
    DEBUG_PRINT(("\n"));
}

device_init(Accel_register_device)
