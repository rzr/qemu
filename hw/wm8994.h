#ifndef QEMU_HW_WM8994_H
#define QEMU_HW_WM8994_H

#include "qdev.h"

void wm8994_set_active(DeviceState *dev, bool active);

void wm8994_data_req_set(DeviceState *dev,
                         void (*data_req)(void*, int),
                         void *opaque);

int wm8994_dac_write(DeviceState *dev, void *buf, int num_bytes);

#endif
