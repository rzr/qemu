/*
 * Samsung exynos4210 wm8994 driver
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

#include "wm8994.h"
#include "hw.h"
#include "i2c.h"
#include "audio/audio.h"

/* #define DEBUG_WM8994 */

#ifdef DEBUG_WM8994
#define DPRINTF(fmt, ...) \
    do { \
        fprintf(stdout, "WM8994: [%s:%d] " fmt, __func__, __LINE__, \
            ## __VA_ARGS__); \
    } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

/* Registers */
#define WM8994_SOFTWARE_RESET                   0x00
#define WM8994_POWER_MANAGEMENT_2               0x02
#define WM8994_SPEAKER_VOLUME_LEFT              0x26 /* Speaker */
#define WM8994_SPEAKER_VOLUME_RIGHT             0x27
#define WM8994_CHIP_REVISION                    0x100
#define WM8994_AIF1_RATE                        0x210
#define WM8994_AIF1_CONTROL_1                   0x300
#define WM8994_GPIO_1                           0x700
#define WM8994_GPIO_3                           0x702
#define WM8994_GPIO_4                           0x703
#define WM8994_GPIO_5                           0x704
#define WM8994_GPIO_6                           0x705
#define WM8994_GPIO_7                           0x706
#define WM8994_GPIO_8                           0x707
#define WM8994_GPIO_9                           0x708
#define WM8994_GPIO_10                          0x709
#define WM8994_GPIO_11                          0x70A
#define WM8994_PULL_CONTROL_2                   0x721
#define WM8994_INPUT_MIXER_1                    0x15
#define WM8994_LEFT_LINE_INPUT_1_2_VOLUME       0x18
#define WM8994_LEFT_LINE_INPUT_3_4_VOLUME       0x19
#define WM8994_RIGHT_LINE_INPUT_1_2_VOLUME      0x1A
#define WM8994_RIGHT_LINE_INPUT_3_4_VOLUME      0x1B
#define WM8994_LEFT_OUTPUT_VOLUME               0x1C
#define WM8994_RIGHT_OUTPUT_VOLUME              0x1D
#define WM8994_LEFT_OPGA_VOLUME                 0x20
#define WM8994_RIGHT_OPGA_VOLUME                0x21
#define WM8994_LINE_MIXER_1                     0x34
#define WM8994_LINE_MIXER_2                     0x35
#define WM8994_ANTIPOP_1                        0x38
#define WM8994_LDO_1                            0x3B
#define WM8994_LDO_2                            0x3C
#define WM8994_AIF1_ADC1_LEFT_VOLUME            0x400
#define WM8994_AIF1_ADC1_RIGHT_VOLUME           0x401
#define WM8994_AIF1_DAC1_LEFT_VOLUME            0x402
#define WM8994_AIF1_DAC1_RIGHT_VOLUME           0x403
#define WM8994_AIF1_ADC2_LEFT_VOLUME            0x404
#define WM8994_AIF1_ADC2_RIGHT_VOLUME           0x405
#define WM8994_AIF1_DAC2_LEFT_VOLUME            0x406
#define WM8994_AIF1_DAC2_RIGHT_VOLUME           0x407
#define WM8994_AIF1_DAC1_FILTERS_2              0x421
#define WM8994_AIF1_DAC2_FILTERS_2              0x423
#define WM8994_AIF2_ADC_LEFT_VOLUME             0x500
#define WM8994_AIF2_ADC_RIGHT_VOLUME            0x501
#define WM8994_AIF2_DAC_LEFT_VOLUME             0x502
#define WM8994_AIF2_DAC_RIGHT_VOLUME            0x503
#define WM8994_AIF2_DAC_FILTERS_2               0x521
#define WM8994_DAC1_LEFT_VOLUME                 0x610
#define WM8994_DAC1_RIGHT_VOLUME                0x611
#define WM8994_DAC2_LEFT_VOLUME                 0x612
#define WM8994_DAC2_RIGHT_VOLUME                0x613
#define WM8994_POWER_MANAGEMENT_1               0x01
#define WM8994_POWER_MANAGEMENT_3               0x03
#define WM8994_ANTIPOP_2                        0x39
#define WM8994_AIF1_CLOCKING_1                  0x200
#define WM8994_FLL1_CONTROL_1                   0x220
#define WM8994_FLL1_CONTROL_2                   0x221
#define WM8994_FLL1_CONTROL_3                   0x222
#define WM8994_FLL1_CONTROL_4                   0x223
#define WM8994_FLL1_CONTROL_5                   0x224
#define WM8994_AIF1_MASTER_SLAVE                0x302
#define WM8994_AIF1_BCLK                        0x303
#define WM8994_AIF1DAC_LRCLK                    0x305
#define WM8994_AIF1_DAC1_FILTERS_1              0x420

#define WM8994_REGS_NUM                         0x74A

/*
 * R528 (0x210) - AIF1 Rate
 */
#define WM8994_AIF1_SR_MASK                  0x00F0  /* AIF1_SR - [7:4] */
#define WM8994_AIF1_SR_SHIFT                      4  /* AIF1_SR - [7:4] */
#define WM8994_AIF1_SR_WIDTH                      4  /* AIF1_SR - [7:4] */
#define WM8994_AIF1CLK_RATE_MASK             0x000F  /* AIF1CLK_RATE - [3:0] */
#define WM8994_AIF1CLK_RATE_SHIFT                 0  /* AIF1CLK_RATE - [3:0] */
#define WM8994_AIF1CLK_RATE_WIDTH                 4  /* AIF1CLK_RATE - [3:0] */

/*
 * R768 (0x300) - AIF1 Control (1)
 */
#define WM8994_AIF1ADCL_SRC                     0x8000  /* AIF1ADCL_SRC */
#define WM8994_AIF1ADCL_SRC_MASK                0x8000  /* AIF1ADCL_SRC */
#define WM8994_AIF1ADCL_SRC_SHIFT                   15  /* AIF1ADCL_SRC */
#define WM8994_AIF1ADCL_SRC_WIDTH                    1  /* AIF1ADCL_SRC */
#define WM8994_AIF1ADCR_SRC                     0x4000  /* AIF1ADCR_SRC */
#define WM8994_AIF1ADCR_SRC_MASK                0x4000  /* AIF1ADCR_SRC */
#define WM8994_AIF1ADCR_SRC_SHIFT                   14  /* AIF1ADCR_SRC */
#define WM8994_AIF1ADCR_SRC_WIDTH                    1  /* AIF1ADCR_SRC */
#define WM8994_AIF1ADC_TDM                      0x2000  /* AIF1ADC_TDM */
#define WM8994_AIF1ADC_TDM_MASK                 0x2000  /* AIF1ADC_TDM */
#define WM8994_AIF1ADC_TDM_SHIFT                    13  /* AIF1ADC_TDM */
#define WM8994_AIF1ADC_TDM_WIDTH                     1  /* AIF1ADC_TDM */
#define WM8994_AIF1_BCLK_INV                    0x0100  /* AIF1_BCLK_INV */
#define WM8994_AIF1_BCLK_INV_MASK               0x0100  /* AIF1_BCLK_INV */
#define WM8994_AIF1_BCLK_INV_SHIFT                   8  /* AIF1_BCLK_INV */
#define WM8994_AIF1_BCLK_INV_WIDTH                   1  /* AIF1_BCLK_INV */
#define WM8994_AIF1_LRCLK_INV                   0x0080  /* AIF1_LRCLK_INV */
#define WM8994_AIF1_LRCLK_INV_MASK              0x0080  /* AIF1_LRCLK_INV */
#define WM8994_AIF1_LRCLK_INV_SHIFT                  7  /* AIF1_LRCLK_INV */
#define WM8994_AIF1_LRCLK_INV_WIDTH                  1  /* AIF1_LRCLK_INV */
#define WM8994_AIF1_WL_MASK                     0x0060  /* AIF1_WL - [6:5] */
#define WM8994_AIF1_WL_SHIFT                         5  /* AIF1_WL - [6:5] */
#define WM8994_AIF1_WL_WIDTH                         2  /* AIF1_WL - [6:5] */
#define WM8994_AIF1_FMT_MASK                    0x0018  /* AIF1_FMT - [4:3] */
#define WM8994_AIF1_FMT_SHIFT                        3  /* AIF1_FMT - [4:3] */
#define WM8994_AIF1_FMT_WIDTH                        2  /* AIF1_FMT - [4:3] */

/*
 * R1056 (0x420) - AIF1 DAC1 Filters (1)
 */
#define WM8994_AIF1DAC1_MUTE                0x0200  /* AIF1DAC1_MUTE */
#define WM8994_AIF1DAC1_MUTE_MASK           0x0200  /* AIF1DAC1_MUTE */
#define WM8994_AIF1DAC1_MUTE_SHIFT               9  /* AIF1DAC1_MUTE */
#define WM8994_AIF1DAC1_MUTE_WIDTH               1  /* AIF1DAC1_MUTE */
#define WM8994_AIF1DAC1_MONO                0x0080  /* AIF1DAC1_MONO */
#define WM8994_AIF1DAC1_MONO_MASK           0x0080  /* AIF1DAC1_MONO */
#define WM8994_AIF1DAC1_MONO_SHIFT               7  /* AIF1DAC1_MONO */
#define WM8994_AIF1DAC1_MONO_WIDTH               1  /* AIF1DAC1_MONO */
#define WM8994_AIF1DAC1_MUTERATE            0x0020  /* AIF1DAC1_MUTERATE */
#define WM8994_AIF1DAC1_MUTERATE_MASK       0x0020  /* AIF1DAC1_MUTERATE */
#define WM8994_AIF1DAC1_MUTERATE_SHIFT           5  /* AIF1DAC1_MUTERATE */
#define WM8994_AIF1DAC1_MUTERATE_WIDTH           1  /* AIF1DAC1_MUTERATE */
#define WM8994_AIF1DAC1_UNMUTE_RAMP         0x0010  /* AIF1DAC1_UNMUTE_RAMP */
#define WM8994_AIF1DAC1_UNMUTE_RAMP_MASK    0x0010  /* AIF1DAC1_UNMUTE_RAMP */
#define WM8994_AIF1DAC1_UNMUTE_RAMP_SHIFT        4  /* AIF1DAC1_UNMUTE_RAMP */
#define WM8994_AIF1DAC1_UNMUTE_RAMP_WIDTH        1  /* AIF1DAC1_UNMUTE_RAMP */
#define WM8994_AIF1DAC1_DEEMP_MASK          0x0006  /* AIF1DAC1_DEEMP - [2:1] */
#define WM8994_AIF1DAC1_DEEMP_SHIFT              1  /* AIF1DAC1_DEEMP - [2:1] */
#define WM8994_AIF1DAC1_DEEMP_WIDTH              2  /* AIF1DAC1_DEEMP - [2:1] */

/*
 * R38 (0x26), R39 (0x27) - Speaker Volume
 */
#define WM8994_SPKOUT_VU                       0x0100  /* SPKOUT_VU */
#define WM8994_SPKOUT_VU_MASK                  0x0100  /* SPKOUT_VU */
#define WM8994_SPKOUT_VU_SHIFT                      8  /* SPKOUT_VU */
#define WM8994_SPKOUT_VU_WIDTH                      1  /* SPKOUT_VU */
#define WM8994_SPKOUTL_ZC                      0x0080  /* SPKOUTL_ZC */
#define WM8994_SPKOUTL_ZC_MASK                 0x0080  /* SPKOUTL_ZC */
#define WM8994_SPKOUTL_ZC_SHIFT                     7  /* SPKOUTL_ZC */
#define WM8994_SPKOUTL_ZC_WIDTH                     1  /* SPKOUTL_ZC */
#define WM8994_SPKOUTL_MUTE_N                  0x0040  /* SPKOUTL_MUTE_N */
#define WM8994_SPKOUTL_MUTE_N_MASK             0x0040  /* SPKOUTL_MUTE_N */
#define WM8994_SPKOUTL_MUTE_N_SHIFT                 6  /* SPKOUTL_MUTE_N */
#define WM8994_SPKOUTL_MUTE_N_WIDTH                 1  /* SPKOUTL_MUTE_N */
#define WM8994_SPKOUTL_VOL_MASK                0x003F  /* SPKOUTL_VOL - [5:0] */
#define WM8994_SPKOUTL_VOL_SHIFT                    0  /* SPKOUTL_VOL - [5:0] */
#define WM8994_SPKOUTL_VOL_WIDTH                    6  /* SPKOUTL_VOL - [5:0] */

#define CODEC "wm8994"

typedef struct {
    const char *name;
    uint16_t address;
} WM8994Reg;

#ifdef DEBUG_WM8994
static WM8994Reg wm8994_regs[] = {
    {"SOFTWARE_RESET",               WM8994_SOFTWARE_RESET },
    {"POWER_MANAGEMENT_2",           WM8994_POWER_MANAGEMENT_2 },
    {"SPEAKER_VOLUME_LEFT",          WM8994_SPEAKER_VOLUME_LEFT },
    {"SPEAKER_VOLUME_RIGHT",         WM8994_SPEAKER_VOLUME_RIGHT },
    {"CHIP_REVISION",                WM8994_CHIP_REVISION },
    {"AIF1_RATE",                    WM8994_AIF1_RATE },
    {"AIF1_CONTROL_1",               WM8994_AIF1_CONTROL_1 },
    {"GPIO_1",                       WM8994_GPIO_1 },
    {"GPIO_3",                       WM8994_GPIO_3 },
    {"GPIO_4",                       WM8994_GPIO_4 },
    {"GPIO_5",                       WM8994_GPIO_5 },
    {"GPIO_6",                       WM8994_GPIO_6 },
    {"GPIO_7",                       WM8994_GPIO_7 },
    {"GPIO_8",                       WM8994_GPIO_8 },
    {"GPIO_9",                       WM8994_GPIO_9 },
    {"GPIO_10",                      WM8994_GPIO_10 },
    {"GPIO_11",                      WM8994_GPIO_11 },
    {"PULL_CONTROL_2",               WM8994_PULL_CONTROL_2 },
    {"INPUT_MIXER_1",                WM8994_INPUT_MIXER_1 },
    {"LEFT_LINE_INPUT_1_2_VOLUME",   WM8994_LEFT_LINE_INPUT_1_2_VOLUME },
    {"LEFT_LINE_INPUT_3_4_VOLUME",   WM8994_LEFT_LINE_INPUT_3_4_VOLUME },
    {"RIGHT_LINE_INPUT_1_2_VOLUME",  WM8994_RIGHT_LINE_INPUT_1_2_VOLUME },
    {"RIGHT_LINE_INPUT_3_4_VOLUME",  WM8994_RIGHT_LINE_INPUT_3_4_VOLUME },
    {"LEFT_OUTPUT_VOLUME",           WM8994_LEFT_OUTPUT_VOLUME },
    {"RIGHT_OUTPUT_VOLUME",          WM8994_RIGHT_OUTPUT_VOLUME },
    {"LEFT_OPGA_VOLUME",             WM8994_LEFT_OPGA_VOLUME },
    {"RIGHT_OPGA_VOLUME",            WM8994_RIGHT_OPGA_VOLUME },
    {"LINE_MIXER_1",                 WM8994_LINE_MIXER_1 },
    {"LINE_MIXER_2",                 WM8994_LINE_MIXER_2 },
    {"ANTIPOP_1",                    WM8994_ANTIPOP_1 },
    {"LDO_1",                        WM8994_LDO_1 },
    {"LDO_2",                        WM8994_LDO_2 },
    {"AIF1_ADC1_LEFT_VOLUME",        WM8994_AIF1_ADC1_LEFT_VOLUME },
    {"AIF1_ADC1_RIGHT_VOLUME",       WM8994_AIF1_ADC1_RIGHT_VOLUME },
    {"AIF1_DAC1_LEFT_VOLUME",        WM8994_AIF1_DAC1_LEFT_VOLUME },
    {"AIF1_DAC1_RIGHT_VOLUME",       WM8994_AIF1_DAC1_RIGHT_VOLUME },
    {"AIF1_ADC2_LEFT_VOLUME",        WM8994_AIF1_ADC2_LEFT_VOLUME },
    {"AIF1_ADC2_RIGHT_VOLUME",       WM8994_AIF1_ADC2_RIGHT_VOLUME },
    {"AIF1_DAC2_LEFT_VOLUME",        WM8994_AIF1_DAC2_LEFT_VOLUME },
    {"AIF1_DAC2_RIGHT_VOLUME",       WM8994_AIF1_DAC2_RIGHT_VOLUME },
    {"AIF1_DAC1_FILTERS_2",          WM8994_AIF1_DAC1_FILTERS_2 },
    {"AIF1_DAC2_FILTERS_2",          WM8994_AIF1_DAC2_FILTERS_2 },
    {"AIF2_ADC_LEFT_VOLUME",         WM8994_AIF2_ADC_LEFT_VOLUME },
    {"AIF2_ADC_RIGHT_VOLUME",        WM8994_AIF2_ADC_RIGHT_VOLUME },
    {"AIF2_DAC_LEFT_VOLUME",         WM8994_AIF2_DAC_LEFT_VOLUME },
    {"AIF2_DAC_RIGHT_VOLUME",        WM8994_AIF2_DAC_RIGHT_VOLUME },
    {"AIF2_DAC_FILTERS_2",           WM8994_AIF2_DAC_FILTERS_2 },
    {"DAC1_LEFT_VOLUME",             WM8994_DAC1_LEFT_VOLUME },
    {"DAC1_RIGHT_VOLUME",            WM8994_DAC1_RIGHT_VOLUME },
    {"DAC2_LEFT_VOLUME",             WM8994_DAC2_LEFT_VOLUME },
    {"DAC2_RIGHT_VOLUME",            WM8994_DAC2_RIGHT_VOLUME },
    {"POWER_MANAGEMENT_1",           WM8994_POWER_MANAGEMENT_1 },
    {"POWER_MANAGEMENT_3",           WM8994_POWER_MANAGEMENT_3 },
    {"ANTIPOP_2",                    WM8994_ANTIPOP_2 },
    {"AIF1_CLOCKING_1",              WM8994_AIF1_CLOCKING_1 },
    {"FLL1_CONTROL_1",               WM8994_FLL1_CONTROL_1 },
    {"FLL1_CONTROL_2",               WM8994_FLL1_CONTROL_2 },
    {"FLL1_CONTROL_3",               WM8994_FLL1_CONTROL_3 },
    {"FLL1_CONTROL_4",               WM8994_FLL1_CONTROL_4 },
    {"FLL1_CONTROL_5",               WM8994_FLL1_CONTROL_5 },
    {"AIF1_MASTER_SLAVE",            WM8994_AIF1_MASTER_SLAVE },
    {"AIF1_BCLK",                    WM8994_AIF1_BCLK },
    {"AIF1DAC_LRCLK",                WM8994_AIF1DAC_LRCLK },
    {"AIF1_DAC1_FILTERS_1",          WM8994_AIF1_DAC1_FILTERS_1 },
};
#endif

typedef struct {
    I2CSlave i2c;
    uint16_t i2c_addr;
    int i2c_idx;

    uint16_t reg[WM8994_REGS_NUM];

    void (*data_req)(void *, int);
    void *opaque;

    SWVoiceOut *dac_voice;
    QEMUSoundCard card;

    bool active;
} WM8994State;

static struct {
    int val, rate;
} srs[] = {
    { 0,   8000 },
    { 1,  11025 },
    { 2,  12000 },
    { 3,  16000 },
    { 4,  22050 },
    { 5,  24000 },
    { 6,  32000 },
    { 7,  44100 },
    { 8,  48000 },
    { 9,  88200 },
    { 10, 96000 },
};

#ifdef DEBUG_WM8994
static char wm8994_regname_buff[50];
static const char *wm8994_regname(WM8994State *s,
                                  uint16_t address)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(wm8994_regs); i++) {
        if (address == wm8994_regs[i].address) {
            return wm8994_regs[i].name;
        }
    }

    snprintf(
        &wm8994_regname_buff[0],
        sizeof(wm8994_regname_buff),
        "reg(%.4X)",
        (unsigned int)address);

    return &wm8994_regname_buff[0];
}
#endif

