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
#include "qemu-common.h"
#include "qemu-thread.h"
#include "tizen/src/debug_ch.h"
#include <libavformat/avformat.h>
#include "maru_new_codec.h"

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, new_codec);

#define NEW_CODEC_DEV_NAME     "new_codec"
#define NEW_CODEC_VERSION      1

/*  Needs 16M to support 1920x1080 video resolution.
 *  Output size for encoding has to be greater than (width * height * 6)
 */
#define NEW_CODEC_MEM_SIZE     (2 * 16 * 1024 * 1024)
#define NEW_CODEC_REG_SIZE     (256)

#define GEN_MASK(x) ((1<<(x))-1)
#define ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) & ~GEN_MASK(x))
#define ROUND_UP_2(x) ROUND_UP_X(x, 1)
#define ROUND_UP_4(x) ROUND_UP_X(x, 2)
#define ROUND_UP_8(x) ROUND_UP_X(x, 3)
#define DIV_ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) >> (x))

typedef struct DeviceMemEntry {
    uint8_t *buf;
    uint32_t buf_size;
    uint32_t buf_id;

    QTAILQ_ENTRY(DeviceMemEntry) node;   
} DeviceMemEntry;

static QTAILQ_HEAD(codec_rq, DeviceMemEntry) codec_rq =
   QTAILQ_HEAD_INITIALIZER(codec_rq);

static QTAILQ_HEAD(codec_wq, DeviceMemEntry) codec_wq =
   QTAILQ_HEAD_INITIALIZER(codec_wq);

static QTAILQ_HEAD(codec_pop_wq, DeviceMemEntry) codec_pop_wq =
   QTAILQ_HEAD_INITIALIZER(codec_pop_wq);

static QTAILQ_HEAD(codec_irq_queue, DeviceMemEntry) codec_irq_queue =
   QTAILQ_HEAD_INITIALIZER(codec_irq_queue);

typedef struct PixFmtInfo {
    uint8_t x_chroma_shift;
    uint8_t y_chroma_shift;
} PixFmtInfo;

static PixFmtInfo pix_fmt_info[PIX_FMT_NB];

static void new_codec_thread_init(NewCodecState *s)
{
    int index = 0;
    QemuThread *pthread = NULL;
    TRACE("Enter, %s\n", __func__);

    pthread = g_malloc0(sizeof(QemuThread) * CODEC_WORK_THREAD_MAX);
    if (!pthread) {
        ERR("Failed to allocate wrk_thread memory.\n");
        return;
    }
    qemu_cond_init(&s->wrk_thread.cond);
    qemu_mutex_init(&s->wrk_thread.mutex);

    qemu_mutex_lock(&s->codec_mutex);
    s->isrunning = 1;
    qemu_mutex_unlock(&s->codec_mutex);

    for (; index < CODEC_WORK_THREAD_MAX; index++) {
        qemu_thread_create(&pthread[index],
            new_codec_dedicated_thread, (void *)s, QEMU_THREAD_JOINABLE);
    }

    s->wrk_thread.wrk_thread = pthread;
    TRACE("Leave, %s\n", __func__);
}

static void new_codec_thread_exit(NewCodecState *s)
{
    TRACE("Enter, %s\n", __func__);
    int index;

    /* stop to run dedicated threads. */
    s->isrunning = 0;

    for (index = 0; index < CODEC_WORK_THREAD_MAX; index++) {
        qemu_thread_join(&s->wrk_thread.wrk_thread[index]);
    }

    TRACE("destroy mutex and conditional.\n");
    qemu_mutex_destroy(&s->wrk_thread.mutex);
    qemu_cond_destroy(&s->wrk_thread.cond);

    if (s->wrk_thread.wrk_thread) {
        g_free (s->wrk_thread.wrk_thread);
        s->wrk_thread.wrk_thread = NULL;
    }

    TRACE("Leave, %s\n", __func__);
}

void new_codec_wakeup_thread_and_run(NewCodecState *s, int api_index)
{
    uint32_t mem_offset, mem_type;
    uint8_t *device_mem = NULL;
   
    mem_offset = s->ioparam.mem_offset;
    mem_type = s->ioparam.mem_type;
    TRACE("mem_offset: %d, mem_type: %d\n", mem_offset, mem_type);

    device_mem = (uint8_t *)s->vaddr + mem_offset;
    if (!device_mem) {
        ERR("[%d] device memory mapped region is null\n");
        return;
    }

    if (mem_type == CODEC_SHARED_DEVICE_MEM) {
        TRACE("memory type is CODEC_SHARED_DEVICE_MEM.\n");

        /* in case of CODEC_PICTURE_COPY, there is no data from guest. */
        switch (api_index) {
        case CODEC_INIT ... CODEC_ENCODE_AUDIO:
            {
                DeviceMemEntry *elem = NULL;
                int readbuf_size, size;
                uint8_t *readbuf = NULL, *device_mem = NULL;

                elem = g_malloc0(sizeof(DeviceMemEntry));
                if (!elem) {
                    ERR("[%d] failed to allocate memory. size: %d\n",
                            __LINE__, sizeof(DeviceMemEntry));
                    return;
                }

                memcpy(&readbuf_size, device_mem, sizeof(readbuf_size));
                TRACE("readbuf size: %d\n", readbuf_size);
                if (readbuf_size < 0) {
                    ERR("readbuf size is negative or oversize\n");
                    return;
                }

                size = sizeof(readbuf_size);
                readbuf = g_malloc0(readbuf_size);
                if (!readbuf) {
                    ERR("failed to get context data.\n");
                    return;
                }
                memcpy(readbuf, device_mem + size, readbuf_size);

                elem->buf = readbuf;
                elem->buf_size = readbuf_size;
                elem->buf_id = s->ioparam.file_index;

                qemu_mutex_lock(&s->codec_job_queue_mutex);
                QTAILQ_INSERT_TAIL(&codec_rq, elem, node);
                qemu_mutex_unlock(&s->codec_job_queue_mutex);
            }
            break;
        case CODEC_PICTURE_COPY ... CODEC_DEINIT: 
            TRACE("don't need to get data from guest. %d\n", api_index);
            break;
        default:
            TRACE("invalid api index: %d.\n", api_index);
            break;
        }
    } else {
        TRACE("memory type is CODEC_FIXED_DEVICE_MEM.\n");
    }

    TRACE("api index: %d.\n", api_index);
    s->ioparam.api_index = api_index;
    qemu_cond_signal(&s->wrk_thread.cond);
}

