/*
 * Virtual Codec Device
 *
 * Copyright (c) 2013 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "maru_brillcodec.h"

#include "libavresample/avresample.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"

#include "debug_ch.h"

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, brillcodec);

// device memory
#define CODEC_META_DATA_SIZE    (256)

// libav
#define GEN_MASK(x) ((1 << (x)) - 1)
#define ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) & ~GEN_MASK(x))
#define ROUND_UP_2(x) ROUND_UP_X(x, 1)
#define ROUND_UP_4(x) ROUND_UP_X(x, 2)
#define ROUND_UP_8(x) ROUND_UP_X(x, 3)
#define DIV_ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) >> (x))

#define DEFAULT_VIDEO_GOP_SIZE 15

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
    int32_t reserved;
    int64_t channel_layout;
};

DeviceMemEntry *entry[CODEC_CONTEXT_MAX];

// define a queue to manage ioparam, context data
typedef struct CodecDataStg {
    CodecParam *param_buf;
    DeviceMemEntry *data_buf;

    QTAILQ_ENTRY(CodecDataStg) node;
} CodecDataStg;

// define two queue to store input and output buffers.
struct codec_wq codec_wq = QTAILQ_HEAD_INITIALIZER(codec_wq);
static QTAILQ_HEAD(codec_rq, CodecDataStg) codec_rq =
   QTAILQ_HEAD_INITIALIZER(codec_rq);

// codec functions
static bool codec_init(MaruBrillCodecState *, int, void *);
static bool codec_deinit(MaruBrillCodecState *, int, void *);
static bool codec_decode_video(MaruBrillCodecState *, int, void *);
static bool codec_encode_video(MaruBrillCodecState *, int, void *);
static bool codec_decode_audio(MaruBrillCodecState *, int, void *);
static bool codec_encode_audio(MaruBrillCodecState *, int, void *);
static bool codec_picture_copy(MaruBrillCodecState *, int, void *);
static bool codec_flush_buffers(MaruBrillCodecState *, int, void *);

typedef bool (*CodecFuncEntry)(MaruBrillCodecState *, int, void *);
static CodecFuncEntry codec_func_handler[] = {
    codec_init,
    codec_decode_video,
    codec_encode_video,
    codec_decode_audio,
    codec_encode_audio,
    codec_picture_copy,
    codec_deinit,
    codec_flush_buffers,
};

static AVCodecParserContext *maru_brill_codec_parser_init(AVCodecContext *avctx);

static void maru_brill_codec_push_readqueue(MaruBrillCodecState *s, CodecParam *ioparam);
static void maru_brill_codec_push_writequeue(MaruBrillCodecState *s, void* opaque,
                                            size_t data_size, int ctx_id,
                                            DataHandler *handler);

static void *maru_brill_codec_store_inbuf(uint8_t *mem_base, CodecParam *ioparam);

// default handler
static void default_get_data(void *dst, void *src, size_t size, enum AVPixelFormat pix_fmt) {
    memcpy(dst, src, size);
}

static void default_release(void *opaque) {
    g_free(opaque);
}

static DataHandler default_data_handler = {
    .get_data = default_get_data,
    .release = default_release,
};


// default video decode data handler
static void extract(void *dst, void *src, size_t size, enum AVPixelFormat pix_fmt) {
    AVFrame *frame = (AVFrame *)src;
    avpicture_layout((AVPicture *)src, pix_fmt, frame->width, frame->height, dst, size);
}

static void release(void *buf) {}

static DataHandler default_video_decode_data_handler = {
    .get_data = extract,
    .release = release,
};

static void maru_brill_codec_thread_exit(MaruBrillCodecState *s)
{
    int index;

    TRACE("enter: %s\n", __func__);

    /* stop to run dedicated threads. */
    s->is_thread_running = false;

    for (index = 0; index < s->worker_thread_cnt; index++) {
        qemu_thread_join(&s->threadpool.threads[index]);
    }

    TRACE("destroy mutex and conditional.\n");
    qemu_mutex_destroy(&s->threadpool.mutex);
    qemu_cond_destroy(&s->threadpool.cond);

    if (s->threadpool.threads) {
        g_free(s->threadpool.threads);
        s->threadpool.threads = NULL;
    }

    TRACE("leave: %s\n", __func__);
}

void maru_brill_codec_wakeup_threads(MaruBrillCodecState *s, int api_index)
{
    CodecParam *ioparam = NULL;

    ioparam = g_malloc0(sizeof(CodecParam));
    if (!ioparam) {
        ERR("failed to allocate ioparam\n");
        return;
    }

    memcpy(ioparam, &s->ioparam, sizeof(CodecParam));

    TRACE("wakeup thread. ctx_id: %u, api_id: %u, mem_offset: 0x%x\n",
        ioparam->ctx_index, ioparam->api_index, ioparam->mem_offset);

    qemu_mutex_lock(&s->context_mutex);

    if (ioparam->api_index != CODEC_INIT) {
        if (!CONTEXT(s, ioparam->ctx_index).opened_context) {
            INFO("abandon api %d for context %d\n",
                    ioparam->api_index, ioparam->ctx_index);
            qemu_mutex_unlock(&s->context_mutex);
            return;
        }
    }

    qemu_mutex_unlock(&s->context_mutex);

    maru_brill_codec_push_readqueue(s, ioparam);

    qemu_mutex_lock(&s->context_mutex);
    // W/A for threads starvation.
    while (s->idle_thread_cnt == 0) {
        qemu_mutex_unlock(&s->context_mutex);
        TRACE("Worker threads are exhausted\n");
        usleep(2000); // wait 2ms.
        qemu_mutex_lock(&s->context_mutex);
    }
    qemu_cond_signal(&s->threadpool.cond);
    qemu_mutex_unlock(&s->context_mutex);

    TRACE("after sending conditional signal\n");
}

