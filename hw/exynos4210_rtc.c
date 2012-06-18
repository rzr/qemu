/*
 * Samsung exynos4210 Real Time Clock
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *  Ogurtsov Oleg <o.ogurtsov@samsung.com>
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

/* Description:
 * Register RTCCON:
 *  CLKSEL Bit[1] not used
 *  CLKOUTEN Bit[9] not used
 */

#include "sysbus.h"
#include "qemu-timer.h"
#include "qemu-common.h"
#include "ptimer.h"

#include "hw.h"
#include "qemu-timer.h"
#include "sysemu.h"

#include "exynos4210.h"

#define DEBUG_RTC 0
/* Get time from host */
#define EXYNOS4210_RTC_GETSYSTIME 1

#if DEBUG_RTC
#define DPRINTF(fmt, ...) \
        do { fprintf(stdout, "RTC: [%24s:%5d] " fmt, __func__, __LINE__, \
                ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

#define     EXYNOS4210_RTC_REG_MEM_SIZE     0x0100

#define     INTP            0x0030
#define     RTCCON          0x0040
#define     TICCNT          0x0044
#define     RTCALM          0x0050
#define     ALMSEC          0x0054
#define     ALMMIN          0x0058
#define     ALMHOUR         0x005C
#define     ALMDAY          0x0060
#define     ALMMON          0x0064
#define     ALMYEAR         0x0068
#define     BCDSEC          0x0070
#define     BDCMIN          0x0074
#define     BCDHOUR         0x0078
#define     BCDDAY          0x007C
#define     BCDDAYWEEK      0x0080
#define     BCDMON          0x0084
#define     BCDYEAR         0x0088
#define     CURTICNT        0x0090

#define     TICK_TIMER_ENABLE   0x0100
#define     TICNT_THRESHHOLD    2


#define     RTC_ENABLE          0x0001

#define     INTP_TICK_ENABLE    0x0001
#define     INTP_ALM_ENABLE     0x0002

#define     ALARM_INT_ENABLE    0x0040

#define     RTC_BASE_FREQ       32768

typedef struct Exynos4210RTCState {
    SysBusDevice busdev;
    MemoryRegion iomem;

    /* registers */
    uint32_t    reg_intp;
    uint32_t    reg_rtccon;
    uint32_t    reg_ticcnt;
    uint32_t    reg_rtcalm;
    uint32_t    reg_almsec;
    uint32_t    reg_almmin;
    uint32_t    reg_almhour;
    uint32_t    reg_almday;
    uint32_t    reg_almmon;
    uint32_t    reg_almyear;
    uint32_t    reg_curticcnt;

    ptimer_state    *ptimer;        /* tick timer */
    ptimer_state    *ptimer_1Hz;    /* clock timer */
    uint32_t        freq;

    qemu_irq        tick_irq;   /* Time Tick Generator irq */
    qemu_irq        alm_irq;    /* alarm irq */

    struct tm   current_tm;     /* current time */
} Exynos4210RTCState;

#define TICCKSEL(value) ((value & (0x0F << 4)) >> 4)

/*** VMState ***/
static const VMStateDescription vmstate_exynos4210_rtc_state = {
    .name = "exynos4210.rtc",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(reg_intp, Exynos4210RTCState),
        VMSTATE_UINT32(reg_rtccon, Exynos4210RTCState),
        VMSTATE_UINT32(reg_ticcnt, Exynos4210RTCState),
        VMSTATE_UINT32(reg_rtcalm, Exynos4210RTCState),
        VMSTATE_UINT32(reg_almsec, Exynos4210RTCState),
        VMSTATE_UINT32(reg_almmin, Exynos4210RTCState),
        VMSTATE_UINT32(reg_almhour, Exynos4210RTCState),
        VMSTATE_UINT32(reg_almday, Exynos4210RTCState),
        VMSTATE_UINT32(reg_almmon, Exynos4210RTCState),
        VMSTATE_UINT32(reg_almyear, Exynos4210RTCState),
        VMSTATE_UINT32(reg_curticcnt, Exynos4210RTCState),
        VMSTATE_PTIMER(ptimer, Exynos4210RTCState),
        VMSTATE_PTIMER(ptimer_1Hz, Exynos4210RTCState),
        VMSTATE_UINT32(freq, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_sec, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_min, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_hour, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_wday, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_mday, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_mon, Exynos4210RTCState),
        VMSTATE_INT32(current_tm.tm_year, Exynos4210RTCState),
        VMSTATE_END_OF_LIST()
    }
};

static inline int rtc_to_bcd(int a, int count)
{
    int ret = (((a % 100) / 10) << 4) | (a % 10);
    if (count == 3) {
        ret |= (((a % 1000) / 100) << 8);
    }
    return ret;
}

static inline int rtc_from_bcd(uint32_t a, int count)
{
    int ret = (((a >> 4) & 0x0f) * 10) + (a & 0x0f);
    if (count == 3) {
        ret += (((a >> 8) & 0x0f) * 100);
    }
    return ret;
}

static void check_alarm_raise(Exynos4210RTCState *s)
{
    unsigned int alarm_raise = 0;
    struct tm stm = s->current_tm;

    if ((s->reg_rtcalm & 0x01) &&
        (rtc_to_bcd(stm.tm_sec, 2) == s->reg_almsec)) {
        alarm_raise = 1;
    }
    if ((s->reg_rtcalm & 0x02) &&
        (rtc_to_bcd(stm.tm_min, 2) == s->reg_almmin)) {
        alarm_raise = 1;
    }
    if ((s->reg_rtcalm & 0x04) &&
        (rtc_to_bcd(stm.tm_hour, 2) == s->reg_almhour)) {
        alarm_raise = 1;
    }
    if ((s->reg_rtcalm & 0x08) &&
        (rtc_to_bcd(stm.tm_mday, 2) == s->reg_almday)) {
        alarm_raise = 1;
    }
    if ((s->reg_rtcalm & 0x10) &&
         (rtc_to_bcd(stm.tm_mon, 2) == s->reg_almmon)) {
        alarm_raise = 1;
    }
    if ((s->reg_rtcalm & 0x20) &&
        (rtc_to_bcd(stm.tm_year, 3) == s->reg_almyear)) {
        alarm_raise = 1;
    }

    if (alarm_raise) {
        DPRINTF("ALARM IRQ\n");
        /* set irq status */
        s->reg_intp |= INTP_ALM_ENABLE;
        qemu_irq_raise(s->alm_irq);
    }
}

/*
 * RTC update frequency
 * Parameters:
 *     reg_value - current RTCCON register or his new value
 */
static void exynos4210_rtc_update_freq(Exynos4210RTCState *s,
                                       uint32_t reg_value)
{
    uint32_t freq;

    freq = s->freq;
    /* set frequncy for time generator */
    s->freq = RTC_BASE_FREQ / (1 << TICCKSEL(reg_value));

    if (freq != s->freq) {
        ptimer_set_freq(s->ptimer, s->freq);
        DPRINTF("freq=%dHz\n", s->freq);
    }
}

/* month is between 0 and 11. */
static int get_days_in_month(int month, int year)
{
    static const int days_tab[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int d;
    if ((unsigned)month >= 12) {
        return 31;
    }
    d = days_tab[month];
    if (month == 1) {
        if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0)) {
            d++;
        }
    }
    return d;
}

