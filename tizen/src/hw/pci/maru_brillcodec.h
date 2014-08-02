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

#ifndef __MARU_BRILLCODEC_H__
#define __MARU_BRILLCODEC_H__

#include "hw/pci/pci.h"

#include "libavcodec/avcodec.h"

#define CODEC_CONTEXT_MAX           1024
#define CODEC_MEM_SIZE          (32 * 1024 * 1024)

#define CONTEXT(s, id)        (s->context[id])

/*
 *  Codec Device Structures
 */
typedef struct CodecParam {
    int32_t     api_index;
    int32_t     ctx_index;
    uint32_t    mem_offset;
} CodecParam;

typedef struct CodecContext {
    AVCodecContext          *avctx;
    AVFrame                 *frame;
    AVCodecParserContext    *parser_ctx;
    uint8_t                 *parser_buf;
    uint16_t                parser_use;
    bool                    occupied_context;
    bool                    occupied_thread;
    bool                    opened_context;
    bool                    requested_close;
} CodecContext;

typedef struct CodecThreadPool {
    QemuThread          *threads;
    QemuMutex           mutex;
    QemuCond            cond;
} CodecThreadPool;

typedef struct DataHandler {
    void (*get_data)(void *dst, void *src, size_t size, enum AVPixelFormat pix_fmt);
    void (*release)(void *opaque);
} DataHandler;

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
    bool                is_thread_running;
    uint32_t            worker_thread_cnt;
    uint32_t            idle_thread_cnt;

    int                 irq_raised;

    CodecContext        context[CODEC_CONTEXT_MAX];
    CodecParam          ioparam;
} MaruBrillCodecState;

typedef struct DeviceMemEntry {
    void *opaque;
    uint32_t data_size;
    uint32_t ctx_id;

    DataHandler *handler;

    QTAILQ_ENTRY(DeviceMemEntry) node;
} DeviceMemEntry;

QTAILQ_HEAD(codec_wq, DeviceMemEntry);
extern struct codec_wq codec_wq;

extern DeviceMemEntry *entry[CODEC_CONTEXT_MAX];

int maru_brill_codec_query_list(MaruBrillCodecState *s);
int maru_brill_codec_get_context_index(MaruBrillCodecState *s);
void maru_brill_codec_wakeup_threads(MaruBrillCodecState *s, int api_index);
void maru_brill_codec_release_context(MaruBrillCodecState *s, int32_t ctx_id);
void maru_brill_codec_pop_writequeue(MaruBrillCodecState *s, uint32_t ctx_idx);

void *maru_brill_codec_threads(void *opaque);

#endif // __MARU_BRILLCODEC_H__
