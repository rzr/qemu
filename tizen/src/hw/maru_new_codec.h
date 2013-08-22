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
#include "qemu-thread.h"
#include "maru_device_ids.h"

#include <libavformat/avformat.h>

#define CODEC_CONTEXT_MAX           1024

#define VIDEO_CODEC_MEM_OFFSET_MAX  16
#define AUDIO_CODEC_MEM_OFFSET_MAX  64

/*
 *  Codec Device Structures
 */
typedef struct CodecParam {
    int32_t     api_index;
    int32_t     ctx_index;
    int32_t     file_index;
    uint32_t    mem_offset;
} CodecParam;

struct video_data {
    int width, height;
    int fps_n, fps_d;
    int par_n, par_d;
    int pix_fmt, bpp;
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
    CodecParam              ioparam;
} CodecContext;

typedef struct CodecThreadPool {
    QemuThread          *wrk_thread;
    QemuMutex           mutex;
    QemuCond            cond;
    uint32_t            state;
    uint8_t             isrunning;
} CodecThreadPool;

typedef struct NewCodecState {
    PCIDevice           dev;

    uint8_t             *vaddr;
    MemoryRegion        vram;
    MemoryRegion        mmio;

    QEMUBH              *codec_bh;
    QemuMutex           codec_mutex;
    QemuMutex           codec_job_queue_mutex;
    CodecThreadPool     wrk_thread;

    CodecContext        codec_ctx[CODEC_CONTEXT_MAX];
    CodecParam          ioparam;

    uint32_t            context_index;
    uint8_t             isrunning;
} NewCodecState;

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

#if 0
enum media_type {
    MEDIA_TYPE_UNKNOWN = -1,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
};
#endif

enum thread_state {
    CODEC_TASK_INIT = 0,
    CODEC_TASK_FIN = 0x1f,
};

#if 0
/*
 *  Codec Thread Functions
 */
void *new_codec_dedicated_thread(void *opaque);

void *new_codec_pop_readqueue(NewCodecState *s, int32_t file_index);
void new_codec_pop_writequeue(NewCodecState *s, int32_t file_index);
#endif

/*
 *  Codec Device Functions
 */
int new_codec_device_init(PCIBus *bus);
#if 0
uint64_t new_codec_read(void *opaque, target_phys_addr_t addr,
                    unsigned size);
void new_codec_write(void *opaque, target_phys_addr_t addr,
                uint64_t value, unsigned size);
#endif

#if 0
/*
 *  Codec Helper Functions
 */
void new_codec_reset_parser_info(NewCodecState *s, int32_t ctx_index);
void new_codec_reset_avcontext(NewCodecState *s, int32_t value);

/*
 *  FFMPEG Functions
 */
int new_avcodec_query_list(NewCodecState *s);
int new_avcodec_alloc_context(NewCodecState *s, int index);

int new_avcodec_init(NewCodecState *s, int ctx_id, int f_id);
int new_avcodec_deinit(NewCodecState *s, int ctx_id, int f_id);
int new_avcodec_decode_video(NewCodecState *s, int ctx_id, int f_id);
int new_avcodec_encode_video(NewCodecState *s, int ctx_id, int f_id);
int new_avcodec_decode_audio(NewCodecState *s, int ctx_id, int f_id);
int new_avcodec_encode_audio(NewCodecState *s, int ctx_id, int f_id);
int new_avcodec_picture_copy(NewCodecState *s, int ctx_id, int f_id);

AVCodecParserContext *new_avcodec_parser_init(AVCodecContext *avctx);
int new_avcodec_parser_parse (AVCodecParserContext *pctx, AVCodecContext *avctx,
                            uint8_t *inbuf, int inbuf_size,
                            int64_t pts, int64_t dts, int64_t pos);
#endif
