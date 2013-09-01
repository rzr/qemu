/*
 * Virtual Codec Device
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
#include "qemu-common.h"

#include "libavformat/avformat.h"
#include "osutil.h"
#include "maru_device_ids.h"
#include "maru_new_codec.h"
#include "tizen/src/debug_ch.h"

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, new_codec);

#define NEW_CODEC_DEV_NAME     "new_codec"
#define NEW_CODEC_VERSION      1

#define NEW_CODEC_MEM_SIZE          (32 * 1024 * 1024)
#define NEW_CODEC_REG_SIZE          (256)
#define NEW_CODEC_META_DATA_SIZE    (256)

#define GEN_MASK(x) ((1 << (x)) - 1)
#define ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) & ~GEN_MASK(x))
#define ROUND_UP_2(x) ROUND_UP_X(x, 1)
#define ROUND_UP_4(x) ROUND_UP_X(x, 2)
#define ROUND_UP_8(x) ROUND_UP_X(x, 3)
#define DIV_ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) >> (x))

#define DEFAULT_VIDEO_GOP_SIZE 15

#ifdef CODEC_DEBUG 
#include <sys/time.h>
#define CODEC_CURRENT_TIME \
{ \
    struct timeval now; \
    gettimeofday(&now, NULL); \
    INFO("[%s][%d] current time %ld:%ld\n", __func__, __LINE__, now.tv_sec, now.tv_usec); \
}
#else
#define CODEC_CURRENT_TIME
#endif

typedef struct DeviceMemEntry {
    uint8_t *buf;
    uint32_t buf_size;
    uint32_t buf_id;
    uint32_t ctx_id;

    QTAILQ_ENTRY(DeviceMemEntry) node;
} DeviceMemEntry;

static DeviceMemEntry *entry[CODEC_CONTEXT_MAX];

typedef struct CodecParamStg {
    void *buf;

    QTAILQ_ENTRY(CodecParamStg) node;
} CodecParamStg;

static QTAILQ_HEAD(codec_rq, DeviceMemEntry) codec_rq =
   QTAILQ_HEAD_INITIALIZER(codec_rq);

static QTAILQ_HEAD(codec_wq, DeviceMemEntry) codec_wq =
   QTAILQ_HEAD_INITIALIZER(codec_wq);

static QTAILQ_HEAD(ioparam_queue, CodecParamStg) ioparam_queue =
   QTAILQ_HEAD_INITIALIZER(ioparam_queue);

typedef struct PixFmtInfo {
    uint8_t x_chroma_shift;
    uint8_t y_chroma_shift;
} PixFmtInfo;

static PixFmtInfo pix_fmt_info[PIX_FMT_NB];

static void new_codec_reset_parser_info(NewCodecState *s, int32_t ctx_index);

static int new_codec_query_list(NewCodecState *s);
static void new_codec_alloc_context(NewCodecState *s, int index);
static void new_codec_release_context(NewCodecState *s, int32_t value);

static void new_codec_init(NewCodecState *s, int ctx_id, int f_id);
static void new_codec_deinit(NewCodecState *s, int ctx_id, int f_id);
static void new_codec_decode_video(NewCodecState *s, int ctx_id, int f_id);
static void new_codec_encode_video(NewCodecState *s, int ctx_id, int f_id);
static void new_codec_decode_audio(NewCodecState *s, int ctx_id, int f_id);
static void new_codec_encode_audio(NewCodecState *s, int ctx_id, int f_id);
static void new_codec_picture_copy(NewCodecState *s, int ctx_id, int f_id);

static AVCodecParserContext *new_codec_parser_init(AVCodecContext *avctx);
static int new_codec_parser_parse (AVCodecParserContext *pctx, AVCodecContext *avctx,
                            uint8_t *inbuf, int inbuf_size,
                            int64_t pts, int64_t dts, int64_t pos);

static void *new_codec_dedicated_thread(void *opaque);
static void *new_codec_pop_readqueue(NewCodecState *s, int32_t file_index);
static void new_codec_pop_writequeue(NewCodecState *s, int32_t file_index);

typedef void (*CodecFuncEntry)(NewCodecState *, int, int);

static CodecFuncEntry codec_func_handler[] = {
    new_codec_init,
    new_codec_decode_video,
    new_codec_encode_video,
    new_codec_decode_audio,
    new_codec_encode_audio,
    new_codec_picture_copy,
    new_codec_deinit,
};

static int worker_thread_cnt = 0;

static void get_host_cpu_cores(void)
{
    worker_thread_cnt = get_number_of_processors();
    INFO("number of dedicated threads: %d\n", worker_thread_cnt);
}

static void new_codec_dedicated_thread_init(NewCodecState *s)
{
    int index = 0;
    QemuThread *pthread = NULL;

    TRACE("enter: %s\n", __func__);

    pthread = g_malloc0(sizeof(QemuThread) * worker_thread_cnt);
    if (!pthread) {
        ERR("Failed to allocate wrk_thread memory.\n");
        return;
    }
    qemu_cond_init(&s->wrk_thread.cond);
    qemu_mutex_init(&s->wrk_thread.mutex);
    s->isrunning = 1;

    for (; index < worker_thread_cnt; index++) {
        qemu_thread_create(&pthread[index],
            new_codec_dedicated_thread, (void *)s, QEMU_THREAD_JOINABLE);
    }

    s->wrk_thread.wrk_thread = pthread;
    TRACE("leave: %s\n", __func__);
}

static void new_codec_thread_exit(NewCodecState *s)
{
    int index;

    TRACE("enter: %s\n", __func__);

    /* stop to run dedicated threads. */
    s->isrunning = 0;

    for (index = 0; index < worker_thread_cnt; index++) {
        qemu_thread_join(&s->wrk_thread.wrk_thread[index]);
    }

    TRACE("destroy mutex and conditional.\n");
    qemu_mutex_destroy(&s->wrk_thread.mutex);
    qemu_cond_destroy(&s->wrk_thread.cond);

    if (s->wrk_thread.wrk_thread) {
        g_free (s->wrk_thread.wrk_thread);
        s->wrk_thread.wrk_thread = NULL;
    }

    TRACE("leave: %s\n", __func__);
}

static void new_codec_add_ioparam_queue(NewCodecState *s, void *ctx_info)
{
    CodecParamStg *elem = NULL;

    elem = g_malloc0(sizeof(CodecParamStg));
    if (!elem) {
        ERR("failed to allocate ioparam_queue. %d\n", sizeof(CodecParamStg));
        return;
    }

    elem->buf = ctx_info;

    qemu_mutex_lock(&s->ioparam_queue_mutex);
    QTAILQ_INSERT_TAIL(&ioparam_queue, elem, node);
    qemu_mutex_unlock(&s->ioparam_queue_mutex);
}

static void new_codec_push_readqueue(NewCodecState *s, CodecParam *ioparam)
{
    DeviceMemEntry *elem = NULL;
    int readbuf_size, size = 0;
    uint8_t *readbuf = NULL;
    uint8_t *device_mem = NULL;

    TRACE("push_readqueue. mem_offset: 0x%x\n", ioparam->mem_offset);

    device_mem = (uint8_t *)s->vaddr + ioparam->mem_offset;
    if (!device_mem) {
        ERR("[%d] device memory region is null\n");
        return;
    }

    elem = g_malloc0(sizeof(DeviceMemEntry));
    if (!elem) {
        ERR("failed to allocate a buffer for readqueue. size: %d\n",
            sizeof(DeviceMemEntry));
        return;
    }

    memcpy(&readbuf_size, device_mem, sizeof(readbuf_size));
    size = sizeof(readbuf_size);

    TRACE("read data as many as %d size from guest\n", readbuf_size);
    if (readbuf_size <= 0) {
        ERR("readbuf size is smaller than zero. %d\n", readbuf_size);
    } else {
        readbuf = g_malloc0(readbuf_size);
        if (!readbuf) {
            ERR("failed to allocate a read buffer. size: %d\n", readbuf_size);
        } else {
            TRACE("copy input buffer from guest.\n");
            memcpy(readbuf, device_mem + size, readbuf_size);
//          memset(device_mem, 0x00, sizeof(readbuf_size) + readbuf_size);
        }
    }
    memset(device_mem, 0x00, sizeof(readbuf_size));

    elem->buf = readbuf;
    elem->buf_size = readbuf_size;
    elem->buf_id = ioparam->file_index;
    elem->ctx_id = ioparam->ctx_index;

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_rq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);
}