/* update 'tm' to the next second */
static void rtc_next_second(struct tm *tm)
{
    int days_in_month;

    tm->tm_sec++;
    if ((unsigned)tm->tm_sec >= 60) {
        tm->tm_sec = 0;
        tm->tm_min++;
        if ((unsigned)tm->tm_min >= 60) {
            tm->tm_min = 0;
            tm->tm_hour++;
            if ((unsigned)tm->tm_hour >= 24) {
                tm->tm_hour = 0;
                /* next day */
                tm->tm_wday++;
                if ((unsigned)tm->tm_wday >= 7) {
                    tm->tm_wday = 0;
                }
                days_in_month = get_days_in_month(tm->tm_mon,
                                                  tm->tm_year + 1900);
                tm->tm_mday++;
                if (tm->tm_mday < 1) {
                    tm->tm_mday = 1;
                } else if (tm->tm_mday > days_in_month) {
                    tm->tm_mday = 1;
                    tm->tm_mon++;
                    if (tm->tm_mon >= 12) {
                        tm->tm_mon = 0;
                        tm->tm_year++;
                    }
                }
            }
        }
    }
}

/*
 * tick handler
 */
static void exynos4210_rtc_tick(void *opaque)
{
    Exynos4210RTCState *s = (Exynos4210RTCState *)opaque;

    DPRINTF("TICK IRQ\n");
    /* set irq status */
    s->reg_intp |= INTP_TICK_ENABLE;
    /* raise IRQ */
    qemu_irq_raise(s->tick_irq);

    /* restart timer */
    ptimer_set_count(s->ptimer, s->reg_ticcnt);
    ptimer_run(s->ptimer, 1);
}

/*
 * 1Hz clock handler
 */
