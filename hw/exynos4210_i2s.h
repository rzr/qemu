#ifndef QEMU_HW_EXYNOS4210_I2S_H
#define QEMU_HW_EXYNOS4210_I2S_H

#include "qdev.h"

#define EXYNOS4210_I2S_WORD_LEN 4

typedef struct Exynos4210I2SSlave Exynos4210I2SSlave;

#define TYPE_EXYNOS4210_I2S_SLAVE "exynos4210.i2s-slave"
#define EXYNOS4210_I2S_SLAVE(obj) \
     OBJECT_CHECK(Exynos4210I2SSlave, (obj), TYPE_EXYNOS4210_I2S_SLAVE)
#define EXYNOS4210_I2S_SLAVE_CLASS(klass) \
     OBJECT_CLASS_CHECK(Exynos4210I2SSlaveClass, (klass), \
                        TYPE_EXYNOS4210_I2S_SLAVE)
#define EXYNOS4210_I2S_SLAVE_GET_CLASS(obj) \
     OBJECT_GET_CLASS(Exynos4210I2SSlaveClass, (obj), TYPE_EXYNOS4210_I2S_SLAVE)

typedef struct {
    DeviceClass parent_class;

    int (*init)(Exynos4210I2SSlave *s);

    void (*dma_enable)(Exynos4210I2SSlave *s, bool enable);
} Exynos4210I2SSlaveClass;

struct Exynos4210I2SSlave {
    DeviceState qdev;
};

BusState *exynos4210_i2s_bus_new(const char *name,
                                 target_phys_addr_t addr,
                                 qemu_irq irq);

#define EXYNOS4210_I2S_SLAVE_FROM_QDEV(dev) \
    DO_UPCAST(Exynos4210I2SSlave, qdev, dev)
#define FROM_EXYNOS4210_I2S_SLAVE(type, dev) DO_UPCAST(type, i2s, dev)

bool exynos4210_i2s_dma_enabled(BusState *qbus);

uint32_t exynos4210_i2s_dma_get_words_available(BusState *qbus);

void exynos4210_i2s_dma_read(BusState *qbus, void *buf, uint32_t num_words);

void exynos4210_i2s_dma_advance(BusState *qbus, uint32_t num_words);

extern const VMStateDescription vmstate_exynos4210_i2s_slave;

#define VMSTATE_EXYNOS4210_I2S_SLAVE(_field, _state) { \
    .name       = (stringify(_field)), \
    .size       = sizeof(Exynos4210I2SSlave), \
    .vmsd       = &vmstate_exynos4210_i2s_slave, \
    .flags      = VMS_STRUCT, \
    .offset     = vmstate_offset_value(_state, _field, Exynos4210I2SSlave), \
}

#endif