static int new_codec_header(void *opaque, int ctx_index, uint8_t *mem_start)
{
    int size = 0;
    uint8_t *device_mem = NULL;
    CodecParam *ioparam = (CodecParam *)opaque;

    TRACE("enter: %s\n", __func__);

    TRACE("context_index: %d\n", ctx_index);
    device_mem = mem_start + ((ctx_index - 1) * 256);
    if (!device_mem) {
        ERR("device memory is null.\n");
        return -1;
    }

    memcpy(&ioparam->api_index, device_mem, sizeof(ioparam->api_index));
    size = sizeof(ioparam->api_index);
    memcpy(&ioparam->mem_offset, device_mem + size, sizeof(ioparam->mem_offset));
    size += sizeof(ioparam->mem_offset);

    TRACE("api_index: %d, mem_offset 0x%x\n",
        ioparam->api_index, ioparam->mem_offset);

    TRACE("leave: %s\n", __func__);

    return size;
}

static void new_codec_switch_thread(NewCodecState *s, int32_t ctx_idx)
{
    CodecParam *ioparam;

    ioparam = g_malloc0(sizeof(CodecParam));
    if (!ioparam) {
        ERR("switch_thread. failed to allocate memory.\n");
        return;
    }
    ioparam->file_index = s->ioparam.file_index;
    ioparam->ctx_index = ctx_idx;

    TRACE("before parse codec_header, ctx_id: %d\n", ctx_idx);
    new_codec_header(ioparam, ctx_idx, s->vaddr);

#if 1
    switch (ioparam->api_index) {
    case CODEC_INIT:
        new_codec_init(s, ctx_idx, s->ioparam.file_index);
        g_free(ioparam);
        break;
    case CODEC_DECODE_VIDEO ... CODEC_ENCODE_AUDIO:
        new_codec_push_readqueue(s, ioparam);

        new_codec_add_ioparam_queue(s, (void *)ioparam);
        qemu_cond_signal(&s->wrk_thread.cond);

        break;
    default:
        TRACE("no need to push data to readqueue. %d\n", ioparam->api_index);
        break;
    }
#endif
    // FIXME: in case of default, need to call the below functions?
//    new_codec_add_ioparam_queue(s, (void *)ioparam);
//    qemu_cond_signal(&s->wrk_thread.cond);
}

static void new_codec_wakeup_thread(NewCodecState *s, int api_index)
{
    int32_t ctx_id = 0;
    CodecParam *ioparam;
//    CodecParam *ctx_info = NULL;

    ctx_id = s->ioparam.ctx_index;
    TRACE("[%d] context index: %d\n", __LINE__, ctx_id);

#if 0
    ioparam = &(s->context[ctx_id].ioparam);
    memset(ioparam, 0x00, sizeof(CodecParam));
    memcpy(ioparam, &s->ioparam, sizeof(CodecParam));
#endif

    ioparam = g_malloc0(sizeof(CodecParam));
    memcpy(ioparam, &s->ioparam, sizeof(CodecParam));

    TRACE("[%d] mem_offset: 0x%x\n", __LINE__, ioparam->mem_offset);
    TRACE("api_index: %d, context_id: %d\n",
        ioparam->api_index, ioparam->ctx_index);

    switch (ioparam->api_index) {
    case CODEC_INIT:
//        TRACE("no use large memory size\n");
        break;
    case CODEC_DECODE_VIDEO ... CODEC_ENCODE_AUDIO:
        new_codec_push_readqueue(s, ioparam);
        break;
    case CODEC_PICTURE_COPY ... CODEC_DEINIT:
        TRACE("no data from guest.\n");
        break;
//  case CODEC_DEINIT:
//      new_codec_deinit(s, ctx_id, ioparam->file_index);
//      break;
    default:
        TRACE("invalid codec command.\n");
        break;
    }

    new_codec_add_ioparam_queue(s, (void *)ioparam);
    qemu_cond_signal(&s->wrk_thread.cond);
}

static void *new_codec_dedicated_thread(void *opaque)
{
    NewCodecState *s = (NewCodecState *)opaque;

    TRACE("enter: %s\n", __func__);

    qemu_mutex_lock(&s->context_mutex);
    while (s->isrunning) {
        int ctx_id, f_id, api_id;
        CodecParamStg *elem = NULL;

        qemu_cond_wait(&s->wrk_thread.cond, &s->context_mutex);

        qemu_mutex_lock(&s->ioparam_queue_mutex);
        elem = QTAILQ_FIRST(&ioparam_queue);
        if (!elem) {
            qemu_mutex_unlock(&s->ioparam_queue_mutex);
            continue;
        }
        QTAILQ_REMOVE(&ioparam_queue, elem, node);
        qemu_mutex_unlock(&s->ioparam_queue_mutex);

        api_id = ((CodecParam *)elem->buf)->api_index;
        ctx_id = ((CodecParam *)elem->buf)->ctx_index;
        f_id = ((CodecParam *)elem->buf)->file_index;

        TRACE("api_id: %d ctx_id: %d f_id: %x\n", api_id, ctx_id, f_id);
        if (elem->buf) {
            TRACE("release a buffer of CodecParam\n");
            g_free(elem->buf);
        }
        if (elem) {
            TRACE("release an element of CodecParamStg\n");
            g_free(elem);
        }

        codec_func_handler[api_id](s, ctx_id, f_id);

        qemu_mutex_lock(&s->wrk_thread.mutex);
        s->wrk_thread.state = CODEC_TASK_END;
        qemu_mutex_unlock(&s->wrk_thread.mutex);

        if (api_id == CODEC_DEINIT) {
            TRACE("deinit func does not need to raise interrupt.\n");
        } else {
            TRACE("switch context to raise interrupt.\n");
            qemu_bh_schedule(s->codec_bh);
        }
    }
    qemu_mutex_unlock(&s->context_mutex);
    new_codec_thread_exit(s);

    TRACE("leave: %s\n", __func__);
    return NULL;
}

static void new_serialize_video_data(const struct video_data *video,
                                    AVCodecContext *avctx)
{
    if (video->width) {
        avctx->width = video->width;
    }
    if (video->height) {
        avctx->height = video->height;
    }
    if (video->fps_n) {
        avctx->time_base.num = video->fps_n;
    }
    if (video->fps_d) {
        avctx->time_base.den = video->fps_d;
    }
    if (video->pix_fmt > PIX_FMT_NONE) {
        avctx->pix_fmt = video->pix_fmt;
    }
    if (video->par_n) {
        avctx->sample_aspect_ratio.num = video->par_n;
    }
    if (video->par_d) {
        avctx->sample_aspect_ratio.den = video->par_d;
    }
    if (video->bpp) {
        avctx->bits_per_coded_sample = video->bpp;
    }
    if (video->ticks_per_frame) {
        avctx->ticks_per_frame = video->ticks_per_frame;
    }
}

static void new_deserialize_video_data (const AVCodecContext *avctx,
                                    struct video_data *video)
{
    memset(video, 0x00, sizeof(struct video_data));

    video->width = avctx->width;
    video->height = avctx->height;
    video->fps_n = avctx->time_base.num;
    video->fps_d = avctx->time_base.den;
    video->pix_fmt = avctx->pix_fmt;
    video->par_n = avctx->sample_aspect_ratio.num;
    video->par_d = avctx->sample_aspect_ratio.den;
    video->bpp = avctx->bits_per_coded_sample;
    video->ticks_per_frame = avctx->ticks_per_frame;
}

static void new_serialize_audio_data (const struct audio_data *audio,
                                    AVCodecContext *avctx)
{
    TRACE("audio. sample_fmt: %d\n", audio->sample_fmt);

    if (audio->channels) {
        avctx->channels = audio->channels;
    }
    if (audio->sample_rate) {
        avctx->sample_rate = audio->sample_rate;
    }
    if (audio->block_align) {
        avctx->block_align = audio->block_align;
    }

    if (audio->sample_fmt > AV_SAMPLE_FMT_NONE) {
        avctx->sample_fmt = audio->sample_fmt;
    }
}

