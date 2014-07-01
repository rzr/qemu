/*
 * Virtual Codec device
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *  DongKyun Yun
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

#include "maru_codec.h"
#include "qemu-common.h"

#define MARU_CODEC_DEV_NAME     "codec"
#define MARU_CODEC_VERSION      14

/*  Needs 16M to support 1920x1080 video resolution.
 *  Output size for encoding has to be greater than (width * height * 6)
 */
#define MARU_CODEC_MEM_SIZE     (2 * 16 * 1024 * 1024)
#define MARU_CODEC_REG_SIZE     (256)
#define MARU_CODEC_IRQ 0x7f

#define GEN_MASK(x) ((1<<(x))-1)
#define ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) & ~GEN_MASK(x))
#define ROUND_UP_2(x) ROUND_UP_X(x, 1)
#define ROUND_UP_4(x) ROUND_UP_X(x, 2)
#define ROUND_UP_8(x) ROUND_UP_X(x, 3)
#define DIV_ROUND_UP_X(v, x) (((v) + GEN_MASK(x)) >> (x))

typedef struct PixFmtInfo {
    uint8_t x_chroma_shift;
    uint8_t y_chroma_shift;
} PixFmtInfo;

static PixFmtInfo pix_fmt_info[PIX_FMT_NB];

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, marucodec);

void codec_thread_init(SVCodecState *s)
{
    int index = 0;
    QemuThread *pthread = NULL;
    TRACE("Enter, %s\n", __func__);

    pthread = g_malloc0(sizeof(QemuThread) * CODEC_MAX_THREAD);
    if (!pthread) {
        ERR("Failed to allocate wrk_thread memory.\n");
        return;
    }
    qemu_cond_init(&s->codec_thread.cond);
    qemu_mutex_init(&s->codec_thread.mutex);

    qemu_mutex_lock(&s->thread_mutex);
    s->isrunning = 1;
    qemu_mutex_unlock(&s->thread_mutex);

    for (; index < CODEC_MAX_THREAD; index++) {
        qemu_thread_create(&pthread[index], codec_worker_thread, (void *)s,
            QEMU_THREAD_JOINABLE);
    }

    s->codec_thread.wrk_thread = pthread;
    TRACE("Leave, %s\n", __func__);
}

void codec_thread_exit(SVCodecState *s)
{
    TRACE("Enter, %s\n", __func__);
    int index;

    /* stop to run dedicated threads. */
    s->isrunning = 0;

    for (index = 0; index < CODEC_MAX_THREAD; index++) {
        qemu_thread_join(&s->codec_thread.wrk_thread[index]);
    }

    TRACE("destroy mutex and conditional.\n");
    qemu_mutex_destroy(&s->codec_thread.mutex);
    qemu_cond_destroy(&s->codec_thread.cond);

    TRACE("Leave, %s\n", __func__);
}

void wake_codec_worker_thread(SVCodecState *s)
{
    TRACE("Enter, %s\n", __func__);

    qemu_cond_signal(&s->codec_thread.cond);
    TRACE("sent a conditional signal to a worker thread.\n");

    TRACE("Leave, %s\n", __func__);
}

void *codec_worker_thread(void *opaque)
{
    SVCodecState *s = (SVCodecState *)opaque;
    QemuThread thread;
    AVCodecContext *avctx;

    TRACE("Enter, %s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    while (s->isrunning) {
        TRACE("wait for conditional signal.\n");
        qemu_cond_wait(&s->codec_thread.cond, &s->thread_mutex);

        qemu_thread_get_self(&thread);
#ifdef CONFIG_LINUX
        TRACE("wake up a worker thread. :%x\n", thread.thread);
#endif
        avctx = s->codec_ctx[s->codec_param.ctx_index].avctx;
        if (avctx) {
            if (avctx->codec->decode) {
                decode_codec(s);
            } else {
                encode_codec(s);
            }
        } else {
            ERR("there is a synchrous problem "
                "between each context.\n");
            continue;
        }

        s->codec_thread.state = MARU_CODEC_IRQ;
        qemu_bh_schedule(s->tx_bh);
    }
    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("Leave, %s\n", __func__);

    return NULL;
}

int decode_codec(SVCodecState *s)
{
    AVCodecContext *avctx;
    uint32_t len = 0, ctx_index;

    TRACE("Enter, %s\n", __func__);

    qemu_mutex_lock(&s->codec_thread.mutex);
    ctx_index = s->codec_param.ctx_index;
    qemu_mutex_unlock(&s->codec_thread.mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        len = qemu_avcodec_decode_video(s, ctx_index);
    } else {
        len = qemu_avcodec_decode_audio(s, ctx_index);
    }

    TRACE("Leave, %s\n", __func__);
    return len;
}

int encode_codec(SVCodecState *s)
{
    AVCodecContext *avctx;
    uint32_t len = 0, ctx_index;

    TRACE("Enter, %s\n", __func__);

    qemu_mutex_unlock(&s->thread_mutex);

    ctx_index = s->codec_param.ctx_index;
    avctx = s->codec_ctx[ctx_index].avctx;
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        len = qemu_avcodec_encode_video(s, ctx_index);
    } else {
        len = qemu_avcodec_encode_audio(s, ctx_index);
    }

    qemu_mutex_lock(&s->thread_mutex);

    TRACE("Leave, %s\n", __func__);
    return len;
}

static int qemu_serialize_rational(const AVRational *elem, uint8_t *buff)
{
    int size = 0;

    memcpy(buff + size, &elem->num, sizeof(elem->num));
    size += sizeof(elem->num);
    memcpy(buff + size, &elem->den, sizeof(elem->den));
    size += sizeof(elem->den);

    return size;
}

static int qemu_deserialize_rational(const uint8_t *buff, AVRational *elem)
{
    int size = 0;

    memset(elem, 0, sizeof(*elem));

    memcpy(&elem->num, buff + size, sizeof(elem->num));
    size += sizeof(elem->num);
    memcpy(&elem->den, buff + size, sizeof(elem->den));
    size += sizeof(elem->den);

    return size;
}

