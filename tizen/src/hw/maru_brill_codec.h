/*
 * Virtual Codec device
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 */

#include <stdio.h>
#include <sys/types.h>

#include "hw/hw.h"
#include "sysemu/kvm.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_ids.h"
#include "qemu/thread.h"

#include "tizen/src/debug_ch.h"
#include "maru_device_ids.h"
#include "libavformat/avformat.h"

#define CODEC_CONTEXT_MAX           1024

#define VIDEO_CODEC_MEM_OFFSET_MAX  16
#define AUDIO_CODEC_MEM_OFFSET_MAX  64

/*
 *  Codec Device Structures
 */
typedef struct CodecParam {
    int32_t     api_index;
    int32_t     ctx_index;
    uint32_t    file_index;
    uint32_t    mem_offset;
} CodecParam;

struct video_data {
    int width;
    int height;
    int fps_n;
    int fps_d;
    int par_n;
    int par_d;
    int pix_fmt;
    int bpp;
    int ticks_per_frame;
};

struct audio_data {
    int channels;
    int sample_rate;
    int block_align;
    int depth;
    int sample_fmt;
    int frame_size;
    int bits_per_smp_fmt;
    int64_t channel_layout;
};

typedef struct CodecContext {
    AVCodecContext          *avctx;
    AVFrame                 *frame;
    AVCodecParserContext    *parser_ctx;
    uint8_t                 *parser_buf;
    uint16_t                parser_use;
    uint16_t                occupied;
    uint32_t                file_index;
    bool                    opened;
} CodecContext;

typedef struct CodecThreadPool {
    QemuThread          *threads;
    QemuMutex           mutex;
    QemuCond            cond;
    uint32_t            state;
    uint8_t             isrunning;
} CodecThreadPool;

typedef struct MaruBrillCodecState {
    PCIDevice           dev;

    uint8_t             *vaddr;
    MemoryRegion        vram;
    MemoryRegion        mmio;

    QEMUBH              *codec_bh;
    QemuMutex           context_mutex;
    QemuMutex           context_queue_mutex;
    QemuMutex           ioparam_queue_mutex;

    CodecThreadPool     threadpool;

    CodecContext        context[CODEC_CONTEXT_MAX];
    CodecParam          ioparam;

    uint32_t            context_index;
    uint8_t             isrunning;
} MaruBrillCodecState;

enum codec_io_cmd {
    CODEC_CMD_API_INDEX             = 0x28,
    CODEC_CMD_CONTEXT_INDEX         = 0x2C,
    CODEC_CMD_FILE_INDEX            = 0x30,
    CODEC_CMD_DEVICE_MEM_OFFSET     = 0x34,
    CODEC_CMD_GET_THREAD_STATE      = 0x38,
    CODEC_CMD_GET_QUEUE             = 0x3C,
    CODEC_CMD_POP_WRITE_QUEUE       = 0x40,
    CODEC_CMD_RELEASE_CONTEXT       = 0x44,
    CODEC_CMD_GET_VERSION           = 0x50,
    CODEC_CMD_GET_ELEMENT           = 0x54,
    CODEC_CMD_GET_CONTEXT_INDEX     = 0x58,
};

enum codec_api_type {
    CODEC_INIT = 0,
    CODEC_DECODE_VIDEO,
    CODEC_ENCODE_VIDEO,
    CODEC_DECODE_AUDIO,
    CODEC_ENCODE_AUDIO,
    CODEC_PICTURE_COPY,
    CODEC_DEINIT,
 };

enum codec_type {
    CODEC_TYPE_UNKNOWN = -1,
    CODEC_TYPE_DECODE,
    CODEC_TYPE_ENCODE,
};

enum thread_state {
    CODEC_TASK_START    = 0,
    CODEC_TASK_END      = 0x1f,
};

/*
 *  Codec Device Functions
 */
int maru_brill_codec_pci_device_init(PCIBus *bus);
