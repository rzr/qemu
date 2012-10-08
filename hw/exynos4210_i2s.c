/*
 * Samsung exynos4210 I2S driver
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *  Vorobiov Stanislav <s.vorobiov@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "exynos4210_i2s.h"
#include "exec-memory.h"

/* #define DEBUG_I2S */

#ifdef DEBUG_I2S
#define DPRINTF(fmt, ...) \
    do { \
        fprintf(stdout, "I2S: [%s:%d] " fmt, __func__, __LINE__, \
            ## __VA_ARGS__); \
    } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

/* Registers */
#define IISCON       0x00
#define IISMOD       0x04
#define IISFIC       0x08
#define IISPSR       0x0C
#define IISTXD       0x10
#define IISRXD       0x14
#define IISFICS      0x18
#define IISTXDS      0x1C
#define IISAHB       0x20
#define IISSTR0      0x24
#define IISSIZE      0x28
#define IISTRNCNT    0x2C
#define IISLVL0ADDR  0x30
#define IISLVL1ADDR  0x34
#define IISLVL2ADDR  0x38
#define IISLVL3ADDR  0x3C
#define IISSTR1      0x40

#define I2S_REGS_MEM_SIZE 0x44
#define I2S_REGS_NUM (I2S_REGS_MEM_SIZE >> 2)

#define I_(reg) (reg / sizeof(uint32_t))

/* Interface Control Register */
#define IISCON_I2SACTIVE    (1 << 0)
#define IISCON_RXDMACTIVE   (1 << 1)
#define IISCON_TXDMACTIVE   (1 << 2)
#define IISCON_RXCHPAUSE    (1 << 3)
#define IISCON_TXCHPAUSE    (1 << 4)
#define IISCON_RXDMAPAUSE   (1 << 5)
#define IISCON_TXDMAPAUSE   (1 << 6)
#define IISCON_FRXFULL      (1 << 7)
#define IISCON_FTX0FULL     (1 << 8)
#define IISCON_FRXEMPT      (1 << 9)
#define IISCON_FTX0EMPT     (1 << 10)
#define IISCON_LRI          (1 << 11)
#define IISCON_FTX1FULL     (1 << 12)
#define IISCON_FTX2FULL     (1 << 13)
#define IISCON_FTX1EMPT     (1 << 14)
#define IISCON_FTX2EMPT     (1 << 15)
#define IISCON_FTXURINTEN   (1 << 16)
#define IISCON_FTXURSTATUS  (1 << 17)
#define IISCON_TXSDMACTIVE  (1 << 18)
#define IISCON_TXSDMAPAUSE  (1 << 20)
#define IISCON_FTXSFULL     (1 << 21)
#define IISCON_FTXSEMPT     (1 << 22)
#define IISCON_FTXSURINTEN  (1 << 23)
#define IISCON_FTXSURSTATUS (1 << 24)
#define IISCON_FRXOFINTEN   (1 << 25)
#define IISCON_FRXOFSTATUS  (1 << 26)
#define IISCON_SW_RST       (1 << 31)

#define IISCON_WRITE_MASK   (0x8295007F)

/* AHB DMA Control Register */
#define IISAHB_WRITE_MASK     0xFF0000FB
#define IISAHB_LVL3EN         (1 << 27)
#define IISAHB_LVL2EN         (1 << 26)
#define IISAHB_LVL1EN         (1 << 25)
#define IISAHB_LVL0EN         (1 << 24)
#define IISAHB_LVL3INT        (1 << 23)
#define IISAHB_LVL2INT        (1 << 22)
#define IISAHB_LVL1INT        (1 << 21)
#define IISAHB_LVL0INT        (1 << 20)
#define IISAHB_LVL3CLR        (1 << 19)
#define IISAHB_LVL2CLR        (1 << 18)
#define IISAHB_LVL1CLR        (1 << 17)
#define IISAHB_LVL0CLR        (1 << 16)
#define IISAHB_DMA_STRADDRRST (1 << 7)
#define IISAHB_DMA_STRADDRTOG (1 << 6)
#define IISAHB_DMARLD         (1 << 5)
#define IISAHB_INTMASK        (1 << 3)
#define IISAHB_DMAINT         (1 << 2)
#define IISAHB_DMACLR         (1 << 1)
#define IISAHB_DMAEN          (1 << 0)

/* AHB DMA Size Register */
#define IISSIZE_TRNS_SIZE_OFFSET  16
#define IISSIZE_TRNS_SIZE_MASK    0xFFFF

/* AHB DMA Transfer Count Register */
#define IISTRNCNT_OFFSET 0
#define IISTRNCNT_MASK   0xFFFFFF

typedef struct {
    const char *name;
    target_phys_addr_t offset;
    uint32_t reset_value;
} Exynos4210I2SReg;

static Exynos4210I2SReg exynos4210_i2s_regs[I2S_REGS_NUM] = {
    {"IISCON",         IISCON,         0x00000000},
    {"IISMOD",         IISMOD,         0x00000000},
    {"IISFIC",         IISFIC,         0x00000000},
    {"IISPSR",         IISPSR,         0x00000000},
    {"IISTXD",         IISTXD,         0x00000000},
    {"IISRXD",         IISRXD,         0x00000000},
    {"IISFICS",        IISFICS,        0x00000000},
    {"IISTXDS",        IISTXDS,        0x00000000},
    {"IISAHB",         IISAHB,         0x00000000},
    {"IISSTR0",        IISSTR0,        0x00000000},
    {"IISSIZE",        IISSIZE,        0x7FFF0000},
    {"IISTRNCNT",      IISTRNCNT,      0x00000000},
    {"IISLVL0ADDR",    IISLVL0ADDR,    0x00000000},
    {"IISLVL1ADDR",    IISLVL1ADDR,    0x00000000},
    {"IISLVL2ADDR",    IISLVL2ADDR,    0x00000000},
    {"IISLVL3ADDR",    IISLVL3ADDR,    0x00000000},
    {"IISSTR1",        IISSTR1,        0x00000000},
};

static struct {
    uint32_t ahb_en_bit;
    uint32_t ahb_int_bit;
    uint32_t ahb_clr_bit;
    int reg_offset;
} lvl_regs[] = {
    { IISAHB_LVL3EN, IISAHB_LVL3INT, IISAHB_LVL3CLR, IISLVL3ADDR },
    { IISAHB_LVL2EN, IISAHB_LVL2INT, IISAHB_LVL2CLR, IISLVL2ADDR },
    { IISAHB_LVL1EN, IISAHB_LVL1INT, IISAHB_LVL1CLR, IISLVL1ADDR },
    { IISAHB_LVL0EN, IISAHB_LVL0INT, IISAHB_LVL0CLR, IISLVL0ADDR },
};

#define TYPE_EXYNOS4210_I2S_BUS "exynos4210-i2s"

typedef struct {
    BusState qbus;

    MemoryRegion iomem;
    qemu_irq irq;
    uint32_t reg[I2S_REGS_NUM];
} Exynos4210I2SBus;

static Exynos4210I2SSlave *get_slave(Exynos4210I2SBus *bus)
{
    BusChild *kid = QTAILQ_FIRST(&bus->qbus.children);
    DeviceState *dev;

    if (kid) {
        dev = kid->child;
        if (dev) {
            return EXYNOS4210_I2S_SLAVE_FROM_QDEV(dev);
        }
    }

    return NULL;
}

static void reset_internal(BusState *qbus, bool reset_slave)
{
    Exynos4210I2SBus *bus = DO_UPCAST(Exynos4210I2SBus, qbus, qbus);
    Exynos4210I2SSlave *s;
    int i;

    DPRINTF("enter\n");

    qemu_irq_lower(bus->irq);

    for (i = 0; i < ARRAY_SIZE(exynos4210_i2s_regs); i++) {
        bus->reg[i] = exynos4210_i2s_regs[i].reset_value;
    }

    if (reset_slave) {
        s = get_slave(bus);

        if (s != NULL) {
            device_reset(&s->qdev);
        }
    }
}

static uint32_t get_dma_size_words(Exynos4210I2SBus *bus)
{
    return (bus->reg[I_(IISSIZE)] >> IISSIZE_TRNS_SIZE_OFFSET) &
            IISSIZE_TRNS_SIZE_MASK;
}

static uint32_t get_dma_trncnt_words(Exynos4210I2SBus *bus)
{
    return (bus->reg[I_(IISTRNCNT)] >> IISTRNCNT_OFFSET) &
            IISTRNCNT_MASK;
}

static void set_dma_trncnt_words(Exynos4210I2SBus *bus, uint32_t trncnt_words)
{
    bus->reg[I_(IISTRNCNT)] &= ~(IISTRNCNT_MASK << IISTRNCNT_OFFSET);
    bus->reg[I_(IISTRNCNT)] |=
        ((trncnt_words & IISTRNCNT_MASK) << IISTRNCNT_OFFSET);
}

static int exynos4210_i2s_bus_reset(BusState *qbus);

static void exynos4210_i2s_bus_class_init(ObjectClass *klass, void *data)
{
    BusClass *k = BUS_CLASS(klass);

    k->reset    = exynos4210_i2s_bus_reset;
}

static struct TypeInfo exynos4210_i2s_bus_info = {
    .name          = TYPE_EXYNOS4210_I2S_BUS,
    .parent        = TYPE_BUS,
    .instance_size = sizeof(Exynos4210I2SBus),
    .class_init    = exynos4210_i2s_bus_class_init,
};

static const VMStateDescription vmstate_exynos4210_i2s_bus = {
    .name = "exynos4210.i2s_bus",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(reg, Exynos4210I2SBus,
                                  I2S_REGS_NUM),
        VMSTATE_END_OF_LIST()
    }
};

