/*
 * Maru virtio pci
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 * refer to hw/virtio/virtio-pci.c
 */

#include "hw/virtio/virtio-pci.h"

#include "hw/maru_device_ids.h"
#include "maru_virtio_evdi.h"
#include "maru_virtio_esm.h"
#include "maru_virtio_hwkey.h"
#include "maru_virtio_keyboard.h"
#include "maru_virtio_touchscreen.h"
#include "maru_virtio_sensor.h"
#include "maru_virtio_jack.h"
#include "maru_virtio_power.h"
#include "maru_virtio_nfc.h"
#include "maru_virtio_vmodem.h"

typedef struct VirtIOTouchscreenPCI VirtIOTouchscreenPCI;
typedef struct VirtIOEVDIPCI VirtIOEVDIPCI;
typedef struct VirtIOESMPCI VirtIOESMPCI;
typedef struct VirtIOHWKeyPCI VirtIOHWKeyPCI;
typedef struct VirtIOKeyboardPCI VirtIOKeyboardPCI;
typedef struct VirtIOSENSORPCI VirtIOSENSORPCI;
typedef struct VirtIONFCPCI VirtIONFCPCI;
typedef struct VirtIOPOWERPCI VirtIOPOWERPCI;
typedef struct VirtIOJACKPCI VirtIOJACKPCI;
typedef struct VirtIOVModemPCI VirtIOVModemPCI;

/*
 * virtio-touchscreen-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_TOUCHSCREEN_PCI "virtio-touchscreen-pci"
#define VIRTIO_TOUCHSCREEN_PCI(obj) \
        OBJECT_CHECK(VirtIOTouchscreenPCI, (obj), TYPE_VIRTIO_TOUCHSCREEN_PCI)

struct VirtIOTouchscreenPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOTouchscreen vdev;
};

/*
 * virtio-keyboard-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_KEYBOARD_PCI "virtio-keyboard-pci"
#define VIRTIO_KEYBOARD_PCI(obj) \
        OBJECT_CHECK(VirtIOKeyboardPCI, (obj), TYPE_VIRTIO_KEYBOARD_PCI)

struct VirtIOKeyboardPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOKeyboard vdev;
};

/*
 * virtio-evdi-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_EVDI_PCI "virtio-evdi-pci"
#define VIRTIO_EVDI_PCI(obj) \
        OBJECT_CHECK(VirtIOEVDIPCI, (obj), TYPE_VIRTIO_EVDI_PCI)

struct VirtIOEVDIPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOEVDI vdev;
};

/*
 * virtio-esm-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_ESM_PCI "virtio-esm-pci"
#define VIRTIO_ESM_PCI(obj) \
        OBJECT_CHECK(VirtIOESMPCI, (obj), TYPE_VIRTIO_ESM_PCI)
struct VirtIOESMPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOESM vdev;
};

/*
 * virtio-hwkey-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_HWKEY_PCI "virtio-hwkey-pci"
#define VIRTIO_HWKEY_PCI(obj) \
        OBJECT_CHECK(VirtIOHWKeyPCI, (obj), TYPE_VIRTIO_HWKEY_PCI)
struct VirtIOHWKeyPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOHWKey vdev;
};

/*
 * virtio-sensor-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_SENSOR_PCI "virtio-sensor-pci"
#define VIRTIO_SENSOR_PCI(obj) \
        OBJECT_CHECK(VirtIOSENSORPCI, (obj), TYPE_VIRTIO_SENSOR_PCI)

struct VirtIOSENSORPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOSENSOR vdev;
};

/*
 * virtio-nfc-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_NFC_PCI "virtio-nfc-pci"
#define VIRTIO_NFC_PCI(obj) \
        OBJECT_CHECK(VirtIONFCPCI, (obj), TYPE_VIRTIO_NFC_PCI)

struct VirtIONFCPCI {
    VirtIOPCIProxy parent_obj;
    VirtIONFC vdev;
};

/*
 * virtio-jack-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_JACK_PCI "virtio-jack-pci"
#define VIRTIO_JACK_PCI(obj) \
        OBJECT_CHECK(VirtIOJACKPCI, (obj), TYPE_VIRTIO_JACK_PCI)

struct VirtIOJACKPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOJACK vdev;
};

/*
 * virtio-power-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_POWER_PCI "virtio-power-pci"
#define VIRTIO_POWER_PCI(obj) \
        OBJECT_CHECK(VirtIOPOWERPCI, (obj), TYPE_VIRTIO_POWER_PCI)

struct VirtIOPOWERPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOPOWER vdev;
};

/*
 * virtio-vmodem-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_VMODEM_PCI "virtio-vmodem-pci"
#define VIRTIO_VMODEM_PCI(obj) \
        OBJECT_CHECK(VirtIOVModemPCI, (obj), TYPE_VIRTIO_VMODEM_PCI)
struct VirtIOVModemPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOVModem vdev;
};


/* virtio-touchscreen-pci */