static void wm8994_update_format(WM8994State *s);

static int vmstate_wm8994_post_load(void *opaque, int version)
{
    WM8994State *s = opaque;

    DPRINTF("enter\n");

    wm8994_update_format(s);

    return 0;
}

static const VMStateDescription vmstate_wm8994 = {
    .name = CODEC,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .post_load = vmstate_wm8994_post_load,
    .fields      = (VMStateField[]) {
        VMSTATE_I2C_SLAVE(i2c, WM8994State),
        VMSTATE_UINT16(i2c_addr, WM8994State),
        VMSTATE_INT32(i2c_idx, WM8994State),
        VMSTATE_UINT16_ARRAY(reg, WM8994State,
                                  WM8994_REGS_NUM),
        VMSTATE_BOOL(active, WM8994State),
        VMSTATE_END_OF_LIST()
    }
};

static uint8_t wm8994_volume(WM8994State *s, uint16_t reg)
{
    return (s->reg[reg] >> WM8994_SPKOUTL_VOL_SHIFT) &
           (WM8994_SPKOUTL_VOL_MASK >> WM8994_SPKOUTL_VOL_SHIFT);
}

static int wm8994_rate(WM8994State *s, uint16_t reg)
{
    int i;
    int rate = (s->reg[reg] >> WM8994_AIF1_SR_SHIFT) &
               (WM8994_AIF1_SR_MASK >> WM8994_AIF1_SR_SHIFT);

    for (i = 0; i < ARRAY_SIZE(srs); ++i) {
        if (rate == srs[i].val) {
            return srs[i].rate;
        }
    }

    return 0;
}

