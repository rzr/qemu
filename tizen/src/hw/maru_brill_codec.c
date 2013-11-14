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

#include "maru_brill_codec.h"

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, brillcodec);

// device
#define CODEC_DEVICE_NAME   "brilcodec"
#define CODEC_VERSION       1

// device memory
#define CODEC_META_DATA_SIZE    (256)

#define CODEC_MEM_SIZE          (32 * 1024 * 1024)
#define CODEC_REG_SIZE          (256)

// libav
#define GEN_MASK(x) ((1 << (x)) - 1)
#define ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) & ~GEN_MASK(x))
#define ROUND_UP_2(x) ROUND_UP_X(x, 1)
#define ROUND_UP_4(x) ROUND_UP_X(x, 2)
#define ROUND_UP_8(x) ROUND_UP_X(x, 3)
#define DIV_ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) >> (x))

#define DEFAULT_VIDEO_GOP_SIZE 15


// define a queue to manage ioparam, context data
typedef struct DeviceMemEntry {
    uint8_t *buf;
    uint32_t buf_size;
    uint32_t buf_id;
    uint32_t ctx_id;

    QTAILQ_ENTRY(DeviceMemEntry) node;
} DeviceMemEntry;

typedef struct CodecDataStg {
    CodecParam *param_buf;
    DeviceMemEntry *data_buf;

    QTAILQ_ENTRY(CodecDataStg) node;
} CodecDataStg;

// define two queue to store input and output buffers.
static QTAILQ_HEAD(codec_wq, DeviceMemEntry) codec_wq =
   QTAILQ_HEAD_INITIALIZER(codec_wq);

static QTAILQ_HEAD(codec_rq, CodecDataStg) codec_rq =
   QTAILQ_HEAD_INITIALIZER(codec_rq);

static DeviceMemEntry *entry[CODEC_CONTEXT_MAX];

// pixel info
typedef struct PixFmtInfo {
    uint8_t x_chroma_shift;
    uint8_t y_chroma_shift;
} PixFmtInfo;

static PixFmtInfo pix_fmt_info[PIX_FMT_NB];

// thread
static int worker_thread_cnt = 0;
static int idle_thread_cnt = 0;
#define DEFAULT_WORKER_THREAD_CNT 8

static void *maru_brill_codec_threads(void *opaque);

// irq
static int irq_raised = 0;

// static void maru_brill_codec_reset_parser_info(MaruBrillCodecState *s, int32_t ctx_index);
static int maru_brill_codec_query_list(MaruBrillCodecState *s);
static void maru_brill_codec_release_context(MaruBrillCodecState *s, int32_t value);

// codec functions
static bool codec_init(MaruBrillCodecState *, int, int, void *);
static bool codec_deinit(MaruBrillCodecState *, int, int, void *);
static bool codec_decode_video(MaruBrillCodecState *, int, int, void *);
static bool codec_encode_video(MaruBrillCodecState *, int, int, void *);
static bool codec_decode_audio(MaruBrillCodecState *, int, int, void *);
static bool codec_encode_audio(MaruBrillCodecState *, int , int, void *);
static bool codec_picture_copy(MaruBrillCodecState *, int , int, void *);
static bool codec_flush_buffers(MaruBrillCodecState *, int , int, void *);

typedef bool (*CodecFuncEntry)(MaruBrillCodecState *, int, int, void *);
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
#if 0
static int maru_brill_codec_parser_parse (AVCodecParserContext *pctx, AVCodecContext *avctx,
                            uint8_t *inbuf, int inbuf_size,
                            int64_t pts, int64_t dts, int64_t pos);
#endif

static void maru_brill_codec_pop_writequeue(MaruBrillCodecState *s, int32_t f_id);
static void maru_brill_codec_push_readqueue(MaruBrillCodecState *s, CodecParam *ioparam);
static void maru_brill_codec_push_writequeue(MaruBrillCodecState *s, void* buf,
                                            uint32_t buf_size, int ctx_id, int f_id);

static void *maru_brill_codec_store_inbuf(uint8_t *mem_base_offset, CodecParam *ioparam);

static void maru_brill_codec_get_cpu_cores(void)
{
    worker_thread_cnt = get_number_of_processors();
    if (worker_thread_cnt < DEFAULT_WORKER_THREAD_CNT) {
        worker_thread_cnt = DEFAULT_WORKER_THREAD_CNT;
    }

    TRACE("number of threads: %d\n", worker_thread_cnt);
}