void *new_codec_dedicated_thread(void *opaque)
{
    NewCodecState *s = (NewCodecState *)opaque;

    TRACE("Enter, %s\n", __func__);

    qemu_mutex_lock(&s->codec_mutex);
    while (s->isrunning) {
        int mem_type;
 
        qemu_cond_wait(&s->wrk_thread.cond, &s->codec_mutex);
        mem_type = new_codec_api_selection(s);
#if 0 
        TRACE("change task state.\n");
        if (mem_type == CODEC_FIXED_DEVICE_MEM) {
            qemu_mutex_lock(&s->wrk_thread.mutex);
            s->wrk_thread.state = CODEC_FIXED_TASK_FIN;
            qemu_mutex_unlock(&s->wrk_thread.mutex);
        } else if (mem_type == CODEC_SHARED_DEVICE_MEM) {
            qemu_mutex_lock(&s->wrk_thread.mutex);
            s->wrk_thread.state = CODEC_SHARED_TASK_FIN;
            qemu_mutex_unlock(&s->wrk_thread.mutex);
        }
#endif
        qemu_bh_schedule(s->codec_bh);
    }
    qemu_mutex_unlock(&s->codec_mutex);
    new_codec_thread_exit(s);

    TRACE("Leave, %s\n", __func__);
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
    if (video->pix_fmt) {
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
    if (audio->channels) {
        avctx->channels = audio->channels;
    }
    if (audio->sample_rate) {
        avctx->sample_rate = audio->sample_rate;
    }
    if (audio->block_align) {
        avctx->block_align = audio->block_align;
    }
    if (audio->bit_rate) {
        avctx->bit_rate = audio->bit_rate;
    }
    if (audio->sample_fmt) {
        avctx->sample_fmt = audio->sample_fmt;
    }
}

int new_codec_get_context_index_from_file(NewCodecState *s,
                                            uint32_t file_index)
{
    int ctx_idx = 0, ret;


    for (; ctx_idx < CODEC_CONTEXT_MAX; ctx_idx++) {
        if (s->codec_ctx[ctx_idx].file_index == file_index) {
            ret = ctx_idx;
            TRACE("get context index: %d successfully.\n", ctx_idx);
            break;
        }
    }

    if (ctx_idx == CODEC_CONTEXT_MAX) {
        ERR("failed to get context index from file index\n");
        ret = -1;
    }
    return ret;
}

void new_codec_release_queue_buf(DeviceMemEntry *elem)
{
    if (elem->buf) {
        TRACE("[%d] release buf.\n", __LINE__);
        g_free(elem->buf);
    } else {
        TRACE("[%d] not release buf.\n", __LINE__);
    }

    if (elem) {
        TRACE("[%d] release elem.\n", __LINE__);
        g_free(elem);
    } else {
        TRACE("[%d] not release elem.\n", __LINE__);
    }
}

uint8_t *new_codec_get_memory_buf(NewCodecState *s, uint8_t mem_type,
                                uint32_t mem_offset, int32_t file_index) 
{
    DeviceMemEntry *elem;
    uint8_t *buf;

    if (mem_type == CODEC_FIXED_DEVICE_MEM) { 
        buf = s->vaddr + mem_offset + sizeof(int);
    } else {
        TRACE("[%s][%d] pop codec_rq.\n", __func__, __LINE__);
        elem = (DeviceMemEntry *)new_codec_pop_readqueue(s, file_index);
        if (!elem) {
            ERR("[%s][%d] failed to pop from readqueue.\n", __func__, __LINE__);
            return NULL;
        }

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_REMOVE (&codec_rq, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);

        buf = elem->buf;
    }

    return buf;
}

void new_codec_reset_parser_info(NewCodecState *s, int32_t ctx_index)
{
    TRACE("[%s] Enter\n", __func__);

    s->codec_ctx[ctx_index].parser_buf = NULL;
    s->codec_ctx[ctx_index].parser_use = false;

    TRACE("[%s] Leave\n", __func__);
}

void new_codec_reset_codec_context(NewCodecState *s, int32_t file_index)
{
    DeviceMemEntry *rq_elem, *wq_elem, *pop_wq_elem;
    DeviceMemEntry *next;
    int ctx_idx;

    TRACE("[%s] Enter\n", __func__);
    qemu_mutex_lock(&s->wrk_thread.mutex);

    for (ctx_idx = 0; ctx_idx < CODEC_CONTEXT_MAX; ctx_idx++) {
        if (s->codec_ctx[ctx_idx].file_index == file_index) {
            TRACE("reset %d context\n", ctx_idx);
            s->codec_ctx[ctx_idx].avctx_use = false;
            break;
        }
    }

    QTAILQ_FOREACH_SAFE(rq_elem, &codec_rq, node, next) {
        if (rq_elem && rq_elem->buf_id == file_index) {
            INFO("remove unused node from codec_rq. file: %p\n", file_index);
            qemu_mutex_lock(&s->codec_job_queue_mutex);
            QTAILQ_REMOVE(&codec_rq, rq_elem, node);
            qemu_mutex_unlock(&s->codec_job_queue_mutex);
            if (rq_elem->buf) {
                TRACE("[%d] release buf.\n", __LINE__);
                g_free(rq_elem->buf);
            }

            if (rq_elem) {
                TRACE("[%d] release elem.\n", __LINE__);
                g_free(rq_elem);
            }
        } else {
            TRACE("remain this node in the codec_rq. :%x\n", rq_elem->buf_id);
        }
    }

    QTAILQ_FOREACH_SAFE(wq_elem, &codec_wq, node, next) {
        if (wq_elem && wq_elem->buf_id == file_index) {
            INFO("remove nodes from codec_wq. file: %p\n", file_index);

            qemu_mutex_lock(&s->codec_job_queue_mutex);
            QTAILQ_REMOVE(&codec_wq, wq_elem, node);
            qemu_mutex_unlock(&s->codec_job_queue_mutex);
            if (wq_elem->buf) {
                TRACE("[%d] release buf.\n", __LINE__);
                g_free(wq_elem->buf);
            }

            if (wq_elem) {
                TRACE("[%d] release elem.\n", __LINE__);
                g_free(wq_elem);
            }

        } else {
            TRACE("remain this node in the codec_wq. :%x\n", wq_elem->buf_id);
        }
    }

    QTAILQ_FOREACH_SAFE(pop_wq_elem, &codec_pop_wq, node, next) {
        if (pop_wq_elem && pop_wq_elem->buf_id == file_index) {
            INFO("remove nodes from codec_pop_wq. file: %p\n", file_index);

            qemu_mutex_lock(&s->codec_job_queue_mutex);
            QTAILQ_REMOVE(&codec_pop_wq, pop_wq_elem, node);
            qemu_mutex_unlock(&s->codec_job_queue_mutex);
            if (pop_wq_elem->buf) {
                TRACE("[%d] release buf.\n", __LINE__);
                g_free(pop_wq_elem->buf);
            }

            if (pop_wq_elem) {
                TRACE("[%d] release elem.\n", __LINE__);
                g_free(pop_wq_elem);
            }

        } else {
            TRACE("remain this node in the codec_pop_wq. :%x\n",
                pop_wq_elem->buf_id);
        }
    }

    new_codec_reset_parser_info(s, ctx_idx);

    qemu_mutex_unlock(&s->wrk_thread.mutex);
    TRACE("[%s] Leave\n", __func__);
}

int new_avcodec_get_buffer(AVCodecContext *context, AVFrame *picture)
{
    int ret;
    TRACE("avcodec_default_get_buffer\n");

    picture->reordered_opaque = context->reordered_opaque;
    picture->opaque = NULL;

    ret = avcodec_default_get_buffer(context, picture);

    return ret;
}

void new_avcodec_release_buffer(AVCodecContext *context, AVFrame *picture)
{
    TRACE("avcodec_default_release_buffer\n");
    avcodec_default_release_buffer(context, picture);
}

static void new_init_pix_fmt_info(void)
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
}