void *maru_brill_codec_threads(void *opaque)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;
    bool ret = false;

    TRACE("enter: %s\n", __func__);

    while (s->is_thread_running) {
        int ctx_id = 0, api_id = 0;
        CodecDataStg *elem = NULL;
        DeviceMemEntry *indata_buf = NULL;

        qemu_mutex_lock(&s->context_mutex);
        ++(s->idle_thread_cnt); // protected under mutex.
        qemu_cond_wait(&s->threadpool.cond, &s->context_mutex);
        --(s->idle_thread_cnt); // protected under mutex.
        qemu_mutex_unlock(&s->context_mutex);

        qemu_mutex_lock(&s->ioparam_queue_mutex);
        elem = QTAILQ_FIRST(&codec_rq);
        if (elem) {
            QTAILQ_REMOVE(&codec_rq, elem, node);
            qemu_mutex_unlock(&s->ioparam_queue_mutex);
        } else {
            qemu_mutex_unlock(&s->ioparam_queue_mutex);
            continue;
        }

        if (!elem->param_buf) {
            continue;
        }

        api_id = elem->param_buf->api_index;
        ctx_id = elem->param_buf->ctx_index;

        indata_buf = elem->data_buf;

        TRACE("api_id: %d ctx_id: %d\n", api_id, ctx_id);

        qemu_mutex_lock(&s->context_mutex);
        CONTEXT(s, ctx_id).occupied_thread = true;
        qemu_mutex_unlock(&s->context_mutex);

        ret = codec_func_handler[api_id](s, ctx_id, indata_buf);
        if (!ret) {
            ERR("fail api %d for context %d\n", api_id, ctx_id);
            g_free(elem->param_buf);
            continue;
        }

        TRACE("release a buffer of CodecParam\n");
        g_free(elem->param_buf);
        elem->param_buf = NULL;

        if (elem->data_buf) {
            if (elem->data_buf->opaque) {
                TRACE("release inbuf\n");
                g_free(elem->data_buf->opaque);
                elem->data_buf->opaque = NULL;
            }

            TRACE("release a buffer indata_buf\n");
            g_free(elem->data_buf);
            elem->data_buf = NULL;
        }

        TRACE("release an element of CodecDataStg\n");
        g_free(elem);

        qemu_mutex_lock(&s->context_mutex);
        if (CONTEXT(s, ctx_id).requested_close) {
            INFO("make worker thread to handle deinit\n");
            // codec_deinit(s, ctx_id, NULL);
            maru_brill_codec_release_context(s, ctx_id);
            CONTEXT(s, ctx_id).requested_close = false;
        }
        qemu_mutex_unlock(&s->context_mutex);

        TRACE("switch context to raise interrupt.\n");
        qemu_bh_schedule(s->codec_bh);

        qemu_mutex_lock(&s->context_mutex);
        CONTEXT(s, ctx_id).occupied_thread = false;
        qemu_mutex_unlock(&s->context_mutex);
    }

    maru_brill_codec_thread_exit(s);

    TRACE("leave: %s\n", __func__);
    return NULL;
}

// queue
static void maru_brill_codec_push_readqueue(MaruBrillCodecState *s,
                                            CodecParam *ioparam)
{
    CodecDataStg *elem = NULL;
    DeviceMemEntry *data_buf = NULL;

    elem = g_malloc0(sizeof(CodecDataStg));
    if (!elem) {
        ERR("failed to allocate ioparam_queue. %d\n", sizeof(CodecDataStg));
        return;
    }

    elem->param_buf = ioparam;

    switch(ioparam->api_index) {
    case CODEC_INIT ... CODEC_ENCODE_AUDIO:
        data_buf = maru_brill_codec_store_inbuf((uint8_t *)s->vaddr, ioparam);
        break;
    default:
        TRACE("no buffer from guest\n");
        break;
    }

    elem->data_buf = data_buf;

    qemu_mutex_lock(&s->ioparam_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_rq, elem, node);
    qemu_mutex_unlock(&s->ioparam_queue_mutex);
}

static void *maru_brill_codec_store_inbuf(uint8_t *mem_base,
                                        CodecParam *ioparam)
{
    DeviceMemEntry *elem = NULL;
    int readbuf_size, size = 0;
    uint8_t *readbuf = NULL;
    uint8_t *device_mem = mem_base + ioparam->mem_offset;

    elem = g_malloc0(sizeof(DeviceMemEntry));
    if (!elem) {
        ERR("failed to allocate readqueue node. size: %d\n",
            sizeof(DeviceMemEntry));
        return NULL;
    }

    memcpy(&readbuf_size, device_mem, sizeof(readbuf_size));
    size = sizeof(readbuf_size);

    TRACE("readbuf size: %d\n", readbuf_size);
    if (readbuf_size <= 0) {
        TRACE("inbuf size is 0. api_id %d, ctx_id %d, mem_offset %x\n",
            ioparam->api_index, ioparam->ctx_index, ioparam->mem_offset);
    } else {
        readbuf = g_malloc0(readbuf_size);
        if (!readbuf) {
            ERR("failed to allocate a read buffer. size: %d\n", readbuf_size);
        } else {
            TRACE("copy input buffer from guest. ctx_id: %d, mem_offset: %x\n",
                ioparam->ctx_index, ioparam->mem_offset);
            memcpy(readbuf, device_mem + size, readbuf_size);
        }
    }
    // memset(device_mem, 0x00, sizeof(readbuf_size));

    elem->opaque = readbuf;
    elem->data_size = readbuf_size;
    elem->ctx_id = ioparam->ctx_index;

    return elem;
}

static void maru_brill_codec_push_writequeue(MaruBrillCodecState *s, void* opaque,
                                            size_t data_size, int ctx_id,
                                            DataHandler *handler)
{
    DeviceMemEntry *elem = NULL;
    elem = g_malloc0(sizeof(DeviceMemEntry));

    elem->opaque = opaque;
    elem->data_size = data_size;
    elem->ctx_id = ctx_id;

    if (handler) {
        elem->handler = handler;
    } else {
        elem->handler = &default_data_handler;
    }

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);
}

void maru_brill_codec_pop_writequeue(MaruBrillCodecState *s, uint32_t ctx_idx)
{
    DeviceMemEntry *elem = NULL;
    uint32_t mem_offset = 0;

    TRACE("enter: %s\n", __func__);

    if (ctx_idx < 1 || ctx_idx > (CODEC_CONTEXT_MAX - 1)) {
        ERR("invalid buffer index. %d\n", ctx_idx);
        return;
    }

    TRACE("pop_writeqeue. context index: %d\n", ctx_idx);
    elem = entry[ctx_idx];
    if (elem) {
        mem_offset = s->ioparam.mem_offset;

        // check corrupted mem_offset
        if (mem_offset < CODEC_MEM_SIZE) {
            elem->handler->get_data(s->vaddr + mem_offset, elem->opaque, elem->data_size, s->context[ctx_idx].avctx->pix_fmt);
            elem->handler->release(elem->opaque);
        } else {
            TRACE("mem_offset is corrupted!!\n");
        }

        TRACE("pop_writequeue. release elem: %p\n", elem);
        g_free(elem);

        entry[ctx_idx] = NULL;
    } else {
        TRACE("there is no buffer to copy data to guest\n");
    }

    TRACE("leave: %s\n", __func__);
}

static void serialize_video_data(const struct video_data *video,
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

    INFO("codec_init. video, resolution: %dx%d, framerate: %d/%d "
        "pixel_fmt: %d sample_aspect_ratio: %d/%d bpp %d\n",
        avctx->width, avctx->height, avctx->time_base.num,
        avctx->time_base.den, avctx->pix_fmt, avctx->sample_aspect_ratio.num,
        avctx->sample_aspect_ratio.den, avctx->bits_per_coded_sample);
}