static int qemu_serialize_frame(const AVFrame *elem, uint8_t *buff)
{
    int size = 0;

    memcpy(buff + size, &elem->key_frame, sizeof(elem->key_frame));
    size += sizeof(elem->key_frame);
    memcpy(buff + size, &elem->pict_type, sizeof(elem->pict_type));
    size += sizeof(elem->pict_type);
    memcpy(buff + size, &elem->pts, sizeof(elem->pts));
    size += sizeof(elem->pts);
    memcpy(buff + size, &elem->coded_picture_number,
            sizeof(elem->coded_picture_number));
    size += sizeof(elem->coded_picture_number);
    memcpy(buff + size, &elem->display_picture_number,
            sizeof(elem->display_picture_number));
    size += sizeof(elem->display_picture_number);
    memcpy(buff + size, &elem->quality, sizeof(elem->quality));
    size += sizeof(elem->quality);
    memcpy(buff + size, &elem->age, sizeof(elem->age));
    size += sizeof(elem->age);
    memcpy(buff + size, &elem->reference, sizeof(elem->reference));
    size += sizeof(elem->reference);
    memcpy(buff + size, &elem->reordered_opaque,
            sizeof(elem->reordered_opaque));
    size += sizeof(elem->reordered_opaque);
    memcpy(buff + size, &elem->repeat_pict, sizeof(elem->repeat_pict));
    size += sizeof(elem->repeat_pict);
    memcpy(buff + size, &elem->interlaced_frame,
            sizeof(elem->interlaced_frame));
    size += sizeof(elem->interlaced_frame);

    return size;
}

static int qemu_deserialize_frame(const uint8_t *buff, AVFrame *elem)
{
    int size = 0;

    memset(elem, 0, sizeof(*elem));

    memcpy(&elem->linesize, buff + size, sizeof(elem->linesize));
    size += sizeof(elem->linesize);
    memcpy(&elem->key_frame, buff + size, sizeof(elem->key_frame));
    size += sizeof(elem->key_frame);
    memcpy(&elem->pict_type, buff + size, sizeof(elem->pict_type));
    size += sizeof(elem->pict_type);
    memcpy(&elem->pts, buff + size, sizeof(elem->pts));
    size += sizeof(elem->pts);
    memcpy(&elem->coded_picture_number, buff + size,
            sizeof(elem->coded_picture_number));
    size += sizeof(elem->coded_picture_number);
    memcpy(&elem->display_picture_number, buff + size,
            sizeof(elem->display_picture_number));
    size += sizeof(elem->display_picture_number);
    memcpy(&elem->quality, buff + size, sizeof(elem->quality));
    size += sizeof(elem->quality);
    memcpy(&elem->age, buff + size, sizeof(elem->age));
    size += sizeof(elem->age);
    memcpy(&elem->reference, buff + size, sizeof(elem->reference));
    size += sizeof(elem->reference);
    memcpy(&elem->qstride, buff + size, sizeof(elem->qstride));
    size += sizeof(elem->qstride);
    memcpy(&elem->motion_subsample_log2, buff + size,
            sizeof(elem->motion_subsample_log2));
    size += sizeof(elem->motion_subsample_log2);
    memcpy(&elem->error, buff + size, sizeof(elem->error));
    size += sizeof(elem->error);
    memcpy(&elem->type, buff + size, sizeof(elem->type));
    size += sizeof(elem->type);
    memcpy(&elem->repeat_pict, buff + size, sizeof(elem->repeat_pict));
    size += sizeof(elem->repeat_pict);
    memcpy(&elem->qscale_type, buff + size, sizeof(elem->qscale_type));
    size += sizeof(elem->qscale_type);
    memcpy(&elem->interlaced_frame, buff + size,
            sizeof(elem->interlaced_frame));
    size += sizeof(elem->interlaced_frame);
    memcpy(&elem->top_field_first, buff + size, sizeof(elem->top_field_first));
    size += sizeof(elem->top_field_first);
    memcpy(&elem->palette_has_changed, buff + size,
            sizeof(elem->palette_has_changed));
    size += sizeof(elem->palette_has_changed);
    memcpy(&elem->buffer_hints, buff + size, sizeof(elem->buffer_hints));
    size += sizeof(elem->buffer_hints);
    memcpy(&elem->reordered_opaque, buff + size,
            sizeof(elem->reordered_opaque));
    size += sizeof(elem->reordered_opaque);

    return size;
}

void qemu_parser_init(SVCodecState *s, int ctx_index)
{
    TRACE("[%s] Enter\n", __func__);

    s->codec_ctx[ctx_index].parser_buf = NULL;
    s->codec_ctx[ctx_index].parser_use = false;

    TRACE("[%s] Leave\n", __func__);
}

void qemu_reset_codec_info(SVCodecState *s, uint32_t value)
{
    int ctx_idx;

    TRACE("[%s] Enter\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    for (ctx_idx = 0; ctx_idx < CODEC_CONTEXT_MAX; ctx_idx++) {
        if (s->codec_ctx[ctx_idx].file_index == value) {
            TRACE("reset %d context\n", ctx_idx);
            qemu_mutex_unlock(&s->thread_mutex);
            qemu_av_free(s, ctx_idx);
            qemu_mutex_lock(&s->thread_mutex);
            s->codec_ctx[ctx_idx].avctx_use = false;
            break;
        }
    }
    qemu_parser_init(s, ctx_idx);

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("[%s] Leave\n", __func__);
}

/* void av_register_all() */
void qemu_av_register_all(void)
{
    av_register_all();
    TRACE("av_register_all\n");
}

/* int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic) */
int qemu_avcodec_get_buffer(AVCodecContext *context, AVFrame *picture)
{
    int ret;
    TRACE("avcodec_default_get_buffer\n");

    picture->reordered_opaque = context->reordered_opaque;
    picture->opaque = NULL;

    ret = avcodec_default_get_buffer(context, picture);

    return ret;
}

