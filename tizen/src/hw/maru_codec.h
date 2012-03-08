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
#include <libswscale/swscale.h>

// #define CODEC_HOST

/*
 *  Codec Device Structures
 */

typedef struct _SVCodecParam {
    uint32_t        func_num;
    uint32_t        in_args[20];
    uint32_t        ret_args;
} SVCodecParam;


typedef struct _SVCodecState {
    PCIDevice           dev;
    SVCodecParam        codecParam;

    int                 mmioIndex;

    uint32_t            mem_addr;
    uint32_t            mmio_addr;

    int                 index;

	/* For Memory API in QEMU 1.0 */

    uint8_t*            vram_ptr;
    MemoryRegion        vram;

	MemoryRegion		mmio;

} SVCodecState;

enum {
    FUNC_NUM            = 0x00,
    IN_ARGS             = 0x04,
    RET_STR             = 0x08,
};

enum {
    EMUL_AV_REGISTER_ALL = 1,
    EMUL_AVCODEC_OPEN,
    EMUL_AVCODEC_CLOSE,
    EMUL_AVCODEC_ALLOC_CONTEXT,
    EMUL_AVCODEC_ALLOC_FRAME,
    EMUL_AV_FREE_CONTEXT,
    EMUL_AV_FREE_FRAME,
    EMUL_AV_FREE_PALCTRL,
    EMUL_AV_FREE_EXTRADATA,
    EMUL_AVCODEC_FLUSH_BUFFERS,
    EMUL_AVCODEC_DECODE_VIDEO,
    EMUL_AVCODEC_ENCODE_VIDEO,
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
static int codec_operate(uint32_t value, SVCodecState *opaque);

/*
 *  FFMPEG APIs
 */

static void qemu_parser_init (void);

static void qemu_restore_context (AVCodecContext *dst, AVCodecContext *src);

static void qemu_av_register_all (void);

static int qemu_avcodec_open (SVCodecState *s);

static int qemu_avcodec_close (SVCodecState *s);

static void qemu_avcodec_alloc_context (void);

static void qemu_avcodec_alloc_frame (void);

static void qemu_av_free_context (void);

static void qemu_av_free_picture (void);

static void qemu_av_free_palctrl (void);

static void qemu_av_free_extradata (void);

static void qemu_avcodec_flush_buffers (void);

static int qemu_avcodec_decode_video (SVCodecState *s);

static int qemu_avcodec_encode_video (SVCodecState *s);

static void qemu_av_picture_copy (SVCodecState *s);

static void qemu_av_parser_init (SVCodecState *s);

static int qemu_av_parser_parse (SVCodecState *s);

static void qemu_av_parser_close (void);

static int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture);

static void qemu_avcodec_release_buffer (AVCodecContext *context, AVFrame *picture);