static void deserialize_video_data (const AVCodecContext *avctx,
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

static void serialize_audio_data (const struct audio_data *audio,
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

    if (audio->sample_fmt > AV_SAMPLE_FMT_NONE) {
        avctx->sample_fmt = audio->sample_fmt;
    }

    INFO("codec_init. audio, channel %d sample_rate %d sample_fmt %d ch_layout %lld\n",
        avctx->channels, avctx->sample_rate, avctx->sample_fmt, avctx->channel_layout);
}

void maru_brill_codec_release_context(MaruBrillCodecState *s, int32_t ctx_id)
{
    DeviceMemEntry *wq_elem = NULL, *wnext = NULL;
    CodecDataStg *rq_elem = NULL, *rnext = NULL;

    TRACE("enter: %s\n", __func__);

    TRACE("release %d of context\n", ctx_id);

    qemu_mutex_lock(&s->threadpool.mutex);
    if (CONTEXT(s, ctx_id).opened_context) {
        // qemu_mutex_unlock(&s->threadpool.mutex);
        codec_deinit(s, ctx_id, NULL);
        // qemu_mutex_lock(&s->threadpool.mutex);
    }
    CONTEXT(s, ctx_id).occupied_context = false;
    qemu_mutex_unlock(&s->threadpool.mutex);

    // TODO: check if foreach statment needs lock or not.
    QTAILQ_FOREACH_SAFE(rq_elem, &codec_rq, node, rnext) {
        if (rq_elem && rq_elem->data_buf &&
            (rq_elem->data_buf->ctx_id == ctx_id)) {

            TRACE("remove unused node from codec_rq. ctx_id: %d\n", ctx_id);
            qemu_mutex_lock(&s->context_queue_mutex);
            QTAILQ_REMOVE(&codec_rq, rq_elem, node);
            qemu_mutex_unlock(&s->context_queue_mutex);
            if (rq_elem && rq_elem->data_buf) {
                TRACE("release rq_buffer: %p\n", rq_elem->data_buf);
                g_free(rq_elem->data_buf);
            }

            TRACE("release rq_elem: %p\n", rq_elem);
            g_free(rq_elem);
        } else {
            TRACE("no elem of %d context in the codec_rq.\n", ctx_id);
        }
    }

    QTAILQ_FOREACH_SAFE(wq_elem, &codec_wq, node, wnext) {
        if (wq_elem && wq_elem->ctx_id == ctx_id) {
            TRACE("remove unused node from codec_wq. ctx_id: %d\n", ctx_id);
            qemu_mutex_lock(&s->context_queue_mutex);
            QTAILQ_REMOVE(&codec_wq, wq_elem, node);
            qemu_mutex_unlock(&s->context_queue_mutex);

            if (wq_elem && wq_elem->opaque) {
                TRACE("release wq_buffer: %p\n", wq_elem->opaque);
                g_free(wq_elem->opaque);
                wq_elem->opaque = NULL;
            }

            TRACE("release wq_elem: %p\n", wq_elem);
            g_free(wq_elem);
        } else {
            TRACE("no elem of %d context in the codec_wq.\n", ctx_id);
        }
    }

    TRACE("leave: %s\n", __func__);
}

int maru_brill_codec_query_list (MaruBrillCodecState *s)
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
                for (i = 0; codec->pix_fmts[i] != -1 && i < 4; i++) {
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
            ERR("unknown media type: %d\n", media_type);
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

        TRACE("register %s %s\n", codec->name, codec->decode ? "decoder" : "encoder");
        codec = av_codec_next(codec);
    }

    return 0;
}

int maru_brill_codec_get_context_index(MaruBrillCodecState *s)
{
    int ctx_id;

    TRACE("enter: %s\n", __func__);

    // requires mutex_lock? its function is protected by critical section.
    qemu_mutex_lock(&s->threadpool.mutex);
    for (ctx_id = 1; ctx_id < CODEC_CONTEXT_MAX; ctx_id++) {
        if (CONTEXT(s, ctx_id).occupied_context == false) {
            TRACE("get %d of codec context successfully.\n", ctx_id);
            CONTEXT(s, ctx_id).occupied_context = true;
            break;
        }
    }
    qemu_mutex_unlock(&s->threadpool.mutex);

    if (ctx_id == CODEC_CONTEXT_MAX) {
        ERR("failed to get available codec context. ");
        ERR("try to run codec again.\n");
        ctx_id = -1;
    }

    TRACE("leave: %s\n", __func__);

    return ctx_id;
}


// allocate avcontext and avframe struct.
static AVCodecContext *maru_brill_codec_alloc_context(MaruBrillCodecState *s, int ctx_id)
{
    TRACE("enter: %s\n", __func__);

    TRACE("allocate %d of context and frame.\n", ctx_id);
    CONTEXT(s, ctx_id).avctx = avcodec_alloc_context3(NULL);
    CONTEXT(s, ctx_id).frame = avcodec_alloc_frame();
    CONTEXT(s, ctx_id).opened_context = false;

    TRACE("leave: %s\n", __func__);

    return CONTEXT(s, ctx_id).avctx;
}

static AVCodec *maru_brill_codec_find_avcodec(uint8_t *mem_buf)
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
    INFO("%s!! find %s %s\n", codec ? "success" : "failure",
        codec_name, encode ? "encoder" : "decoder");

    return codec;
}

static void read_codec_init_data(AVCodecContext *avctx, uint8_t *mem_buf)
{
    struct video_data video = { 0, };
    struct audio_data audio = { 0, };
    int bitrate = 0, size = 0;

    memcpy(&video, mem_buf + size, sizeof(video));
    size = sizeof(video);
    serialize_video_data(&video, avctx);

    memcpy(&audio, mem_buf + size, sizeof(audio));
    size += sizeof(audio);
    serialize_audio_data(&audio, avctx);

    memcpy(&bitrate, mem_buf + size, sizeof(bitrate));
    size += sizeof(bitrate);
    if (bitrate) {
        avctx->bit_rate = bitrate;
    }

    memcpy(&avctx->codec_tag, mem_buf + size, sizeof(avctx->codec_tag));
    size += sizeof(avctx->codec_tag);
    memcpy(&avctx->extradata_size,
            mem_buf + size, sizeof(avctx->extradata_size));
    size += sizeof(avctx->extradata_size);
    INFO("extradata size: %d.\n", avctx->extradata_size);

    if (avctx->extradata_size > 0) {
        avctx->extradata =
            av_mallocz(ROUND_UP_X(avctx->extradata_size +
                        FF_INPUT_BUFFER_PADDING_SIZE, 4));
        if (avctx->extradata) {
            memcpy(avctx->extradata, mem_buf + size, avctx->extradata_size);
        }
    } else {
        TRACE("no extra data.\n");
        avctx->extradata =
            av_mallocz(ROUND_UP_X(FF_INPUT_BUFFER_PADDING_SIZE, 4));
    }
}