/* void avcodec_default_release_buffer(AVCodecContext *ctx, AVFrame *frame) */
void qemu_avcodec_release_buffer(AVCodecContext *context, AVFrame *picture)
{
    TRACE("avcodec_default_release_buffer\n");
    avcodec_default_release_buffer(context, picture);
}

static void qemu_init_pix_fmt_info(void)
{
    /* YUV formats */
    pix_fmt_info[PIX_FMT_YUV420P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUV420P].y_chroma_shift = 1;

    pix_fmt_info[PIX_FMT_YUV422P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUV422P].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUV444P].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUV444P].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUYV422].x_chroma_shift = 1;
    pix_fmt_info[PIX_FMT_YUYV422].y_chroma_shift = 0;

    pix_fmt_info[PIX_FMT_YUV410P].x_chroma_shift = 2;
    pix_fmt_info[PIX_FMT_YUV410P].y_chroma_shift = 2;

    pix_fmt_info[PIX_FMT_YUV411P].x_chroma_shift = 2;
    pix_fmt_info[PIX_FMT_YUV411P].y_chroma_shift = 0;

    /* JPEG YUV */
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

    pix_fmt_info[PIX_FMT_YUVA420P].x_chroma_shift = 1,
    pix_fmt_info[PIX_FMT_YUVA420P].y_chroma_shift = 1;
}

static uint8_t *qemu_malloc_avpicture (int picture_size)
{
    uint8_t *ptr = NULL;

    ptr = av_mallocz(picture_size);
    if (!ptr) {
        ERR("failed to allocate memory.\n");
        return NULL;
    }

    return ptr;
}