#ifdef DEBUG_I2S
static const char *exynos4210_i2s_regname(Exynos4210I2SBus *bus,
                                          target_phys_addr_t offset)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(exynos4210_i2s_regs); i++) {
        if (offset == exynos4210_i2s_regs[i].offset) {
            return exynos4210_i2s_regs[i].name;
        }
    }

    return NULL;
}
#endif

static void exynos4210_i2s_bus_write(void *opaque,
                                     target_phys_addr_t offset,
                                     uint64_t value,
                                     unsigned size)
{
    Exynos4210I2SBus *bus = opaque;
    Exynos4210I2SSlave *s;
    Exynos4210I2SSlaveClass *sc;
    int i;

    s = get_slave(bus);

    assert(s != NULL);

    if (s == NULL) {
        hw_error("Exynos I2S cannot operate without a slave\n");
    }

    sc = EXYNOS4210_I2S_SLAVE_GET_CLASS(s);

    switch (offset) {
    case IISCON:
        if (!(value & IISCON_SW_RST) &&
             (bus->reg[I_(IISCON)] & IISCON_SW_RST)) {
            reset_internal(&bus->qbus, 1);
        }

        bus->reg[I_(offset)] = (bus->reg[I_(offset)] & ~IISCON_WRITE_MASK) |
                               (value & IISCON_WRITE_MASK);

        DPRINTF("0x%.8X -> %s\n",
                bus->reg[I_(offset)],
                exynos4210_i2s_regname(bus, offset));

        break;
    case IISAHB:
        if (((value & IISAHB_DMAEN) != 0) && ((value & IISAHB_DMARLD) == 0)) {
            hw_error("Non auto-reload DMA is not supported\n");
        }

        if (((value & IISAHB_DMAEN) != 0) && ((value & IISAHB_INTMASK) == 0)) {
            hw_error("Non buffer level DMA interrupt is not supported\n");
        }

        if (((value & IISAHB_DMAEN) != 0) &&
            ((value & IISAHB_DMA_STRADDRTOG) != 0)) {
            hw_error("DMA start address toggle is not supported\n");
        }

        for (i = 0; i < ARRAY_SIZE(lvl_regs); ++i) {
            if ((value & lvl_regs[i].ahb_clr_bit) &&
                (bus->reg[I_(IISAHB)] & lvl_regs[i].ahb_int_bit)) {
                qemu_irq_lower(bus->irq);
                bus->reg[I_(IISAHB)] &= ~lvl_regs[i].ahb_int_bit;
            }
        }

        if ((value & IISAHB_DMACLR) &&
            (bus->reg[I_(IISAHB)] & IISAHB_DMAINT)) {
            qemu_irq_lower(bus->irq);
            bus->reg[I_(IISAHB)] &= ~IISAHB_DMAINT;
        }

        if ((value & IISAHB_DMAEN) !=
            (bus->reg[I_(IISAHB)] & IISAHB_DMAEN)) {
            if ((value & IISAHB_DMAEN) == 0) {
                qemu_irq_lower(bus->irq);
            }
            if (sc->dma_enable) {
                sc->dma_enable(s, ((value & IISAHB_DMAEN) != 0));
            }
        }

        bus->reg[I_(offset)] = (bus->reg[I_(offset)] & ~IISAHB_WRITE_MASK) |
                               (value & IISAHB_WRITE_MASK);

        DPRINTF("0x%.8X -> %s\n",
                bus->reg[I_(offset)],
                exynos4210_i2s_regname(bus, offset));

        break;
    case IISTRNCNT:
        hw_error("Cannot write IISTRNCNT\n");
        break;
    case IISSIZE:
        if (value == 0) {
            hw_error("IISSIZE cannot be 0\n");
        }

        bus->reg[I_(offset)] = value;

        if (get_dma_size_words(bus) <= get_dma_trncnt_words(bus)) {
            set_dma_trncnt_words(bus, 0);
        }

        DPRINTF("0x%.8X -> %s\n",
                bus->reg[I_(offset)],
                exynos4210_i2s_regname(bus, offset));

        break;
    case IISLVL0ADDR:
    case IISLVL1ADDR:
    case IISLVL2ADDR:
    case IISLVL3ADDR:
    case IISTXD:
    case IISMOD:
    case IISFIC:
    case IISPSR:
    case IISTXDS:
    case IISFICS:
    case IISSTR0:
    case IISSTR1:
        bus->reg[I_(offset)] = value;

        DPRINTF("0x%.8X -> %s\n",
                bus->reg[I_(offset)],
                exynos4210_i2s_regname(bus, offset));

        break;
    default:
        hw_error("Bad offset: 0x%X\n", (int)offset);
    }
}