// write the result of codec_init
static int write_codec_init_data(AVCodecContext *avctx, uint8_t *mem_buf)
{
    int size = 0;

    if (avctx->codec->type == AVMEDIA_TYPE_AUDIO) {
        int osize = av_get_bytes_per_sample(avctx->sample_fmt);

        INFO("avcodec_open. sample_fmt %d, bytes_per_sample %d\n", avctx->sample_fmt, osize);

        if ((avctx->codec_id == AV_CODEC_ID_AAC) && avctx->codec->encode2) {
            osize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        }
        memcpy(mem_buf, &avctx->sample_fmt, sizeof(avctx->sample_fmt));
        size = sizeof(avctx->sample_fmt);

        // frame_size: samples per packet, initialized when calling 'init'
        memcpy(mem_buf + size, &avctx->frame_size, sizeof(avctx->frame_size));
        size += sizeof(avctx->frame_size);

        memcpy(mem_buf + size, &osize, sizeof(osize));
        size += sizeof(osize);
    }

    return size;
}

static uint8_t *resample_audio_buffer(AVCodecContext * avctx, AVFrame *samples,
                                    int *out_size, int out_sample_fmt)
{
    AVAudioResampleContext *avr = NULL;
    uint8_t *resampled_audio = NULL;
    int buffer_size = 0, out_linesize = 0;
    int nb_samples = samples->nb_samples;
    // int out_sample_fmt = avctx->sample_fmt - 5;

    avr = avresample_alloc_context();
    if (!avr) {
        ERR("failed to allocate avresample context\n");
        return NULL;
    }

    TRACE("resample audio. channel_layout %lld sample_rate %d "
        "in_sample_fmt %d out_sample_fmt %d\n",
        avctx->channel_layout, avctx->sample_rate,
        avctx->sample_fmt, out_sample_fmt);

    av_opt_set_int(avr, "in_channel_layout", avctx->channel_layout, 0);
    av_opt_set_int(avr, "in_sample_fmt", avctx->sample_fmt, 0);
    av_opt_set_int(avr, "in_sample_rate", avctx->sample_rate, 0);
    av_opt_set_int(avr, "out_channel_layout", avctx->channel_layout, 0);
    av_opt_set_int(avr, "out_sample_fmt", out_sample_fmt, 0);
    av_opt_set_int(avr, "out_sample_rate", avctx->sample_rate, 0);

    TRACE("open avresample context\n");
    if (avresample_open(avr) < 0) {
        ERR("failed to open avresample context\n");
        avresample_free(&avr);
        return NULL;
    }

    *out_size =
        av_samples_get_buffer_size(&out_linesize, avctx->channels,
                                    nb_samples, out_sample_fmt, 0);

    TRACE("resample audio. out_linesize %d nb_samples %d\n", out_linesize, nb_samples);

    if (*out_size < 0) {
        ERR("failed to get size of sample buffer %d\n", *out_size);
        avresample_close(avr);
        avresample_free(&avr);
        return NULL;
    }

    resampled_audio = av_mallocz(*out_size);
    if (!resampled_audio) {
        ERR("failed to allocate resample buffer\n");
        avresample_close(avr);
        avresample_free(&avr);
        return NULL;
    }

    buffer_size = avresample_convert(avr, &resampled_audio,
        out_linesize, nb_samples,
        samples->data, samples->linesize[0],
        samples->nb_samples);
    TRACE("resample_audio out_size %d buffer_size %d\n", *out_size, buffer_size);

    avresample_close(avr);
    avresample_free(&avr);

    return resampled_audio;
}

static int parse_and_decode_video(AVCodecContext *avctx, AVFrame *picture,
                                AVCodecParserContext *pctx, int ctx_id,
                                AVPacket *packet, int *got_picture,
                                int idx, int64_t in_offset)
{
    uint8_t *parser_outbuf = NULL;
    int parser_outbuf_size = 0;
    uint8_t *parser_buf = packet->data;
    int parser_buf_size = packet->size;
    int ret = 0, len = -1;
    int64_t pts = 0, dts = 0, pos = 0;

    pts = dts = idx;
    pos = in_offset;

    do {
        if (pctx) {
            ret = av_parser_parse2(pctx, avctx, &parser_outbuf,
                    &parser_outbuf_size, parser_buf, parser_buf_size,
                    pts, dts, pos);

            if (ret) {
                parser_buf_size -= ret;
                parser_buf += ret;
            }

            TRACE("after parsing ret: %d parser_outbuf_size %d parser_buf_size %d pts %lld\n",
                    ret, parser_outbuf_size, parser_buf_size, pctx->pts);

            /* if there is no output, we must break and wait for more data.
             * also the timestamp in the context is not updated.
             */
            if (parser_outbuf_size == 0) {
                if (parser_buf_size > 0) {
                    TRACE("parsing data have been left\n");
                    continue;
                } else {
                    TRACE("finish parsing data\n");
                    break;
                }
            }

            packet->data = parser_outbuf;
            packet->size = parser_outbuf_size;
        } else {
            TRACE("not using parser %s\n", avctx->codec->name);
        }

        len = avcodec_decode_video2(avctx, picture, got_picture, packet);
        TRACE("decode_video. len %d, got_picture %d\n", len, *got_picture);

        if (!pctx) {
            if (len == 0 && (*got_picture) == 0) {
                ERR("decoding video didn't return any data! ctx_id %d len %d\n", ctx_id, len);
                break;
            } else if (len < 0) {
                ERR("decoding video error! ctx_id %d len %d\n", ctx_id, len);
                break;
            }
            parser_buf_size -= len;
            parser_buf += len;
        } else {
            if (len == 0) {
                ERR("decoding video didn't return any data! ctx_id %d len %d\n", ctx_id, len);
                *got_picture = 0;
                break;
            } else if (len < 0) {
                ERR("decoding video error! trying next ctx_id %d len %d\n", ctx_id, len);
                break;
            }
        }
    } while (parser_buf_size > 0);

    return len;
}