static int new_avcodec_picture_get_size(AVPicture *picture, uint8_t *ptr,
                                int pix_fmt, int width,
                                int height, bool encode)
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
            TRACE("calculate encoded picture.\n");
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
        TRACE("strides %d %d %d\n", stride, stride2, stride2);
        break;
    default:
        picture->data[0] = NULL;
        picture->data[1] = NULL;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        fsize = -1;
        ERR("pixel format: %d was wrong.\n", pix_fmt);
    }

    return fsize;
}

// FFmpeg Functions
int new_avcodec_query_list (NewCodecState *s)
{
    AVCodec *codec = NULL;
    uint32_t size = 0, mem_size = 0;
    uint32_t data_length = 0;

    /* register avcodec */
    TRACE("register avcodec\n");
    av_register_all();

    codec = av_codec_next(NULL);
    if (!codec) {
        ERR("failed to get codec info.\n");
        return -1;
    }

    // a region to store the number of codecs.
    data_length = 32 + 64 + 2 * sizeof(uint16_t);
    while (codec) {
        uint16_t codec_type =
            codec->decode ? CODEC_TYPE_DECODE : CODEC_TYPE_ENCODE;
        uint16_t media_type = codec->type;

        memset ((uint8_t *)s->vaddr + mem_size,
            0x00, (data_length + sizeof(data_length)));
        mem_size += (data_length + sizeof(data_length));
       
        memcpy((uint8_t *)s->vaddr + size, &data_length, sizeof(uint32_t));
        size += sizeof(uint32_t);
        memcpy((uint8_t *)s->vaddr + size, &codec_type, sizeof(codec_type));
        size += sizeof(codec_type);
        memcpy((uint8_t *)s->vaddr + size, &media_type, sizeof(media_type));
        size += sizeof(media_type);
        memcpy((uint8_t *)s->vaddr + size, codec->name, strlen(codec->name));
        size += 32;
        memcpy((uint8_t *)s->vaddr + size,
                codec->long_name, strlen(codec->long_name));
        size += 64;

        codec = av_codec_next(codec);
    }

//    data_length = 0;
//    memcpy((uint8_t *)s->vaddr + size, &data_length, sizeof(uint32_t));
    memset((uint8_t *)s->vaddr + size, 0x00, sizeof(uint32_t));

    return 0;
}

int new_avcodec_alloc_context(NewCodecState *s)
{
    int index;

    TRACE("[%s] Enter\n", __func__);

    for (index = 0; index < CODEC_CONTEXT_MAX; index++) {
        if (s->codec_ctx[index].avctx_use == false) {
            TRACE("Succeeded to get %d of context.\n", index);
            s->codec_ctx[index].avctx_use = true;
            break;
        }
    }

    if (index == CODEC_CONTEXT_MAX) {
        ERR("Failed to get available codec context.");
        ERR(" Try to run codec again.\n");
        return -1;
    }

    TRACE("allocate %d of context and frame.\n", index);
    s->codec_ctx[index].avctx = avcodec_alloc_context();
    s->codec_ctx[index].frame = avcodec_alloc_frame();

    TRACE("ioparam file_index: %p\n", s->ioparam.file_index);
    s->codec_ctx[index].file_index = s->ioparam.file_index;
    new_codec_reset_parser_info(s, index);
    new_init_pix_fmt_info();

    TRACE("[%s] Leave\n", __func__);

    return index;
}