static void exynos4210_rtc_1Hz_tick(void *opaque)
{
    Exynos4210RTCState *s = (Exynos4210RTCState *)opaque;

    rtc_next_second(&s->current_tm);
    /* DPRINTF("1Hz tick\n"); */

    /* raise IRQ */
    if (s->reg_rtcalm & ALARM_INT_ENABLE) {
        check_alarm_raise(s);
    }

    ptimer_set_count(s->ptimer_1Hz, RTC_BASE_FREQ);
    ptimer_run(s->ptimer_1Hz, 1);
}

/*
 * RTC Read
 */
static uint64_t exynos4210_rtc_read(void *opaque, target_phys_addr_t offset,
        unsigned size)
{
    uint32_t value = 0;
    Exynos4210RTCState *s = (Exynos4210RTCState *)opaque;

    switch (offset) {
    case INTP:
        value = s->reg_intp;
        break;
    case RTCCON:
        value = s->reg_rtccon;
        break;
    case TICCNT:
        value = s->reg_ticcnt;
        break;
    case RTCALM:
        value = s->reg_rtcalm;
        break;
    case ALMSEC:
        value = s->reg_almsec;
        break;
    case ALMMIN:
        value = s->reg_almmin;
        break;
    case ALMHOUR:
        value = s->reg_almhour;
        break;
    case ALMDAY:
        value = s->reg_almday;
        break;
    case ALMMON:
        value = s->reg_almmon;
        break;
    case ALMYEAR:
        value = s->reg_almyear;
        break;

    case BCDSEC:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_sec, 2);
        break;
    case BDCMIN:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_min, 2);
        break;
    case BCDHOUR:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_hour, 2);
        break;
    case BCDDAYWEEK:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_wday, 2);
        break;
    case BCDDAY:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_mday, 2);
        break;
    case BCDMON:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_mon + 1, 2);
        break;
    case BCDYEAR:
        value = (uint32_t)rtc_to_bcd(s->current_tm.tm_year, 3);
        break;

    case CURTICNT:
        s->reg_curticcnt = ptimer_get_count(s->ptimer);
        value = s->reg_curticcnt;
        break;

    default:
        fprintf(stderr,
                "[exynos4210.rtc: bad read offset " TARGET_FMT_plx "]\n",
                offset);
        break;
    }
    return value;
}

/*
 * RTC Write
 */
static void exynos4210_rtc_write(void *opaque, target_phys_addr_t offset,
        uint64_t value, unsigned size)
{
    Exynos4210RTCState *s = (Exynos4210RTCState *)opaque;

    switch (offset) {
    case INTP:
        if (value & INTP_ALM_ENABLE) {
            qemu_irq_lower(s->alm_irq);
            s->reg_intp &= (~INTP_ALM_ENABLE);
        }
        if (value & INTP_TICK_ENABLE) {
            qemu_irq_lower(s->tick_irq);
            s->reg_intp &= (~INTP_TICK_ENABLE);
        }
        break;
    case RTCCON:
        if (value & RTC_ENABLE) {
            exynos4210_rtc_update_freq(s, value);
        }
        if ((value & RTC_ENABLE) > (s->reg_rtccon & RTC_ENABLE)) {
            /* clock timer */
            ptimer_set_count(s->ptimer_1Hz, RTC_BASE_FREQ);
            ptimer_run(s->ptimer_1Hz, 1);
            DPRINTF("run clock timer\n");
        }
        if ((value & RTC_ENABLE) < (s->reg_rtccon & RTC_ENABLE)) {
            /* tick timer */
            ptimer_stop(s->ptimer);
            /* clock timer */
            ptimer_stop(s->ptimer_1Hz);
            DPRINTF("stop all timers\n");
        }
        if (value & RTC_ENABLE) {
            if ((value & TICK_TIMER_ENABLE) >
                (s->reg_rtccon & TICK_TIMER_ENABLE) &&
                (s->reg_ticcnt)) {
                ptimer_set_count(s->ptimer, s->reg_ticcnt);
                ptimer_run(s->ptimer, 1);
                DPRINTF("run tick timer\n");
            }
            if ((value & TICK_TIMER_ENABLE) <
                (s->reg_rtccon & TICK_TIMER_ENABLE)) {
                ptimer_stop(s->ptimer);
            }
        }
        s->reg_rtccon = value;
        break;
    case TICCNT:
        if (value > TICNT_THRESHHOLD) {
            s->reg_ticcnt = value;
        } else {
            fprintf(stderr,
                    "[exynos4210.rtc: bad TICNT value %u ]\n",
                    (uint32_t)value);
        }
        break;

    case RTCALM:
        s->reg_rtcalm = value;
        break;
    case ALMSEC:
        s->reg_almsec = (value & 0x7f);
        break;
    case ALMMIN:
        s->reg_almmin = (value & 0x7f);
        break;
    case ALMHOUR:
        s->reg_almhour = (value & 0x3f);
        break;
    case ALMDAY:
        s->reg_almday = (value & 0x3f);
        break;
    case ALMMON:
        s->reg_almmon = (value & 0x1f);
        break;
    case ALMYEAR:
        s->reg_almyear = (value & 0x0fff);
        break;

    case BCDSEC:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_sec = rtc_from_bcd(value, 2);
        }
        break;
    case BDCMIN:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_min = rtc_from_bcd(value, 2);
        }
        break;
    case BCDHOUR:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_hour = rtc_from_bcd(value, 2);
        }
        break;
    case BCDDAYWEEK:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_wday = rtc_from_bcd(value, 2);
        }
        break;
    case BCDDAY:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_mday = rtc_from_bcd(value, 2);
        }
        break;
    case BCDMON:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_mon = rtc_from_bcd(value, 2) - 1;
        }
        break;
    case BCDYEAR:
        if (s->reg_rtccon & RTC_ENABLE) {
            s->current_tm.tm_year = rtc_from_bcd(value, 3);
        }
        break;

    default:
        fprintf(stderr,
                "[exynos4210.rtc: bad write offset " TARGET_FMT_plx "]\n",
                offset);
        break;

    }
}