// codec functions
static bool codec_init(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVCodec *codec = NULL;
    int size = 0, ret = -1;
    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;
    int tempbuf_size = 0;

    TRACE("enter: %s\n", __func__);

    elem = (DeviceMemEntry *)data_buf;

    // allocate AVCodecContext
    avctx = maru_brill_codec_alloc_context(s, ctx_id);
    if (!avctx) {
        ERR("[%d] failed to allocate context.\n", __LINE__);
        ret = -1;
    } else {
        codec = maru_brill_codec_find_avcodec(elem->opaque);
        if (codec) {
            size = sizeof(int32_t) + 32; // buffer size of codec_name
            read_codec_init_data(avctx, elem->opaque + size);

            // in case of aac encoder, sample format is float
            if (!strcmp(codec->name, "aac") && codec->encode2) {
                TRACE("convert sample format into SAMPLE_FMT_FLTP\n");
                avctx->sample_fmt = AV_SAMPLE_FMT_FLTP;

                avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

                INFO("aac encoder!! channels %d channel_layout %lld\n", avctx->channels, avctx->channel_layout);
                avctx->channel_layout = av_get_default_channel_layout(avctx->channels);
            }

            TRACE("audio sample format %d\n", avctx->sample_fmt);
            TRACE("strict_std_compliance %d\n", avctx->strict_std_compliance);

            ret = avcodec_open2(avctx, codec, NULL);
            INFO("avcodec_open. ret 0x%x ctx_id %d\n", ret, ctx_id);

            TRACE("channels %d sample_rate %d sample_fmt %d "
                "channel_layout %lld frame_size %d\n",
                avctx->channels, avctx->sample_rate, avctx->sample_fmt,
                avctx->channel_layout, avctx->frame_size);

            tempbuf_size = (sizeof(avctx->sample_fmt) + sizeof(avctx->frame_size)
                           + sizeof(avctx->extradata_size) + avctx->extradata_size)
                           + sizeof(int);

            CONTEXT(s, ctx_id).opened_context = true;
            CONTEXT(s, ctx_id).parser_ctx =
                maru_brill_codec_parser_init(avctx);
        } else {
            ERR("failed to find codec. ctx_id: %d\n", ctx_id);
            ret = -1;
        }
    }

    tempbuf_size += sizeof(ret);

    tempbuf = g_malloc(tempbuf_size);
    if (!tempbuf) {
        ERR("failed to allocate a buffer\n");
        tempbuf_size = 0;
    } else {
        memcpy(tempbuf, &ret, sizeof(ret));
        size = sizeof(ret);
        if (ret < 0) {
            ERR("failed to open codec contex.\n");
        } else {
            size += write_codec_init_data(avctx, tempbuf + size);
            TRACE("codec_init. copyback!! size %d\n", size);
            {
                memcpy(tempbuf + size, &avctx->extradata_size, sizeof(avctx->extradata_size));
                size += sizeof(avctx->extradata_size);

                INFO("codec_init. extradata_size: %d\n", avctx->extradata_size);
                if (avctx->extradata) {
                    memcpy(tempbuf + size, avctx->extradata, avctx->extradata_size);
                    size += avctx->extradata_size;
                }
            }
        }
    }

    maru_brill_codec_push_writequeue(s, tempbuf, tempbuf_size, ctx_id, NULL);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_deinit(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVFrame *frame = NULL;
    AVCodecParserContext *parserctx = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = CONTEXT(s, ctx_id).avctx;
    frame = CONTEXT(s, ctx_id).frame;
    parserctx = CONTEXT(s, ctx_id).parser_ctx;
    if (!avctx || !frame) {
        TRACE("%d of AVCodecContext or AVFrame is NULL. "
            " Those resources have been released before.\n", ctx_id);
        return false;
    }

    INFO("close avcontext of %d\n", ctx_id);
    // qemu_mutex_lock(&s->threadpool.mutex);
    avcodec_close(avctx);
    CONTEXT(s, ctx_id).opened_context = false;
    // qemu_mutex_unlock(&s->threadpool.mutex);

    if (avctx->extradata) {
        TRACE("free context extradata\n");
        av_free(avctx->extradata);
        CONTEXT(s, ctx_id).avctx->extradata = NULL;
    }

    if (frame) {
        TRACE("free frame\n");
        // av_free(frame);
        avcodec_free_frame(&frame);
        CONTEXT(s, ctx_id).frame = NULL;
    }

    if (avctx) {
        TRACE("free codec context\n");
        av_free(avctx);
        CONTEXT(s, ctx_id).avctx = NULL;
    }

    if (parserctx) {
        INFO("close parser context\n");
        av_parser_close(parserctx);
        CONTEXT(s, ctx_id).parser_ctx = NULL;
    }

    maru_brill_codec_push_writequeue(s, NULL, 0, ctx_id, NULL);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_flush_buffers(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    bool ret = true;

    TRACE("enter: %s\n", __func__);

    avctx = CONTEXT(s, ctx_id).avctx;
    if (!avctx) {
        ERR("%d of AVCodecContext is NULL.\n", ctx_id);
        ret = false;
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        ret = false;
    } else {
        TRACE("flush %d context of buffers.\n", ctx_id);
        AVCodecParserContext *pctx = NULL;
        uint8_t *poutbuf = NULL;
        int poutbuf_size = 0;
        int res = 0;

        uint8_t p_inbuf[FF_INPUT_BUFFER_PADDING_SIZE];
        int p_inbuf_size = FF_INPUT_BUFFER_PADDING_SIZE;

        memset(&p_inbuf, 0x00, p_inbuf_size);

        pctx = CONTEXT(s, ctx_id).parser_ctx;
        if (pctx) {
            res = av_parser_parse2(pctx, avctx, &poutbuf, &poutbuf_size,
                    p_inbuf, p_inbuf_size, -1, -1, -1);
            INFO("before flush buffers, using parser. res: %d\n", res);
        }

        avcodec_flush_buffers(avctx);
    }

    maru_brill_codec_push_writequeue(s, NULL, 0, ctx_id, NULL);

    TRACE("leave: %s\n", __func__);

    return ret;
}

static bool codec_decode_video(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVFrame *picture = NULL;
    AVCodecParserContext *pctx = NULL;
    AVPacket avpkt;

    int got_picture = 0, len = -1;
    int idx = 0, size = 0;
    int64_t in_offset = 0;
    uint8_t *inbuf = NULL;
    int inbuf_size = 0;
    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;
    int tempbuf_size = 0;

    TRACE("enter: %s\n", __func__);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->opaque) {
        memcpy(&inbuf_size, elem->opaque, sizeof(inbuf_size));
        size += sizeof(inbuf_size);
        memcpy(&idx, elem->opaque + size, sizeof(idx));
        size += sizeof(idx);
        memcpy(&in_offset, elem->opaque + size, sizeof(in_offset));
        size += sizeof(in_offset);
        TRACE("decode_video. inbuf_size %d\n", inbuf_size);

        if (inbuf_size > 0) {
            inbuf = elem->opaque + size;
        }
    } else {
        TRACE("decode_video. no input buffer\n");
        // FIXME: improve error handling
        // return false;
    }

    av_init_packet(&avpkt);
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;


    avctx = CONTEXT(s, ctx_id).avctx;
    picture = CONTEXT(s, ctx_id).frame;
    if (!avctx) {
        ERR("decode_video. %d of AVCodecContext is NULL.\n", ctx_id);
    } else if (!avctx->codec) {
        ERR("decode_video. %d of AVCodec is NULL.\n", ctx_id);
    } else if (!picture) {
        ERR("decode_video. %d of AVFrame is NULL.\n", ctx_id);
    } else {
        pctx = CONTEXT(s, ctx_id).parser_ctx;

        len = parse_and_decode_video(avctx, picture, pctx, ctx_id,
                                    &avpkt, &got_picture, idx, in_offset);
    }

    tempbuf_size = sizeof(len) + sizeof(got_picture) + sizeof(struct video_data);

    tempbuf = g_malloc(tempbuf_size);
    if (!tempbuf) {
        ERR("failed to allocate decoded audio buffer\n");
        tempbuf_size = 0;
    } else {
        struct video_data video;

        memcpy(tempbuf, &len, sizeof(len));
        size = sizeof(len);
        memcpy(tempbuf + size, &got_picture, sizeof(got_picture));
        size += sizeof(got_picture);
        if (avctx) {
            deserialize_video_data(avctx, &video);
            memcpy(tempbuf + size, &video, sizeof(struct video_data));
        }
    }

    maru_brill_codec_push_writequeue(s, tempbuf, tempbuf_size, ctx_id, NULL);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_picture_copy (MaruBrillCodecState *s, int ctx_id, void *elem)
{
    AVCodecContext *avctx = NULL;
    AVPicture *src = NULL;
    int pict_size = 0;
    bool ret = true;

    TRACE("enter: %s\n", __func__);

    TRACE("copy decoded image of %d context.\n", ctx_id);

    avctx = CONTEXT(s, ctx_id).avctx;
    src = (AVPicture *)CONTEXT(s, ctx_id).frame;
    if (!avctx) {
        ERR("picture_copy. %d of AVCodecContext is NULL.\n", ctx_id);
        ret = false;
    } else if (!avctx->codec) {
        ERR("picture_copy. %d of AVCodec is NULL.\n", ctx_id);
        ret = false;
    } else if (!src) {
        ERR("picture_copy. %d of AVFrame is NULL.\n", ctx_id);
        ret = false;
    } else {
        TRACE("decoded image. pix_fmt: %d width: %d, height: %d\n",
                avctx->pix_fmt, avctx->width, avctx->height);
        pict_size = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);

        if ((pict_size) < 0) {
            ERR("picture size: %d\n", pict_size);
            ret = false;
        } else {
            TRACE("picture size: %d\n", pict_size);
            maru_brill_codec_push_writequeue(s, src, pict_size, ctx_id, &default_video_decode_data_handler);
        }
    }

    TRACE("leave: %s\n", __func__);

    return ret;
}

static bool codec_decode_audio(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx;
    AVPacket avpkt;
    AVFrame *audio_out = NULL;
    uint8_t *inbuf = NULL;
    int inbuf_size = 0, size = 0;
    int len = -1, got_frame = 0;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;
    int tempbuf_size = 0;

    uint8_t *out_buf = NULL;
    int out_buf_size = 0;
    int out_sample_fmt = -1;

    TRACE("enter: %s\n", __func__);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->opaque) {
        memcpy(&inbuf_size, elem->opaque, sizeof(inbuf_size));
        size = sizeof(inbuf_size);
        TRACE("decode_audio. inbuf_size %d\n", inbuf_size);

        if (inbuf_size > 0) {
            inbuf = elem->opaque + size;
        }
    } else {
        ERR("decode_audio. no input buffer\n");
        // FIXME: improve error handling
        // return false;
    }

    av_init_packet(&avpkt);
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;

    avctx = CONTEXT(s, ctx_id).avctx;
    // audio_out = CONTEXT(s, ctx_id).frame;
    audio_out = avcodec_alloc_frame();
    if (!avctx) {
        ERR("decode_audio. %d of AVCodecContext is NULL\n", ctx_id);
    } else if (!avctx->codec) {
        ERR("decode_audio. %d of AVCodec is NULL\n", ctx_id);
    } else if (!audio_out) {
        ERR("decode_audio. %d of AVFrame is NULL\n", ctx_id);
    } else {
        // avcodec_get_frame_defaults(audio_out);

        len = avcodec_decode_audio4(avctx, audio_out, &got_frame, &avpkt);
        TRACE("decode_audio. len %d, channel_layout %lld, got_frame %d\n",
            len, avctx->channel_layout, got_frame);
        if (got_frame) {
            if (av_sample_fmt_is_planar(avctx->sample_fmt)) {
                // convert PLANAR to LINEAR format
                out_sample_fmt = avctx->sample_fmt - 5;

                out_buf = resample_audio_buffer(avctx, audio_out, &out_buf_size, out_sample_fmt);
            } else {
                // TODO: not planar format
                INFO("decode_audio. cannot handle planar audio format\n");
                len = -1;
            }
        }
    }

    tempbuf_size = (sizeof(len) + sizeof(got_frame));
    if (len < 0) {
        ERR("failed to decode audio. ctx_id: %d len: %d got_frame: %d\n",
            ctx_id, len, got_frame);
        got_frame = 0;
    } else {
        tempbuf_size += (sizeof(out_sample_fmt) + sizeof(avctx->sample_rate)
                        + sizeof(avctx->channels) +  sizeof(avctx->channel_layout)
                        + sizeof(out_buf_size) + out_buf_size);
    }

    tempbuf = g_malloc(tempbuf_size);
    if (!tempbuf) {
        ERR("failed to allocate decoded audio buffer\n");
    } else {
        memcpy(tempbuf, &len, sizeof(len));
        size = sizeof(len);
        memcpy(tempbuf + size, &got_frame, sizeof(got_frame));
        size += sizeof(got_frame);
        if (got_frame) {
            memcpy(tempbuf + size, &out_sample_fmt, sizeof(out_sample_fmt));
            size += sizeof(out_sample_fmt);
            memcpy(tempbuf + size, &avctx->sample_rate, sizeof(avctx->sample_rate));
            size += sizeof(avctx->sample_rate);
            memcpy(tempbuf + size, &avctx->channels, sizeof(avctx->channels));
            size += sizeof(avctx->channels);
            memcpy(tempbuf + size, &avctx->channel_layout, sizeof(avctx->channel_layout));
            size += sizeof(avctx->channel_layout);

            memcpy(tempbuf + size, &out_buf_size, sizeof(out_buf_size));
            size += sizeof(out_buf_size);
            if (out_buf) {
                TRACE("copy resampled audio buffer\n");
                memcpy(tempbuf + size, out_buf, out_buf_size);
            }
        }
    }

    maru_brill_codec_push_writequeue(s, tempbuf, tempbuf_size, ctx_id, NULL);

    if (audio_out) {
        avcodec_free_frame(&audio_out);
    }

    if (out_buf) {
        TRACE("and release decoded_audio buffer\n");
        av_free(out_buf);
    }


    TRACE("leave: %s\n", __func__);
    return true;
}

