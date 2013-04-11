#ifndef _QEMU_VIGS_DEVICE_H
#define _QEMU_VIGS_DEVICE_H

#include "qemu-common.h"
#include "pci.h"

struct winsys_interface;

typedef struct VIGSDevice
{
    PCIDevice pci_dev;

    struct winsys_interface *wsi;
} VIGSDevice;

#endif