static DeviceMemEntry *get_device_mem_ptr(NewCodecState *s, uint32_t file_index)
{
    DeviceMemEntry *elem = NULL;

    TRACE("pop element from codec readqueue.\n");
    elem = (DeviceMemEntry *)new_codec_pop_readqueue(s, file_index);
    if (!elem) {
        ERR("failed to pop from the queue.\n");
        return NULL;
    }

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_REMOVE(&codec_rq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);

    return elem;
}

static void new_codec_reset_parser_info(NewCodecState *s, int32_t ctx_index)
{
    s->context[ctx_index].parser_buf = NULL;
    s->context[ctx_index].parser_use = false;
}

static void new_codec_release_context(NewCodecState *s, int32_t file_index)
{
    DeviceMemEntry *rq_elem = NULL, *wq_elem = NULL;
    DeviceMemEntry *next = NULL;
    int ctx_id;

    TRACE("enter: %s\n", __func__);

    qemu_mutex_lock(&s->wrk_thread.mutex);

    for (ctx_id = 1; ctx_id < CODEC_CONTEXT_MAX; ctx_id++) {
        if (s->context[ctx_id].file_index == file_index) {
            TRACE("reset %d context\n", ctx_id);
            s->context[ctx_id].occupied = false;
            break;
        }
    }

    if (ctx_id == CODEC_CONTEXT_MAX) {
        ERR("cannot find a context for 0x%x\n", file_index);
    } else {
        if (s->context[ctx_id].opened) {
            INFO("%d of context has not closed yet.\n", ctx_id);
            new_codec_deinit(s, ctx_id, file_index);
        }

        QTAILQ_FOREACH_SAFE(rq_elem, &codec_rq, node, next) {
            if (rq_elem && rq_elem->buf_id == file_index) {
//        if (rq_elem && rq_elem->ctx_id == ctx_id) {
                TRACE("remove unused node from codec_rq. file: %p\n", file_index);
                qemu_mutex_lock(&s->context_queue_mutex);
                QTAILQ_REMOVE(&codec_rq, rq_elem, node);
                qemu_mutex_unlock(&s->context_queue_mutex);
                if (rq_elem && rq_elem->buf) {
                    TRACE("release_context. release rq_buffer: %p\n", rq_elem->buf);
                    g_free(rq_elem->buf);
                }

                TRACE("release rq_elem: %p\n", rq_elem);
                g_free(rq_elem);
            } else {
            TRACE("remain this node in the codec_rq. :%x\n", rq_elem->buf_id);
//                TRACE("remain this node in the codec_rq. :%x\n", ctx_id);
            }
        }

        QTAILQ_FOREACH_SAFE(wq_elem, &codec_wq, node, next) {
            if (wq_elem && wq_elem->buf_id == file_index) {
//          if (wq_elem && wq_elem->ctx_id == ctx_id) {
                TRACE("remove nodes from codec_wq."
                    " elem: %p, buf_id: %p, ctx_id: %d\n", wq_elem, wq_elem->ctx_id, file_index);
//              TRACE("remove nodes from codec_wq. ctx_id: %d\n", ctx_id);
                qemu_mutex_lock(&s->context_queue_mutex);
                QTAILQ_REMOVE(&codec_wq, wq_elem, node);
                qemu_mutex_unlock(&s->context_queue_mutex);

                if (wq_elem && wq_elem->buf) {
                    TRACE("release_context. release wq_buffer: %p\n", wq_elem->buf);
                    g_free(wq_elem->buf);
                    wq_elem->buf = NULL;
                }

                TRACE("release wq_elem: %p\n", wq_elem);
                g_free(wq_elem);
            } else {
                TRACE("remain this node in the codec_wq. :%x\n", wq_elem->buf_id);
//                TRACE("remain this node in the codec_wq. :%x\n", ctx_id);
            }
        }

        new_codec_reset_parser_info(s, ctx_id);
    }

    qemu_mutex_unlock(&s->wrk_thread.mutex);
    TRACE("leave: %s\n", __func__);
}

#if 0
int new_codec_get_buffer(AVCodecContext *context, AVFrame *picture)
{
    int ret;
    TRACE("%s\n", __func__);

    picture->reordered_opaque = context->reordered_opaque;
    picture->opaque = NULL;

    ret = avcodec_default_get_buffer(context, picture);

    return ret;
}

void new_codec_release_buffer(AVCodecContext *context, AVFrame *picture)
{
    TRACE("%s\n", __func__);
    avcodec_default_release_buffer(context, picture);
}
#endif

static void initialize_pixel_fmt_info(void)
{
    /* YUV formats */
    pix_fmt_info[PIX_FMT_YUV420P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUV420P].y_chroma_shift = 1;

    pix_fmt_info[PIX_FMT_YUV422P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUV422P].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUV444P].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_YUV444P].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUYV422].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUYV422].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUV410P].x_chroma_shift = 2;
    pix_fmt_info[PIX_FMT_YUV410P].y_chroma_shift = 2;

    pix_fmt_info[PIX_FMT_YUV411P].x_chroma_shift = 2;
    pix_fmt_info[PIX_FMT_YUV411P].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUVJ420P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUVJ420P].y_chroma_shift = 1;

    pix_fmt_info[PIX_FMT_YUVJ422P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUVJ422P].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUVJ444P].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_YUVJ444P].y_chroma_shift = 0;

    /* RGB formats */
    pix_fmt_info[PIX_FMT_RGB24].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_RGB24].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_BGR24].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_BGR24].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_RGB32].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_RGB32].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_RGB565].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_RGB565].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_RGB555].x_chroma_shift = 0;
    pix_fmt_info[PIX_FMT_RGB555].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUVA420P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUVA420P].y_chroma_shift = 1;
}