static bool codec_encode_video(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVFrame *pict = NULL;
    AVPacket avpkt;
    uint8_t *inbuf = NULL, *outbuf = NULL;
    int inbuf_size = 0, outbuf_size = 0;
    int got_frame = 0, ret = 0, size = 0;
    int64_t in_timestamp = 0;
    int coded_frame = 0, key_frame = 0;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;
    int tempbuf_size = 0;

    TRACE("enter: %s\n", __func__);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->opaque) {
        memcpy(&inbuf_size, elem->opaque, sizeof(inbuf_size));
        size += sizeof(inbuf_size);
        memcpy(&in_timestamp, elem->opaque + size, sizeof(in_timestamp));
        size += sizeof(in_timestamp);
        TRACE("encode video. inbuf_size %d\n", inbuf_size);

        if (inbuf_size > 0) {
            inbuf = elem->opaque + size;
        }
    } else {
        TRACE("encode video. no input buffer.\n");
        // FIXME: improve error handling
        // return false;
    }

    // initialize AVPacket
    av_init_packet(&avpkt);
    avpkt.data = NULL;
    avpkt.size = 0;

    avctx = CONTEXT(s, ctx_id).avctx;
    pict = CONTEXT(s, ctx_id).frame;
    if (!avctx || !pict) {
        ERR("%d of context or frame is NULL\n", ctx_id);
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
    } else {
        TRACE("pixel format: %d inbuf: %p, picture data: %p\n",
            avctx->pix_fmt, inbuf, pict->data[0]);

        ret = avpicture_fill((AVPicture *)pict, inbuf, avctx->pix_fmt,
                            avctx->width, avctx->height);
        if (ret < 0) {
            ERR("after avpicture_fill, ret:%d\n", ret);
        } else {
            if (avctx->time_base.num == 0) {
                pict->pts = AV_NOPTS_VALUE;
            } else {
                AVRational bq =
                            {1, (G_USEC_PER_SEC * G_GINT64_CONSTANT(1000))};
                pict->pts = av_rescale_q(in_timestamp, bq, avctx->time_base);
            }
            TRACE("encode video. ticks_per_frame:%d, pts:%lld\n",
                avctx->ticks_per_frame, pict->pts);

            outbuf_size =
                (avctx->width * avctx->height * 6) + FF_MIN_BUFFER_SIZE;

            outbuf = g_malloc0(outbuf_size);

            avpkt.data = outbuf;
            avpkt.size = outbuf_size;

            if (!outbuf) {
                ERR("failed to allocate a buffer of encoding video.\n");
            } else {
                ret = avcodec_encode_video2(avctx, &avpkt, pict, &got_frame);

                TRACE("encode video. ret %d got_picture %d outbuf_size %d\n", ret, got_frame, avpkt.size);
                if (avctx->coded_frame) {
                    TRACE("encode video. keyframe %d\n", avctx->coded_frame->key_frame);
                }
            }
        }
    }

    tempbuf_size = sizeof(ret);
    if (ret < 0) {
        ERR("failed to encode video. ctx_id %d ret %d\n", ctx_id, ret);
    } else {
        tempbuf_size += avpkt.size + sizeof(coded_frame) + sizeof(key_frame);
    }

    // write encoded video data
    tempbuf = g_malloc0(tempbuf_size);
    if (!tempbuf) {
        ERR("encode video. failed to allocate encoded out buffer.\n");
    } else {
        memcpy(tempbuf, &avpkt.size, sizeof(avpkt.size));
        size = sizeof(avpkt.size);

        if ((got_frame) && outbuf) {
            // inform gstreamer plugin about the status of encoded frames
            // A flag for output buffer in gstreamer is depending on the status.
            if (avctx->coded_frame) {
                coded_frame = 1;
                // if key_frame is 0, this frame cannot be decoded independently.
                key_frame = avctx->coded_frame->key_frame;
            }
            memcpy(tempbuf + size, &coded_frame, sizeof(coded_frame));
            size += sizeof(coded_frame);
            memcpy(tempbuf + size, &key_frame, sizeof(key_frame));
            size += sizeof(key_frame);
            memcpy(tempbuf + size, outbuf, avpkt.size);
        }
    }

    if (outbuf) {
        TRACE("release encoded output buffer. %p\n", outbuf);
        g_free(outbuf);
    }

    maru_brill_codec_push_writequeue(s, tempbuf, tempbuf_size, ctx_id, NULL);

    TRACE("leave: %s\n", __func__);
    return true;
}