static uint64_t exynos4210_i2s_bus_read(void *opaque,
                                        target_phys_addr_t offset,
                                        unsigned size)
{
    Exynos4210I2SBus *bus = opaque;

    if (offset > (I2S_REGS_MEM_SIZE - sizeof(uint32_t))) {
        hw_error("Bad offset: 0x%X\n", (int)offset);
        return 0;
    }

    DPRINTF("%s -> 0x%.8X\n",
            exynos4210_i2s_regname(bus, offset),
            bus->reg[I_(offset)]);

    return bus->reg[I_(offset)];
}

static int exynos4210_i2s_bus_reset(BusState *qbus)
{
    reset_internal(qbus, 0);

    return 0;
}

static const MemoryRegionOps exynos4210_i2s_bus_ops = {
    .read = exynos4210_i2s_bus_read,
    .write = exynos4210_i2s_bus_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .max_access_size = 4,
        .unaligned = false
    },
};

BusState *exynos4210_i2s_bus_new(const char *name,
                                 target_phys_addr_t addr,
                                 qemu_irq irq)
{
    Exynos4210I2SBus *bus;

    bus = FROM_QBUS(Exynos4210I2SBus,
                    qbus_create(TYPE_EXYNOS4210_I2S_BUS, NULL, name));
    vmstate_register(NULL, -1, &vmstate_exynos4210_i2s_bus, bus);

    memory_region_init_io(&bus->iomem,
                          &exynos4210_i2s_bus_ops,
                          bus,
                          "exynos4210.i2s",
                          I2S_REGS_MEM_SIZE);

    memory_region_add_subregion(get_system_memory(),
                                addr,
                                &bus->iomem);

    bus->irq = irq;

    return &bus->qbus;
}