static int get_picture_size(AVPicture *picture, uint8_t *ptr, int pix_fmt,
                            int width, int height, bool encode)
{
    int size, w2, h2, size2;
    int stride, stride2;
    int fsize;
    PixFmtInfo *pinfo;

    pinfo = &pix_fmt_info[pix_fmt];

    switch (pix_fmt) {
    case PIX_FMT_YUV420P:
    case PIX_FMT_YUV422P:
    case PIX_FMT_YUV444P:
    case PIX_FMT_YUV410P:
    case PIX_FMT_YUV411P:
    case PIX_FMT_YUVJ420P:
    case PIX_FMT_YUVJ422P:
    case PIX_FMT_YUVJ444P:
        stride = ROUND_UP_4(width);
        h2 = ROUND_UP_X(height, pinfo->y_chroma_shift);
        size = stride * h2;
        w2 = DIV_ROUND_UP_X(width, pinfo->x_chroma_shift);
        stride2 = ROUND_UP_4(w2);
        h2 = DIV_ROUND_UP_X(height, pinfo->y_chroma_shift);
        size2 = stride2 * h2;
        fsize = size + 2 * size2;
        TRACE("stride: %d, stride2: %d, size: %d, size2: %d, fsize: %d\n",
            stride, stride2, size, size2, fsize);

        if (!encode && !ptr) {
            TRACE("allocate a buffer for a decoded picture.\n");
            ptr = av_mallocz(fsize);
            if (!ptr) {
                ERR("[%d] failed to allocate memory.\n", __LINE__);
                return -1;
            }
        } else {
//            TRACE("calculate encoded picture.\n");
            if (!ptr) {
                return -1;
            }
        }

        picture->data[0] = ptr;
        picture->data[1] = picture->data[0] + size;
        picture->data[2] = picture->data[1] + size2;
        picture->data[3] = NULL;
        picture->linesize[0] = stride;
        picture->linesize[1] = stride2;
        picture->linesize[2] = stride2;
        picture->linesize[3] = 0;
        TRACE("planes %d %d %d\n", 0, size, size + size2);
        TRACE("strides %d %d %d\n", 0, stride, stride2, stride2);
        break;
    case PIX_FMT_YUVA420P:
        stride = ROUND_UP_4(width);
        h2 = ROUND_UP_X(height, pinfo->y_chroma_shift);
        size = stride * h2;
        w2 = DIV_ROUND_UP_X(width, pinfo->x_chroma_shift);
        stride2 = ROUND_UP_4(w2);
        h2 = DIV_ROUND_UP_X(height, pinfo->y_chroma_shift);
        size2 = stride2 * h2;
        fsize = 2 * size + 2 * size2;
        if (!encode && !ptr) {
            TRACE("allocate a buffer for a decoded picture.\n");
            ptr = av_mallocz(fsize);
            if (!ptr) {
                ERR("[%d] failed to allocate memory.\n", __LINE__);
                return -1;
            }
        } else {
//          TRACE("calculate encoded picture.\n");
            if (!ptr) {
                return -1;
            }
        }

        picture->data[0] = ptr;
        picture->data[1] = picture->data[0] + size;
        picture->data[2] = picture->data[1] + size2;
        picture->data[3] = picture->data[2] + size2;
        picture->linesize[0] = stride;
        picture->linesize[1] = stride2;
        picture->linesize[2] = stride2;
        picture->linesize[3] = stride;
        TRACE("planes %d %d %d\n", 0, size, size + size2);
        TRACE("strides %d %d %d\n", 0, stride, stride2, stride2);
        break;
    case PIX_FMT_RGB24:
    case PIX_FMT_BGR24:
        stride = ROUND_UP_4 (width * 3);
        fsize = stride * height;
        TRACE("stride: %d, size: %d\n", stride, fsize);

        if (!encode && !ptr) {
            TRACE("allocate a buffer for a decoded picture.\n");
            ptr = av_mallocz(fsize);
            if (!ptr) {
                ERR("[%d] failed to allocate memory.\n", __LINE__);
                return -1;
            }
        } else {
            TRACE("calculate encoded picture.\n");
            if (!ptr) {
                return -1;
            }
        }

        picture->data[0] = ptr;
        picture->data[1] = NULL;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        picture->linesize[0] = stride;
        picture->linesize[1] = 0;
        picture->linesize[2] = 0;
        picture->linesize[3] = 0;
        break;
    case PIX_FMT_RGB32:
        stride = width * 4;
        fsize = stride * height;
        TRACE("stride: %d, size: %d\n", stride, fsize);

        if (!encode && !ptr) {
            TRACE("allocate a buffer for a decoded picture.\n");
            ptr = av_mallocz(fsize);
            if (!ptr) {
                ERR("[%d] failed to allocate memory.\n", __LINE__);
                return -1;
            }
        } else {
//            TRACE("calculate encoded picture.\n");
            if (!ptr) {
                return -1;
            }
        }

        picture->data[0] = ptr;
        picture->data[1] = NULL;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        picture->linesize[0] = stride;
        picture->linesize[1] = 0;
        picture->linesize[2] = 0;
        picture->linesize[3] = 0;
        break;
    case PIX_FMT_RGB555:
    case PIX_FMT_RGB565:
        stride = ROUND_UP_4 (width * 2);
        fsize = stride * height;
        TRACE("stride: %d, size: %d\n", stride, fsize);

        if (!encode && !ptr) {
            TRACE("allocate a buffer for a decoded picture.\n");
            ptr = av_mallocz(fsize);
            if (!ptr) {
                ERR("[%d] failed to allocate memory.\n", __LINE__);
                return -1;
            }
        } else {
//            TRACE("calculate encoded picture.\n");
            if (!ptr) {
                return -1;
            }
        }

        picture->data[0] = ptr;
        picture->data[1] = NULL;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        picture->linesize[0] = stride;
        picture->linesize[1] = 0;
        picture->linesize[2] = 0;
        picture->linesize[3] = 0;
        break;
    case PIX_FMT_PAL8:
        stride = ROUND_UP_4(width);
        size = stride * height;
        fsize = size + 256 * 4;
        TRACE("stride: %d, size: %d\n", stride, fsize);

        if (!encode && !ptr) {
            TRACE("allocate a buffer for a decoded picture.\n");
            ptr = av_mallocz(fsize);
            if (!ptr) {
                ERR("[%d] failed to allocate memory.\n", __LINE__);
                return -1;
            }
        } else {
//            TRACE("calculate encoded picture.\n");
            if (!ptr) {
                return -1;
            }
        }
        picture->data[0] = ptr;
        picture->data[1] = ptr + size;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        picture->linesize[0] = stride;
        picture->linesize[1] = 4;
        picture->linesize[2] = 0;
        picture->linesize[3] = 0;
        break;
    default:
        picture->data[0] = NULL;
        picture->data[1] = NULL;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        fsize = -1;
        ERR("pixel format: %d was wrong.\n", pix_fmt);
        break;
    }

    return fsize;
}

// FFmpeg Functions
static int new_codec_query_list (NewCodecState *s)
{
    AVCodec *codec = NULL;
    uint32_t size = 0, mem_size = 0;
    uint32_t data_len = 0, length = 0;
    int32_t codec_type, media_type;
    int32_t codec_fmts[4], i;

    /* register avcodec */
    TRACE("register avcodec\n");
    av_register_all();

    codec = av_codec_next(NULL);
    if (!codec) {
        ERR("failed to get codec info.\n");
        return -1;
    }

    // a region to store the number of codecs.
    length = 32 + 64 + 6 * sizeof(int32_t);
    mem_size = size = sizeof(uint32_t);

    while (codec) {
        codec_type =
            codec->decode ? CODEC_TYPE_DECODE : CODEC_TYPE_ENCODE;
        media_type = codec->type;

        memset(codec_fmts, -1, sizeof(codec_fmts));
        if (media_type == AVMEDIA_TYPE_VIDEO) {
            if (codec->pix_fmts) {
                for (i = 0; codec->pix_fmts[i] != -1; i++) {
                    codec_fmts[i] = codec->pix_fmts[i];
                }
            }
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            if (codec->sample_fmts) {
                for (i = 0; codec->sample_fmts[i] != -1; i++) {
                    codec_fmts[i] = codec->sample_fmts[i];
                }
            }
        } else {
            ERR("%s of media type is unknown.\n", codec->name);
        }

        memset(s->vaddr + mem_size, 0x00, length);
        mem_size += length;

        data_len += length;
        memcpy(s->vaddr, &data_len, sizeof(data_len));

        memcpy(s->vaddr + size, &codec_type, sizeof(codec_type));
        size += sizeof(codec_type);
        memcpy(s->vaddr + size, &media_type, sizeof(media_type));
        size += sizeof(media_type);
        memcpy(s->vaddr + size, codec->name, strlen(codec->name));
        size += 32;
        memcpy(s->vaddr + size,
           codec->long_name, strlen(codec->long_name));
        size += 64;
        memcpy(s->vaddr + size, codec_fmts, sizeof(codec_fmts));
        size += sizeof(codec_fmts);

        codec = av_codec_next(codec);
    }

    return 0;
}

static int new_codec_get_context_index(NewCodecState *s)
{
    int index;

    TRACE("enter: %s\n", __func__);

    for (index = 1; index < CODEC_CONTEXT_MAX; index++) {
        if (s->context[index].occupied == false) {
            TRACE("get %d of codec context successfully.\n", index);
            s->context[index].occupied = true;
            break;
        }
    }

    if (index == CODEC_CONTEXT_MAX) {
        ERR("failed to get available codec context. ");
        ERR("try to run codec again.\n");
        index = -1;
    }

    return index;
}

static void new_codec_alloc_context(NewCodecState *s, int index)
{
    TRACE("enter: %s\n", __func__);

    TRACE("allocate %d of context and frame.\n", index);
    s->context[index].avctx = avcodec_alloc_context();
    s->context[index].frame = avcodec_alloc_frame();
    s->context[index].opened = false;

    s->context[index].parser_buf = NULL;
    s->context[index].parser_use = false;

    TRACE("leave: %s\n", __func__);
}