#if 0
void new_avcodec_flush_buffers(NewCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;

    TRACE("Enter\n");
    qemu_mutex_lock(&s->wrk_thread.mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    if (avctx) {
        avcodec_flush_buffers(avctx);
    } else {
        ERR("[%s] %d of AVCodecContext is NULL\n", __func__, ctx_index);
    }

    qemu_mutex_unlock(&s->wrk_thread.mutex);
    TRACE("[%s] Leave\n", __func__);
}
#endif

void new_avcodec_init_to(AVCodecContext *avctx, int ret, int ctx_index, uint8_t *mem_buf)
{
    int size = 0;

    memcpy(mem_buf, &ret, sizeof(ret));
    size = sizeof(ret);
    if (!ret) {
        if (avctx->codec->type == AVMEDIA_TYPE_AUDIO) {
            memcpy(mem_buf + size,
                &avctx->sample_fmt, sizeof(avctx->sample_fmt));
            size += sizeof(avctx->sample_fmt);
        }
        memcpy(mem_buf + size, &ctx_index, sizeof(ctx_index));
    }
}

void new_avcodec_decode_video_to(AVCodecContext *avctx, int len, int got_pic_ptr, uint8_t *mem_buf)
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

void new_avcodec_decode_audio_to(AVCodecContext *avctx, int len, int frame_size_ptr, int16_t *samples, int outbuf_size, uint8_t *mem_buf)
{
    int size = 0;

    memcpy(mem_buf, &avctx->channel_layout, sizeof(avctx->channel_layout));
    size = sizeof(avctx->channel_layout);
    memcpy(mem_buf + size, &len, sizeof(len));
    size += sizeof(len);
    memcpy(mem_buf + size, &frame_size_ptr, sizeof(frame_size_ptr));
    size += sizeof(frame_size_ptr);
    if (len > 0) {
        memcpy(mem_buf + size, samples, outbuf_size);
    }
}


int new_avcodec_init(NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx = NULL;
    AVCodecParserContext *parser = NULL;
    AVCodec *codec = NULL;
    uint16_t media_type, encode;
    char codec_name[32] = {0, };
    int size = 0, ctx_index = 0, file_index, ret;
    struct video_data video;
    struct audio_data audio;
    uint8_t *mem_buf = NULL;
   
    TRACE("enter: %s\n", __func__);
    memset (&video, 0, sizeof(struct video_data));
    memset (&audio, 0, sizeof(struct audio_data));

    TRACE("allocate context and frame\n");
    ctx_index = new_avcodec_alloc_context(s);

    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%d] failed to allocate context.\n", __LINE__);
        return -1;
    }

    file_index = s->codec_ctx[ctx_index].file_index;
    mem_buf = new_codec_get_memory_buf(s, ioparam->mem_type,
                                    ioparam->mem_offset, file_index);

    memcpy(&encode, mem_buf, sizeof(encode));
    size = sizeof(encode);
    memcpy(&media_type, mem_buf + size, sizeof(media_type));
    size += sizeof(media_type);
    memcpy(codec_name, mem_buf + size, sizeof(codec_name));
    size += sizeof(codec_name);

    if (media_type == AVMEDIA_TYPE_VIDEO) {
        memcpy(&video, mem_buf + size, sizeof(video));
        size += sizeof(video);
    } else if (media_type == AVMEDIA_TYPE_AUDIO) {
        memcpy(&audio, mem_buf + size, sizeof(audio));
        size += sizeof(audio);
    } else {
        ERR("unknown media type.\n");
        return -1;
    }

    memcpy(&avctx->extradata_size,
        mem_buf + size, sizeof(avctx->extradata_size));
    size += sizeof(avctx->extradata_size);
    if (avctx->extradata_size > 0) {
        TRACE("extradata size: %d.\n", avctx->extradata_size);
        avctx->extradata =
            g_malloc0 (ROUND_UP_X(avctx->extradata_size +
                FF_INPUT_BUFFER_PADDING_SIZE, 4));
        if (avctx->extradata) {
            memcpy(avctx->extradata, mem_buf + size, avctx->extradata_size);
        }
    } else {
        TRACE("no extra data.\n");
        avctx->extradata =
            g_malloc0 (ROUND_UP_X(FF_INPUT_BUFFER_PADDING_SIZE, 4));
    }

    if (!avctx->extradata) {
        ERR("[%d] failed to allocate memory!!\n", __LINE__);
    }

    TRACE("init media_type: %d, codec_type: %d, name: %s\n",
        media_type, encode, codec_name);

    if (encode) {
        codec = avcodec_find_encoder_by_name (codec_name);
    } else {
        codec = avcodec_find_decoder_by_name (codec_name);
    }

    if (!codec) {
        ERR("Failed to get codec for %s.\n", codec_name);
    }

    // initialize codec context.
    if (media_type == AVMEDIA_TYPE_VIDEO) {
        new_serialize_video_data(&video, avctx);
    } else if (media_type == AVMEDIA_TYPE_AUDIO) {
        new_serialize_audio_data(&audio, avctx);
    }
    avctx->workaround_bugs |= FF_BUG_AUTODETECT;
    avctx->error_recognition = 1;

    ret = avcodec_open(avctx, codec);
    if (ret < 0) {
        ERR("Failed to open codec contex.\n");
    }
    TRACE("avcodec_open done: %d\n", ret);

    if (ioparam->mem_type == CODEC_FIXED_DEVICE_MEM) {
        DeviceMemEntry *elem = NULL;

        mem_buf = s->vaddr + ioparam->mem_offset;
        new_avcodec_init_to(avctx, ret, ctx_index, mem_buf);

        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return -1;
        }

        elem->buf_id = s->codec_ctx[ctx_index].file_index;

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_irq_queue, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    } else {
        DeviceMemEntry *elem = NULL;
        int tempbuf_size = 0;
        uint8_t *tempbuf = NULL;

        new_codec_release_queue_buf(elem);

        TRACE("push data into codec_wq\n");
        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return -1;
        }

        tempbuf_size = sizeof(ret) + sizeof(ctx_index);
        if (codec->type == AVMEDIA_TYPE_AUDIO) {
            TRACE("after avcodec_open, sample_fmt: %d\n", avctx->sample_fmt);
            tempbuf_size += sizeof(avctx->sample_fmt);
        }
        TRACE("[%s] tempbuf size: %d\n", __func__, tempbuf_size);

        tempbuf = g_malloc0(tempbuf_size);
        if (!tempbuf) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, tempbuf_size);
            g_free(elem);
            return -1;
        }

        new_avcodec_init_to(avctx, ret, ctx_index, tempbuf);
        TRACE("before push codec_wq, file_index of %d: %p\n",
            ctx_index, s->codec_ctx[ctx_index].file_index);

        elem->buf = tempbuf;
        elem->buf_size = tempbuf_size;
        elem->buf_id = s->codec_ctx[ctx_index].file_index;
        TRACE("push codec_wq, buf_size: %d\n", tempbuf_size);

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    }

    parser = new_avcodec_parser_init(avctx);
    s->codec_ctx[ctx_index].parser_ctx = parser;

    TRACE("leave: %s\n", __func__);

    return 0;
}

void new_avcodec_deinit(NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx;
    AVFrame *frame;
    AVCodecParserContext *parserctx;
    int ret, ctx_index;

    TRACE("enter: %s\n", __func__);
    ctx_index = ioparam->ctx_index;

    avctx = s->codec_ctx[ctx_index].avctx;
    frame = s->codec_ctx[ctx_index].frame;
    parserctx = s->codec_ctx[ctx_index].parser_ctx;
    if (!avctx || !frame) {
        ERR("context or frame %d is NULL.\n", ctx_index);
        return;
    }

    ret = avcodec_close(avctx);
    TRACE("Close codec context of %d.\n", ctx_index);

    if (avctx->extradata) {
        TRACE("free context extradata\n");
        av_free(avctx->extradata);
        s->codec_ctx[ctx_index].avctx->extradata = NULL;
    }

    if (avctx->palctrl) {
        TRACE("free context palctrl \n");
        av_free(avctx->palctrl);
        s->codec_ctx[ctx_index].avctx->palctrl = NULL;
    }

    if (frame) {
        TRACE("free frame\n");
        av_free(frame);
        s->codec_ctx[ctx_index].frame = NULL;
    }

    if (avctx) {
        TRACE("free codec context\n");
        av_free(avctx);
        s->codec_ctx[ctx_index].avctx = NULL;
    }

    if (parserctx) {
        av_parser_close(parserctx);
        s->codec_ctx[ctx_index].parser_ctx = NULL;
    }

    TRACE("leave: %s\n", __func__);
}