static audfmt_e wm8994_wl(WM8994State *s, uint16_t reg)
{
    int wl = (s->reg[reg] >> WM8994_AIF1_WL_SHIFT) &
             (WM8994_AIF1_WL_MASK >> WM8994_AIF1_WL_SHIFT);

    switch (wl) {
    case 0: return AUD_FMT_S16;
    case 3: return AUD_FMT_S32;
    case 1:
        /*
         * Unsupported format (20 bits per channel), but kernel
         * sets this sometimes when not playing anything
         */
        return AUD_FMT_S16;
    case 2:
        /*
         * Unsupported format (24 bits per channel), but kernel
         * sets this sometimes when not playing anything
         */
        return AUD_FMT_S16;
    default:
        hw_error("Unknown format\n");
    }
}

static void wm8994_audio_out_cb(void *opaque, int free_b)
{
    WM8994State *s = (WM8994State *) opaque;

    if (s->data_req) {
        s->data_req(s->opaque, free_b);
    }
}

static void wm8994_update_volume(WM8994State *s)
{
    int volume_left  = wm8994_volume(s, WM8994_SPEAKER_VOLUME_LEFT);
    int volume_right = wm8994_volume(s, WM8994_SPEAKER_VOLUME_RIGHT);

    if (s->dac_voice) {
        /* Speaker */
        AUD_set_volume_out(s->dac_voice, 1, volume_left, volume_right);
    }
}