static Property virtio_touchscreen_pci_properties[] = {
    DEFINE_PROP_UINT32(TOUCHSCREEN_OPTION_NAME,
        VirtIOTouchscreenPCI,vdev.max_finger, DEFAULT_MAX_FINGER),
    DEFINE_PROP_END_OF_LIST(),
};

static int virtio_touchscreen_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOTouchscreenPCI *dev = VIRTIO_TOUCHSCREEN_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_touchscreen_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    dc->props = virtio_touchscreen_pci_properties;
    k->init = virtio_touchscreen_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_TOUCHSCREEN;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_touchscreen_pci_instance_init(Object *obj)
{
    VirtIOTouchscreenPCI *dev = VIRTIO_TOUCHSCREEN_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_TOUCHSCREEN);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);

    dev->vdev.max_finger = DEFAULT_MAX_FINGER;
}

static TypeInfo virtio_touchscreen_pci_info = {
    .name          = TYPE_VIRTIO_TOUCHSCREEN_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOTouchscreenPCI),
    .instance_init = virtio_touchscreen_pci_instance_init,
    .class_init    = virtio_touchscreen_pci_class_init,
};

/* virtio-keyboard-pci */

static int virtio_keyboard_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOKeyboardPCI *dev = VIRTIO_KEYBOARD_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_keyboard_pci_class_init(ObjectClass *klass, void *data)
{
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_keyboard_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_KEYBOARD;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_keyboard_pci_instance_init(Object *obj)
{
    VirtIOKeyboardPCI *dev = VIRTIO_KEYBOARD_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_KEYBOARD);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_keyboard_pci_info = {
    .name          = TYPE_VIRTIO_KEYBOARD_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOKeyboardPCI),
    .instance_init = virtio_keyboard_pci_instance_init,
    .class_init    = virtio_keyboard_pci_class_init,
};

/* virtio-esm-pci */

static int virtio_esm_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOESMPCI *dev = VIRTIO_ESM_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_esm_pci_class_init(ObjectClass *klass, void *data)
{
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_esm_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_ESM;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_esm_pci_instance_init(Object *obj)
{
    VirtIOESMPCI *dev = VIRTIO_ESM_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_ESM);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_esm_pci_info = {
    .name          = TYPE_VIRTIO_ESM_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOESMPCI),
    .instance_init = virtio_esm_pci_instance_init,
    .class_init    = virtio_esm_pci_class_init,
};

/* virtio-hwkey-pci */

static int virtio_hwkey_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOHWKeyPCI *dev = VIRTIO_HWKEY_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_hwkey_pci_class_init(ObjectClass *klass, void *data)
{
//    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_hwkey_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_HWKEY;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_hwkey_pci_instance_init(Object *obj)
{
    VirtIOHWKeyPCI *dev = VIRTIO_HWKEY_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_HWKEY);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_hwkey_pci_info = {
    .name          = TYPE_VIRTIO_HWKEY_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOHWKeyPCI),
    .instance_init = virtio_hwkey_pci_instance_init,
    .class_init    = virtio_hwkey_pci_class_init,
};

/* virtio-evdi-pci */

static int virtio_evdi_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOEVDIPCI *dev = VIRTIO_EVDI_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_evdi_pci_class_init(ObjectClass *klass, void *data)
{
//    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_evdi_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_EVDI;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_evdi_pci_instance_init(Object *obj)
{
    VirtIOEVDIPCI *dev = VIRTIO_EVDI_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_EVDI);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_evdi_pci_info = {
    .name          = TYPE_VIRTIO_EVDI_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOEVDIPCI),
    .instance_init = virtio_evdi_pci_instance_init,
    .class_init    = virtio_evdi_pci_class_init,
};

/* virtio-sensor-pci */

