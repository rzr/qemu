/*
 * Virtual Codec device
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
 *  DongKyun Yun <dk77.yun@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include "hw.h"
#include "kvm.h"
#include "pci.h"
#include "pci_ids.h"
#include "tizen/src/debug_ch.h"
#include "maru_pci_ids.h"

#include <libavformat/avformat.h>

#define CODEC_MAX_CONTEXT   10

/*
 *  Codec Device Structures
 */

typedef struct _SVCodecParam {
    uint32_t        apiIndex;
    uint32_t        ctxIndex;
    uint32_t        in_args[20];
    uint32_t        ret_args;
    uint32_t        mmapOffset;
    uint32_t        fileIndex;
} SVCodecParam;

typedef struct _SVCodecContext {
    AVCodecContext          *pAVCtx;
    AVFrame                 *pFrame;
    AVCodecParserContext    *pParserCtx;
    uint8_t                 *pParserBuffer;
    bool                    bParser;
    bool                    bUsed;
    uint32_t				nFileValue;
} SVCodecContext;

typedef struct _SVCodecState {
    PCIDevice           dev;
    SVCodecContext      ctxArr[CODEC_MAX_CONTEXT];
    SVCodecParam        codecParam;
    pthread_mutex_t     codec_mutex;

    int                 mmioIndex;
    uint32_t            mem_addr;
    uint32_t            mmio_addr;

    uint8_t*            vaddr;
    MemoryRegion        vram;
    MemoryRegion        mmio;
} SVCodecState;

enum {
    CODEC_API_INDEX         = 0x00,
    CODEC_IN_PARAM          = 0x04,
    CODEC_RETURN_VALUE      = 0x08,
    CODEC_CONTEXT_INDEX     = 0x0c,
    CODEC_MMAP_OFFSET       = 0x10,
    CODEC_FILE_INDEX        = 0x14,
    CODEC_CLOSED            = 0x18,
};

enum {
    EMUL_AV_REGISTER_ALL = 1,
    EMUL_AVCODEC_ALLOC_CONTEXT,
    EMUL_AVCODEC_ALLOC_FRAME,
    EMUL_AVCODEC_OPEN,
    EMUL_AVCODEC_CLOSE,
    EMUL_AV_FREE_CONTEXT,
    EMUL_AV_FREE_FRAME,
    EMUL_AV_FREE_PALCTRL,
    EMUL_AV_FREE_EXTRADATA,
    EMUL_AVCODEC_FLUSH_BUFFERS,
    EMUL_AVCODEC_DECODE_VIDEO,
    EMUL_AVCODEC_ENCODE_VIDEO,
    EMUL_AVCODEC_DECODE_AUDIO,
    EMUL_AVCODEC_ENCODE_AUDIO,
    EMUL_AV_PICTURE_COPY,
    EMUL_AV_PARSER_INIT,
    EMUL_AV_PARSER_PARSE,
    EMUL_AV_PARSER_CLOSE,
};


/*
 *  Codec Device APIs
 */
int codec_init (PCIBus *bus);

uint64_t codec_read (void *opaque, target_phys_addr_t addr, unsigned size);

void codec_write (void *opaque, target_phys_addr_t addr, uint64_t value, unsigned size);

static int codec_operate(uint32_t apiIndex, uint32_t ctxIndex, SVCodecState *state);

/*
 *  Codec Helper APIs
 */
static void qemu_parser_init (SVCodecState *s, int ctxIndex);

static void qemu_restore_context (AVCodecContext *dst, AVCodecContext *src);

/*
 *  FFMPEG APIs
 */

static void qemu_av_register_all (void);

static int qemu_avcodec_open (SVCodecState *s, int ctxIndex);

static int qemu_avcodec_close (SVCodecState *s, int ctxIndex);

static void qemu_avcodec_alloc_context (SVCodecState *s);

static void qemu_avcodec_alloc_frame (SVCodecState *s);

static void qemu_av_free_context (SVCodecState* s, int ctxIndex);

static void qemu_av_free_picture (SVCodecState* s, int ctxIndex);

static void qemu_av_free_palctrl (SVCodecState* s, int ctxIndex);

static void qemu_av_free_extradata (SVCodecState* s, int ctxIndex);

static void qemu_avcodec_flush_buffers (SVCodecState *s, int ctxIndex);

static int qemu_avcodec_decode_video (SVCodecState *s, int ctxIndex);

static int qemu_avcodec_encode_video (SVCodecState *s, int ctxIndex);

static int qemu_avcodec_decode_audio (SVCodecState *s, int ctxIndex);

static int qemu_avcodec_encode_audio (SVCodecState *s, int ctxIndex);

static void qemu_av_picture_copy (SVCodecState *s, int ctxIndex);

static void qemu_av_parser_init (SVCodecState *s, int ctxIndex);

static int qemu_av_parser_parse (SVCodecState *s, int ctxIndex);

static void qemu_av_parser_close (SVCodecState *s, int ctxIndex);

static int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture);

static void qemu_avcodec_release_buffer (AVCodecContext *context, AVFrame *picture);