static int qemu_avpicture_fill(AVPicture *picture, uint8_t *ptr,
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
            ptr = qemu_malloc_avpicture(fsize);
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
    case PIX_FMT_YUVA420P:
        stride = ROUND_UP_4 (width);
        h2 = ROUND_UP_X (height, pinfo->y_chroma_shift);
        size = stride * h2;
        w2 = DIV_ROUND_UP_X (width, pinfo->x_chroma_shift);
        stride2 = ROUND_UP_4 (w2);
        h2 = DIV_ROUND_UP_X (height, pinfo->y_chroma_shift);
        size2 = stride2 * h2;
        fsize = 2 * size + 2 * size2;
        TRACE("stride %d, stride2 %d, size %d, size2 %d, fsize %d\n",
            stride, stride2, size, size2, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
        }
        picture->data[0] = ptr;
        picture->data[1] = picture->data[0] + size;
        picture->data[2] = picture->data[1] + size2;
        picture->data[3] = picture->data[2] + size2;
        picture->linesize[0] = stride;
        picture->linesize[1] = stride2;
        picture->linesize[2] = stride2;
        picture->linesize[3] = stride;
        TRACE("planes %d %d %d %d\n", 0, size, size + size2, size + 2 * size2);
        TRACE("strides %d %d %d %d\n", stride, stride2, stride2, stride);
        break;
    case PIX_FMT_RGB24:
    case PIX_FMT_BGR24:
        stride = ROUND_UP_4 (width * 3);
        fsize = stride * height;
        TRACE("stride: %d, fsize: %d\n", stride, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
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
        TRACE("stride: %d, fsize: %d\n", stride, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
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
    case PIX_FMT_YUYV422:
    case PIX_FMT_UYVY422:
        stride = ROUND_UP_4 (width * 2);
        fsize = stride * height;
        TRACE("stride: %d, fsize: %d\n", stride, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
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
    case PIX_FMT_UYYVYY411:
        /* FIXME, probably not the right stride */
        stride = ROUND_UP_4 (width);
        size = stride * height;
        fsize = size + size / 2;
        TRACE("stride %d, size %d, fsize %d\n", stride, size, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
        }
        picture->data[0] = ptr;
        picture->data[1] = NULL;
        picture->data[2] = NULL;
        picture->data[3] = NULL;
        picture->linesize[0] = width + width / 2;
        picture->linesize[1] = 0;
        picture->linesize[2] = 0;
        picture->linesize[3] = 0;
        break;
    case PIX_FMT_GRAY8:
        stride = ROUND_UP_4 (width);
        fsize = stride * height;
        TRACE("stride %d, fsize %d\n", stride, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
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
    case PIX_FMT_MONOWHITE:
    case PIX_FMT_MONOBLACK:
        stride = ROUND_UP_4 ((width + 7) >> 3);
        fsize = stride * height;
        TRACE("stride %d, fsize %d\n", stride, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
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
        /* already forced to be with stride, so same result as other function */
        stride = ROUND_UP_4 (width);
        size = stride * height;
        fsize = size + 256 * 4;
        TRACE("stride %d, size %d, fsize %d\n", stride, size, fsize);
        if (!encode && !ptr) {
            ptr = qemu_malloc_avpicture(fsize);
        }
        picture->data[0] = ptr;
        picture->data[1] = ptr + size;    /* palette is stored here as 256 32 bit words */
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

/* int avcodec_open(AVCodecContext *avctx, AVCodec *codec) */
int qemu_avcodec_open(SVCodecState *s)
{
    AVCodecContext *avctx;
    AVCodec *codec;
    enum CodecID codec_id;
    off_t offset;
    int ret = -1;
    int bEncode = 0;
    int size = 0;
    int ctx_index = 0;

    av_register_all();
    TRACE("av_register_all\n");

    ctx_index = qemu_avcodec_alloc_context(s);
    if (ctx_index < 0) {
        return ret;
    }

    qemu_mutex_lock(&s->thread_mutex);
    offset = s->codec_param.mmap_offset;
    qemu_mutex_unlock(&s->thread_mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of AVCodecContext is NULL!\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->thread_mutex);
        return ret;
    }

    TRACE("[%s] Context Index:%d, offset:%d\n", __func__, ctx_index, offset);
    memcpy(&avctx->bit_rate, (uint8_t *)s->vaddr + offset, sizeof(int));
    size = sizeof(int);
    memcpy(&avctx->bit_rate_tolerance,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->flags, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    size += qemu_deserialize_rational((uint8_t *)s->vaddr + offset + size,
                                        &avctx->time_base);
    memcpy(&avctx->width, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->height, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->gop_size, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->pix_fmt, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->sample_rate,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->channels, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->codec_tag, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->block_align,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->rc_strategy,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->strict_std_compliance,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->rc_qsquish,
            (uint8_t *)s->vaddr + offset + size, sizeof(float));
    size += sizeof(float);
    size += qemu_deserialize_rational((uint8_t *)s->vaddr + offset + size,
            &avctx->sample_aspect_ratio);
    memcpy(&avctx->qmin, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->qmax, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->pre_me, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->trellis, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->extradata_size,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&codec_id, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&bEncode, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    TRACE("Context Index:%d, width:%d, height:%d\n",
            ctx_index, avctx->width, avctx->height);

    if (avctx->extradata_size > 0) {
        avctx->extradata = (uint8_t *)av_mallocz(avctx->extradata_size +
                             ROUND_UP_X(FF_INPUT_BUFFER_PADDING_SIZE, 4));
        memcpy(avctx->extradata,
                (uint8_t *)s->vaddr + offset + size, avctx->extradata_size);
        size += avctx->extradata_size;
    } else {
        TRACE("[%s] allocate dummy extradata\n", __func__);
        avctx->extradata =
                av_mallocz(ROUND_UP_X(FF_INPUT_BUFFER_PADDING_SIZE, 4));
    }

    if (bEncode) {
        TRACE("[%s] find encoder :%d\n", __func__, codec_id);
        codec = avcodec_find_encoder(codec_id);
    } else {
        TRACE("[%s] find decoder :%d\n", __func__, codec_id);
        codec = avcodec_find_decoder(codec_id);
    }

    if (!codec) {
        ERR("[%s] failed to find codec of %d\n", __func__, codec_id);
    }

    if (codec->type == AVMEDIA_TYPE_AUDIO) {
        s->codec_ctx[ctx_index].mem_index = s->codec_param.mem_index;
        TRACE("set mem_index: %d into ctx_index: %d.\n",
            s->codec_ctx[ctx_index].mem_index, ctx_index);
    }

#if 0
    avctx->get_buffer = qemu_avcodec_get_buffer;
    avctx->release_buffer = qemu_avcodec_release_buffer;
#endif

    ret = avcodec_open(avctx, codec);
    if (ret != 0) {
        ERR("[%s] avcodec_open failure, %d\n", __func__, ret);
    }

    memcpy((uint8_t *)s->vaddr + offset, &ctx_index, sizeof(int));
    size = sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->pix_fmt, sizeof(int));
    size += sizeof(int);
    size += qemu_serialize_rational(&avctx->time_base,
            (uint8_t *)s->vaddr + offset + size);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->channels, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->sample_fmt, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->codec_type, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->codec_id, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->coded_width, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->coded_height, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->ticks_per_frame, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->chroma_sample_location, sizeof(int));
    size += sizeof(int);
#if 0
    memcpy((uint8_t *)s->vaddr + offset + size,
            avctx->priv_data, codec->priv_data_size);
    size += codec->priv_data_size;
#endif
    memcpy((uint8_t *)s->vaddr + offset + size, &ret, sizeof(int));
    size += sizeof(int);

    TRACE("Leave, %s\n", __func__);
    return ret;
}

/* int avcodec_close(AVCodecContext *avctx) */
int qemu_avcodec_close(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;
    off_t offset;
    int ret = -1;

    TRACE("Enter, %s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    offset = s->codec_param.mmap_offset;

    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of AVCodecContext is NULL\n", __func__, ctx_index);
        memcpy((uint8_t *)s->vaddr + offset, &ret, sizeof(int));
        qemu_mutex_unlock(&s->thread_mutex);
        return ret;
    }

    ret = avcodec_close(avctx);
    TRACE("after avcodec_close. ret:%d\n", ret);

    memcpy((uint8_t *)s->vaddr + offset, &ret, sizeof(int));

    qemu_mutex_unlock(&s->thread_mutex);

    TRACE("[%s] Leave\n", __func__);
    return ret;
}

/* AVCodecContext* avcodec_alloc_context (void) */
int qemu_avcodec_alloc_context(SVCodecState *s)
{
    int index;

    TRACE("[%s] Enter\n", __func__);

    for (index = 0; index < CODEC_CONTEXT_MAX; index++) {
        if (s->codec_ctx[index].avctx_use == false) {
            TRACE("Succeeded to get %d of context.\n", index);
            s->codec_ctx[index].avctx_use = true;
            break;
        }
        TRACE("Failed to get context.\n");
    }

    if (index == CODEC_CONTEXT_MAX) {
        ERR("Failed to get available codec context.");
        ERR(" Try to run codec again.\n");
        return -1;
    }

    TRACE("allocate %d of context and frame.\n", index);
    s->codec_ctx[index].avctx = avcodec_alloc_context();
    s->codec_ctx[index].frame = avcodec_alloc_frame();

    s->codec_ctx[index].file_index = s->codec_param.file_index;
    qemu_parser_init(s, index);
    qemu_init_pix_fmt_info();

    TRACE("[%s] Leave\n", __func__);

    return index;
}

/* void av_free(void *ptr) */
void qemu_av_free(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;
    AVFrame *avframe;

    TRACE("enter %s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    avframe = s->codec_ctx[ctx_index].frame;

    if (avctx && avctx->palctrl) {
        av_free(avctx->palctrl);
        avctx->palctrl = NULL;
    }

    if (avctx && avctx->extradata) {
        TRACE("free extradata\n");
        av_free(avctx->extradata);
        avctx->extradata = NULL;
    }

    if (avctx && avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        int audio_idx = s->codec_ctx[ctx_index].mem_index;
        TRACE("reset audio mem_idex: %d\n", __LINE__, audio_idx);
        s->audio_codec_offset[audio_idx] = 0;
    }

    if (avctx) {
        TRACE("free codec context of %d.\n", ctx_index);
        av_free(avctx);
        s->codec_ctx[ctx_index].avctx = NULL;
    }

    if (avframe) {
        TRACE("free codec frame of %d.\n", ctx_index);
        av_free(avframe);
        s->codec_ctx[ctx_index].frame = NULL;
    }

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("leave %s\n", __func__);
}

/* void avcodec_flush_buffers(AVCodecContext *avctx) */
void qemu_avcodec_flush_buffers(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;

    TRACE("Enter\n");
    qemu_mutex_lock(&s->thread_mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    if (avctx) {
        avcodec_flush_buffers(avctx);
    } else {
        ERR("[%s] %d of AVCodecContext is NULL\n", __func__, ctx_index);
    }

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("[%s] Leave\n", __func__);
}

/* int avcodec_decode_video(AVCodecContext *avctx, AVFrame *picture,
 *                          int *got_picture_ptr, const uint8_t *buf,
 *                          int buf_size)
 */
int qemu_avcodec_decode_video(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;
    AVFrame *picture;
    AVPacket avpkt;
    int got_picture_ptr;
    uint8_t *buf;
    uint8_t *parser_buf;
    bool parser_use;
    int buf_size;
    int size = 0;
    int ret = -1;
    off_t offset;

    TRACE("Enter, %s\n", __func__);
    qemu_mutex_lock(&s->codec_thread.mutex);

    TRACE("[%s] Video Context Index : %d\n", __func__, ctx_index);

    avctx = s->codec_ctx[ctx_index].avctx;
    picture = s->codec_ctx[ctx_index].frame;
    if (!avctx || !picture) {
        ERR("[%s] %d of Context or Frame is NULL!\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->codec_thread.mutex);
        return ret;
    }

    offset = s->codec_param.mmap_offset;

    parser_buf = s->codec_ctx[ctx_index].parser_buf;
    parser_use = s->codec_ctx[ctx_index].parser_use;
    TRACE("[%s] Parser Buffer : %p, Parser:%d\n", __func__,
            parser_buf, parser_use);

    memcpy(&avctx->reordered_opaque,
            (uint8_t *)s->vaddr + offset, sizeof(int64_t));
    size = sizeof(int64_t);
    memcpy(&avctx->skip_frame,
            (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);
    memcpy(&buf_size, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);

    picture->reordered_opaque = avctx->reordered_opaque;

    if (parser_buf && parser_use) {
        buf = parser_buf;
    } else if (buf_size > 0) {
        TRACE("[%s] not use parser, codec_id:%x\n", __func__, avctx->codec_id);
        buf = (uint8_t *)s->vaddr + offset + size;
        size += buf_size;
    } else {
        TRACE("There is no input buffer\n");
        buf = NULL;
    }

    memset(&avpkt, 0, sizeof(AVPacket));
    avpkt.data = buf;
    avpkt.size = buf_size;
    TRACE("packet buf:%p, size:%d\n", buf, buf_size);

    ret = avcodec_decode_video2(avctx, picture, &got_picture_ptr, &avpkt);

    TRACE("[%s] after decoding video, ret:%d\n", __func__, ret);
    if (ret < 0) {
        ERR("[%s] failed to decode video!!, ret:%d\n", __func__, ret);
    } else {
        if (ret == 0) {
            TRACE("[%s] no frame. packet size:%d\n", __func__, avpkt.size);
        }
        TRACE("decoded frame number:%d\n", avctx->frame_number);
    }

    memcpy((uint8_t *)s->vaddr + offset, &avctx->pix_fmt, sizeof(int));
    size = sizeof(int);
    size += qemu_serialize_rational(&avctx->time_base,
                                    (uint8_t *)s->vaddr + offset + size);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->width, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->height, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->has_b_frames, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->frame_number, sizeof(int));
    size += sizeof(int);
    size += qemu_serialize_rational(&avctx->sample_aspect_ratio,
                                    (uint8_t *)s->vaddr + offset + size);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->internal_buffer_count, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->profile, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->level, sizeof(int));
    size += sizeof(int);
    size += qemu_serialize_frame(picture, (uint8_t *)s->vaddr + offset + size);

    memcpy((uint8_t *)s->vaddr + offset + size, &got_picture_ptr, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &ret, sizeof(int));
    size += sizeof(int);

#if 0
    memcpy((uint8_t *)s->vaddr + offset + size, dst.data[0], numbytes);
    av_free(buffer);

    if (parser_buf && parser_use) {
        TRACE("[%s] Free input buffer after decoding video\n", __func__);
        TRACE("[%s] input buffer : %p, %p\n",
            __func__, avpkt.data, parser_buf);
        av_free(avpkt.data);
        s->codec_ctx[ctx_index].parser_buf = NULL;
    }
#endif

    qemu_mutex_unlock(&s->codec_thread.mutex);
    TRACE("Leave, %s\n", __func__);

    return ret;
}

/* int avcodec_encode_video(AVCodecContext *avctx, uint8_t *buf,
 *                          int buf_size, const AVFrame *pict)
 */
int qemu_avcodec_encode_video(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx = NULL;
    AVFrame *pict = NULL;
    uint8_t *inputBuf = NULL;
    int outbufSize = 0;
    int bPict = -1;
    int size = 0;
    int ret = -1;
    off_t offset;

    TRACE("Enter, %s\n", __func__);
    qemu_mutex_lock(&s->codec_thread.mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    pict = s->codec_ctx[ctx_index].frame;
    if (!avctx || !pict) {
        ERR("[%s] %d of Context or Frame is NULL\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->thread_mutex);
        return ret;
    }

    offset = s->codec_param.mmap_offset;

    size = sizeof(int);
    memcpy(&bPict, (uint8_t *)s->vaddr + offset, size);
    TRACE("[%s] avframe is :%d\n", __func__, bPict);

    if (bPict == 0) {
        memcpy(&outbufSize, (uint8_t *)s->vaddr + offset + size, size);
        size += sizeof(int);
        size +=
            qemu_deserialize_frame((uint8_t *)s->vaddr + offset + size, pict);

        inputBuf = (uint8_t *)s->vaddr + offset + size;
        if (!inputBuf) {
            ERR("[%s] failed to get input buffer\n", __func__);
            return ret;
        }

        ret = qemu_avpicture_fill((AVPicture *)pict, inputBuf, avctx->pix_fmt,
                            avctx->width, avctx->height, true);

        if (ret < 0) {
            ERR("after avpicture_fill, ret:%d\n", ret);
        }
        TRACE("before encode video, ticks_per_frame:%d, pts:%lld\n",
               avctx->ticks_per_frame, pict->pts);
    } else {
        TRACE("flush encoded buffers\n");
        pict = NULL;
    }

    ret = avcodec_encode_video(avctx, (uint8_t *)s->vaddr + offset,
                                outbufSize, pict);
    TRACE("encode video, ret:%d, pts:%lld, outbuf size:%d\n",
            ret, pict->pts, outbufSize);

    if (ret < 0) {
        ERR("failed to encode video.\n");
    }

    memcpy((uint8_t *)s->vaddr + offset + outbufSize, &ret, sizeof(int));

    qemu_mutex_unlock(&s->codec_thread.mutex);
    TRACE("Leave, %s\n", __func__);

    return ret;
}

/*
 *  int avcodec_decode_audio3(AVCodecContext *avctx, int16_t *samples,
 *                            int *frame_size_ptr, AVPacket *avpkt)
 */
int qemu_avcodec_decode_audio(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx;
    AVPacket avpkt;
    int16_t *samples;
    int frame_size_ptr;
    uint8_t *buf;
    uint8_t *parser_buf;
    bool parser_use;
    int buf_size, outbuf_size;
    int size;
    int ret = -1;
    off_t offset;

    TRACE("Enter, %s\n", __func__);

    TRACE("Audio Context Index : %d\n", ctx_index);
    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of Context is NULL!\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->codec_thread.mutex);
        return ret;
    }

    if (!avctx->codec) {
        ERR("[%s] %d of Codec is NULL\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->codec_thread.mutex);
        return ret;
    }

    offset = s->codec_param.mmap_offset;

    parser_buf = s->codec_ctx[ctx_index].parser_buf;
    parser_use = s->codec_ctx[ctx_index].parser_use;

    memcpy(&buf_size, (uint8_t *)s->vaddr + offset, sizeof(int));
    size = sizeof(int);
    if (parser_buf && parser_use) {
        TRACE("[%s] use parser, buf:%p codec_id:%x\n",
                __func__, parser_buf, avctx->codec_id);
        buf = parser_buf;
    } else if (buf_size > 0) {
        TRACE("[%s] not use parser, codec_id:%x\n", __func__, avctx->codec_id);
        buf = (uint8_t *)s->vaddr + offset + size;
        size += buf_size;
    } else {
        TRACE("no input buffer\n");
        buf = NULL;
    }

    av_init_packet(&avpkt);
    avpkt.data = buf;
    avpkt.size = buf_size;

    frame_size_ptr = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    outbuf_size = frame_size_ptr;
    samples = av_malloc(frame_size_ptr);

    ret = avcodec_decode_audio3(avctx, samples, &frame_size_ptr, &avpkt);
    TRACE("after decoding audio!, ret:%d\n", ret);

    memcpy((uint8_t *)s->vaddr + offset, &avctx->bit_rate, sizeof(int));
    size = sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->sample_rate, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->channels, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
        &avctx->channel_layout, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy((uint8_t *)s->vaddr + offset + size, &avctx->sub_id, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->frame_size, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size,
            &avctx->frame_number, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, samples, outbuf_size);
    size += outbuf_size;
    memcpy((uint8_t *)s->vaddr + offset + size, &frame_size_ptr, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &ret, sizeof(int));
    size += sizeof(int);

    TRACE("before free input buffer and output buffer!\n");
    if (samples) {
        TRACE("release allocated audio buffer.\n");
        av_free(samples);
        samples = NULL;
    }

    if (parser_buf && parser_use) {
        TRACE("[%s] free parser buf\n", __func__);
        av_free(avpkt.data);
        s->codec_ctx[ctx_index].parser_buf = NULL;
    }

    TRACE("[%s] Leave\n", __func__);

    return ret;
}

int qemu_avcodec_encode_audio(SVCodecState *s, int ctx_index)
{
    WARN("[%s] Does not support audio encoder using FFmpeg\n", __func__);
    return 0;
}

/* void av_picture_copy(AVPicture *dst, const AVPicture *src,
 *                      enum PixelFormat pix_fmt, int width, int height)
 */
void qemu_av_picture_copy(SVCodecState *s, int ctx_index)
{
    AVCodecContext *avctx = NULL;
    AVPicture dst;
    AVPicture *src = NULL;
    int numBytes = 0;
    uint8_t *buffer = NULL;
    off_t offset = 0;

    TRACE("Enter :%s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    src = (AVPicture *)s->codec_ctx[ctx_index].frame;
    if (!avctx && !src) {
        ERR("[%s] %d of context or frame is NULL\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->thread_mutex);
        return;
    }

    offset = s->codec_param.mmap_offset;

    numBytes = qemu_avpicture_fill(&dst, NULL, avctx->pix_fmt,
                                  avctx->width, avctx->height, false);
    TRACE("after avpicture_fill: %d\n", numBytes);
    if (numBytes < 0) {
        ERR("picture size:%d is wrong.\n", numBytes);
        qemu_mutex_unlock(&s->thread_mutex);
        return;
    }

    av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);
    buffer = dst.data[0];
    memcpy((uint8_t *)s->vaddr + offset, buffer, numBytes);
    TRACE("after copy image buffer from host to guest.\n");

    if (buffer) {
        TRACE("release allocated video frame.\n");
        av_free(buffer);
    }

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("Leave :%s\n", __func__);
}

/* AVCodecParserContext *av_parser_init(int codec_id) */
void qemu_av_parser_init(SVCodecState *s, int ctx_index)
{
    AVCodecParserContext *parser_ctx = NULL;
    AVCodecContext *avctx;

    TRACE("Enter :%s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of AVCodecContext is NULL!!\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->thread_mutex);
        return;
    }

    TRACE("before av_parser_init, codec_type:%d codec_id:%x\n",
            avctx->codec_type, avctx->codec_id);

    parser_ctx = av_parser_init(avctx->codec_id);
    if (parser_ctx) {
        TRACE("[%s] using parser\n", __func__);
        s->codec_ctx[ctx_index].parser_use = true;
    } else {
        TRACE("[%s] no parser\n", __func__);
        s->codec_ctx[ctx_index].parser_use = false;
    }
    s->codec_ctx[ctx_index].parser_ctx = parser_ctx;

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("[%s] Leave\n", __func__);
}

/* int av_parser_parse(AVCodecParserContext *s, AVCodecContext *avctx,
 *                     uint8_t **poutbuf, int *poutbuf_size,
 *                     const uint8_t *buf, int buf_size,
 *                     int64_t pts, int64_t dts)
 */
int qemu_av_parser_parse(SVCodecState *s, int ctx_index)
{
    AVCodecParserContext *parser_ctx = NULL;
    AVCodecContext *avctx = NULL;
    uint8_t *poutbuf;
    int poutbuf_size = 0;
    uint8_t *inbuf = NULL;
    int inbuf_size;
    int64_t pts;
    int64_t dts;
    int64_t pos;
    int size, ret = -1;
    off_t offset;

    TRACE("Enter %s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    parser_ctx = s->codec_ctx[ctx_index].parser_ctx;
    avctx = s->codec_ctx[ctx_index].avctx;
    if (!avctx) {
        ERR("[%s] %d of AVCodecContext is NULL\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->thread_mutex);
        return ret;
    }

    if (!parser_ctx) {
        ERR("[%s] %d of AVCodecParserContext is NULL\n", __func__, ctx_index);
        qemu_mutex_unlock(&s->thread_mutex);
        return ret;
    }

    offset = s->codec_param.mmap_offset;

    memcpy(&parser_ctx->pts,
        (uint8_t *)s->vaddr + offset, sizeof(int64_t));
    size = sizeof(int64_t);
    memcpy(&parser_ctx->dts,
        (uint8_t *)s->vaddr + offset + size, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy(&parser_ctx->pos,
        (uint8_t *)s->vaddr + offset + size, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy(&pts, (uint8_t *)s->vaddr + offset + size, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy(&dts, (uint8_t *)s->vaddr + offset + size, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy(&pos, (uint8_t *)s->vaddr + offset + size, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy(&inbuf_size, (uint8_t *)s->vaddr + offset + size, sizeof(int));
    size += sizeof(int);

    if (inbuf_size > 0) {
        inbuf = av_mallocz(inbuf_size);
        memcpy(inbuf, (uint8_t *)s->vaddr + offset + size, inbuf_size);
    } else {
        inbuf = NULL;
        INFO("input buffer size for parser is zero.\n");
    }

    TRACE("[%s] inbuf:%p inbuf_size :%d\n", __func__, inbuf, inbuf_size);
    ret = av_parser_parse2(parser_ctx, avctx, &poutbuf, &poutbuf_size,
                           inbuf, inbuf_size, pts, dts, pos);
    TRACE("after parsing, outbuf size :%d, ret:%d\n", poutbuf_size, ret);

    if (poutbuf) {
        s->codec_ctx[ctx_index].parser_buf = poutbuf;
    }

    TRACE("[%s] inbuf : %p, outbuf : %p\n", __func__, inbuf, poutbuf);
    memcpy((uint8_t *)s->vaddr + offset, &parser_ctx->pts, sizeof(int64_t));
    size = sizeof(int64_t);
    memcpy((uint8_t *)s->vaddr + offset + size, &poutbuf_size, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t *)s->vaddr + offset + size, &ret, sizeof(int));
    size += sizeof(int);
    if (poutbuf && poutbuf_size > 0) {
        memcpy((uint8_t *)s->vaddr + offset + size, poutbuf, poutbuf_size);
    } else {
        av_free(inbuf);
    }

#if 0
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        TRACE("[%s] free parser inbuf\n", __func__);
        av_free(inbuf);
    }
#endif

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("Leave, %s\n", __func__);

    return ret;
}

/* void av_parser_close(AVCodecParserContext *s) */
void qemu_av_parser_close(SVCodecState *s, int ctx_index)
{
    AVCodecParserContext *parser_ctx;

    TRACE("Enter, %s\n", __func__);
    qemu_mutex_lock(&s->thread_mutex);

    parser_ctx = s->codec_ctx[ctx_index].parser_ctx;
    if (!parser_ctx) {
        ERR("AVCodecParserContext is NULL\n");
        qemu_mutex_unlock(&s->thread_mutex);
        return;
    }
    av_parser_close(parser_ctx);

    qemu_mutex_unlock(&s->thread_mutex);
    TRACE("Leave, %s\n", __func__);
}

int codec_operate(uint32_t api_index, uint32_t ctx_index, SVCodecState *s)
{
    int ret = -1;

    TRACE("[%s] context : %d\n", __func__, ctx_index);
    switch (api_index) {
    /* FFMPEG API */
    case EMUL_AV_REGISTER_ALL:
        qemu_av_register_all();
        break;
    case EMUL_AVCODEC_OPEN:
        ret = qemu_avcodec_open(s);
        break;
    case EMUL_AVCODEC_CLOSE:
        ret = qemu_avcodec_close(s, ctx_index);
        qemu_av_free(s, ctx_index);
        break;
    case EMUL_AVCODEC_FLUSH_BUFFERS:
        qemu_avcodec_flush_buffers(s, ctx_index);
        break;
    case EMUL_AVCODEC_DECODE_VIDEO:
    case EMUL_AVCODEC_ENCODE_VIDEO:
    case EMUL_AVCODEC_DECODE_AUDIO:
    case EMUL_AVCODEC_ENCODE_AUDIO:
        wake_codec_worker_thread(s);
        break;
    case EMUL_AV_PICTURE_COPY:
        qemu_av_picture_copy(s, ctx_index);
        break;
    case EMUL_AV_PARSER_INIT:
        qemu_av_parser_init(s, ctx_index);
        break;
    case EMUL_AV_PARSER_PARSE:
        ret = qemu_av_parser_parse(s, ctx_index);
        break;
    case EMUL_AV_PARSER_CLOSE:
        qemu_av_parser_close(s, ctx_index);
        break;
    default:
        WARN("api index %d does not exist!.\n", api_index);
    }
    return ret;
}

static uint32_t qemu_get_mmap_offset(SVCodecState *s)
{
    int index = 0;

    for (; index < AUDIO_CODEC_MEM_OFFSET_MAX; index++)  {
        if (s->audio_codec_offset[index] == 0) {
            s->audio_codec_offset[index] = 1;
            break;
        }
    }
    TRACE("return mmap offset: %d\n", index);

    return index;
}

/*
 *  Codec Device APIs
 */
uint64_t codec_read(void *opaque, hwaddr addr, unsigned size)
{
    SVCodecState *s = (SVCodecState *)opaque;
    uint64_t ret = 0;

    switch (addr) {
    case CODEC_CMD_GET_THREAD_STATE:
        qemu_mutex_lock(&s->thread_mutex);
        ret = s->codec_thread.state;
        s->codec_thread.state = 0;
        qemu_mutex_unlock(&s->thread_mutex);
        TRACE("ret: %d, thread state: %d\n", ret, s->codec_thread.state);
        qemu_irq_lower(s->dev.irq[0]);
        break;
    case CODEC_CMD_GET_VERSION:
        ret = MARU_CODEC_VERSION;
        TRACE("codec version: %d\n", ret);
        break;
    case CODEC_CMD_GET_DEVICE_MEM:
        qemu_mutex_lock(&s->thread_mutex);
        ret = s->device_mem_avail;
        if (s->device_mem_avail != 1) {
            s->device_mem_avail = 1;
        }
        qemu_mutex_unlock(&s->thread_mutex);
        break;
    case CODEC_CMD_GET_MMAP_OFFSET:
        ret = qemu_get_mmap_offset(s);
        TRACE("mem index: %d\n", ret);
        break;
    default:
        ERR("no avaiable command for read. %d\n", addr);
    }

    return ret;
}

void codec_write(void *opaque, hwaddr addr,
                uint64_t value, unsigned size)
{
    SVCodecState *s = (SVCodecState *)opaque;

    switch (addr) {
    case CODEC_CMD_API_INDEX:
        codec_operate(value, s->codec_param.ctx_index, s);
        break;
    case CODEC_CMD_CONTEXT_INDEX:
        s->codec_param.ctx_index = value;
        TRACE("Context Index: %d\n", s->codec_param.ctx_index);
        break;
    case CODEC_CMD_FILE_INDEX:
        s->codec_param.file_index = value;
        break;
    case CODEC_CMD_DEVICE_MEM_OFFSET:
        s->codec_param.mmap_offset = value;
        TRACE("MMAP Offset: %d\n", s->codec_param.mmap_offset);
        break;
    case CODEC_CMD_RESET_CODEC_INFO:
        qemu_reset_codec_info(s, value);
        break;
    case CODEC_CMD_SET_DEVICE_MEM:
        qemu_mutex_lock(&s->thread_mutex);
        s->device_mem_avail = value;
        qemu_mutex_unlock(&s->thread_mutex);
        break;
    case CODEC_CMD_SET_MMAP_OFFSET:
        s->codec_param.mem_index = value;
        break;
    default:
        ERR("no avaiable command for write. %d\n", addr);
    }
}

static const MemoryRegionOps codec_mmio_ops = {
    .read = codec_read,
    .write = codec_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void codec_tx_bh(void *opaque)
{
    SVCodecState *s = (SVCodecState *)opaque;

    int ctx_index;
    AVCodecContext *ctx;

    ctx_index = s->codec_param.ctx_index;
    ctx = s->codec_ctx[ctx_index].avctx;

    TRACE("Enter, %s\n", __func__);

    /* raise irq as soon as a worker thread had finished a job*/
    if (s->codec_thread.state) {
        TRACE("raise codec irq. state:%d, codec:%d\n",
              s->codec_thread.state, ctx->codec_type);
        qemu_irq_raise(s->dev.irq[0]);
    }

    TRACE("Leave, %s\n", __func__);
}

static int codec_initfn(PCIDevice *dev)
{
    SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    INFO("[%s] device init\n", __func__);

    memset(&s->codec_param, 0, sizeof(SVCodecParam));

    qemu_mutex_init(&s->thread_mutex);
    codec_thread_init(s);
    s->tx_bh = qemu_bh_new(codec_tx_bh, s);

    pci_config_set_interrupt_pin(pci_conf, 1);

    memory_region_init_ram(&s->vram, OBJECT(s), "codec.ram", MARU_CODEC_MEM_SIZE);
    s->vaddr = memory_region_get_ram_ptr(&s->vram);

    memory_region_init_io(&s->mmio, OBJECT(s), &codec_mmio_ops, s,
                        "codec-mmio", MARU_CODEC_REG_SIZE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->vram);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    return 0;
}

static void codec_exitfn(PCIDevice *dev)
{
    SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
    INFO("[%s] device exit\n", __func__);

    qemu_bh_delete(s->tx_bh);

    memory_region_destroy(&s->vram);
    memory_region_destroy(&s->mmio);
}

int codec_init(PCIBus *bus)
{
    INFO("[%s] device create\n", __func__);
    pci_create_simple(bus, -1, MARU_CODEC_DEV_NAME);
    return 0;
}

static void codec_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = codec_initfn;
    k->exit = codec_exitfn;
    k->vendor_id = PCI_VENDOR_ID_TIZEN;
    k->device_id = PCI_DEVICE_ID_VIRTUAL_CODEC;
    k->class_id = PCI_CLASS_MULTIMEDIA_AUDIO;
    dc->desc = "Virtual Codec device for Tizen emulator";
}

static TypeInfo codec_info = {
    .name          = MARU_CODEC_DEV_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(SVCodecState),
    .class_init    = codec_class_init,
};

static void codec_register_types(void)
{
    type_register_static(&codec_info);
}

type_init(codec_register_types)