int new_avcodec_decode_video(NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx = NULL;
    AVCodecParserContext *pctx = NULL;
    AVFrame *picture = NULL;
    AVPacket avpkt;
    int got_pic_ptr = 0, len = 0;
    uint8_t *inbuf = NULL;
    int inbuf_size;
    int size = 0, file_index, ctx_index;
    uint8_t *mem_buf = NULL;

    TRACE("enter: %s\n", __func__);
    ctx_index = ioparam->ctx_index;

    avctx = s->codec_ctx[ctx_index].avctx;
    picture = s->codec_ctx[ctx_index].frame;
    pctx = s->codec_ctx[ctx_index].parser_ctx;
    if (!avctx || !picture) {
        ERR("context or frame %d is NULL.\n", ctx_index);
        return -1;
    }

    file_index = s->codec_ctx[ctx_index].file_index;
    mem_buf = new_codec_get_memory_buf(s, ioparam->mem_type,
                                    ioparam->mem_offset, file_index);

    memcpy(&inbuf_size, mem_buf, sizeof(inbuf_size));
    size = sizeof(inbuf_size);
    if (inbuf_size > 0) {
        TRACE("input buffer size: %d.\n", inbuf_size);
        inbuf = (uint8_t *)mem_buf + size;
    } else {
        TRACE("There is no input buffer.\n");
        inbuf = NULL;
    }

#if 0
    if (pctx) {
        int parser_ret;
        parser_ret =
            new_avcodec_parser_parse (pctx, avctx, inbuf, inbuf_size,
                                    outbuf, outbuf_size, pts, dts, pos);
        TRACE("returned parser: %d.\n", parse_ret);
    }
#endif

    memset(&avpkt, 0x00, sizeof(AVPacket));
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;

    len = avcodec_decode_video2(avctx, picture, &got_pic_ptr, &avpkt);
    TRACE("after decoding video. len: %d, have_data: %d, pix_fmt: %d\n", len, got_pic_ptr, avctx->pix_fmt);

    if (len < 0) {
        ERR("failed to decode a frame. len: %d\n", len);
    }

    if (ioparam->mem_type == CODEC_FIXED_DEVICE_MEM) {
        DeviceMemEntry *elem = NULL;

        mem_buf = s->vaddr + ioparam->mem_offset;
        new_avcodec_decode_video_to(avctx, len, got_pic_ptr, mem_buf);

        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return -1;
        }

        elem->buf_id = s->codec_ctx[ctx_index].file_index;

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_irq_queue, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    } else {
        DeviceMemEntry *elem = NULL;
        int tempbuf_size = 0;
        uint8_t *tempbuf = NULL;

        new_codec_release_queue_buf(elem);

        TRACE("push data into codec_wq\n");
        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return -1;
        }

        tempbuf_size =
            sizeof(len) + sizeof(got_pic_ptr) + sizeof(struct video_data);
        TRACE("[%s] tempbuf size: %d\n", __func__, tempbuf_size);

        tempbuf = g_malloc0(tempbuf_size);
        if (!tempbuf) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, tempbuf_size);
            return -1;
        }

        new_avcodec_decode_video_to(avctx, len, got_pic_ptr, tempbuf);
        TRACE("before push codec_wq, file_index of %d: %p\n", ctx_index, s->codec_ctx[ctx_index].file_index);

        elem->buf = tempbuf;
        elem->buf_size = tempbuf_size;
        elem->buf_id = s->codec_ctx[ctx_index].file_index;

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    }

    TRACE("leave: %s\n", __func__);

    return 0;
}

void new_avcodec_picture_copy (NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx;
    AVPicture *src;
    AVPicture dst;
    uint8_t *buffer;
    int pict_size, ctx_index = 0;

    TRACE("enter: %s\n", __func__);
    ctx_index = ioparam->ctx_index;

    TRACE("copy decoded image of %d context.\n", ctx_index);

    avctx = s->codec_ctx[ctx_index].avctx;
    src = (AVPicture *)s->codec_ctx[ctx_index].frame;
    if (!avctx || !src) {
        ERR("%d of context or frame is NULL.\n", ctx_index);
        return;
    }

    pict_size =
        new_avcodec_picture_get_size(&dst, NULL, avctx->pix_fmt,
                            avctx->width, avctx->height, false);

    TRACE("decoded image size, pix_fmt: %d width: %d, height: %d, pict_size: %d\n",
        avctx->pix_fmt, avctx->width, avctx->height, pict_size);

    if ((pict_size) < 0) {
        ERR ("picture size is negative.\n");
        return;
    }

    av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);
    buffer = dst.data[0];

    if (ioparam->mem_type == CODEC_FIXED_DEVICE_MEM) {
        DeviceMemEntry *elem = NULL;

        memcpy(s->vaddr + ioparam->mem_offset, buffer, pict_size);
        g_free(buffer);

        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return;
        }

        elem->buf_id = s->codec_ctx[ctx_index].file_index;

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_irq_queue, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    } else {
        DeviceMemEntry *elem = NULL;
        int tempbuf_size = 0;

        new_codec_release_queue_buf(elem);

        TRACE("push data into codec_wq\n");
        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return;
        }

        tempbuf_size = pict_size;
        TRACE("[%s] tempbuf size: %d\n", __func__, tempbuf_size);

        TRACE("before push codec_wq, file_index of %d: %p\n",
            ctx_index, s->codec_ctx[ctx_index].file_index);

        elem->buf = buffer;
        elem->buf_size = tempbuf_size;
        elem->buf_id = s->codec_ctx[ctx_index].file_index;
        TRACE("push codec_wq, buf_size: %d\n", tempbuf_size);
       
        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    }

    TRACE("leave: %s\n", __func__);
}

