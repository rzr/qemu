#include "vigs_device.h"
#include "vigs_log.h"
#include "vigs_server.h"
#include "vigs_id_gen.h"
#include "vigs_backend.h"
#include "hw.h"
#include "console.h"
#include "kvm.h"
#include "hax.h"

#define PCI_VENDOR_ID_VIGS 0x19B2
#define PCI_DEVICE_ID_VIGS 0x1011

#define VIGS_REG_RAM_OFFSET 0
#define VIGS_REG_CR0        8
#define VIGS_REG_CR1        16
#define VIGS_REG_CR2        24
#define VIGS_REG_CR3        32
#define VIGS_REG_CR4        40
#define VIGS_REGS_SIZE      64

#define VIGS_IO_SIZE 0x4000

#define VIGS_MAX_USERS (VIGS_IO_SIZE / VIGS_REGS_SIZE)

struct vigs_user
{
    /*
     * For x86 only.
     */
    target_ulong cr[5];
};

#ifdef CONFIG_KVM
static __inline void vigs_cpu_synchronize_state(struct vigs_user *user)
{
    if (kvm_enabled()) {
        memcpy(&((CPUX86State*)cpu_single_env)->cr[0], &user->cr[0], sizeof(user->cr));
    }
}
#else
static __inline void vigs_cpu_synchronize_state(struct vigs_user *user)
{
}
#endif

typedef struct VIGSState
{
    VIGSDevice dev;

    void *display;

    MemoryRegion vram_bar;
    uint32_t vram_size;

    MemoryRegion ram_bar;
    uint32_t ram_size;

    MemoryRegion io_bar;

    struct vigs_server *server;
    struct vigs_user users[VIGS_MAX_USERS];

    /*
     * Our display.
     */
    DisplayState *ds;
} VIGSState;

#define TYPE_VIGS_DEVICE "vigs"

extern const char *vigs_backend;

static void vigs_hw_update(void *opaque)
{
    VIGSState *s = opaque;

    if (!ds_get_data(s->ds)) {
        return;
    }

    vigs_server_update_display(s->server);

    dpy_update(s->ds, 0, 0, ds_get_width(s->ds), ds_get_height(s->ds));
}

static void vigs_hw_invalidate(void *opaque)
{
}

static void vigs_dpy_resize(void *user_data,
                            uint32_t width,
                            uint32_t height)
{
    VIGSState *s = user_data;

    if ((width != ds_get_width(s->ds)) ||
        (height != ds_get_height(s->ds)))
    {
        qemu_console_resize(s->ds, width, height);
    }
}

static uint32_t vigs_dpy_get_stride(void *user_data)
{
    VIGSState *s = user_data;

    return ds_get_linesize(s->ds);
}

static uint32_t vigs_dpy_get_bpp(void *user_data)
{
    VIGSState *s = user_data;

    return ds_get_bytes_per_pixel(s->ds);
}

static uint8_t *vigs_dpy_get_data(void *user_data)
{
    VIGSState *s = user_data;

    return ds_get_data(s->ds);
}

static uint64_t vigs_io_read(void *opaque, target_phys_addr_t offset,
                             unsigned size)
{
    return 0;
}

static void vigs_io_write(void *opaque, target_phys_addr_t offset,
                          uint64_t value, unsigned size)
{
    VIGSState *s = opaque;
    int user_index = (offset / VIGS_REGS_SIZE);
    offset -= user_index * VIGS_REGS_SIZE;

    assert(user_index < VIGS_MAX_USERS);

    if (user_index >= VIGS_MAX_USERS) {
        VIGS_LOG_CRITICAL("bad user index = %d", user_index);
        return;
    }

    switch (offset) {
    case VIGS_REG_RAM_OFFSET:
        /*
         * 'vigs_cpu_synchronize_state' is required here in order to be able to
         * access target's virtual memory directly on KVM.
         */
        vigs_cpu_synchronize_state(&s->users[user_index]);
        vigs_server_dispatch(s->server, value);
        break;
    case VIGS_REG_CR0:
        s->users[user_index].cr[0] = value;
        VIGS_LOG_TRACE("user %d, CR0 = 0x%X", user_index, (uint32_t)s->users[user_index].cr[0]);
        break;
    case VIGS_REG_CR1:
        s->users[user_index].cr[1] = value;
        VIGS_LOG_TRACE("user %d, CR1 = 0x%X", user_index, (uint32_t)s->users[user_index].cr[1]);
        break;
    case VIGS_REG_CR2:
        s->users[user_index].cr[2] = value;
        VIGS_LOG_TRACE("user %d, CR2 = 0x%X", user_index, (uint32_t)s->users[user_index].cr[2]);
        break;
    case VIGS_REG_CR3:
        s->users[user_index].cr[3] = value;
        VIGS_LOG_TRACE("user %d, CR3 = 0x%X", user_index, (uint32_t)s->users[user_index].cr[3]);
        break;
    case VIGS_REG_CR4:
        s->users[user_index].cr[4] = value;
        VIGS_LOG_TRACE("user %d, CR4 = 0x%X", user_index, (uint32_t)s->users[user_index].cr[4]);
        break;
    default:
        VIGS_LOG_CRITICAL("user %d, bad offset = %d", user_index, offset);
        break;
    }
}

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

    memory_region_init_ram(&s->vram_bar,
                           TYPE_VIGS_DEVICE ".vram",
                           s->vram_size);

    memory_region_init_ram(&s->ram_bar,
                           TYPE_VIGS_DEVICE ".ram",
                           s->ram_size);

    memory_region_init_io(&s->io_bar,
                          &vigs_io_ops,
                          s,
                          TYPE_VIGS_DEVICE ".io",
                          VIGS_IO_SIZE);

    vigs_id_gen_init();

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

    s->ds = graphic_console_init(vigs_hw_update,
                                 vigs_hw_invalidate,
                                 NULL,
                                 NULL,
                                 s);

    if (!s->ds) {
        goto fail;
    }

    s->server = vigs_server_create(memory_region_get_ram_ptr(&s->vram_bar),
                                   memory_region_get_ram_ptr(&s->ram_bar),
                                   &vigs_dpy_ops,
                                   s,
                                   backend);

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

    memory_region_destroy(&s->io_bar);
    memory_region_destroy(&s->ram_bar);
    memory_region_destroy(&s->vram_bar);

    vigs_id_gen_cleanup();

    vigs_log_cleanup();

    return -1;
}

static void vigs_device_reset(DeviceState *d)
{
    VIGSState *s = container_of(d, VIGSState, dev.pci_dev.qdev);

    vigs_server_reset(s->server);

    vigs_id_gen_reset();

    VIGS_LOG_INFO("VIGS reset");
}

static void vigs_device_exit(PCIDevice *dev)
{
    VIGSState *s = DO_UPCAST(VIGSState, dev.pci_dev, dev);

    vigs_server_destroy(s->server);

    memory_region_destroy(&s->io_bar);
    memory_region_destroy(&s->ram_bar);
    memory_region_destroy(&s->vram_bar);

    vigs_id_gen_cleanup();

    VIGS_LOG_INFO("VIGS deinitialized");

    vigs_log_cleanup();
}

static Property vigs_properties[] = {
    {
        .name   = "display",
        .info   = &qdev_prop_ptr,
        .offset = offsetof(VIGSState, display),
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