static void wm8994_update_active(WM8994State *s)
{
    if (s->dac_voice) {
        AUD_set_active_out(s->dac_voice, s->active);
    }
}

static void wm8994_update_format(WM8994State *s)
{
    struct audsettings out_fmt;

    if (s->dac_voice) {
        AUD_set_active_out(s->dac_voice, 0);
    }

    /* Setup output */
    out_fmt.endianness = 0;
    out_fmt.nchannels = 2;
    out_fmt.freq = wm8994_rate(s, WM8994_AIF1_RATE);
    out_fmt.fmt = wm8994_wl(s, WM8994_AIF1_CONTROL_1);

    s->dac_voice =
        AUD_open_out(&s->card,
                     s->dac_voice,
                     CODEC ".speaker",
                     s,
                     wm8994_audio_out_cb,
                     &out_fmt);

    wm8994_update_volume(s);

    wm8994_update_active(s);
}

static void wm8994_reset(DeviceState *dev)
{
    WM8994State *s = FROM_I2C_SLAVE(WM8994State, I2C_SLAVE_FROM_QDEV(dev));

    DPRINTF("enter\n");

    s->i2c_addr = 0;
    s->i2c_idx = 0;
    s->active = 0;

    memset(s->reg, 0, sizeof(s->reg));
    s->reg[WM8994_SOFTWARE_RESET]       = 0x8994;
    s->reg[WM8994_POWER_MANAGEMENT_2]   = 0x6000;
    s->reg[WM8994_SPEAKER_VOLUME_LEFT]  = (0x79 << WM8994_SPKOUTL_VOL_SHIFT);
    s->reg[WM8994_SPEAKER_VOLUME_RIGHT] = (0x79 << WM8994_SPKOUTL_VOL_SHIFT);
    s->reg[WM8994_AIF1_RATE]            = (srs[0].val << WM8994_AIF1_SR_SHIFT);
    s->reg[WM8994_AIF1_CONTROL_1]       = (0x0 << WM8994_AIF1_WL_SHIFT);

    wm8994_update_format(s);
}