int new_avcodec_decode_audio(NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx;
    AVPacket avpkt;
    int16_t *samples;
    int frame_size_ptr;
    uint8_t *buf;
    uint8_t *parser_buf;
    bool parser_use;
    int buf_size, outbuf_size;
    int size, len, file_index, ctx_index = 0;
    uint8_t *mem_buf = NULL;

    TRACE("Enter, %s\n", __func__);
    ctx_index = ioparam->ctx_index;

    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of Context is NULL!\n", __func__, ctx_index);
        return -1;
    }

    if (s->codec_ctx[ctx_index].parser_ctx) {
        parser_buf = s->codec_ctx[ctx_index].parser_buf;
        parser_use = s->codec_ctx[ctx_index].parser_use;
    }

    file_index = s->codec_ctx[ctx_index].file_index;
    mem_buf = new_codec_get_memory_buf(s, ioparam->mem_type,
                                    ioparam->mem_offset, file_index);

    memcpy(&buf_size, mem_buf, sizeof(buf_size));
    size = sizeof(int);

    TRACE ("before decoding audio. inbuf_size: %d\n", buf_size);
#if 0
    if (parser_buf && parser_use) {
        TRACE("[%s] use parser, buf:%p codec_id:%x\n",
                __func__, parser_buf, avctx->codec_id);
        buf = parser_buf;
    } else if (buf_size > 0) {
#endif
    if (buf_size > 0) {
        buf = mem_buf + size;
    } else {
        TRACE("audio input buffer is NULL\n");
        buf = NULL;
    }

#if 0
    memcpy(&buf_size, (uint8_t *)s->vaddr, sizeof(int));
    size = sizeof(int);
    TRACE("input buffer size : %d\n", buf_size);

    if (parser_buf && parser_use) {
        TRACE("[%s] use parser, buf:%p codec_id:%x\n",
                __func__, parser_buf, avctx->codec_id);
        buf = parser_buf;
    } else if (buf_size > 0) {
        TRACE("[%s] not use parser, codec_id:%x\n", __func__, avctx->codec_id);
        buf = av_mallocz(buf_size);
        if (!buf) {
            buf = NULL;
        } else {
            memcpy(buf, (uint8_t *)s->vaddr + size, buf_size);
        }
//        buf = (uint8_t *)s->vaddr + size;
    } else {
        TRACE("no input buffer\n");
        buf = NULL;
    }
#endif

    av_init_packet(&avpkt);
    avpkt.data = buf;
    avpkt.size = buf_size;

    frame_size_ptr = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    outbuf_size = frame_size_ptr;
    samples = av_mallocz(frame_size_ptr);
    if (!samples) {
        ERR("[%d] failed to allocate memory\n", __LINE__);
        len = -1;   
    } else {
        len = avcodec_decode_audio3(avctx, samples, &frame_size_ptr, &avpkt);
    }

    TRACE("after decoding audio!, ret:%d. channel_layout: %ld\n", len, avctx->channel_layout);
    if (len < 0) {
        ERR("failed to decode audio\n", len);
    }
   
    if (ioparam->mem_type == CODEC_FIXED_DEVICE_MEM) {
        DeviceMemEntry *elem = NULL;

        mem_buf = s->vaddr + ioparam->mem_offset;
        new_avcodec_decode_audio_to(avctx, len, frame_size_ptr,
                        samples, outbuf_size, mem_buf);

        if (samples) {
            av_free(samples);
            TRACE("[%d] release audio outbuf.\n", __LINE__);
        }

        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return -1;
        }

        elem->buf_id = s->codec_ctx[ctx_index].file_index;

        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_irq_queue, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    } else {
        DeviceMemEntry *elem = NULL;
        int tempbuf_size = 0;
        uint8_t *tempbuf = NULL;

        new_codec_release_queue_buf(elem);

        TRACE("[decode_audio] push data into codec_wq\n");
        elem = g_malloc0(sizeof(DeviceMemEntry));
        if (!elem) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, sizeof(DeviceMemEntry));
            return -1;
        }

        tempbuf_size = sizeof(avctx->channel_layout) + sizeof(len) +
            sizeof(frame_size_ptr);
        if (len > 0) {
            tempbuf_size += outbuf_size;
        } 
        TRACE("[%s] tempbuf size: %d\n", __func__, tempbuf_size);

        tempbuf = g_malloc0(tempbuf_size);
        if (!tempbuf) {
            ERR("[%d] failed to allocate memory. size: %d\n",
                __LINE__, tempbuf_size);
            return -1;
        }

        new_avcodec_decode_audio_to(avctx, len, frame_size_ptr,
                                samples, outbuf_size, tempbuf); 
   
        if (samples) {
            av_free(samples);
            TRACE("[%d] release audio outbuf.\n", __LINE__);
        }
 
        TRACE("before push codec_wq, file_index of %d: %p\n",
            ctx_index, s->codec_ctx[ctx_index].file_index);
        elem->buf = tempbuf;
        elem->buf_size = tempbuf_size;
        elem->buf_id = s->codec_ctx[ctx_index].file_index;

        TRACE("before add codec_wq\n");
        qemu_mutex_lock(&s->codec_job_queue_mutex);
        QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
        qemu_mutex_unlock(&s->codec_job_queue_mutex);
    }

#if 0
    if (parser_buf && parser_use) {
        TRACE("[%s] free parser buf\n", __func__);
        av_free(avpkt.data);
        s->codec_ctx[ctx_index].parser_buf = NULL;
    }
#endif
    TRACE("[%s] Leave\n", __func__);

    return len;
}

