/*
 * Samsung exynos4210 Audio driver
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
#include "wm8994.h"

/* #define DEBUG_EXYNOS4210_AUDIO */

#ifdef DEBUG_EXYNOS4210_AUDIO
#define DPRINTF(fmt, ...) \
    do { \
        fprintf(stdout, "AUDIO: [%s:%d] " fmt, __func__, __LINE__, \
            ## __VA_ARGS__); \
    } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

#define AUDIO_MAX_WORDS (4096 / EXYNOS4210_I2S_WORD_LEN)

typedef struct {
    Exynos4210I2SSlave i2s;

    DeviceState *wm8994;
} Exynos4210AudioState;

static void exynos4210_audio_callback(void *opaque, int free_out_bytes)
{
    Exynos4210AudioState *s = (Exynos4210AudioState *)opaque;
    uint8_t buff[AUDIO_MAX_WORDS * EXYNOS4210_I2S_WORD_LEN];
    int free_out_words = free_out_bytes / EXYNOS4210_I2S_WORD_LEN;
    uint32_t num_words;

    if (free_out_words <= 0) {
        return;
    }

    if (free_out_words > AUDIO_MAX_WORDS) {
        free_out_words = AUDIO_MAX_WORDS;
    }

    if (!exynos4210_i2s_dma_enabled(qdev_get_parent_bus(&s->i2s.qdev))) {
        return;
    }

    num_words = exynos4210_i2s_dma_get_words_available(
                    qdev_get_parent_bus(&s->i2s.qdev));

    num_words = MIN(num_words, free_out_words);

    exynos4210_i2s_dma_read(qdev_get_parent_bus(&s->i2s.qdev),
                            &buff[0],
                            num_words);

    num_words = wm8994_dac_write(s->wm8994,
        &buff[0],
        num_words * EXYNOS4210_I2S_WORD_LEN) / EXYNOS4210_I2S_WORD_LEN;

    exynos4210_i2s_dma_advance(qdev_get_parent_bus(&s->i2s.qdev), num_words);
}

static void exynos4210_audio_dma_enable(Exynos4210I2SSlave *i2s, bool enable)
{
    Exynos4210AudioState *s =
        FROM_EXYNOS4210_I2S_SLAVE(Exynos4210AudioState, i2s);

    DPRINTF("enter %d\n", enable);

    wm8994_set_active(s->wm8994, enable);
}

static void exynos4210_audio_reset(DeviceState *dev)
{
    DPRINTF("enter\n");
}

static int exynos4210_audio_init(Exynos4210I2SSlave *i2s)
{
    Exynos4210AudioState *s =
        FROM_EXYNOS4210_I2S_SLAVE(Exynos4210AudioState, i2s);

    wm8994_data_req_set(s->wm8994, exynos4210_audio_callback, s);

    return 0;
}

static const VMStateDescription vmstate_exynos4210_audio = {
    .name = "exynos4210.audio",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_EXYNOS4210_I2S_SLAVE(i2s, Exynos4210AudioState),
        VMSTATE_END_OF_LIST()
    }
};

static Property exynos4210_audio_properties[] = {
    {
        .name   = "wm8994",
        .info   = &qdev_prop_ptr,
        .offset = offsetof(Exynos4210AudioState, wm8994),
    },
    DEFINE_PROP_END_OF_LIST(),
};

static void exynos4210_audio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    Exynos4210I2SSlaveClass *sc = EXYNOS4210_I2S_SLAVE_CLASS(klass);

    sc->init = exynos4210_audio_init;
    sc->dma_enable = exynos4210_audio_dma_enable;
    dc->reset = exynos4210_audio_reset;
    dc->props = exynos4210_audio_properties;
    dc->vmsd = &vmstate_exynos4210_audio;
}

static TypeInfo exynos4210_audio_info = {
    .name          = "exynos4210.audio",
    .parent        = TYPE_EXYNOS4210_I2S_SLAVE,
    .instance_size = sizeof(Exynos4210AudioState),
    .class_init    = exynos4210_audio_class_init,
};

static void exynos4210_audio_register_types(void)
{
    type_register_static(&exynos4210_audio_info);
}

type_init(exynos4210_audio_register_types)