/*
 * Set default values to timer fields and registers
 */
static void exynos4210_rtc_reset(DeviceState *d)
{
    Exynos4210RTCState *s = (Exynos4210RTCState *)d;

#if EXYNOS4210_RTC_GETSYSTIME
    time_t ct;
#ifdef _WIN32
    struct tm *ptm;
#endif

    time(&ct);

#ifndef _WIN32
    gmtime_r(&ct, &s->current_tm);
#else
    ptm = gmtime(&ct);
    s->current_tm = *ptm;
#endif
    DPRINTF("Get time from host: %d-%d-%d %2d:%02d:%02d\n",
            s->current_tm.tm_year, s->current_tm.tm_mon, s->current_tm.tm_mday,
            s->current_tm.tm_hour, s->current_tm.tm_min, s->current_tm.tm_sec);
#endif

    s->reg_intp = 0;
    s->reg_rtccon = 0;
    s->reg_ticcnt = 0;
    s->reg_rtcalm = 0;
    s->reg_almsec = 0;
    s->reg_almmin = 0;
    s->reg_almhour = 0;
    s->reg_almday = 0;
    s->reg_almmon = 0;
    s->reg_almyear = 0;

    s->reg_curticcnt = 0;

    exynos4210_rtc_update_freq(s, s->reg_rtccon);
    ptimer_stop(s->ptimer);
    ptimer_stop(s->ptimer_1Hz);
}

static const MemoryRegionOps exynos4210_rtc_ops = {
    .read = exynos4210_rtc_read,
    .write = exynos4210_rtc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

/*
 * RTC timer initialization
 */
static int exynos4210_rtc_init(SysBusDevice *dev)
{
    Exynos4210RTCState *s = FROM_SYSBUS(Exynos4210RTCState, dev);
    QEMUBH *bh;

    bh = qemu_bh_new(exynos4210_rtc_tick, s);
    s->ptimer = ptimer_init(bh);
    ptimer_set_freq(s->ptimer, RTC_BASE_FREQ);
    exynos4210_rtc_update_freq(s, 0);

    bh = qemu_bh_new(exynos4210_rtc_1Hz_tick, s);
    s->ptimer_1Hz = ptimer_init(bh);
    ptimer_set_freq(s->ptimer_1Hz, RTC_BASE_FREQ);

    sysbus_init_irq(dev, &s->alm_irq);
    sysbus_init_irq(dev, &s->tick_irq);

    memory_region_init_io(&s->iomem, &exynos4210_rtc_ops, s, "exynos4210-rtc",
            EXYNOS4210_RTC_REG_MEM_SIZE);
    sysbus_init_mmio(dev, &s->iomem);

    return 0;
}

static void exynos4210_rtc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = exynos4210_rtc_init;
    dc->reset = exynos4210_rtc_reset;
    dc->vmsd = &vmstate_exynos4210_rtc_state;
}

static TypeInfo exynos4210_rtc_info = {
    .name          = "exynos4210.rtc",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Exynos4210RTCState),
    .class_init    = exynos4210_rtc_class_init,
};

static void exynos4210_rtc_register_types(void)
{
    type_register_static(&exynos4210_rtc_info);
}

type_init(exynos4210_rtc_register_types)