int new_avcodec_encode_video(NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx = NULL;
    AVFrame *pict = NULL;
    uint8_t *inbuf = NULL;
    int inbuf_size, outbuf_size, len;
    int64_t in_timestamp;
    int size = 0, ret = -1, ctx_index;

    TRACE("Enter, %s\n", __func__);
    ctx_index = ioparam->ctx_index;

    avctx = s->codec_ctx[ctx_index].avctx;
    pict = s->codec_ctx[ctx_index].frame;
    if (!avctx || !pict) {
        ERR("[%s] %d of Context or Frame is NULL\n", __func__, ctx_index);
        return ret;
    }

    memcpy(&outbuf_size, (uint8_t *)s->vaddr, sizeof(inbuf_size));
    size = sizeof(outbuf_size);
    memcpy(&in_timestamp, (uint8_t *)s->vaddr + size, sizeof(in_timestamp));
    size += sizeof(in_timestamp);
    inbuf = (uint8_t *)s->vaddr + size;
    if (!inbuf) {
        ERR("[%s] failed to get input buffer\n", __func__);
        return ret;
    }

    ret = new_avcodec_picture_get_size((AVPicture *)pict, inbuf, avctx->pix_fmt,
            avctx->width, avctx->height, true);

    if (ret < 0) {
        ERR("after avpicture_fill, ret:%d\n", ret);
    }
    TRACE("before encode video, ticks_per_frame:%d, pts:%lld\n",
            avctx->ticks_per_frame, pict->pts);

#if 1
    if (avctx->time_base.num == 0) {
        pict->pts = AV_NOPTS_VALUE;
    } else {
        AVRational bq = {1, (G_USEC_PER_SEC * G_GINT64_CONSTANT(1000))};
        pict->pts = av_rescale_q(in_timestamp, bq, avctx->time_base);
    }
#endif

    len = avcodec_encode_video(avctx, (uint8_t *)s->vaddr, outbuf_size, pict);
    if (ret < 0) {
        ERR("failed to encode video.\n");
    }
    TRACE("encode video, ret:%d, pts:%lld, outbuf size:%d\n",
            ret, pict->pts, outbuf_size);

    memcpy((uint8_t *)s->vaddr + outbuf_size, &len, sizeof(len));

    if (pict->pict_type) {
        pict->pict_type = 0;
    }

    TRACE("Leave, %s\n", __func__);

    return ret;
}

int new_avcodec_encode_audio(NewCodecState *s, CodecParam *ioparam)
{
    AVCodecContext *avctx;
    uint8_t *audio_in;
//    uint32_t audio_in_size;
    uint32_t max_size = 0;
    uint8_t *audio_out;
    int size = 0, ret = -1, ctx_index;

    TRACE("[%s] Enter\n", __func__);
    ctx_index = ioparam->ctx_index;

    TRACE("Audio Context Index : %d\n", ctx_index);
    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of Context is NULL!\n", __func__, ctx_index);
        return ret;
    }

    if (!avctx->codec) {
        ERR("[%s] %d of Codec is NULL\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->codec_mutex);
        return ret;
    }

#if 0
    memcpy(&audio_in_size, (uint8_t *)s->vaddr, sizeof(audio_in_size));
    size = sizeof(audio_in_size);
#endif
    size = 0;
    memcpy(&max_size, (uint8_t *)s->vaddr + size, sizeof(max_size));
    size += sizeof(max_size);
    audio_in = (uint8_t *)s->vaddr + size;

    audio_out = av_mallocz(max_size + FF_MIN_BUFFER_SIZE);

    TRACE("before encoding audio, max_size:%d\n", max_size);

    ret = avcodec_encode_audio (avctx, audio_out, max_size, (short *)audio_in);
    TRACE("after encoding audio, ret:%d, frame_number:%d, frame_bits:%d\n",
        ret, avctx->frame_number, avctx->frame_bits);

    memcpy ((uint8_t *)s->vaddr, &ret, sizeof(ret));
    size = sizeof(ret);
    if (ret < 0) {
        ERR("failed to encode audio\n");
    } else {
        memcpy ((uint8_t *)s->vaddr + size, &avctx->frame_number, sizeof(avctx->frame_number));
        size += sizeof(avctx->frame_number);
        memcpy ((uint8_t *)s->vaddr + size, &avctx->frame_bits, sizeof(avctx->frame_bits));
        size += sizeof(avctx->frame_bits);
        memcpy ((uint8_t *)s->vaddr + size, audio_out, max_size);
    }

    av_free (audio_out);

    TRACE("[%s] Leave\n", __func__);

    return 0;
}

AVCodecParserContext *new_avcodec_parser_init(AVCodecContext *avctx)
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
        break;
    }

    return parser;
}

int new_avcodec_parser_parse (AVCodecParserContext *pctx, AVCodecContext *avctx, 
                            uint8_t *inbuf, int inbuf_size,
                            uint8_t * outbuf, int outbuf_size,
                            int64_t pts, int64_t dts, int64_t pos)
{
    if (!avctx || !pctx) {
        ERR("Codec or Parser Context is empty\n");
        return -1;
    }

    return 0;
}

int new_codec_api_selection (NewCodecState *s)
{
    CodecParam ioparam = {0, };

    memcpy(&ioparam, &s->ioparam, sizeof(CodecParam));

    if (ioparam.api_index < 0 || ioparam.api_index > CODEC_DEINIT) {
        ERR("invalid api index: %d\n", ioparam.api_index);
        return -1;
    }

    if (ioparam.ctx_index < 0 || ioparam.ctx_index >= CODEC_CONTEXT_MAX) {
        ERR("invalid context index: %d\n", ioparam.ctx_index);
        return -1;
    }
 
#if defined(CONFIG_LINUX) && defined(CODEC_DEBUG)
    QemuThread current_thread;

    qemu_thread_get_self(&current_thread);
    TRACE("worker thread: %x for context: %d\n",
        current_thread.thread, ctx_index);
#endif

    switch (ioparam.api_index) {
    case CODEC_INIT:
        new_avcodec_init(s, &ioparam);
        break;
    case CODEC_DECODE_VIDEO: 
        new_avcodec_decode_video(s, &ioparam);
        break;
    case CODEC_DECODE_AUDIO:
        new_avcodec_decode_audio(s, &ioparam);
        break;
    case CODEC_ENCODE_VIDEO:
        new_avcodec_encode_video(s, &ioparam);
        break;
    case CODEC_ENCODE_AUDIO:
        new_avcodec_encode_audio(s, &ioparam);
        break;
    case CODEC_PICTURE_COPY:
        new_avcodec_picture_copy(s, &ioparam);
        break;
    case CODEC_DEINIT:
        new_avcodec_deinit(s, &ioparam);
        break;
    default:
        ERR("unusable api index: %d.\n", ioparam.api_index);
    }

    return (int)ioparam.mem_type;
}

#if 0
void new_codec_set_device_mem (NewCodecState *s, uint32_t value)
{
    qemu_mutex_lock(&s->wrk_thread.mutex);
    TRACE("set device mem: %d\n", value);
    s->device_mem_avail = value;
    qemu_mutex_unlock(&s->wrk_thread.mutex);
}
#endif

void *new_codec_pop_readqueue(NewCodecState *s, int32_t file_index)
{
    DeviceMemEntry *elem = NULL;
    
    elem = QTAILQ_FIRST(&codec_rq);
    if (!elem) {
        ERR("codec_rq is empty.\n");
        return NULL;
    }

    if (!elem->buf || (elem->buf_size == 0)) {
        ERR("cannot copy data to guest\n");
        return NULL;
    }

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

    TRACE("pop codec_rq. id: %x buf_size: %d\n",
        elem->buf_id, elem->buf_size);
    return elem;
}