static int virtio_sensor_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOSENSORPCI *dev = VIRTIO_SENSOR_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static Property virtio_sensor_pci_properties[] = {
    DEFINE_PROP_STRING(ATTRIBUTE_NAME_SENSORS, VirtIOSENSORPCI, vdev.sensors),
    DEFINE_VIRTIO_COMMON_FEATURES(VirtIOPCIProxy, host_features),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_sensor_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_sensor_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_SENSOR;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
    dc->props = virtio_sensor_pci_properties;
}

static void virtio_sensor_pci_instance_init(Object *obj)
{
    VirtIOSENSORPCI *dev = VIRTIO_SENSOR_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_SENSOR);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_sensor_pci_info = {
    .name          = TYPE_VIRTIO_SENSOR_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOSENSORPCI),
    .instance_init = virtio_sensor_pci_instance_init,
    .class_init    = virtio_sensor_pci_class_init,
};

/* virtio NFC */

static int virtio_nfc_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIONFCPCI *dev = VIRTIO_NFC_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_nfc_pci_class_init(ObjectClass *klass, void *data)
{
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_nfc_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_NFC;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_nfc_pci_instance_init(Object *obj)
{
    VirtIONFCPCI *dev = VIRTIO_NFC_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_NFC);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_nfc_pci_info = {
    .name          = TYPE_VIRTIO_NFC_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIONFCPCI),
    .instance_init = virtio_nfc_pci_instance_init,
    .class_init    = virtio_nfc_pci_class_init,
};

/* virtio-jack-pci */

static int virtio_jack_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOJACKPCI *dev = VIRTIO_JACK_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static Property virtio_jack_pci_properties[] = {
    DEFINE_PROP_STRING(ATTRIBUTE_NAME_JACKS, VirtIOJACKPCI, vdev.jacks),
    DEFINE_VIRTIO_COMMON_FEATURES(VirtIOPCIProxy, host_features),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_jack_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_jack_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_JACK;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
    dc->props = virtio_jack_pci_properties;
}

static void virtio_jack_pci_instance_init(Object *obj)
{
    VirtIOJACKPCI *dev = VIRTIO_JACK_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_JACK);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_jack_pci_info = {
    .name          = TYPE_VIRTIO_JACK_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOJACKPCI),
    .instance_init = virtio_jack_pci_instance_init,
    .class_init    = virtio_jack_pci_class_init,
};

/* virtio-power-pci */

static int virtio_power_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOPOWERPCI *dev = VIRTIO_POWER_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_power_pci_class_init(ObjectClass *klass, void *data)
{
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_power_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_POWER;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_power_pci_instance_init(Object *obj)
{
    VirtIOPOWERPCI *dev = VIRTIO_POWER_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_POWER);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_power_pci_info = {
    .name          = TYPE_VIRTIO_POWER_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOPOWERPCI),
    .instance_init = virtio_power_pci_instance_init,
    .class_init    = virtio_power_pci_class_init,
};

/* virtio-vmodem-pci */

static int virtio_vmodem_pci_init(VirtIOPCIProxy *vpci_dev)
{
    VirtIOVModemPCI *dev = VIRTIO_VMODEM_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    if (qdev_init(vdev) < 0) {
        return -1;
    }
    return 0;
}

static void virtio_vmodem_pci_class_init(ObjectClass *klass, void *data)
{
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->init = virtio_vmodem_pci_init;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_VMODEM;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_vmodem_pci_instance_init(Object *obj)
{
    VirtIOVModemPCI *dev = VIRTIO_VMODEM_PCI(obj);
    object_initialize(&dev->vdev, sizeof(dev->vdev), TYPE_VIRTIO_VMODEM);
    object_property_add_child(obj, "virtio-backend", OBJECT(&dev->vdev), NULL);
}

static TypeInfo virtio_vmodem_pci_info = {
    .name          = TYPE_VIRTIO_VMODEM_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOVModemPCI),
    .instance_init = virtio_vmodem_pci_instance_init,
    .class_init    = virtio_vmodem_pci_class_init,
};

static void maru_virtio_pci_register_types(void)
{
    type_register_static(&virtio_evdi_pci_info);
    type_register_static(&virtio_esm_pci_info);
    type_register_static(&virtio_hwkey_pci_info);
    type_register_static(&virtio_keyboard_pci_info);
    type_register_static(&virtio_touchscreen_pci_info);
    type_register_static(&virtio_sensor_pci_info);
    type_register_static(&virtio_nfc_pci_info);
    type_register_static(&virtio_jack_pci_info);
    type_register_static(&virtio_power_pci_info);
    type_register_static(&virtio_vmodem_pci_info);
}

type_init(maru_virtio_pci_register_types)
