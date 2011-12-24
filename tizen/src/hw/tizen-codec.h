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

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

// #define CODEC_THREAD
// #define CODEC_KVM

/*
 *  Codec Device Structures
 */
typedef struct _SVCodecThreadInfo
{
    pthread_t 		codec_thread;
    pthread_cond_t 	cond;
    pthread_mutex_t	lock;
} SVCodecThreadInfo;

typedef struct _SVCodecParam {
    uint32_t		func_num;
    uint32_t		in_args[20];
    uint32_t		ret_args;
} SVCodecParam;

typedef struct _SVCodecInfo {
	uint8_t			num;
	uint32_t		param[10];
	uint32_t		tmpParam[10];
	uint32_t		param_size[10];
} SVCodecInfo;

typedef struct _SVCodecState {
    PCIDevice       	dev; 
    SVCodecThreadInfo	thInfo;
	SVCodecInfo			codecInfo;	

    int             	svcodec_mmio;

    uint8_t*        	vaddr;
    ram_addr_t      	vram_offset;

    uint32_t        	mem_addr;
    uint32_t        	mmio_addr;

	int 				index;
	bool				bstart;
} SVCodecState;

/*
 *  Codec Device APIs
 */

int pci_codec_init (PCIBus *bus);
static int codec_operate(uint32_t value, SVCodecState *opaque);


/*
 *  Codec Helper APIs
 */
void codec_set_context (AVCodecContext *dstctx,
                        AVCodecContext *srcctx);

#ifdef CODEC_THREAD
static int codec_copy_info (SVCodecState *s);
static int codec_thread_init (void *opaque);
static void* codec_worker_thread (void *opaque);
static void wake_codec_wrkthread(SVCodecState *s);
static void sleep_codec_wrkthread(SVCodecState *s);
static void codec_thread_destroy(void *opaque);
#endif

/*
 *  FFMPEG APIs
 */

void qemu_parser_init (void);

void qemu_restore_context (AVCodecContext *dst, AVCodecContext *src);

void qemu_av_register_all (void);

int qemu_avcodec_open (SVCodecState *s);

int qemu_avcodec_close (SVCodecState *s);

void qemu_avcodec_alloc_context (void);

void qemu_avcodec_alloc_frame (void);

void qemu_av_free (SVCodecState* s);

void qemu_avcodec_get_context_defaults (void);

void qemu_avcodec_flush_buffers (void);

int qemu_avcodec_default_get_buffer (void);

void qemu_avcodec_default_release_buffer (void);

int qemu_avcodec_decode_video (SVCodecState *s);

// int qemu_avcodec_decode_audio (void);

int qemu_avcodec_encode_video (SVCodecState *s);

// int qemu_avcodec_encode_audio (void);

void qemu_av_picture_copy (SVCodecState *s);

void qemu_av_parser_init (SVCodecState *s);

int qemu_av_parser_parse (SVCodecState *s);

void qemu_av_parser_close (void);

int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture);

void qemu_avcodec_release_buffer (AVCodecContext *context, AVFrame *picture);