static bool codec_encode_audio(MaruBrillCodecState *s, int ctx_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVPacket avpkt;
    uint8_t *audio_in = NULL;
    int32_t audio_in_size = 0;
    int ret = 0, got_pkt = 0, size = 0;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;
    int tempbuf_size = 0;

    AVFrame *in_frame = NULL;
    AVFrame *resampled_frame = NULL;
    uint8_t *samples = NULL;
    int64_t in_timestamp = 0;

    TRACE("enter: %s\n", __func__);

    /*
     *  copy raw audio data from gstreamer encoder plugin
     *  audio_in_size: size of raw audio data
     *  audio_in : raw audio data
     */
    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->opaque) {
        memcpy(&audio_in_size, elem->opaque, sizeof(audio_in_size));
        size += sizeof(audio_in_size);

        memcpy(&in_timestamp, elem->opaque + size, sizeof(in_timestamp));
        size += sizeof(in_timestamp);

        TRACE("encode_audio. audio_in_size %d\n", audio_in_size);
        if (audio_in_size > 0) {
            // audio_in = g_malloc0(audio_in_size);
            // memcpy(audio_in, elem->buf + size, audio_in_size);
            audio_in = elem->opaque + size;
        }
    } else {
        TRACE("encode_audio. no input buffer\n");
        // FIXME: improve error handling
        // return false;
    }

    avctx = CONTEXT(s, ctx_id).avctx;
    if (!avctx) {
        ERR("[%s] %d of Context is NULL!\n", __func__, ctx_id);
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
    } else {
        int bytes_per_sample = 0;
        int audio_in_buffer_size = 0;
        int audio_in_sample_fmt = AV_SAMPLE_FMT_S16;

        in_frame = avcodec_alloc_frame();
        if (!in_frame) {
            // FIXME: error handling
            ERR("encode_audio. failed to allocate in_frame\n");
            ret = -1;
        }

        bytes_per_sample = av_get_bytes_per_sample(audio_in_sample_fmt);
        TRACE("bytes per sample %d, sample format %d\n", bytes_per_sample, audio_in_sample_fmt);

        in_frame->nb_samples = audio_in_size / (bytes_per_sample * avctx->channels);
        TRACE("in_frame->nb_samples %d\n", in_frame->nb_samples);

        in_frame->format = audio_in_sample_fmt;
        in_frame->channel_layout = avctx->channel_layout;

        audio_in_buffer_size = av_samples_get_buffer_size(NULL, avctx->channels, avctx->frame_size, audio_in_sample_fmt, 0);
        TRACE("audio_in_buffer_size: %d, audio_in_size %d\n", audio_in_buffer_size, audio_in_size);

        {
            samples = av_mallocz(audio_in_buffer_size);
            memcpy(samples, audio_in, audio_in_size);

            // g_free(audio_in);
            // audio_in = NULL;

            ret = avcodec_fill_audio_frame(in_frame, avctx->channels, AV_SAMPLE_FMT_S16, (const uint8_t *)samples, audio_in_size, 0);
            TRACE("fill in_frame. ret: %d frame->ch_layout %lld\n", ret, in_frame->channel_layout);
        }

        {
            AVAudioResampleContext *avr = NULL;
            uint8_t *resampled_audio = NULL;
            int resampled_buffer_size = 0, resampled_linesize = 0, convert_size;
            int resampled_nb_samples = 0;
            int resampled_sample_fmt = AV_SAMPLE_FMT_FLTP;

            avr = avresample_alloc_context();

            av_opt_set_int(avr, "in_channel_layout", avctx->channel_layout, 0);
            av_opt_set_int(avr, "in_sample_fmt", audio_in_sample_fmt , 0);
            av_opt_set_int(avr, "in_sample_rate", avctx->sample_rate, 0);
            av_opt_set_int(avr, "out_channel_layout", avctx->channel_layout, 0);
            av_opt_set_int(avr, "out_sample_fmt", resampled_sample_fmt, 0);
            av_opt_set_int(avr, "out_sample_rate", avctx->sample_rate, 0);

            resampled_nb_samples = in_frame->nb_samples; // av_get_bytes_per_samples(resampled_sample_fmt);

            if (avresample_open(avr) < 0) {
                ERR("failed to open avresample context\n");
                avresample_free(&avr);
            }

            resampled_buffer_size = av_samples_get_buffer_size(&resampled_linesize, avctx->channels, resampled_nb_samples, resampled_sample_fmt, 0);
            if (resampled_buffer_size < 0) {
                ERR("failed to get size of sample buffer %d\n", resampled_buffer_size);
                avresample_close(avr);
                avresample_free(&avr);
            }

            TRACE("resampled nb_samples %d linesize %d out_size %d\n", resampled_nb_samples, resampled_linesize, resampled_buffer_size);

            resampled_audio = av_mallocz(resampled_buffer_size);
            if (!resampled_audio) {
                ERR("failed to allocate resample buffer\n");
                avresample_close(avr);
                avresample_free(&avr);
            }

            // in_frame->nb_samples = nb_samples;
            resampled_frame = avcodec_alloc_frame();
            if (!resampled_frame) {
                // FIXME: error handling
                ERR("encode_audio. failed to allocate resampled_frame\n");
                ret = -1;
            }


            bytes_per_sample = av_get_bytes_per_sample(audio_in_sample_fmt);
            TRACE("bytes per sample %d, sample format %d\n", bytes_per_sample, audio_in_sample_fmt);

            resampled_frame->nb_samples = in_frame->nb_samples;
            TRACE("resampled_frame->nb_samples %d\n", resampled_frame->nb_samples);

            resampled_frame->format = resampled_sample_fmt;
            resampled_frame->channel_layout = avctx->channel_layout;

            ret = avcodec_fill_audio_frame(resampled_frame, avctx->channels, resampled_sample_fmt,
                    (const uint8_t *)resampled_audio, resampled_buffer_size, 0);
            TRACE("fill resampled_frame ret: %d frame->ch_layout %lld\n", ret, in_frame->channel_layout);

            convert_size = avresample_convert(avr, resampled_frame->data, resampled_buffer_size, resampled_nb_samples,
                                        in_frame->data, audio_in_size, in_frame->nb_samples);

            TRACE("resample_audio convert_size %d\n", convert_size);

            avresample_close(avr);
            avresample_free(&avr);
        }

        if (ret == 0) {
            av_init_packet(&avpkt);
            // packet data will be allocated by encoder
            avpkt.data = NULL;
            avpkt.size = 0;

            ret = avcodec_encode_audio2(avctx, &avpkt, (const AVFrame *)resampled_frame, &got_pkt);
            TRACE("encode audio. ret %d got_pkt %d avpkt.size %d frame_number %d coded_frame %p\n",
                ret, got_pkt, avpkt.size, avctx->frame_number, avctx->coded_frame);
        }
    }

    tempbuf_size = sizeof(ret);
    if (ret < 0) {
        ERR("failed to encode audio. ctx_id %d ret %d\n", ctx_id, ret);
    } else {
        // tempbuf_size += (max_size); // len;
        tempbuf_size += (sizeof(avpkt.size) + avpkt.size);
    }
    TRACE("encode_audio. writequeue elem buffer size %d\n", tempbuf_size);

    // write encoded audio data
    tempbuf = g_malloc0(tempbuf_size);
    if (!tempbuf) {
        ERR("encode audio. failed to allocate encoded out buffer.\n");
    } else {
        memcpy(tempbuf, &ret, sizeof(ret));
        size = sizeof(ret);
        if (ret == 0) {
            memcpy(tempbuf + size, &avpkt.size, sizeof(avpkt.size));
            size += sizeof(avpkt.size);

            if (got_pkt) {
                memcpy(tempbuf + size, avpkt.data, avpkt.size);
                av_free_packet(&avpkt);
            }
        }
    }

    maru_brill_codec_push_writequeue(s, tempbuf, tempbuf_size, ctx_id, NULL);
    if (in_frame) {
        avcodec_free_frame(&in_frame);
    }

    if (resampled_frame) {
        avcodec_free_frame(&resampled_frame);
    }

    TRACE("[%s] leave:\n", __func__);

    return true;
}

static AVCodecParserContext *maru_brill_codec_parser_init(AVCodecContext *avctx)
{
    AVCodecParserContext *parser = NULL;

    if (!avctx) {
        ERR("context is NULL\n");
        return NULL;
    }

    switch (avctx->codec_id) {
    case CODEC_ID_MPEG4:
    case CODEC_ID_VC1:
        TRACE("not using parser\n");
        break;
    case CODEC_ID_H264:
        if (avctx->extradata_size == 0) {
            TRACE("H.264 with no extradata, creating parser.\n");
            parser = av_parser_init (avctx->codec_id);
        }
        break;
    default:
        parser = av_parser_init(avctx->codec_id);
        if (parser) {
            INFO("using parser: %s\n", avctx->codec->name);
        }
        break;
    }

    return parser;
}