#if 0
void new_codec_flush_buffers(NewCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;

    TRACE("enter:\n");
    qemu_mutex_lock(&s->wrk_thread.mutex);

    avctx = s->context[ctx_index].avctx;
    if (avctx) {
        avcodec_flush_buffers(avctx);
    } else {
        ERR("[%s] %d of AVCodecContext is NULL\n", __func__, ctx_index);
    }

    qemu_mutex_unlock(&s->wrk_thread.mutex);
    TRACE("[%s] leave:\n", __func__);
}
#endif

static void copyback_init_data(AVCodecContext *avctx, uint8_t *mem_buf)
{
    int size = 0;

    if (avctx->codec->type == AVMEDIA_TYPE_AUDIO) {
        int osize = 0;

        TRACE("codec_init. audio sample_fmt: %d\n", avctx->sample_fmt);

        memcpy(mem_buf, &avctx->sample_fmt, sizeof(avctx->sample_fmt));
        size = sizeof(avctx->sample_fmt);
        memcpy(mem_buf + size,
                &avctx->frame_size, sizeof(avctx->frame_size));
        size += sizeof(avctx->frame_size);
        osize = av_get_bits_per_sample(avctx->codec->id) / 8;
        memcpy(mem_buf + size, &osize, sizeof(osize));

        TRACE("codec_init. audio sample_fmt1: %d\n", *(int *)(mem_buf));
    }
}

static void copyback_decode_video_data(AVCodecContext *avctx, int len,
                                        int got_pic_ptr, uint8_t *mem_buf)
{
    struct video_data video;
    int size = 0;

    memcpy(mem_buf, &len, sizeof(len));
    size = sizeof(len);
    memcpy(mem_buf + size, &got_pic_ptr, sizeof(got_pic_ptr));
    size += sizeof(got_pic_ptr);
    new_deserialize_video_data(avctx, &video);
    memcpy(mem_buf + size, &video, sizeof(struct video_data));
}

static void copyback_decode_audio_data(int64_t channel_layout, int len,
                                       int frame_size_ptr, uint8_t *mem_buf)

//                                        int frame_size_ptr, int outbuf_size, uint8_t *mem_buf)
{
    int size = 0;

    TRACE("copy decode_audio. len %d, frame_size %d\n", len, frame_size_ptr);

    memcpy(mem_buf, &channel_layout, sizeof(channel_layout));
    size = sizeof(channel_layout);
    memcpy(mem_buf + size, &len, sizeof(len));
    size += sizeof(len);
    memcpy(mem_buf + size, &frame_size_ptr, sizeof(frame_size_ptr));
}

static void copyback_encode_video_data(int len, uint8_t *mem_buf)
{
    memcpy(mem_buf, &len, sizeof(len));
}

static void copyback_encode_audio_data(int len, uint8_t *mem_buf)
{
    memcpy(mem_buf, &len, sizeof(len));
}

static AVCodec *new_codec_find_avcodec(uint8_t *mem_buf)
{
    AVCodec *codec = NULL;
    int32_t encode, size = 0;
    char codec_name[32] = {0, };

    memcpy(&encode, mem_buf, sizeof(encode));
    size = sizeof(encode);
    memcpy(codec_name, mem_buf + size, sizeof(codec_name));
    size += sizeof(codec_name);

    TRACE("type: %d, name: %s\n", encode, codec_name);

    if (encode) {
        codec = avcodec_find_encoder_by_name (codec_name);
    } else {
        codec = avcodec_find_decoder_by_name (codec_name);
    }
    INFO("%s!! find %s %s\n",
        codec ? "success" : "failure",
        codec_name, encode ? "encoder" : "decoder");

    return codec;
}

static void new_codec_init(NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx = NULL;
    AVCodecParserContext *parser = NULL;
    AVCodec *codec = NULL;
    int size = 0, ret;
    int bitrate = 0;
    uint8_t *meta_buf = NULL;

    TRACE("enter: %s\n", __func__);

    new_codec_alloc_context(s, ctx_id);
    avctx = s->context[ctx_id].avctx;
    if (!avctx) {
        ERR("[%d] failed to allocate context.\n", __LINE__);
        return;
    }

    s->context[ctx_id].file_index = f_id;

    meta_buf = s->vaddr + ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    codec = new_codec_find_avcodec(meta_buf);
    if (codec) {
        size = sizeof(int32_t) + 32; // buffer size of codec_name
        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            struct video_data video = { 0 };

            memcpy(&video, meta_buf + size, sizeof(video));
            size += sizeof(video);
            new_serialize_video_data(&video, avctx);
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            struct audio_data audio = { 0 };

            memcpy(&audio, meta_buf + size, sizeof(audio));
            size += sizeof(audio);
            new_serialize_audio_data(&audio, avctx);
        } else {
            ERR("unknown media type.\n");
        }

        memcpy(&bitrate, meta_buf + size, sizeof(bitrate));
        size += sizeof(bitrate);
        if (bitrate) {
            avctx->bit_rate = bitrate;
        }
        memcpy(&avctx->codec_tag, meta_buf + size, sizeof(avctx->codec_tag));
        size += sizeof(avctx->codec_tag);
        memcpy(&avctx->extradata_size,
            meta_buf + size, sizeof(avctx->extradata_size));
        size += sizeof(avctx->extradata_size);
        if (avctx->extradata_size > 0) {
            TRACE("extradata size: %d.\n", avctx->extradata_size);
            avctx->extradata =
                g_malloc0(ROUND_UP_X(avctx->extradata_size +
                            FF_INPUT_BUFFER_PADDING_SIZE, 4));
            if (avctx->extradata) {
                memcpy(avctx->extradata, meta_buf + size, avctx->extradata_size);
            }
        } else {
            TRACE("no extra data.\n");
            avctx->extradata =
                g_malloc0(ROUND_UP_X(FF_INPUT_BUFFER_PADDING_SIZE, 4));
        }

        ret = avcodec_open(avctx, codec);
        INFO("avcodec_open done: %d\n", ret);

        s->context[ctx_id].opened = true;
    } else {
        ERR("failed to find codec.\n");
        ret = -1;
    }

    memcpy(meta_buf, &ret, sizeof(ret));
    size = sizeof(ret);
    if (ret < 0) {
        ERR("failed to open codec contex.\n");
    } else {
//      uint32_t aaa = ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE);

        INFO("type: %d ctx: %d, meta_buf: 0x%x, size: %d\n",
            codec->type, ctx_id, ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE), size);
        copyback_init_data(avctx, meta_buf + size);

#if 0
        if (avctx->codec->type == AVMEDIA_TYPE_AUDIO) {
        TRACE("audio. codec_init. ret: %d, sample_fmt: %d\n", *(int *)(s->vaddr + aaa + 8), *(int *)(s->vaddr + aaa + 12));
        }
#endif
    }

#if 1
    {
        DeviceMemEntry *elem = NULL;
        elem = g_malloc0(sizeof(DeviceMemEntry));

        elem->buf_id = f_id;
        elem->ctx_id = ctx_id;

        qemu_mutex_lock(&s->context_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->context_queue_mutex);
    }
#endif

    parser = new_codec_parser_init(avctx);
    s->context[ctx_id].parser_ctx = parser;

    TRACE("leave: %s\n", __func__);
}

static void new_codec_deinit(NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx;
    AVFrame *frame;
    AVCodecParserContext *parserctx;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    frame = s->context[ctx_id].frame;
    parserctx = s->context[ctx_id].parser_ctx;
    if (!avctx || !frame) {
        ERR("%d of context or frame is NULL.\n", ctx_id);
    } else {
        avcodec_close(avctx);
        INFO("close avcontext of %d\n", ctx_id);

        if (avctx->extradata) {
            TRACE("free context extradata\n");
            av_free(avctx->extradata);
            s->context[ctx_id].avctx->extradata = NULL;
        }

        if (avctx->palctrl) {
            TRACE("free context palctrl \n");
            av_free(avctx->palctrl);
            s->context[ctx_id].avctx->palctrl = NULL;
        }

        if (frame) {
            TRACE("free frame\n");
            av_free(frame);
            s->context[ctx_id].frame = NULL;
        }

        if (avctx) {
            TRACE("free codec context\n");
            av_free(avctx);
            s->context[ctx_id].avctx = NULL;
        }

        if (parserctx) {
            av_parser_close(parserctx);
            s->context[ctx_id].parser_ctx = NULL;
        }
    }
    s->context[ctx_id].opened = false;

    TRACE("leave: %s\n", __func__);
}