static void maru_brill_codec_threads_create(MaruBrillCodecState *s)
{
    int index;
    QemuThread *pthread = NULL;

    TRACE("enter: %s\n", __func__);

    pthread = g_malloc(sizeof(QemuThread) * worker_thread_cnt);
    if (!pthread) {
        ERR("failed to allocate threadpool memory.\n");
        return;
    }

    qemu_cond_init(&s->threadpool.cond);
    qemu_mutex_init(&s->threadpool.mutex);

    s->is_thread_running = 1;

    for (index = 0; index < worker_thread_cnt; index++) {
        qemu_thread_create(&pthread[index],
            maru_brill_codec_threads, (void *)s, QEMU_THREAD_JOINABLE);
    }

    s->threadpool.threads = pthread;

    TRACE("leave: %s\n", __func__);
}

static void maru_brill_codec_thread_exit(MaruBrillCodecState *s)
{
    int index;

    TRACE("enter: %s\n", __func__);

    /* stop to run dedicated threads. */
    s->is_thread_running = 0;

    for (index = 0; index < worker_thread_cnt; index++) {
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

static void maru_brill_codec_wakeup_threads(MaruBrillCodecState *s, int api_index)
{
    CodecParam *ioparam = NULL;

    ioparam = g_malloc0(sizeof(CodecParam));
    memcpy(ioparam, &s->ioparam, sizeof(CodecParam));

    TRACE("wakeup thread. ctx_id: %u, api_id: %u, mem_offset: 0x%x\n",
        ioparam->ctx_index, ioparam->api_index, ioparam->mem_offset);

    maru_brill_codec_push_readqueue(s, ioparam);

    qemu_mutex_lock(&s->context_mutex);
    // W/A for threads starvation.
    while (idle_thread_cnt == 0) {
        qemu_mutex_unlock(&s->context_mutex);
        TRACE("Worker threads are exhausted\n");
        usleep(2000); // wait 2ms.
        qemu_mutex_lock(&s->context_mutex);
    }
    qemu_cond_signal(&s->threadpool.cond);
    qemu_mutex_unlock(&s->context_mutex);

    TRACE("after sending conditional signal\n");
}

static void *maru_brill_codec_threads(void *opaque)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;

    TRACE("enter: %s\n", __func__);

    while (s->is_thread_running) {
        int ctx_id, f_id, api_id;
        CodecDataStg *elem = NULL;
        DeviceMemEntry *indata_buf = NULL;

        qemu_mutex_lock(&s->context_mutex);
        ++idle_thread_cnt; // protected under mutex.
        qemu_cond_wait(&s->threadpool.cond, &s->context_mutex);
        --idle_thread_cnt; // protected under mutex.
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

        api_id = elem->param_buf->api_index;
        ctx_id = elem->param_buf->ctx_index;
        f_id = elem->param_buf->file_index;
        indata_buf = elem->data_buf;

        TRACE("api_id: %d ctx_id: %d f_id: %x\n", api_id, ctx_id, f_id);

        if (!codec_func_handler[api_id](s, ctx_id, f_id, indata_buf)) {
            ERR("codec_func failure.\n");
            continue;
        }

        if (elem) {
            TRACE("release an element of CodecDataStg\n");

            if (elem->param_buf) {
                TRACE("release a buffer of CodecParam\n");
                g_free(elem->param_buf);
            }

            if (elem->data_buf) {

                if (elem->data_buf->buf) {
                    TRACE("release inbuf\n");
                    g_free(elem->data_buf->buf);
                }

                TRACE("release a buffer indata_buf\n");
                g_free(elem->data_buf);
            }

            g_free(elem);
        }

        if (api_id == CODEC_DEINIT) {
            TRACE("deinit doesn't need to raise interrupt.\n");
        } else {
            TRACE("switch context to raise interrupt.\n");
            qemu_bh_schedule(s->codec_bh);
        }
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
    case CODEC_DECODE_VIDEO ... CODEC_ENCODE_AUDIO:
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

static void *maru_brill_codec_store_inbuf(uint8_t *mem_base_offset,
                                        CodecParam *ioparam)
{
    DeviceMemEntry *elem = NULL;
    int readbuf_size, size = 0;
    uint8_t *readbuf = NULL;
    uint8_t *device_mem = NULL;

    device_mem = mem_base_offset + ioparam->mem_offset;
    if (!device_mem) {
        ERR("[%d] device memory region is null\n");
        return NULL;
    }

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
        TRACE("inbuf size is zero. api_id: %d, ctx_id: %d, mem_offset: %x\n",
            readbuf_size, ioparam->api_index, ioparam->ctx_index, ioparam->mem_offset);
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
    memset(device_mem, 0x00, sizeof(readbuf_size));

    elem->buf = readbuf;
    elem->buf_size = readbuf_size;
    elem->buf_id = ioparam->file_index;
    elem->ctx_id = ioparam->ctx_index;

    return elem;
}

static void maru_brill_codec_push_writequeue(MaruBrillCodecState *s, void* buf,
                                            uint32_t buf_size, int ctx_id, int f_id)
{
    DeviceMemEntry *elem = NULL;
    elem = g_malloc0(sizeof(DeviceMemEntry));

    elem->buf = buf;
    elem->buf_size = buf_size;
    elem->buf_id = f_id;
    elem->ctx_id = ctx_id;

    qemu_mutex_lock(&s->context_queue_mutex);
    QTAILQ_INSERT_TAIL(&codec_wq, elem, node);
    qemu_mutex_unlock(&s->context_queue_mutex);
}

static void maru_brill_codec_pop_writequeue(MaruBrillCodecState *s, int32_t file_index)
{
    DeviceMemEntry *elem = NULL;
    uint32_t mem_offset = 0, ctx_idx;

    TRACE("enter: %s\n", __func__);

    for (ctx_idx = 1; ctx_idx < CODEC_CONTEXT_MAX; ctx_idx++) {
        if (s->context[ctx_idx].file_index == file_index) {
            break;
        }
    }

    if (ctx_idx == CODEC_CONTEXT_MAX) {
        ERR("failed to find a proper entry via file_index. %x\n", file_index);
        return;
    }

    TRACE("pop_writeqeue. context index: %d\n", ctx_idx);

    elem = entry[ctx_idx];
    if (elem) {
        mem_offset = s->ioparam.mem_offset;

        // check corrupted mem_offset
        if (mem_offset < CODEC_MEM_SIZE) {
            if (elem->buf) {
                TRACE("write data %d to guest.  mem_offset: 0x%x\n",
                        elem->buf_size, mem_offset);
                memcpy(s->vaddr + mem_offset, elem->buf, elem->buf_size);

                TRACE("release output buffer: %p\n", elem->buf);
                g_free(elem->buf);
            }
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
}

#if 0
static void maru_brill_codec_reset_parser_info(MaruBrillCodecState *s, int32_t ctx_index)
{
    s->context[ctx_index].parser_buf = NULL;
    s->context[ctx_index].parser_use = false;
}
#endif

static void maru_brill_codec_release_context(MaruBrillCodecState *s, int32_t file_index)
{
    DeviceMemEntry *wq_elem = NULL, *wnext = NULL;
    CodecDataStg *rq_elem = NULL, *rnext = NULL;
    int ctx_id;

    TRACE("enter: %s\n", __func__);

    qemu_mutex_lock(&s->threadpool.mutex);
    for (ctx_id = 1; ctx_id < CODEC_CONTEXT_MAX; ctx_id++) {
        if (s->context[ctx_id].file_index == file_index) {
            TRACE("reset %d context\n", ctx_id);
            s->context[ctx_id].occupied = false;
            break;
        }
    }
    qemu_mutex_unlock(&s->threadpool.mutex);

    if (ctx_id == CODEC_CONTEXT_MAX) {
        WARN("cannot find a context for 0x%x\n", file_index);
    } else {
        codec_deinit(s, ctx_id, file_index, NULL);

        QTAILQ_FOREACH_SAFE(rq_elem, &codec_rq, node, rnext) {
            if (rq_elem && rq_elem->data_buf &&
                (rq_elem->data_buf->buf_id == file_index)) {
                TRACE("remove unused node from codec_rq. file: %p\n", file_index);
                qemu_mutex_lock(&s->context_queue_mutex);
                QTAILQ_REMOVE(&codec_rq, rq_elem, node);
                qemu_mutex_unlock(&s->context_queue_mutex);
                if (rq_elem && rq_elem->data_buf) {
                    TRACE("release_context. release rq_buffer: %p\n",
                        rq_elem->data_buf);
                    g_free(rq_elem->data_buf);
                }

                TRACE("release rq_elem: %p\n", rq_elem);
                g_free(rq_elem);
            } else {
                TRACE("remain this node in the codec_rq. %x\n",
                    rq_elem->data_buf->buf_id);
            }
        }

        QTAILQ_FOREACH_SAFE(wq_elem, &codec_wq, node, wnext) {
            if (wq_elem && wq_elem->buf_id == file_index) {
                TRACE("remove nodes from codec_wq."
                    " elem: %p, buf_id: %p, ctx_id: %d\n", wq_elem, wq_elem->ctx_id, file_index);
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
            }
        }
//        maru_brill_codec_reset_parser_info(s, ctx_id);
    }

    TRACE("leave: %s\n", __func__);
}


// initialize each pixel format.
static void maru_brill_codec_pixfmt_info_init(void)
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

static int maru_brill_codec_get_picture_size(AVPicture *picture, uint8_t *ptr,
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

static int maru_brill_codec_query_list (MaruBrillCodecState *s)
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

static int maru_brill_codec_get_context_index(MaruBrillCodecState *s)
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

    TRACE("leave: %s\n", __func__);

    return index;
}

// allocate avcontext and avframe struct.
static AVCodecContext *maru_brill_codec_alloc_context(MaruBrillCodecState *s, int index)
{
    TRACE("enter: %s\n", __func__);

    TRACE("allocate %d of context and frame.\n", index);
    s->context[index].avctx = avcodec_alloc_context();
    s->context[index].frame = avcodec_alloc_frame();
    s->context[index].opened = false;

    s->context[index].parser_buf = NULL;
    s->context[index].parser_use = false;

    TRACE("leave: %s\n", __func__);

    return s->context[index].avctx;
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
    INFO("%s!! find %s %s\n",
        codec ? "success" : "failure",
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

    memcpy(&audio, mem_buf + size, sizeof(int32_t) * 6);
    size += sizeof(int32_t) * 6;
    memcpy(&audio.bits_per_smp_fmt, mem_buf + size, sizeof(int32_t));
    size += sizeof(int32_t);
    memcpy(&audio.channel_layout, mem_buf + size, sizeof(int64_t));
    size += sizeof(int64_t);
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
    if (avctx->extradata_size > 0) {
        TRACE("extradata size: %d.\n", avctx->extradata_size);
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
static void write_codec_init_data(AVCodecContext *avctx, uint8_t *mem_buf)
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
    }
}

// write the result of codec_decode_video
static void write_codec_decode_video_data(AVCodecContext *avctx, int len,
                                        int got_pic_ptr, uint8_t *mem_buf)
{
    struct video_data video;
    int size = 0;

    memcpy(mem_buf, &len, sizeof(len));
    size = sizeof(len);
    memcpy(mem_buf + size, &got_pic_ptr, sizeof(got_pic_ptr));
    size += sizeof(got_pic_ptr);
    if (avctx) {
        deserialize_video_data(avctx, &video);
        memcpy(mem_buf + size, &video, sizeof(struct video_data));
    }
}

// write the result of codec_decode_audio
static void write_codec_decode_audio_data(int sample_rate, int channel,
                                        int64_t channel_layout, int len,
                                        int frame_size_ptr, uint8_t *mem_buf)
{
    int size = 0;

    TRACE("copy decode_audio. len %d, frame_size %d\n", len, frame_size_ptr);

    memcpy(mem_buf, &sample_rate, sizeof(sample_rate));
    size = sizeof(sample_rate);
    memcpy(mem_buf + size, &channel, sizeof(channel));
    size += sizeof(channel);
    memcpy(mem_buf + size, &channel_layout, sizeof(channel_layout));
    size += sizeof(channel_layout);
    memcpy(mem_buf + size, &len, sizeof(len));
    size += sizeof(len);
    memcpy(mem_buf + size, &frame_size_ptr, sizeof(frame_size_ptr));
}

// codec functions
static bool codec_init(MaruBrillCodecState *s, int ctx_id, int f_id, void *elem)
{
    AVCodecContext *avctx = NULL;
    AVCodec *codec = NULL;
    int size = 0, ret = -1;
    uint8_t *meta_buf = NULL;

    TRACE("enter: %s\n", __func__);

    // assign meta_buf
    meta_buf = s->vaddr + ((ctx_id - 1) * CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    // allocate AVCodecContext
    avctx = maru_brill_codec_alloc_context(s, ctx_id);
    if (!avctx) {
        ERR("[%d] failed to allocate context.\n", __LINE__);
        ret = -1;
    } else {
        s->context[ctx_id].file_index = f_id;

        codec = maru_brill_codec_find_avcodec(meta_buf);
        if (codec) {
            size = sizeof(int32_t) + 32; // buffer size of codec_name
            read_codec_init_data(avctx, meta_buf + size);

            ret = avcodec_open(avctx, codec);
            INFO("avcodec_open done. ctx_id: %d\n", ctx_id);

            s->context[ctx_id].opened = true;
            s->context[ctx_id].parser_ctx =
                maru_brill_codec_parser_init(avctx);
        } else {
            ERR("failed to find codec. ctx_id: %d\n", ctx_id);
        }
    }

    // return the result of avcodec_open
    memcpy(meta_buf, &ret, sizeof(ret));
    size = sizeof(ret);
    if (ret < 0) {
        ERR("failed to open codec contex.\n");
    } else {
        write_codec_init_data(avctx, meta_buf + size);
    }

    maru_brill_codec_push_writequeue(s, NULL, 0, ctx_id, f_id);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_deinit(MaruBrillCodecState *s, int ctx_id, int f_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVFrame *frame = NULL;
    AVCodecParserContext *parserctx = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    frame = s->context[ctx_id].frame;
    parserctx = s->context[ctx_id].parser_ctx;
    if (!avctx || !frame) {
        TRACE("%d of AVCodecContext or AVFrame is NULL. "
            " Those resources have been released before.\n", ctx_id);
        return false;
    } else {
        qemu_mutex_lock(&s->threadpool.mutex);
        if (!s->context[ctx_id].opened) {
            TRACE("%d of context has already been closed\n", ctx_id);
            qemu_mutex_unlock(&s->threadpool.mutex);
            return false;
        }
        avcodec_close(avctx);
        INFO("close avcontext of %d\n", ctx_id);
        s->context[ctx_id].opened = false;
        qemu_mutex_unlock(&s->threadpool.mutex);

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

    TRACE("leave: %s\n", __func__);

    return true;
}

//
static bool codec_flush_buffers(MaruBrillCodecState *s, int ctx_id, int f_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;

    TRACE("enter: %s\n", __func__);

    avctx = s->context[ctx_id].avctx;
    if (!avctx) {
        ERR("%d of AVCodecContext is NULL.\n", ctx_id);
        return false;
    } else {
        avcodec_flush_buffers(avctx);
        TRACE("flush %d context of buffers.\n", ctx_id);
    }

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_decode_video(MaruBrillCodecState *s, int ctx_id, int f_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVFrame *picture = NULL;
    AVPacket avpkt;
    int got_pic_ptr = 0, len = 0;
    uint8_t *inbuf = NULL;
    int inbuf_size, idx, size = 0;
    int64_t in_offset;

    DeviceMemEntry *elem = NULL;
    uint8_t *meta_buf = NULL;

    TRACE("enter: %s\n", __func__);

    meta_buf = s->vaddr + ((ctx_id - 1) * CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    size = sizeof(inbuf_size);
    memcpy(&idx, meta_buf + size, sizeof(idx));
    size += sizeof(idx);
    memcpy(&in_offset, meta_buf + size, sizeof(in_offset));
    size += sizeof(in_offset);
    TRACE("decode_video. input buffer size: %d\n", inbuf_size);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem) {
        inbuf_size = 0;
        TRACE("decode_video. no input buffer. ctx_id: %d, %p, %d\n",
            ctx_id, inbuf, elem->buf_size);
    } else {
        ERR("wrong input data\n");
        return false;
    }

    av_init_packet(&avpkt);
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;

    avctx = s->context[ctx_id].avctx;
    picture = s->context[ctx_id].frame;
    if (!avctx || !picture) {
        ERR("%d of AVCodecContext or AVFrame is NULL.\n", ctx_id);
        len = -1;
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        len = -1;
    } else {
        // in case of skipping frames
        picture->pict_type = -1;

        len = avcodec_decode_video2(avctx, picture, &got_pic_ptr, &avpkt);
        if (len < 0) {
            ERR("failed to decode video. ctx index: %d\n", ctx_id);
        }
    }

    TRACE("after decoding video. len: %d, have_data: %d\n",
        len, got_pic_ptr);

    write_codec_decode_video_data(avctx, len, got_pic_ptr, meta_buf);
    maru_brill_codec_push_writequeue(s, NULL, 0, ctx_id, f_id);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_picture_copy (MaruBrillCodecState *s, int ctx_id, int f_id, void *elem)
{
    AVCodecContext *avctx;
    AVPicture *src;
    AVPicture dst;
    uint8_t *out_buffer = NULL, *tempbuf = NULL;
    int pict_size = 0;

    TRACE("enter: %s\n", __func__);

    TRACE("copy decoded image of %d context.\n", ctx_id);

    avctx = s->context[ctx_id].avctx;
    src = (AVPicture *)s->context[ctx_id].frame;
    if (!avctx || !src) {
        ERR("%d of AVCodecContext or AVFrame is NULL.\n", ctx_id);
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
    } else {
        TRACE("decoded image. pix_fmt: %d width: %d, height: %d\n",
                avctx->pix_fmt, avctx->width, avctx->height);

        pict_size =
            maru_brill_codec_get_picture_size(&dst, NULL, avctx->pix_fmt,
                    avctx->width, avctx->height, false);
        if ((pict_size) < 0) {
            ERR("picture size: %d\n", pict_size);
        } else {
            TRACE("picture size: %d\n", pict_size);
            av_picture_copy(&dst, src, avctx->pix_fmt,
                            avctx->width, avctx->height);

            tempbuf = g_malloc0(pict_size);
            if (!tempbuf) {
                ERR("failed to allocate a picture buffer. size: %d\n", pict_size);
            } else {
                out_buffer = dst.data[0];
                memcpy(tempbuf, out_buffer, pict_size);
            }
            av_free(out_buffer);
        }
    }
    maru_brill_codec_push_writequeue(s, tempbuf, pict_size, ctx_id, f_id);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_decode_audio(MaruBrillCodecState *s, int ctx_id, int f_id, void *data_buf)
{
    AVCodecContext *avctx;
    AVPacket avpkt;
    int16_t *samples = NULL;
    uint8_t *inbuf = NULL;
    int inbuf_size = 0;
    int len = 0, frame_size_ptr = 0;
    uint8_t *meta_buf = NULL;
    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;

    TRACE("enter: %s\n", __func__);

    meta_buf = s->vaddr + ((ctx_id - 1) * CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    TRACE("before decoding audio. inbuf_size: %d\n", inbuf_size);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem) {
        inbuf_size = 0;
        TRACE("decode_audio. no input buffer.\n");
    } else {
        ERR("wrong input data\n");
        return false;
    }

    av_init_packet(&avpkt);
    avpkt.data = inbuf;
    avpkt.size = inbuf_size;

    avctx = s->context[ctx_id].avctx;
    if (!avctx) {
        ERR("[%s] %d of AVCodecContext is NULL!\n", __func__, ctx_id);
        write_codec_decode_audio_data(0, 0, 0, -1, 0, meta_buf);
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        write_codec_decode_audio_data(0, 0, 0, -1, 0, meta_buf);
    } else {
        frame_size_ptr = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        samples = av_mallocz(frame_size_ptr);
        if (!samples) {
            ERR("failed to allocate an outbuf of audio.\n");
            len = -1;
        } else {
            len =
                avcodec_decode_audio3(avctx, samples, &frame_size_ptr, &avpkt);

            TRACE("audio. len %d, channel_layout %lld, frame_size %d\n",
                len, avctx->channel_layout, frame_size_ptr);
        }

        write_codec_decode_audio_data(avctx->sample_rate, avctx->channels,
                                    avctx->channel_layout, len,
                                    frame_size_ptr, meta_buf);
    }

    if (len < 0) {
        ERR("failed to decode audio. ctx_id: %d, len: %d\n", ctx_id, len);
        frame_size_ptr = 0;
    } else {
        if (frame_size_ptr > 0) {
            tempbuf = g_malloc0(frame_size_ptr);
            if (!tempbuf) {
                ERR("failed to allocate decoded audio buffer. "
                    "ctx_id: %d, outbuf_size: %d\n", ctx_id, frame_size_ptr);
            } else {
                memcpy(tempbuf, samples, frame_size_ptr);
            }
            av_free(samples);
        }
    }

    maru_brill_codec_push_writequeue(s, tempbuf, frame_size_ptr, ctx_id, f_id);

    TRACE("leave: %s\n", __func__);

    return true;
}

static bool codec_encode_video(MaruBrillCodecState *s, int ctx_id, int f_id, void *data_buf)
{
    AVCodecContext *avctx = NULL;
    AVFrame *pict = NULL;
    uint8_t *inbuf = NULL, *outbuf = NULL;
    int inbuf_size, outbuf_size, len = 0;
    int ret;
    int64_t in_timestamp;

    uint8_t *meta_buf = NULL;
    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;

    TRACE("enter: %s\n", __func__);

    meta_buf = s->vaddr + ((ctx_id - 1) * CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    memcpy(&in_timestamp, meta_buf + sizeof(inbuf_size), sizeof(in_timestamp));
    TRACE("input buffer size: %d\n", inbuf_size);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem) {
        TRACE("encode_video. no input buffer.\n");
        inbuf_size = 0;
    } else {
        ERR("wrong input data\n");
        return false;
    }

    avctx = s->context[ctx_id].avctx;
    pict = s->context[ctx_id].frame;
    if (!avctx || !pict) {
        ERR("%d of context or frame is NULL\n", ctx_id);
        len = -1;
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
        len = -1;
    } else {
        TRACE("pixel format: %d inbuf: %p, picture data: %p\n",
                avctx->pix_fmt, inbuf, pict->data[0]);

        ret =
            maru_brill_codec_get_picture_size((AVPicture *)pict, inbuf,
                                            avctx->pix_fmt, avctx->width,
                                            avctx->height, true);
        if (ret < 0) {
            ERR("after avpicture_fill, ret:%d\n", ret);
            len = -1;
        } else {
            if (avctx->time_base.num == 0) {
                pict->pts = AV_NOPTS_VALUE;
            } else {
                AVRational bq =
                    {1, (G_USEC_PER_SEC * G_GINT64_CONSTANT(1000))};
                pict->pts = av_rescale_q(in_timestamp, bq, avctx->time_base);
            }
            TRACE("before encode video, ticks_per_frame:%d, pts:%lld\n",
                    avctx->ticks_per_frame, pict->pts);

            outbuf_size =
                (avctx->width * avctx->height * 6) + FF_MIN_BUFFER_SIZE;
            outbuf = g_malloc0(outbuf_size);
            if (!outbuf) {
                ERR("failed to allocate a buffer of encoding video.\n");
                len = -1;
            } else {
                len = avcodec_encode_video(avctx, outbuf, outbuf_size, pict);

                TRACE("encode video! len:%d pts:%lld outbuf:%p outbuf size: %d\n",
                        len, pict->pts, outbuf, outbuf_size);
           }
        }
    }

    // write encoded video data
    memcpy(meta_buf, &len, sizeof(len));
    if (len < 0) {
        ERR("failed to encode video. len: %d\n", len);
    } else {
        tempbuf = g_malloc0(len);
        if (!tempbuf) {
            ERR("failed to allocate an element of writequeue.\n");
        } else {
            memcpy(tempbuf, outbuf, len);
        }
    }

    if (outbuf) {
        TRACE("release encoded output buffer. %p\n", outbuf);
        g_free(outbuf);
    }

    maru_brill_codec_push_writequeue(s, tempbuf, len, ctx_id, f_id);

    TRACE("leave: %s\n", __func__);
    return true;
}

static bool codec_encode_audio(MaruBrillCodecState *s, int ctx_id, int f_id, void *data_buf)
{
    AVCodecContext *avctx;
    uint8_t *inbuf = NULL, *outbuf = NULL;
    int32_t inbuf_size, max_size;
    int len = 0;
    uint8_t *meta_buf = NULL;

    DeviceMemEntry *elem = NULL;
    uint8_t *tempbuf = NULL;

    TRACE("enter: %s\n", __func__);

    meta_buf = s->vaddr + ((ctx_id - 1) * CODEC_META_DATA_SIZE);
    meta_buf += 8; // skipped header.

    memcpy(&inbuf_size, meta_buf, sizeof(inbuf_size));
    memcpy(&max_size, meta_buf + sizeof(inbuf_size), sizeof(max_size));
    TRACE("encoding audio. in_size: %d, max_size: %d\n", inbuf_size, max_size);

    elem = (DeviceMemEntry *)data_buf;
    if (elem && elem->buf) {
        inbuf = elem->buf;
    } else if (elem) {
        TRACE("no input buffer to encode audio.\n");
        inbuf_size = 0;
    } else {
        ERR("wrong input data\n");
        return false;
    }

    avctx = s->context[ctx_id].avctx;
    if (!avctx) {
        ERR("[%s] %d of Context is NULL!\n", __func__, ctx_id);
    } else if (!avctx->codec) {
        ERR("%d of AVCodec is NULL.\n", ctx_id);
    } else {
        outbuf = g_malloc0(max_size + FF_MIN_BUFFER_SIZE);
        if (!outbuf) {
            ERR("failed to allocate a buffer of encoding audio.\n");
            len = -1;
        } else {
            len =
                avcodec_encode_audio(avctx, outbuf, max_size, (short *)inbuf);
            TRACE("after encoding audio. len: %d\n", len);
        }
    }

    // write encoded audio data
    memcpy(meta_buf, &len, sizeof(len));
    if (len < 0) {
        ERR("failed to encode audio. ctx_id: %d len: %d\n", ctx_id, len);
    } else {
        tempbuf = g_malloc0(len);
        if (!tempbuf) {
            ERR("encode_audio. failed to allocate temporary buffer.\n");
        } else {
            memcpy(tempbuf, outbuf, len);
        }
    }

    if (outbuf) {
        av_free(outbuf);
    }

    maru_brill_codec_push_writequeue(s, tempbuf, len, ctx_id, f_id);

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

//
static void maru_brill_codec_bh_callback(void *opaque)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;

    TRACE("enter: %s\n", __func__);

    qemu_mutex_lock(&s->context_queue_mutex);
    if (!QTAILQ_EMPTY(&codec_wq)) {
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("raise irq\n");
        qemu_irq_raise(s->dev.irq[0]);
        irq_raised = 1;
    } else {
        qemu_mutex_unlock(&s->context_queue_mutex);
        TRACE("codec_wq is empty!!\n");
    }

    TRACE("leave: %s\n", __func__);
}

/*
 *  Codec Device APIs
 */
static uint64_t maru_brill_codec_read(void *opaque,
                                        target_phys_addr_t addr,
                                        unsigned size)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;
    uint64_t ret = 0;

    switch (addr) {
    case CODEC_CMD_GET_THREAD_STATE:
#if 0
        if (s->thread_state) {
            s->thread_state = CODEC_TASK_START;

            qemu_mutex_lock(&s->context_queue_mutex);
            if (!QTAILQ_EMPTY(&codec_wq)) {
                ret = CODEC_TASK_END;
            }
            qemu_mutex_unlock(&s->context_queue_mutex);
        }
#endif
        qemu_mutex_lock(&s->context_queue_mutex);
        if (irq_raised) {
            ret = CODEC_TASK_END;
            qemu_irq_lower(s->dev.irq[0]);
            irq_raised = 0;
        }
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("get thread_state. ret: %d\n", ret);
        break;

    case CODEC_CMD_GET_CTX_FROM_QUEUE:
    {
        DeviceMemEntry *head = NULL;

        qemu_mutex_lock(&s->context_queue_mutex);
        head = QTAILQ_FIRST(&codec_wq);
        if (head) {
            ret = head->ctx_id;
            QTAILQ_REMOVE(&codec_wq, head, node);
            entry[ret] = head;
            TRACE("get a elem from codec_wq. 0x%x\n", head);
        } else {
            ret = 0;
        }
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("get a head from a writequeue. head: %x\n", ret);
    }
        break;

    case CODEC_CMD_GET_VERSION:
        ret = CODEC_VERSION;
        TRACE("codec version: %d\n", ret);
        break;

    case CODEC_CMD_GET_ELEMENT:
        ret = maru_brill_codec_query_list(s);
        break;

    case CODEC_CMD_GET_CONTEXT_INDEX:
        ret = maru_brill_codec_get_context_index(s);
        TRACE("get context index: %d\n", ret);
        break;

    default:
        ERR("no avaiable command for read. %d\n", addr);
    }

    return ret;
}

static void maru_brill_codec_write(void *opaque, target_phys_addr_t addr,
                                    uint64_t value, unsigned size)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;

    switch (addr) {
    case CODEC_CMD_API_INDEX:
        TRACE("set codec_cmd value: %d\n", value);
        s->ioparam.api_index = value;
        maru_brill_codec_wakeup_threads(s, value);
        break;

    case CODEC_CMD_CONTEXT_INDEX:
        TRACE("set context_index value: %d\n", value);
        s->ioparam.ctx_index = value;
        break;

    case CODEC_CMD_FILE_INDEX:
        TRACE("set file_index value: 0x%x\n", value);
        s->ioparam.file_index = value;
        break;

    case CODEC_CMD_DEVICE_MEM_OFFSET:
        TRACE("set mem_offset value: 0x%x\n", value);
        s->ioparam.mem_offset = value;
        break;

    case CODEC_CMD_RELEASE_CONTEXT:
        maru_brill_codec_release_context(s, (int32_t)value);
        break;

    case CODEC_CMD_GET_DATA_FROM_QUEUE:
        maru_brill_codec_pop_writequeue(s, (uint32_t)value);
        break;

    default:
        ERR("no available command for write. %d\n", addr);
    }
}

static const MemoryRegionOps maru_brill_codec_mmio_ops = {
    .read = maru_brill_codec_read,
    .write = maru_brill_codec_write,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static int maru_brill_codec_initfn(PCIDevice *dev)
{
    MaruBrillCodecState *s = DO_UPCAST(MaruBrillCodecState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    INFO("device initialization.\n");
    memset(&s->ioparam, 0, sizeof(CodecParam));

    qemu_mutex_init(&s->context_mutex);
    qemu_mutex_init(&s->context_queue_mutex);
    qemu_mutex_init(&s->ioparam_queue_mutex);

    maru_brill_codec_get_cpu_cores();
    maru_brill_codec_threads_create(s);

    maru_brill_codec_pixfmt_info_init();

    // register a function to qemu bottom-halves to switch context.
    s->codec_bh = qemu_bh_new(maru_brill_codec_bh_callback, s);

    pci_config_set_interrupt_pin(pci_conf, 1);

    memory_region_init_ram(&s->vram, "maru_brill_codec.vram", CODEC_MEM_SIZE);
    s->vaddr = (uint8_t *)memory_region_get_ram_ptr(&s->vram);

    memory_region_init_io(&s->mmio, &maru_brill_codec_mmio_ops, s,
                        "maru_brill_codec.mmio", CODEC_REG_SIZE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->vram);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    return 0;
}

static void maru_brill_codec_exitfn(PCIDevice *dev)
{
    MaruBrillCodecState *s = DO_UPCAST(MaruBrillCodecState, dev, dev);
    INFO("device exit\n");

    qemu_mutex_destroy(&s->context_mutex);
    qemu_mutex_destroy(&s->context_queue_mutex);
    qemu_mutex_destroy(&s->ioparam_queue_mutex);

    qemu_bh_delete(s->codec_bh);

    memory_region_destroy(&s->vram);
    memory_region_destroy(&s->mmio);
}

static void maru_brill_codec_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = maru_brill_codec_initfn;
    k->exit = maru_brill_codec_exitfn;
    k->vendor_id = PCI_VENDOR_ID_TIZEN;
    k->device_id = PCI_DEVICE_ID_VIRTUAL_BRILL_CODEC;
    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "Virtual new codec device for Tizen emulator";
}

static TypeInfo codec_device_info = {
    .name          = CODEC_DEVICE_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MaruBrillCodecState),
    .class_init    = maru_brill_codec_class_init,
};

static void codec_register_types(void)
{
    type_register_static(&codec_device_info);
}

type_init(codec_register_types)

int maru_brill_codec_pci_device_init(PCIBus *bus)
{
    INFO("device create.\n");
    pci_create_simple(bus, -1, CODEC_DEVICE_NAME);
    return 0;
}