static int wm8994_tx(I2CSlave *i2c, uint8_t data)
{
    WM8994State *s = (WM8994State *) i2c;

    if (s->i2c_idx < sizeof(s->i2c_addr)) {
        s->i2c_addr |=
            (uint16_t)data << ((sizeof(s->i2c_addr) - s->i2c_idx - 1) * 8);
    } else {
        uint16_t offset = s->i2c_idx - sizeof(s->i2c_addr);
        uint16_t addr = s->i2c_addr + (offset / 2);

        offset %= 2;

        if (addr >= WM8994_REGS_NUM) {
            hw_error("illegal write offset 0x%X\n", s->i2c_addr + offset);
        } else {
            s->reg[addr] &= ~(0xFF << ((1 - offset) * 8));
            s->reg[addr] |= (uint16_t)data << ((1 - offset) * 8);

            if (offset == 1) {
                DPRINTF("0x%.4X -> %s\n",
                        (unsigned int)s->reg[addr],
                         wm8994_regname(s, addr));
                switch (addr) {
                case WM8994_SOFTWARE_RESET:
                    wm8994_reset(&s->i2c.qdev);
                    break;

                case WM8994_SPEAKER_VOLUME_LEFT:
                case WM8994_SPEAKER_VOLUME_RIGHT:
                    wm8994_update_volume(s);
                    break;

                case WM8994_AIF1_RATE:
                case WM8994_AIF1_CONTROL_1:
                    wm8994_update_format(s);
                    break;

                default:
                    break;
                }
            }
        }
    }

    ++s->i2c_idx;

    return 1;
}