bool exynos4210_i2s_dma_enabled(BusState *qbus)
{
    Exynos4210I2SBus *bus = DO_UPCAST(Exynos4210I2SBus, qbus, qbus);

    return ((bus->reg[I_(IISAHB)] & IISAHB_DMAEN) != 0);
}

uint32_t exynos4210_i2s_dma_get_words_available(BusState *qbus)
{
    Exynos4210I2SBus *bus = DO_UPCAST(Exynos4210I2SBus, qbus, qbus);

    return get_dma_size_words(bus);
}

void exynos4210_i2s_dma_read(BusState *qbus, void *buf, uint32_t num_words)
{
    Exynos4210I2SBus *bus = DO_UPCAST(Exynos4210I2SBus, qbus, qbus);
    target_phys_addr_t addr;
    uint32_t size_words, trncnt_words;

    assert(num_words <= exynos4210_i2s_dma_get_words_available(qbus));

    if (num_words > exynos4210_i2s_dma_get_words_available(qbus)) {
        hw_error("Bad DMA read length: %d\n", num_words);
    }

    size_words = get_dma_size_words(bus);
    addr = bus->reg[I_(IISSTR0)];
    trncnt_words = get_dma_trncnt_words(bus);

    assert(trncnt_words < size_words);

    if (num_words > (size_words - trncnt_words)) {
        cpu_physical_memory_read(addr +
                                 (trncnt_words * EXYNOS4210_I2S_WORD_LEN),
            buf,
            (size_words - trncnt_words) * EXYNOS4210_I2S_WORD_LEN);
        num_words -= (size_words - trncnt_words);
        buf += (size_words - trncnt_words) * EXYNOS4210_I2S_WORD_LEN;
        trncnt_words = 0;
    }

    cpu_physical_memory_read(addr + (trncnt_words * EXYNOS4210_I2S_WORD_LEN),
                             buf,
                             num_words * EXYNOS4210_I2S_WORD_LEN);
}

