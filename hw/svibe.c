/*
 * sVibe: Haptic Vibratior Unit controller.
 *
 * Contributed by BuCheon Min <bucheon.min@samsung.com>
 * 
 * Copyright (c) 2010 Samsung Electronics.
 */

//#include <unistd.h>
//#include "qemu-common.h"
#include "hw.h"
#include "isa.h"

/***********************************************************/
/* sVibe device modeling */
#define VIBE_DEFAULT_DATAPORT               0xD000
#define VIBE_CONTROLPORT                    1
#define VIBE_DEVPROPTYPE_LICENSE_KEY        0 /*!< Property type constant to set Application licence key */
#define VIBE_DEVPROPTYPE_PRIORITY           1 /*!< Property type constant to set device priority */
#define VIBE_DEVPROPTYPE_DISABLE_EFFECTS    2 /*!< Property type constant to enable/disable effects on a device */
#define VIBE_DEVPROPTYPE_STRENGTH           3 /*!< Property type constant to set the strength (volume) on a device */
#define VIBE_DEVPROPTYPE_MASTERSTRENGTH     4 /*!< Property type constant to set the strength (volume) on ALL devices */
#define IVTEffect_size                      10
#define IVTEffect_data                      11
#define IVTEffect_start                     12
#define StopAllEffect                       13

typedef struct SVIBEState {
    ISADevice dev;
    uint32_t iobase;
    uint32_t priority;
    uint32_t strength;
    uint32_t master_strength;
    uint32_t count;
    uint32_t size;
    uint8_t *IVT;
    uint8_t stop;
    uint8_t control;
} SVIBEState;

static void svibe_vibrate(SVIBEState *vibe)
{
    if (vibe->IVT == NULL) {
        return;
    }
    vibe->stop = 1;

#if 0
	/* XXX: Don't even look at what is going on here */
    pid_t pid;
    char buf[PATH_MAX+1];
    int cnt, len;
    len = readlink("/proc/self/exe", buf, PATH_MAX);
    for (cnt = len-1; cnt > 0; cnt--) {
        if (buf[cnt] == '/') {
            buf[cnt] = '\0';
            break;
        }
    }
    sprintf(buf, "%s%s", buf, "/data/wave/vibration.wav");
    if ((pid = vfork()) < 0) {
        return;
    }
    if (pid == 0) { /*child*/
        execlp("aplay", "aplay", buf, (char *)0);
        exit(0);
    }
#endif
    return;
}

static void svibe_stop(SVIBEState *vibe)
{
    vibe->stop = 0;
}

static void svibe_dataport_write(void *opaque, uint32_t addr, uint32_t val)
{
    SVIBEState *s = (SVIBEState *)opaque;

    switch (s->control) {
        case VIBE_DEVPROPTYPE_LICENSE_KEY:
        case VIBE_DEVPROPTYPE_DISABLE_EFFECTS:
        case IVTEffect_start:
        case StopAllEffect:
            break;
        case VIBE_DEVPROPTYPE_PRIORITY:
            s->priority = val;
            break;
        case VIBE_DEVPROPTYPE_STRENGTH:
            s->strength = val;
            break;
        case VIBE_DEVPROPTYPE_MASTERSTRENGTH:
            s->master_strength = val;
            break;
        case IVTEffect_size:
            s->size = val;
            s->count = 0;
            if (s->IVT != NULL) {
                qemu_free(s->IVT);
			}
			/* FIXME: Any protection from maliciuos user input? */
            s->IVT = qemu_malloc(s->size * sizeof(uint32_t));
            break;
        case IVTEffect_data:
            if (s->size <= s->count) {
                break;
            }
            s->IVT[s->count] = val;
            s->count++;
            break;
        default:
            hw_error("svibe: bad write offset %x\n", (unsigned int)addr);
    }
}

static uint32_t svibe_dataport_read(void *opaque, uint32_t addr)
{
    uint32_t ret = 0;
    SVIBEState *s = (SVIBEState *)opaque;

    switch (s->control) {
        case VIBE_DEVPROPTYPE_LICENSE_KEY:
        case VIBE_DEVPROPTYPE_DISABLE_EFFECTS:
        case IVTEffect_data:
        case IVTEffect_start:
        case StopAllEffect:
            break;
        case VIBE_DEVPROPTYPE_PRIORITY:
            ret = s->priority;
            break;
        case VIBE_DEVPROPTYPE_STRENGTH:
            ret = s->strength;
            break;
        case VIBE_DEVPROPTYPE_MASTERSTRENGTH:
            ret = s->master_strength;
            break;
        case IVTEffect_size:
            ret = s->size;
            break;
        default:
            hw_error("svibe: bad read offset %x\n", (unsigned int)addr);
    }
    return ret;
}

static void svibe_controlport_write(void *opaque, uint32_t addr, uint32_t val)
{
    SVIBEState *s = (SVIBEState *)opaque;

    s->control = val;
    switch (s->control) {
        case VIBE_DEVPROPTYPE_LICENSE_KEY:
        case VIBE_DEVPROPTYPE_PRIORITY:
        case VIBE_DEVPROPTYPE_DISABLE_EFFECTS:
        case VIBE_DEVPROPTYPE_STRENGTH:
        case VIBE_DEVPROPTYPE_MASTERSTRENGTH:
        case IVTEffect_size:
        case IVTEffect_data:
            break;
        case IVTEffect_start:
            svibe_vibrate(s);
            break;
        case StopAllEffect:
            svibe_stop(s);
            break;
        default:
            hw_error("svibe: bad control offset %x\n", (unsigned int)addr);
    }
}

static const VMStateDescription vmstate_svibe = {
    .name = "Svibe",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField []) {
        VMSTATE_UINT32(priority, SVIBEState),
        VMSTATE_UINT32(strength, SVIBEState),
        VMSTATE_UINT32(master_strength, SVIBEState),
        VMSTATE_UINT32(count, SVIBEState),
        VMSTATE_UINT32(size, SVIBEState),
//    VMSTATE_UINT8_ARRAY(IVT, SVIBEState, (size * sizeof(uint32_t)),
        VMSTATE_UINT8(stop, SVIBEState),
        VMSTATE_UINT8(control, SVIBEState),
        VMSTATE_END_OF_LIST()
    }
};

static int svibe_initfn(ISADevice *dev)
{
    SVIBEState *s = DO_UPCAST(SVIBEState, dev, dev);

    s->priority = 0;
    s->strength = 1000;
    s->master_strength = 1000;
    s->IVT = NULL;

    register_ioport_write(s->iobase, 1, 4, svibe_dataport_write, s);
    register_ioport_read(s->iobase, 1, 4, svibe_dataport_read, s);
    register_ioport_write(s->iobase + VIBE_CONTROLPORT, 1, 1,
			              svibe_controlport_write, s);

	/* TODO: Does this device need an IRQ? */
    return 0;
}

static ISADeviceInfo svibe_info = {
    .qdev.name = "Svibe",
    .qdev.desc = "Vibrator",
    .qdev.size = sizeof(SVIBEState),
    .qdev.vmsd = &vmstate_svibe,
    .init = svibe_initfn,
    .qdev.props = (Property[]) {
        DEFINE_PROP_HEX32("iobase", SVIBEState, iobase, VIBE_DEFAULT_DATAPORT),
        DEFINE_PROP_END_OF_LIST(),
    },
};

static void svibe_register(void)
{
    isa_qdev_register(&svibe_info);
}

device_init(svibe_register)