static void new_codec_decode_video(NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx = NULL;
    AVFrame *picture = NULL;
    AVPacket avpkt;
    int got_pic_ptr = 0, len = 0;
    uint8_t *inbuf = NULL;
    int inbuf_size, idx, size = 0;
    int64_t in_offset;
//    AVCodecParserContext *pctx = NULL;
//    int parser_ret, bsize;
//    uint8_t *bdata;
    DeviceMemEntry *elem = NULL;
    uint8_t *meta_buf = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    picture = s->context[ctx_id].frame;
//    pctx = s->context[ctx_id].parser_ctx;
    if (!avctx || !picture) {
        ERR("%d of AVCodecContext or AVFrame is NULL.\n", ctx_id);
        return;
    }

    if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        return;
    }

    meta_buf = s->vaddr + ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    size = sizeof(inbuf_size);
    memcpy(&idx, meta_buf + size, sizeof(idx));
    size += sizeof(idx);
    memcpy(&in_offset, meta_buf + size, sizeof(in_offset));
    size += sizeof(in_offset);
    TRACE("input buffer size: %d\n", inbuf_size);

    elem = get_device_mem_ptr(s, f_id);
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem && inbuf_size > 0) {
        inbuf_size = 0;
    } else {
        TRACE("decode_video. no input buffer.\n");
    }

#if 0
    // TODO: not sure that it needs to parser a packet or not.
    if (pctx) {
        parser_ret =
            new_codec_parser_parse (pctx, avctx, inbuf, inbuf_size, idx, idx, in_offset);
//                            &bdata, &bsize, idx, idx, in_offset);

        INFO("returned parser_ret: %d.\n", parser_ret);
    }
#endif

    memset(&avpkt, 0x00, sizeof(AVPacket));
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;

    len = avcodec_decode_video2(avctx, picture, &got_pic_ptr, &avpkt);
    if (len < 0) {
        ERR("failed to decode a frame\n");
    }
    TRACE("after decoding video. len: %d, have_data: %d\n", len);
    if (inbuf) {
        TRACE("decode_video input bufffer.\n");
        g_free(inbuf);
        g_free(elem);
    }

    copyback_decode_video_data(avctx, len, got_pic_ptr, meta_buf);
    {
        DeviceMemEntry *elem = NULL;
        elem = g_malloc0(sizeof(DeviceMemEntry));

        elem->buf_id = f_id;
        elem->ctx_id = ctx_id;

        qemu_mutex_lock(&s->context_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->context_queue_mutex);
    }

    TRACE("leave: %s\n", __func__);
}

static void new_codec_picture_copy (NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx;
    AVPicture *src;
    AVPicture dst;
    uint8_t *out_buffer;
    int pict_size;

    TRACE("enter: %s\n", __func__);

    TRACE("copy decoded image of %d context.\n", ctx_id);

    avctx = s->context[ctx_id].avctx;
    src = (AVPicture *)s->context[ctx_id].frame;
    if (!avctx || !src) {
        ERR("%d of AVCodecContext or AVFrame is NULL.\n", ctx_id);
        return;
    }

    if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        return;
    }

//  avctx->file_index = f_id;

    TRACE("decoded image. pix_fmt: %d width: %d, height: %d\n",
        avctx->pix_fmt, avctx->width, avctx->height);

    pict_size = get_picture_size(&dst, NULL, avctx->pix_fmt,
                                avctx->width, avctx->height, false);
    if ((pict_size) < 0) {
        ERR("picture size: %d\n", pict_size);
        return;
    }
    TRACE("picture size: %d\n", pict_size);

    av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);

#if 0
    out_buffer = dst.data[0];

    TRACE("after copying image src to dest.\n");
    if (out_buffer) {
        memcpy(s->vaddr, out_buffer, pict_size);
        av_free (out_buffer);
    }
#endif

#if 1
    {
        DeviceMemEntry *elem = NULL;
        uint8_t *tempbuf = NULL;

        TRACE("push data into codec_wq\n");
        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("failed to allocate memory. size: %d\n", sizeof(DeviceMemEntry));
            return;
        }

        tempbuf = g_malloc0(pict_size);
        if (!tempbuf) {
            ERR("failed to allocate a picture buffer. size: %d\n", pict_size);
            g_free(elem);
            return;
        }

        out_buffer = dst.data[0];
        memcpy(tempbuf, out_buffer, pict_size);

        if (out_buffer) {
            av_free (out_buffer);
        }

        elem->buf = tempbuf;
        elem->buf_size = pict_size;
        elem->buf_id = f_id;
        elem->ctx_id = ctx_id;

        TRACE("push decoded image to codec_wq. %p\n", elem);
        qemu_mutex_lock(&s->context_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->context_queue_mutex);
    }
#endif

    TRACE("leave: %s\n", __func__);
}

static void new_codec_decode_audio(NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx;
    AVPacket avpkt;
    int16_t *samples;
    uint8_t *inbuf = NULL;
    int inbuf_size;
    int len, frame_size_ptr = 0;
#if 0
    uint8_t *parser_buf;
    bool parser_use;
#endif
    uint8_t *meta_buf = NULL;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    if (!avctx) {
        ERR("[%s] %d of AVCodecContext is NULL!\n", __func__, ctx_id);
        return;
    }

    if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        return;
    }

#if 0
    if (s->context[ctx_id].parser_ctx) {
        parser_buf = s->context[ctx_id].parser_buf;
        parser_use = s->context[ctx_id].parser_use;
    }
#endif

    meta_buf = s->vaddr + ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    TRACE("before decoding audio. inbuf_size: %d\n", inbuf_size);

#if 0
    if (parser_buf && parser_use) {
        TRACE("[%s] use parser, buf:%p codec_id:%x\n",
                __func__, parser_buf, avctx->codec_id);
        buf = parser_buf;
    } else if (buf_size > 0) {
#endif
    elem = get_device_mem_ptr(s, f_id);
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem && inbuf_size > 0 ) {
        inbuf_size = 0;
    } else {
        TRACE("decode_audio. no input buffer.\n");
    }

    av_init_packet(&avpkt);
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;

    frame_size_ptr = AVCODEC_MAX_AUDIO_FRAME_SIZE;
//    outbuf_size = frame_size_ptr;
    samples = av_mallocz(frame_size_ptr);
    if (!samples) {
        ERR("failed to allocate an outbuf of audio.\n");
        len = -1;
    } else {
        len =
            avcodec_decode_audio3(avctx, samples, &frame_size_ptr, &avpkt);
        if (len < 0) {
            ERR("failed to decode audio\n", len);
        }
        TRACE("decoding audio. len %d, channel_layout %lld, frame_size %d\n",
            len, avctx->channel_layout, frame_size_ptr);

        if (inbuf) {
            TRACE("release decode_audio inbuf\n");
            g_free(inbuf);
            g_free(elem);
        }
    }

    copyback_decode_audio_data(avctx->channel_layout, len, frame_size_ptr, meta_buf);

    elem = g_malloc0(sizeof(DeviceMemEntry));
    if (len >= 0) {
        tempbuf = g_malloc0(frame_size_ptr);
        if (!tempbuf) {
            ERR("decode_audio. failed to allocate memory, len %d, have_data: %d\n", len, frame_size_ptr);
        } else {
            memcpy(tempbuf, samples, frame_size_ptr);
        }

        if (samples) {
            av_free(samples);
            TRACE("release decode_audio outbuf\n");
        }

        TRACE("elem: %p, buf: %p\n", elem, tempbuf);

        elem->buf = tempbuf;
        elem->buf_size = frame_size_ptr;
    }
    elem->buf_id = f_id;
    elem->ctx_id = ctx_id;

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);

#if 0
    if (parser_buf && parser_use) {
        TRACE("[%s] free parser buf\n", __func__);
        av_free(avpkt.data);
        s->context[ctx_id].parser_buf = NULL;
    }
#endif
    TRACE("leave: %s\n", __func__);
}