void exynos4210_i2s_dma_advance(BusState *qbus, uint32_t num_words)
{
    Exynos4210I2SBus *bus = DO_UPCAST(Exynos4210I2SBus, qbus, qbus);
    uint32_t size_words, trncnt_words;
    int i;

    assert(num_words <= exynos4210_i2s_dma_get_words_available(qbus));

    if (num_words > exynos4210_i2s_dma_get_words_available(qbus)) {
        hw_error("Bad DMA read length: %d\n", num_words);
    }

    size_words = get_dma_size_words(bus);
    trncnt_words = get_dma_trncnt_words(bus);

    for (i = 0; i < ARRAY_SIZE(lvl_regs); ++i) {
        target_phys_addr_t dma_offset;
        uint32_t tmp_num_words = num_words, tmp_trncnt_words = trncnt_words;

        if ((bus->reg[I_(IISAHB)] & lvl_regs[i].ahb_en_bit) == 0) {
            continue;
        }

        if (bus->reg[I_(lvl_regs[i].reg_offset)] < bus->reg[I_(IISSTR0)]) {
            continue;
        }

        dma_offset = bus->reg[I_(lvl_regs[i].reg_offset)] -
                     bus->reg[I_(IISSTR0)];

        if (tmp_num_words > (size_words - tmp_trncnt_words)) {
            if ((dma_offset >= (tmp_trncnt_words * EXYNOS4210_I2S_WORD_LEN)) &&
                (dma_offset < size_words * EXYNOS4210_I2S_WORD_LEN)) {
                bus->reg[I_(IISAHB)] |= lvl_regs[i].ahb_int_bit;
                qemu_irq_raise(bus->irq);
                break;
            }

            tmp_num_words -= (size_words - tmp_trncnt_words);
            tmp_trncnt_words = 0;
        }

        if ((dma_offset >= (tmp_trncnt_words * EXYNOS4210_I2S_WORD_LEN)) &&
            (dma_offset <
                (tmp_trncnt_words + tmp_num_words) * EXYNOS4210_I2S_WORD_LEN)) {
            bus->reg[I_(IISAHB)] |= lvl_regs[i].ahb_int_bit;
            qemu_irq_raise(bus->irq);
        }
    }

    set_dma_trncnt_words(bus, (trncnt_words + num_words) % size_words);
}

const VMStateDescription vmstate_exynos4210_i2s_slave = {
    .name = "Exynos4210I2SSlave",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1
};

static int exynos4210_i2s_slave_qdev_init(DeviceState *dev)
{
    Exynos4210I2SSlave *s = EXYNOS4210_I2S_SLAVE_FROM_QDEV(dev);
    Exynos4210I2SSlaveClass *sc = EXYNOS4210_I2S_SLAVE_GET_CLASS(s);

    return sc->init ? sc->init(s) : 0;
}

static void exynos4210_i2s_slave_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *k = DEVICE_CLASS(klass);
    k->init = exynos4210_i2s_slave_qdev_init;
    k->bus_type = TYPE_EXYNOS4210_I2S_BUS;
}

static TypeInfo exynos4210_i2s_slave_type_info = {
    .name = TYPE_EXYNOS4210_I2S_SLAVE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(Exynos4210I2SSlave),
    .abstract = true,
    .class_size = sizeof(Exynos4210I2SSlaveClass),
    .class_init = exynos4210_i2s_slave_class_init,
};

static void exynos4210_i2s_slave_register_types(void)
{
    type_register_static(&exynos4210_i2s_bus_info);
    type_register_static(&exynos4210_i2s_slave_type_info);
}

type_init(exynos4210_i2s_slave_register_types)
