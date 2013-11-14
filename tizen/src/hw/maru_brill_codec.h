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

#include "hw.h"
#include "kvm.h"
#include "pci.h"
#include "pci_ids.h"
#include "osutil.h"
#include "qemu-common.h"
#include "qemu-thread.h"

#include "libavformat/avformat.h"
#include "maru_device_ids.h"
#include "tizen/src/debug_ch.h"

#define CODEC_CONTEXT_MAX           1024

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
    int32_t width;
    int32_t height;
    int32_t fps_n;
    int32_t fps_d;
    int32_t par_n;
    int32_t par_d;
    int32_t pix_fmt;
    int32_t bpp;
    int32_t ticks_per_frame;
};

struct audio_data {
    int32_t channels;
    int32_t sample_rate;
    int32_t block_align;
    int32_t depth;
    int32_t sample_fmt;
    int32_t frame_size;
    int32_t bits_per_smp_fmt;
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
    uint32_t            thread_state;
    uint8_t             is_thread_running;

    CodecContext        context[CODEC_CONTEXT_MAX];
    CodecParam          ioparam;
} MaruBrillCodecState;

enum codec_io_cmd {
    CODEC_CMD_API_INDEX             = 0x28,
    CODEC_CMD_CONTEXT_INDEX         = 0x2C,
    CODEC_CMD_FILE_INDEX            = 0x30,
    CODEC_CMD_DEVICE_MEM_OFFSET     = 0x34,
    CODEC_CMD_GET_THREAD_STATE      = 0x38,
    CODEC_CMD_GET_CTX_FROM_QUEUE    = 0x3C,
    CODEC_CMD_GET_DATA_FROM_QUEUE   = 0x40,
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
    CODEC_FLUSH_BUFFERS,
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