static void new_codec_encode_video(NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx = NULL;
    AVFrame *pict = NULL;
    uint8_t *inbuf = NULL, *outbuf = NULL;
    int inbuf_size, outbuf_size, len = 0;
    int ret;
    int64_t in_timestamp;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;
    uint8_t *meta_buf = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    pict = s->context[ctx_id].frame;
    if (!avctx || !pict) {
        ERR("%d of context or frame is NULL\n", ctx_id);
        return;
    }

    if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        return;
    }

    meta_buf = s->vaddr + ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    memcpy(&in_timestamp, meta_buf + sizeof(inbuf_size), sizeof(in_timestamp));
    TRACE("input buffer size: %d\n", inbuf_size);

    elem = get_device_mem_ptr(s, f_id);
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem && inbuf_size > 0) {
        inbuf_size = 0;
    } else {
        TRACE("encode_video. no input buffer.\n");
    }

    TRACE("pixel format: %d inbuf: %p, picture data: %p\n",
        avctx->pix_fmt, inbuf, pict->data[0]);

    ret = get_picture_size((AVPicture *)pict, inbuf, avctx->pix_fmt,
                           avctx->width, avctx->height, true);
    if (ret < 0) {
        ERR("after avpicture_fill, ret:%d\n", ret);
        len = -1;
    } else {
#if 0
        if (ret != inbuf_size) {
            ERR("");
        }
#endif
        if (avctx->time_base.num == 0) {
            pict->pts = AV_NOPTS_VALUE;
        } else {
            AVRational bq = {1, (G_USEC_PER_SEC * G_GINT64_CONSTANT(1000))};
            pict->pts = av_rescale_q(in_timestamp, bq, avctx->time_base);
        }
        TRACE("before encode video, ticks_per_frame:%d, pts:%lld\n",
                avctx->ticks_per_frame, pict->pts);

        outbuf_size = (avctx->width * avctx->height * 6) + FF_MIN_BUFFER_SIZE;
        outbuf = g_malloc0(outbuf_size);
        if (!outbuf) {
            ERR("failed to allocate a buffer of encoding video.\n");
            len = -1;
        } else {
            len = avcodec_encode_video(avctx, outbuf, outbuf_size, pict);
            if (len < 0) {
                ERR("failed to encode video. ret: %d\n", len);
            }
            TRACE("encode video, len:%d, pts:%lld, outbuf size: %d\n",
                    len, pict->pts, outbuf_size);
            if (inbuf) {
                TRACE("[%d] release used read bufffer.\n", __LINE__);
                g_free(inbuf);
                g_free(elem);
            }
        }
    }
//    copyback_encode_video_data(len, meta_buf);
    memcpy(meta_buf, &len, sizeof(len));

    elem = g_malloc0(sizeof(DeviceMemEntry));
    if (len > 0) {
        tempbuf = g_malloc0(len);
        if (!tempbuf) {
            ERR("failed to allocate an element of writequeue.\n");
        } else {
            memcpy(tempbuf, outbuf, len);
        }

        if (outbuf) {
            g_free(outbuf);
        }

        elem->buf = tempbuf;
        elem->buf_size = len;
    }
    elem->buf_id = f_id;
    elem->ctx_id = ctx_id;

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);

    TRACE("leave: %s\n", __func__);
}

static void new_codec_encode_audio(NewCodecState *s, int ctx_id, int f_id)
{
    AVCodecContext *avctx;
    uint8_t *inbuf = NULL, *outbuf = NULL;
    int32_t inbuf_size, max_size;
    int len = 0;
    uint8_t *meta_buf = NULL;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    if (!avctx) {
        ERR("[%s] %d of Context is NULL!\n", __func__, ctx_id);
        return;
    }
    if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        return;
    }

    meta_buf = s->vaddr + ((ctx_id - 1) * NEW_CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    memcpy(&max_size, meta_buf + sizeof(inbuf_size), sizeof(max_size));
    TRACE("encoding audio. in_size: %d, max_size: %d\n", inbuf_size, max_size);

    elem = get_device_mem_ptr(s, f_id);
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem && inbuf_size > 0) {
        inbuf_size = 0;
    } else {
        TRACE("encode_audio. no input buffer.\n");
    }

    outbuf = g_malloc0(max_size + FF_MIN_BUFFER_SIZE);
    if (!outbuf) {
        ERR("failed to allocate a buffer of encoding audio.\n");
    } else {
        len =
            avcodec_encode_audio(avctx, outbuf, max_size, (short *)inbuf);
        if (len < 0) {
            ERR("failed to encode audio.\n");
        }
        TRACE("after encoding audio. len: %d\n", len);
        if (inbuf) {
            TRACE("[%d] release used read bufffer.\n", __LINE__);
            g_free(inbuf);
            g_free(elem);
        }
    }

    copyback_encode_audio_data(len, meta_buf);

    elem = g_malloc0(sizeof(DeviceMemEntry));
    if (len > 0) {
        tempbuf = g_malloc0(len);
        if (!tempbuf) {
            ERR("encode_audio. failed to allocate temporary buffer.\n");
        } else {
            memcpy(tempbuf, outbuf, len);
        }

        if (outbuf) {
            g_free(outbuf);
        }

        elem->buf = tempbuf;
        elem->buf_size = len;
    }
    elem->buf_id = f_id;
    elem->ctx_id = ctx_id;

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);

    TRACE("[%s] leave:\n", __func__);
}

static AVCodecParserContext *new_codec_parser_init(AVCodecContext *avctx)
{
    AVCodecParserContext *parser = NULL;

    if (!avctx) {
        ERR("context is NULL\n");
        return NULL;
    }

    switch (avctx->codec_id) {
    case CODEC_ID_MPEG4:
    case CODEC_ID_VC1:
        TRACE("not using parser.\n");
        break;
    case CODEC_ID_H264:
        if (avctx->extradata_size == 0) {
            TRACE("H.264 with no extradata, creating parser.\n");
            parser = av_parser_init (avctx->codec_id);
        }
        break;
    default:
        parser = av_parser_init (avctx->codec_id);
        if (parser) {
            INFO("using parser. %d\n", avctx->codec_id);
        }
        break;
    }

    return parser;
}

static int new_codec_parser_parse(AVCodecParserContext *pctx, AVCodecContext *avctx,
                            uint8_t *inbuf, int inbuf_size,
                            int64_t pts, int64_t dts, int64_t pos)
{
    int ret = 0;
    uint8_t *outbuf;
    int outbuf_size;

    if (!avctx || !pctx) {
        ERR("Codec or Parser Context is empty\n");
        return -1;
    }

    ret = av_parser_parse2(pctx, avctx, &outbuf, &outbuf_size,
            inbuf, inbuf_size, pts, dts, pos);

    INFO("after parsing, idx: %d, outbuf size: %d, inbuf_size: %d, ret: %d\n",
        pts, outbuf_size, inbuf_size, ret);

    return ret;
}

static void *new_codec_pop_readqueue(NewCodecState *s, int32_t file_index)
{
    DeviceMemEntry *elem = NULL;

    qemu_mutex_lock(&s->context_queue_mutex);
    elem = QTAILQ_FIRST(&codec_rq);
    qemu_mutex_unlock(&s->context_queue_mutex);
    if (!elem) {
        ERR("codec_rq is empty.\n");
        return NULL;
    }

#if 0
    if (!elem->buf || (elem->buf_size == 0)) {
        ERR("cannot copy data to guest\n");
        return NULL;
    }
#endif

#if 0
    if (elem->buf_id != file_index) {
        DeviceMemEntry *next = NULL;
        TRACE("failed to pop codec_rq because of file index.\n");
        TRACE("head node file: %p, current file: %p\n", elem->buf_id, file_index);
        next = QTAILQ_NEXT(elem, node);
        if (next) {
            TRACE("next node file: %p\n", next->buf_id);
        }
        return NULL;
    }
#endif

//    TRACE("pop codec_rq. id: %x buf_size: %d\n",
//        elem->buf_id, elem->buf_size);

    return elem;
}