static int wm8994_rx(I2CSlave *i2c)
{
    WM8994State *s = (WM8994State *) i2c;

    if (s->i2c_idx >= sizeof(uint16_t)) {
        DPRINTF("too much data requested (%i bytes)\n", (s->i2c_idx + 1));

        return 0;
    }

    if (s->i2c_addr < WM8994_REGS_NUM) {
        int ret = (s->reg[s->i2c_addr] >> ((1 - s->i2c_idx) * 8)) & 0xFF;

        if (s->i2c_idx == (sizeof(uint16_t) - 1)) {
            DPRINTF("%s -> 0x%.4X\n",
                    wm8994_regname(s, s->i2c_addr),
                    (unsigned int)s->reg[s->i2c_addr]);
        }

        ++s->i2c_idx;

        return ret;
    } else {
        hw_error(
            "wm8994: illegal read offset 0x%x\n", s->i2c_addr + s->i2c_idx);
    }
}

static void wm8994_event(I2CSlave *i2c, enum i2c_event event)
{
    WM8994State *s = (WM8994State *) i2c;

    switch (event) {
    case I2C_START_SEND:
        s->i2c_addr = 0;
    case I2C_START_RECV:
        s->i2c_idx = 0;
        break;
    default:
        break;
    }
}

void wm8994_set_active(DeviceState *dev, bool active)
{
    WM8994State *s = FROM_I2C_SLAVE(WM8994State, I2C_SLAVE_FROM_QDEV(dev));

    DPRINTF("enter %d\n", active);

    s->active = active;

    wm8994_update_active(s);
}