void new_codec_pop_writequeue(NewCodecState *s, int32_t file_index)
{
    DeviceMemEntry *elem = NULL;
    uint32_t mem_offset = 0;

    elem = QTAILQ_FIRST(&codec_pop_wq);
    if (!elem) {
        TRACE("codec_pop_wq is empty.\n");
        return;
    }

    if (!elem->buf || (elem->buf_size == 0)) {
        ERR("cannot copy data to guest\n");
        return;
    }

    TRACE("pop data from codec_pop_wq. size: %d, id: %x, file :%x\n",
        elem->buf_size, elem->buf_id, file_index);
    memcpy(s->vaddr + mem_offset, elem->buf, elem->buf_size);

    qemu_mutex_lock(&s->codec_job_queue_mutex);
    QTAILQ_REMOVE(&codec_pop_wq, elem, node);
    qemu_mutex_unlock(&s->codec_job_queue_mutex);

    if (elem->buf) {
        TRACE("[%d] release buf.\n", __LINE__);
        g_free (elem->buf);
    }

    if (elem) {
        TRACE("[%d] release elem.\n", __LINE__);
        g_free (elem);
    }
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
#if 0 
        if (!QTAILQ_EMPTY(&codec_wq)) {
            ret = CODEC_SHARED_TASK_FIN;
            TRACE("get thread_state. ret: %d\n", ret);
        }
#endif
#if 1 
//        if (s->wrk_thread.state == CODEC_FIXED_TASK_FIN) {
//            ret = s->wrk_thread.state;
//            s->wrk_thread.state = CODEC_TASK_INIT;

        if (!QTAILQ_EMPTY(&codec_irq_queue)) {
            ret = CODEC_FIXED_TASK_FIN;
        } else if (!QTAILQ_EMPTY(&codec_wq)) {
            ret = CODEC_SHARED_TASK_FIN;
        }
#endif
        TRACE("get thread_state. ret: %d\n", ret);
        qemu_irq_lower(s->dev.irq[0]);
        break;
    case CODEC_CMD_GET_VERSION:
        ret = NEW_CODEC_VERSION;
        TRACE("codec version: %d\n", ret);
        break;
    case CODEC_CMD_GET_SHARED_QUEUE:
    {
        DeviceMemEntry *head = NULL;
        head = QTAILQ_FIRST(&codec_wq);
        if (head) {
            ret = head->buf_id;
            qemu_mutex_lock(&s->wrk_thread.mutex);
            QTAILQ_REMOVE(&codec_wq, head, node);
            QTAILQ_INSERT_TAIL(&codec_pop_wq, head, node);
            qemu_mutex_unlock(&s->wrk_thread.mutex);
        } else {
            ret = 0;
        }
        TRACE("get a writequeue for shared mem. ret: %x\n", ret);
    }
        break;
    case CODEC_CMD_GET_FIXED_QUEUE:
    {
        DeviceMemEntry *head = NULL;
        head = QTAILQ_FIRST(&codec_irq_queue);
        if (head) {
            ret = head->buf_id;
            qemu_mutex_lock(&s->codec_job_queue_mutex);
            QTAILQ_REMOVE(&codec_irq_queue, head, node);
            qemu_mutex_unlock(&s->codec_job_queue_mutex);
        } else {
            ret = 0;
        }
        TRACE("get a writequeue for fixed mem. ret: %x\n", ret);
    }
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
        if ((int32_t)value == CODEC_QUERY_LIST) {
            new_avcodec_query_list(s);
        } else { 
            new_codec_wakeup_thread_and_run(s, value);
        }
        break;
    case CODEC_CMD_CONTEXT_INDEX:
        if ((int32_t)value < 0) {
            s->ioparam.ctx_index =
                new_codec_get_context_index_from_file(s,
                                                s->ioparam.file_index);
            TRACE("convert context index: %d\n", s->ioparam.ctx_index);
        } else {
            s->ioparam.ctx_index = value;
            TRACE("[%d] context index: %d\n", __LINE__, s->ioparam.ctx_index);
        }
        break;
    case CODEC_CMD_FILE_INDEX:
        s->ioparam.file_index = value;
        break;
    case CODEC_CMD_DEVICE_MEM_OFFSET:
        s->ioparam.mem_offset = value;
        TRACE("device memory offfset: %d\n", value);
        break;
    case CODEC_CMD_DEVICE_MEM_TYPE:
        s->ioparam.mem_type = value;
        TRACE("device memory type: %d\n", value);
        break;
    case CODEC_CMD_RESET_CODEC_CONTEXT:
        new_codec_reset_codec_context(s, (int32_t)value);
        break;
    case CODEC_CMD_POP_WRITE_QUEUE:
        new_codec_pop_writequeue(s, (int32_t)value);
        break;
    default:
        ERR("no avaiable command for write. %d\n", addr);
    }
}

static const MemoryRegionOps new_codec_mmio_ops = {
    .read = new_codec_read,
    .write = new_codec_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void new_codec_tx_bh(void *opaque)
{
    NewCodecState *s = (NewCodecState *)opaque;

    TRACE("Enter, %s\n", __func__);

    if (!QTAILQ_EMPTY(&codec_irq_queue)) {
        TRACE("raise irq for fixed task.\n");
        qemu_irq_raise(s->dev.irq[0]);
    } else if (!QTAILQ_EMPTY(&codec_wq)) {
        TRACE("raise irq for shared task.\n");
        qemu_irq_raise(s->dev.irq[0]);
    }

    TRACE("Leave, %s\n", __func__);
}

static int new_codec_initfn(PCIDevice *dev)
{
    NewCodecState *s = DO_UPCAST(NewCodecState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    TRACE("device initialization.\n");
    memset(&s->ioparam, 0, sizeof(CodecParam));

    qemu_mutex_init(&s->codec_mutex);
    qemu_mutex_init(&s->codec_job_queue_mutex);

    new_codec_thread_init(s);
    s->codec_bh = qemu_bh_new(new_codec_tx_bh, s);

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
    TRACE("device exit\n");

    qemu_bh_delete(s->codec_bh);

    memory_region_destroy(&s->vram);
    memory_region_destroy(&s->mmio);
}

int new_codec_init(PCIBus *bus)
{
    TRACE("device create.\n");
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