static void new_codec_pop_writequeue(NewCodecState *s, int32_t file_index)
{
    DeviceMemEntry *elem = NULL;
    uint32_t mem_offset = 0, ctx_idx;

    TRACE("enter: %s\n", __func__);

#if 1
    for (ctx_idx = 1; ctx_idx < CODEC_CONTEXT_MAX; ctx_idx++) {
        if (s->context[ctx_idx].file_index == file_index) {
            break;
        }
    }
#endif

    if (ctx_idx == CODEC_CONTEXT_MAX) {
        ERR("failed to find a proper entry via file_index. %x\n", file_index);
        return;
    }
    TRACE("pop_writeqeue. context index: %d\n", ctx_idx);

#if 0
    if (ctx_idx > 1) {
        ERR("[caramis] %d : 0x%x\n", ctx_idx, s->ioparam.mem_offset);
    }
#endif

    elem = entry[ctx_idx];
    if (elem) {
        mem_offset = s->ioparam.mem_offset;

        TRACE("write data as many as %d to guest,  mem_offset: 0x%x\n",
            elem->buf_size, mem_offset);
        memcpy(s->vaddr + mem_offset, elem->buf, elem->buf_size);

        if (elem->buf) {
            TRACE("pop_writequeue. release buffer: %p\n", elem->buf);
            g_free(elem->buf);
        }

        TRACE("pop_writequeue. release elem: %p\n", elem);
        g_free(elem);
        entry[ctx_idx] = NULL;
    } else {
        ERR("failed to get ioparam from file_index %x\n", file_index);
    }

    TRACE("leave: %s\n", __func__);
}

/*
 *  Codec Device APIs
 */
static uint64_t new_codec_read(void *opaque, target_phys_addr_t addr, unsigned size)
{
    NewCodecState *s = (NewCodecState *)opaque;
    uint64_t ret = 0;

    switch (addr) {
    case CODEC_CMD_GET_THREAD_STATE:
        if (s->wrk_thread.state) {
            s->wrk_thread.state = CODEC_TASK_INIT;

            qemu_mutex_lock(&s->context_queue_mutex);
            if (!QTAILQ_EMPTY(&codec_wq)) {
                ret = CODEC_TASK_END;
            }
            qemu_mutex_unlock(&s->context_queue_mutex);
        }
        TRACE("get thread_state. ret: %d\n", ret);
        qemu_irq_lower(s->dev.irq[0]);
        break;

    case CODEC_CMD_GET_QUEUE:
    {
        DeviceMemEntry *head = NULL;

        qemu_mutex_lock(&s->context_queue_mutex);
        head = QTAILQ_FIRST(&codec_wq);
        qemu_mutex_unlock(&s->context_queue_mutex);
        if (head) {
            ret = head->ctx_id;
            qemu_mutex_lock(&s->context_queue_mutex);
            QTAILQ_REMOVE(&codec_wq, head, node);
            qemu_mutex_unlock(&s->context_queue_mutex);
            entry[ret] = head;
//            qemu_mutex_unlock(&s->wrk_thread.mutex);
            TRACE("get a elem from codec_wq. 0x%x\n", head);
        } else {
            ret = 0;
        }
        TRACE("get a head from a writequeue. head: %x\n", ret);
    }
        break;

    case CODEC_CMD_GET_VERSION:
        ret = NEW_CODEC_VERSION;
        TRACE("codec version: %d\n", ret);
        break;

    case CODEC_CMD_GET_ELEMENT:
        ret = new_codec_query_list(s);
        break;

    case CODEC_CMD_GET_CONTEXT_INDEX:
        ret = new_codec_get_context_index(s);
//        TRACE("get context index: %d\n", ret);
        break;

    default:
        ERR("no avaiable command for read. %d\n", addr);
    }

    return ret;
}

static void new_codec_write(void *opaque, target_phys_addr_t addr,
                uint64_t value, unsigned size)
{
    NewCodecState *s = (NewCodecState *)opaque;

    switch (addr) {
    case CODEC_CMD_API_INDEX:
        TRACE("set codec_cmd value: %d\n", value);
        s->ioparam.api_index = value;
        new_codec_wakeup_thread(s, value);
        break;

    case CODEC_CMD_CONTEXT_INDEX:
        TRACE("set context_index value: %d\n", value);
        s->ioparam.ctx_index = value;
///        new_codec_switch_thread(s, value);
        break;

    case CODEC_CMD_FILE_INDEX:
        TRACE("set file_index value: 0x%x\n", value);
        s->ioparam.file_index = value;
        break;

    case CODEC_CMD_DEVICE_MEM_OFFSET:
        TRACE("set mem_offset value: 0x%x\n", value);
#if 0
        if (value == NEW_CODEC_MEM_SIZE) {
            // FIXME: how to handle this case?
            ERR("memory offset is overflow. set offset to 0.\n");
            s->ioparam.mem_offset = 0;
        } else {
            s->ioparam.mem_offset = value;
        }
#endif
        s->ioparam.mem_offset = value;
        break;

    case CODEC_CMD_RELEASE_CONTEXT:
        new_codec_release_context(s, (int32_t)value);
        break;

    case CODEC_CMD_POP_WRITE_QUEUE:
        new_codec_pop_writequeue(s, (uint32_t)value);
        break;
    default:
        ERR("no available command for write. %d\n", addr);
    }
}

static const MemoryRegionOps new_codec_mmio_ops = {
    .read = new_codec_read,
    .write = new_codec_write,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void new_codec_bh_callback(void *opaque)
{
    NewCodecState *s = (NewCodecState *)opaque;

    TRACE("enter: %s\n", __func__);

    qemu_mutex_lock(&s->context_queue_mutex);
    if (!QTAILQ_EMPTY(&codec_wq)) {
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("raise irq for shared task.\n");
        qemu_irq_raise(s->dev.irq[0]);
    } else {
        qemu_mutex_unlock(&s->context_queue_mutex);
        ERR("codec_wq is empty!!\n");
    }

    TRACE("leave: %s\n", __func__);
}

static int new_codec_initfn(PCIDevice *dev)
{
    NewCodecState *s = DO_UPCAST(NewCodecState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    INFO("device initialization.\n");
    memset(&s->ioparam, 0, sizeof(CodecParam));


    get_host_cpu_cores();

    qemu_mutex_init(&s->context_mutex);
    qemu_mutex_init(&s->context_queue_mutex);
    qemu_mutex_init(&s->ioparam_queue_mutex);

    new_codec_dedicated_thread_init(s);
    initialize_pixel_fmt_info();

    // register a function to qemu bottom-halves to switch context.
    s->codec_bh = qemu_bh_new(new_codec_bh_callback, s);

    pci_config_set_interrupt_pin(pci_conf, 1);

    memory_region_init_ram(&s->vram, "new_codec.vram", NEW_CODEC_MEM_SIZE);
    s->vaddr = (uint8_t *)memory_region_get_ram_ptr(&s->vram);

    memory_region_init_io(&s->mmio, &new_codec_mmio_ops, s,
                        "new_codec.mmio", NEW_CODEC_REG_SIZE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->vram);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    return 0;
}

static void new_codec_exitfn(PCIDevice *dev)
{
    NewCodecState *s = DO_UPCAST(NewCodecState, dev, dev);
    INFO("device exit\n");

    qemu_mutex_destroy(&s->context_mutex);
    qemu_mutex_destroy(&s->context_queue_mutex);
    qemu_mutex_destroy(&s->ioparam_queue_mutex);

    qemu_bh_delete(s->codec_bh);

    memory_region_destroy(&s->vram);
    memory_region_destroy(&s->mmio);
}

int new_codec_device_init(PCIBus *bus)
{
    INFO("device create.\n");
    pci_create_simple(bus, -1, NEW_CODEC_DEV_NAME);
    return 0;
}

static void new_codec_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = new_codec_initfn;
    k->exit = new_codec_exitfn;
    k->vendor_id = PCI_VENDOR_ID_TIZEN;
    k->device_id = PCI_DEVICE_ID_VIRTUAL_NEW_CODEC;
    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "Virtual new codec device for Tizen emulator";
}

static TypeInfo codec_info = {
    .name          = NEW_CODEC_DEV_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(NewCodecState),
    .class_init    = new_codec_class_init,
};

static void codec_register_types(void)
{
    type_register_static(&codec_info);
}

type_init(codec_register_types)