void wm8994_data_req_set(DeviceState *dev,
                         void (*data_req)(void*, int),
                         void *opaque)
{
    WM8994State *s = FROM_I2C_SLAVE(WM8994State, I2C_SLAVE_FROM_QDEV(dev));

    s->data_req = data_req;
    s->opaque = opaque;
}

int wm8994_dac_write(DeviceState *dev, void *buf, int num_bytes)
{
    WM8994State *s = FROM_I2C_SLAVE(WM8994State, I2C_SLAVE_FROM_QDEV(dev));

    int sent = 0;

    if (!s->dac_voice) {
        return 0;
    }

    while (sent < num_bytes) {
        int ret = AUD_write(s->dac_voice,
                            (uint8_t *)buf + sent,
                            num_bytes - sent);

        sent += ret;

        if (ret == 0) {
            break;
        }
    }

    return sent;
}

static int wm8994_init(I2CSlave *i2c)
{
    WM8994State *s = FROM_I2C_SLAVE(WM8994State, i2c);

    AUD_register_card(CODEC, &s->card);

    return 0;
}

static void wm8994_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *sc = I2C_SLAVE_CLASS(klass);

    sc->init = wm8994_init;
    sc->event = wm8994_event;
    sc->recv = wm8994_rx;
    sc->send = wm8994_tx;
    dc->reset = wm8994_reset;
    dc->vmsd = &vmstate_wm8994;
}

static TypeInfo wm8994_info = {
    .name          = "wm8994",
    .parent        = TYPE_I2C_SLAVE,
    .instance_size = sizeof(WM8994State),
    .class_init    = wm8994_class_init,
};

static void wm8994_register_types(void)
{
    type_register_static(&wm8994_info);
}

type_init(wm8994_register_types)
