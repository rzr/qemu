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

#include "maru_codec.h"
#include <pthread.h>

#define QEMU_DEV_NAME   "codec"

/*  Needs 16M to support 1920x1080 video resolution.
 *  Output size for encoding has to be greater than (width * height * 6)
 */
#define MARU_CODEC_MMAP_MEM_SIZE    (16 * 1024 * 1024)
#define MARU_CODEC_MMAP_COUNT       (4)
#define MARU_CODEC_MEM_SIZE     (MARU_CODEC_MMAP_COUNT * MARU_CODEC_MMAP_MEM_SIZE)
#define MARU_CODEC_REG_SIZE     (256)

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, marucodec);

static int paramCount = 0;
static int ctxArrIndex = 0;

static void qemu_parser_init (SVCodecState *s, int ctxIndex)
{
    TRACE("[%s] Enter\n", __func__);

    s->ctxArr[ctxIndex].pParserBuffer = NULL;
    s->ctxArr[ctxIndex].bParser = false;

    TRACE("[%s] Leave\n", __func__);
}

static void qemu_codec_close (SVCodecState *s, uint32_t value)
{
    int i, ctxIndex;
    TRACE("[%s] Enter\n", __func__);

    pthread_mutex_lock(&s->codec_mutex);
    for (i = 0; i < CODEC_MAX_CONTEXT; i++) {
        if (s->ctxArr[i].nFileValue == value) {
            ctxIndex = i;
            break;
        }
    }

    TRACE("[%s] Close %d context\n", __func__, ctxIndex);

    s->ctxArr[ctxIndex].bUsed = false;
    pthread_mutex_unlock(&s->codec_mutex);

    TRACE("[%s] Leave\n", __func__);
}

static void qemu_restore_context (AVCodecContext *dst, AVCodecContext *src) {
    TRACE("[%s] Enter\n", __func__);

    dst->av_class = src->av_class;
    dst->extradata = src->extradata;
    dst->codec = src->codec;
    dst->priv_data = src->priv_data;
    dst->opaque = src->opaque;
    dst->get_buffer = src->get_buffer;
    dst->release_buffer = src->release_buffer;
    dst->stats_out = src->stats_out;
    dst->stats_in = src->stats_in;
    dst->rc_override = src->rc_override;
    dst->rc_eq = src->rc_eq;
    dst->slice_offset = src->slice_offset;
    dst->get_format = src->get_format;
    dst->internal_buffer = src->internal_buffer;
    dst->intra_matrix = src->intra_matrix;
    dst->inter_matrix = src->inter_matrix;
    dst->reget_buffer = src->reget_buffer;
    dst->execute = src->execute;
    dst->thread_opaque = src->thread_opaque;
    dst->execute2 = src->execute2;

    TRACE("[%s] Leave\n", __func__);
}

/* void av_register_all() */
static void qemu_av_register_all (void)
{
    av_register_all();
    TRACE("av_register_all\n");
}

/* int avcodec_default_get_buffer (AVCodecContext *s, AVFrame *pic) */
static int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture)
{
    int ret;
    TRACE("avcodec_default_get_buffer\n");

    picture->reordered_opaque = context->reordered_opaque;
    picture->opaque = NULL;

    ret = avcodec_default_get_buffer(context, picture);

    return ret;
}

/* void avcodec_default_release_buffer (AVCodecContext *ctx, AVFrame *frame) */
static void qemu_avcodec_release_buffer (AVCodecContext *context, AVFrame *picture)
{
    TRACE("avcodec_default_release_buffer\n");
    avcodec_default_release_buffer(context, picture);
}

/* int avcodec_open (AVCodecContext *avctx, AVCodec *codec) */
#ifdef CODEC_HOST
static int qemu_avcodec_open (SVCodecState *s, int ctxIndex)
{
    AVCodecContext *avctx;
    AVCodecContext tmpCtx;
    AVCodec *codec;
    AVCodec tmpCodec;
    enum CodecID codec_id;
    int ret;

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!avctx) {
        ERR("[%s][%d] AVCodecContext is NULL!!\n", __func__, __LINE__);
        return -1;
    }
    avctx = gAVCtx;

    size = sizeof(AVCodecContext);
    memcpy(&tmpCtx, avctx, size);

    cpu_synchronize_state(cpu_single_env);

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[0],
                        (uint8_t*)avctx, sizeof(AVCodecContext), 0);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[1],
                        (uint8_t*)&tmpCodec, sizeof(AVCodec), 0);

    /* restore AVCodecContext's pointer variables */
    qemu_restore_context(avctx, &tmpCtx);
    codec_id = tmpCodec.id;

    if (avctx->extradata_size > 0) {
        avctx->extradata = (uint8_t*)av_malloc(avctx->extradata_size);
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[2],
                            (uint8_t*)avctx->extradata, avctx->extradata_size, 0);
    } else {
        avctx->extradata = NULL;
    }

    TRACE("[%s][%d] CODEC ID : %d\n", __func__, __LINE__, codec_id);
    if (tmpCodec.encode) {
        codec = avcodec_find_encoder(codec_id);
    } else {
        codec = avcodec_find_decoder(codec_id);
    }

    avctx->get_buffer = qemu_avcodec_get_buffer;
    avctx->release_buffer = qemu_avcodec_release_buffer;

    ret = avcodec_open(avctx, codec);
    if (ret != 0) {
        perror("Failed to open codec\n");
        return ret;
    }

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[0],
                        (uint8_t*)avctx, sizeof(AVCodecContext), 1);

    return ret;
}
#else
static int qemu_avcodec_open (SVCodecState *s, int ctxIndex)
{
    AVCodecContext *avctx;
    AVCodecContext tempCtx;
    AVCodec *codec;
    AVCodec tmpCodec;
    enum CodecID codec_id;
    off_t offset;
    int ret;
    int size = 0;

    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!avctx) {
        ERR("[%s][%d] AVCodecContext is NULL!!\n", __func__, __LINE__);
        return -1;
    }

    offset = s->codecParam.mmapOffset;

    TRACE("Context Index:%d, offset:%d\n", ctxIndex, offset);

#if 1
    size = sizeof(AVCodecContext);
    memcpy(&tempCtx, avctx, size);
    memcpy(avctx, (uint8_t*)s->vaddr + offset, size);
    memcpy(&tmpCodec, (uint8_t*)s->vaddr + offset + size, sizeof(AVCodec));
    TRACE("codec type:%d, codec id:%d\n",
           ((AVCodec*)((uint8_t*)s->vaddr + offset + size))->type,
           ((AVCodec*)((uint8_t*)s->vaddr + offset + size))->id);
    size += sizeof(AVCodec);
    qemu_restore_context(avctx, &tempCtx);

    TRACE("Context Index:%d, width:%d, height:%d\n", ctxIndex, avctx->width, avctx->height);
#endif

#if 0
    memcpy(&avctx->bit_rate, (uint8_t*)s->vaddr, sizeof(int));
    size = sizeof(int);
    memcpy(&avctx->bit_rate_tolerance, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->flags, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->time_base, (uint8_t*)s->vaddr + size, sizeof(AVRational));
    size += sizeof(AVRational);
    memcpy(&avctx->width, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->height, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->gop_size, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->pix_fmt, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->sample_rate, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->channels, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->codec_tag, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->rc_strategy, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->strict_std_compliance, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->rc_qsquish, (uint8_t*)s->vaddr + size, sizeof(float));
    size += sizeof(float);
    memcpy(&avctx->sample_aspect_ratio, (uint8_t*)s->vaddr + size, sizeof(AVRational));
    size += sizeof(AVRational);
    memcpy(&avctx->mb_qmin, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->mb_qmax, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->pre_me, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->trellis, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&avctx->extradata_size, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&tmpCodec.id, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
    memcpy(&tmpCodec.encode, (uint8_t*)s->vaddr + size, sizeof(int));
    size += sizeof(int);
#endif

    codec_id = tmpCodec.id;
    TRACE("[%s] CODEC ID : %d\n", __func__, codec_id);

    if (avctx->extradata_size > 0) {
        avctx->extradata = (uint8_t*)av_malloc(avctx->extradata_size);
        memcpy(avctx->extradata, (uint8_t*)s->vaddr + offset + size, avctx->extradata_size);
    } else {
        avctx->extradata = NULL;
    }

    if (tmpCodec.encode) {
        codec = avcodec_find_encoder(codec_id);
    } else {
        codec = avcodec_find_decoder(codec_id);
    }

    avctx->get_buffer = qemu_avcodec_get_buffer;
    avctx->release_buffer = qemu_avcodec_release_buffer;

    ret = avcodec_open(avctx, codec);
    if (ret != 0) {
        ERR("Failure AVCODEC_OPEN, %d\n", ret);
    }

#if 1
    memcpy((uint8_t*)s->vaddr + offset, avctx, sizeof(AVCodecContext));
    memcpy((uint8_t*)s->vaddr + offset + sizeof(AVCodecContext), &ret, sizeof(int));
#endif

#if 0
    memcpy((uint8_t*)s->vaddr, &avctx->pix_fmt, sizeof(int));
    size = sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->time_base, sizeof(AVRational));
    size += sizeof(AVRational);
    memcpy((uint8_t*)s->vaddr + size, &avctx->sample_fmt, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->codec_type, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->codec_id, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->coded_width, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->coded_height, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->ticks_per_frame, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->chroma_sample_location, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, avctx->priv_data, codec->priv_data_size);
    size += codec->priv_data_size;
    memcpy((uint8_t*)s->vaddr + size, &ret, sizeof(int));
#endif

    pthread_mutex_unlock(&s->codec_mutex);
    return ret;
}
#endif

/* int avcodec_close (AVCodecContext *avctx) */
static int qemu_avcodec_close (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;
    off_t offset;
    int ret = -1;

    TRACE("Enter\n");
    pthread_mutex_lock(&s->codec_mutex);

    offset = s->codecParam.mmapOffset;

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!avctx) {
        ERR("[%s][%d] AVCodecContext is NULL\n", __func__, __LINE__);
        memcpy((uint8_t*)s->vaddr + offset, &ret, sizeof(int));
        return ret;
    }

    ret = avcodec_close(avctx);
    TRACE("after avcodec_close. ret:%d\n", ret);

#ifndef CODEC_HOST
    memcpy((uint8_t*)s->vaddr + offset, &ret, sizeof(int));
#endif

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("[%s] Leave\n", __func__);
    return ret;
}

/* AVCodecContext* avcodec_alloc_context (void) */
static void qemu_avcodec_alloc_context (SVCodecState* s)
{
    off_t offset;
    int index;

    TRACE("[%s] Enter\n", __func__);
    pthread_mutex_lock(&s->codec_mutex);

    offset = s->codecParam.mmapOffset;

    for (index = 0; index < CODEC_MAX_CONTEXT; index++) {
        if (s->ctxArr[index].bUsed == false) {
            TRACE("[%s] Succeeded to get context[%d].\n", __func__, index);
            ctxArrIndex = index;
            break;
        }
        TRACE("[%s] Failed to get context[%d].\n", __func__, index);
    }

    if (index == CODEC_MAX_CONTEXT) {
        ERR("[%s] Failed to get available codec context from now\n", __func__);
        ERR("[%s] Try to run codec again\n", __func__);
        return;
    }

    TRACE("[%s] context index :%d.\n", __func__, ctxArrIndex);

    s->ctxArr[ctxArrIndex].pAVCtx = avcodec_alloc_context();
    s->ctxArr[ctxArrIndex].nFileValue = s->codecParam.fileIndex;
    s->ctxArr[ctxArrIndex].bUsed = true;
    memcpy((uint8_t*)s->vaddr + offset, &ctxArrIndex, sizeof(int));
    qemu_parser_init(s, ctxArrIndex);

    pthread_mutex_unlock(&s->codec_mutex);

    TRACE("[%s] Leave\n", __func__);
}

/* AVFrame *avcodec_alloc_frame (void) */
static void qemu_avcodec_alloc_frame (SVCodecState* s)
{
    TRACE("[%s] Enter\n", __func__);
    pthread_mutex_lock(&s->codec_mutex);

    s->ctxArr[ctxArrIndex].pFrame = avcodec_alloc_frame();
    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("[%s] Leave\n", __func__);
}

/* void av_free (void *ptr) */
static void qemu_av_free_context (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;

    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (avctx) {
        av_free(avctx);
        s->ctxArr[ctxIndex].pAVCtx = NULL;
    }
    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("free AVCodecContext\n");
}

static void qemu_av_free_picture (SVCodecState* s, int ctxIndex)
{
    AVFrame *avframe;

    pthread_mutex_lock(&s->codec_mutex);

    avframe = s->ctxArr[ctxIndex].pFrame;
    if (avframe) {
        av_free(avframe);
        s->ctxArr[ctxIndex].pFrame = NULL;
    }

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("free AVFrame\n");
}

static void qemu_av_free_palctrl (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;

    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;

    if (avctx->palctrl) {
        av_free(avctx->palctrl);
        avctx->palctrl = NULL;
    }
    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("free AVCodecContext palctrl\n");
}

static void qemu_av_free_extradata (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;

    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (avctx && avctx->extradata && avctx->extradata_size > 0) {
        av_free(avctx->extradata);
        avctx->extradata = NULL;
    }

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("free AVCodecContext extradata\n");
}

/* void avcodec_flush_buffers (AVCodecContext *avctx) */
static void qemu_avcodec_flush_buffers (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;

    TRACE("Enter\n");
    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (avctx) {
        avcodec_flush_buffers(avctx);
    } else {
        ERR("[%s][%d] AVCodecContext is NULL\n", __func__, __LINE__);
    }

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("[%s] Leave\n", __func__);
}

/* int avcodec_decode_video (AVCodecContext *avctx, AVFrame *picture,
 *                          int *got_picture_ptr, const uint8_t *buf,
 *                          int buf_size)
 */
#ifdef CODEC_HOST
static int qemu_avcodec_decode_video (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;
    AVFrame *picture;
    int got_picture_ptr;
    const uint8_t *buf;
    uint8_t *pParserBuffer;
    bool bParser;
    int buf_size;
    int ret;

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    picture = s->ctxArr[ctxIndex].pFrame;
    if (!avctx || !picture) {
        ERR("AVCodecContext or AVFrame is NULL!\n")
        ERR("avctx:0x%x, picture:0x%x\n", avctx, picture);
        return -1;
    }

    pParserBuffer = s->ctxArr[ctxIndex].pParserBuffer;
    bParser = s->ctxArr[ctxIndex].bParser;
    TRACE("Paser Buffer : 0x%x, Parser:%d\n", pParserBuffer, bParser); 

    cpu_synchronize_state(cpu_single_env);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[4],
                        (uint8_t*)&buf_size, sizeof(int), 0);

    if (pParserBuffer && bParser) {
        buf = pParserBuffer;
    } else if (buf_size > 0) {
        TRACE("not use parser, codec_id:%d\n", avctx->codec_id);
        buf = (uint8_t*)av_malloc(buf_size * sizeof(uint8_t));
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[3],
                (uint8_t*)buf, buf_size, 0);
    } else {
        TRACE("There is no input buffer\n");
    }

    avpkt.data = buf;
    avpkt.size = buf_size;

    TRACE("before avcodec_decode_video\n");
    ret = avcodec_decode_video2(avctx, picture, &got_picture_ptr, &avpkt);

    TRACE("after avcodec_decode_video, ret:%d\n", ret);
    if (got_picture_ptr == 0) {
        TRACE("There is no frame\n");
    }

    if (!pParserBuffer && !bParser) {
        TRACE("Free input buffer after decoding video\n");
        av_free(buf);
    }

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[0],
                        (uint8_t*)avctx, sizeof(AVCodecContext), 1);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[1],
                        (uint8_t*)picture, sizeof(AVFrame), 1);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[2],
                        (uint8_t*)&got_picture_ptr, sizeof(int), 1);

    return ret;
}
#else
static int qemu_avcodec_decode_video (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;
    AVFrame *picture;
    AVPacket avpkt;
    int got_picture_ptr;
    uint8_t *buf;
    uint8_t *pParserBuffer;
    bool bParser;
    int buf_size;
    int size;
    int ret;
    off_t offset;

    TRACE("Video Context Index : %d\n", ctxIndex);
    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    picture = s->ctxArr[ctxIndex].pFrame;
    if (!avctx || !picture) {
        ERR("AVCodecContext or AVFrame is NULL!\n");
        return -1;
    }

    offset = s->codecParam.mmapOffset;

    pParserBuffer = s->ctxArr[ctxIndex].pParserBuffer;
    bParser = s->ctxArr[ctxIndex].bParser;
    TRACE("Paser Buffer : 0x%x, Parser:%d\n", pParserBuffer, bParser); 

    memset(&avpkt, 0x00, sizeof(AVPacket));
    memcpy(&buf_size, (uint8_t*)s->vaddr + offset, sizeof(int));
    size = sizeof(int);

    if (pParserBuffer && bParser) {
        buf = pParserBuffer;
    } else if (buf_size > 0) {
        TRACE("not use parser, codec_id:%d\n", avctx->codec_id);
        buf = (uint8_t*)s->vaddr + offset + size;
    } else {
        TRACE("There is no input buffer\n");
        buf = NULL;
    }

    avpkt.data = buf;
    avpkt.size = buf_size;
    
    TRACE("before avcodec_decode_video\n");
    ret = avcodec_decode_video2(avctx, picture, &got_picture_ptr, &avpkt);
    TRACE("after avcodec_decode_video, ret:%d\n", ret);
    if (got_picture_ptr == 0) {
        TRACE("There is no frame\n");
    }

#if 1
    size = sizeof(AVCodecContext);
    memcpy((uint8_t*)s->vaddr + offset, avctx, size);
#endif

#if 0
    memcpy((uint8_t*)s->vaddr, &avctx->pix_fmt, sizeof(int));
    size = sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->time_base, sizeof(AVRational));
    size += sizeof(AVRational);
    memcpy((uint8_t*)s->vaddr + size, &avctx->width, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->height, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->has_b_frames, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->frame_number, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->sample_aspect_ratio, sizeof(AVRational));
    size += sizeof(AVRational);
    memcpy((uint8_t*)s->vaddr + size, &avctx->internal_buffer_count, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->profile, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + size, &avctx->level, sizeof(int));
    size += sizeof(int);
#endif
    memcpy((uint8_t*)s->vaddr + offset + size, picture, sizeof(AVFrame));
    size += sizeof(AVFrame);
    memcpy((uint8_t*)s->vaddr + offset + size, &got_picture_ptr, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + offset + size, &ret, sizeof(int));

    if (!pParserBuffer && !bParser) {
        TRACE("Free input buffer after decoding video\n");
    }

    pthread_mutex_unlock(&s->codec_mutex);
    return ret;
}
#endif

/* int avcodec_encode_video (AVCodecContext *avctx, uint8_t *buf,
 *                          int buf_size, const AVFrame *pict)
 */
#ifdef CODEC_HOST
static int qemu_avcodec_encode_video (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx;
    uint8_t *outBuf, *inBuf;
    int outBufSize, inBufSize;
    AVFrame *pict;
    int ret;

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    pict = s->ctxArr[ctxIndex].pFrame;
    if (!avctx || !pict) {
        ERR("AVCodecContext or AVFrame is NULL\n");
        return -1;
    }

    cpu_synchronize_state(cpu_single_env);

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[2],
                        (uint8_t*)&outBufSize, sizeof(int), 0);

    if (outBufSize > 0) {
        outBuf = (uint8_t*)av_malloc(outBufSize * sizeof(uint8_t));
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[1],
                            (uint8_t*)outBuf, outBufSize, 0);
    } else {
        ERR("input buffer size is 0\n");
        return -1;
    }

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[3],
                        (uint8_t*)pict, sizeof(AVFrame), 0);
    inBufSize = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
    inBuf = (uint8_t*)av_malloc(inBufSize * sizeof(uint8_t));
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[4],
                        (uint8_t*)inBuf, inBufSize, 0);
    
    ret = avpicture_fill((AVPicture*)pict, inBuf, avctx->pix_fmt,
                        avctx->width, avctx->height);
    if (ret < 0) {
        ERR("after avpicture_fill, ret:%d\n", ret);
    }

    ret = avcodec_encode_video (avctx, outBuf, outBufSize, pict);
    TRACE("after avcodec_encode_video, ret:%d\n", ret);

    if (ret > 0) {
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[5],
                            (uint8_t*)avctx->coded_frame, sizeof(AVFrame), 1);
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[6],
                            (uint8_t*)outBuf, outBufSize, 1);
    }
    av_free(inBuf);
    return ret;
}
#else
static int qemu_avcodec_encode_video (SVCodecState* s, int ctxIndex)
{
    AVCodecContext *avctx = NULL;
    AVFrame *pict = NULL;
    uint8_t *inputBuf = NULL;
    int outputBufSize = 0;
    int numBytes = 0;
    int bPict = -1;
    int size = 0;
    int ret = -1;
    off_t offset;

    pthread_mutex_lock(&s->codec_mutex);
    avctx = s->ctxArr[ctxIndex].pAVCtx;
    pict = s->ctxArr[ctxIndex].pFrame;
    if (!avctx || !pict) {
        ERR("AVCodecContext or AVFrame is NULL\n");
        return -1;
    }

    offset = s->codecParam.mmapOffset;

    size = sizeof(int);
    memcpy(&bPict, (uint8_t*)s->vaddr + offset, size);
    TRACE("avframe is :%d\n", bPict);

    if (bPict == 0) {
        memcpy(&outputBufSize, (uint8_t*)s->vaddr + offset + size, size);
        size += sizeof(int);

        memcpy(pict, (uint8_t*)s->vaddr + offset + size, sizeof(AVFrame));
        size += sizeof(AVFrame);

        numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
        TRACE("input buffer size :%d\n", numBytes);

        inputBuf = (uint8_t*)s->vaddr + offset + size;
        if (!inputBuf) {
            ERR("failed to allocate decoded frame inputBuf\n");
            return -1;
        }

        ret = avpicture_fill((AVPicture*)pict, inputBuf, avctx->pix_fmt,
                avctx->width, avctx->height);
        if (ret < 0) {
            ERR("after avpicture_fill, ret:%d\n", ret);
        }

        TRACE("before encode video, ticks_per_frame:%d, pts:%d\n",
                avctx->ticks_per_frame, pict->pts);
    } else {
        TRACE("Flush encoded buffers\n");
        pict = NULL;
    }

    TRACE("before encoding video\n");
    ret = avcodec_encode_video (avctx, (uint8_t*)s->vaddr + offset, outputBufSize, pict);

    TRACE("after encoding video, ret:%d\n");
    if (ret < 0) {
        ERR("Failed to encode video\n");
    }
    memcpy((uint8_t*)s->vaddr + offset + outputBufSize, &ret, sizeof(int));

    pthread_mutex_unlock(&s->codec_mutex);
    return ret;
}
#endif

/* 
 *  int avcodec_decode_audio2 (AVCodecContext *avctx, int16_t *samples,
 *                             int *frame_size_ptr, uint8_t *buf, int buf_size)
 */
static int qemu_avcodec_decode_audio (SVCodecState *s, int ctxIndex)
{
    AVCodecContext *avctx;
    AVPacket avpkt;
    int16_t *samples;
    int frame_size_ptr;
    uint8_t *buf;
    uint8_t *pParserBuffer;
    bool bParser;
    int buf_size, outbuf_size;
    int size;
    int ret;
    off_t offset;

    TRACE("Audio Context Index : %d\n", ctxIndex);
    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!avctx) {
        ERR("[%s][%d] AVCodecContext is NULL!\n", __func__, __LINE__);
        return -1;
    }

    offset = s->codecParam.mmapOffset;

    pParserBuffer = s->ctxArr[ctxIndex].pParserBuffer;
    bParser = s->ctxArr[ctxIndex].bParser;
    TRACE("Paser Buffer : 0x%x, Parser:%d\n", pParserBuffer, bParser);

    memcpy(&buf_size, (uint8_t*)s->vaddr + offset, sizeof(int));
    size = sizeof(int);

    if (pParserBuffer && bParser) {
        buf = pParserBuffer;
    } else if (buf_size > 0) {
        TRACE("not use parser, codec_id:%d\n", avctx->codec_id);
        buf = (uint8_t*)s->vaddr + offset + size;
    } else {
        TRACE("There is no input buffer\n");
        buf = NULL;
    }
    avpkt.data = buf;
    avpkt.size = buf_size;

    frame_size_ptr = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    outbuf_size = frame_size_ptr;
    samples = av_malloc(frame_size_ptr);

    ret = avcodec_decode_audio3(avctx, samples, &frame_size_ptr, &avpkt);
    TRACE("After decoding audio!, ret:%d\n", ret);

#if 1
    size = sizeof(AVCodecContext);
    memcpy((uint8_t*)s->vaddr + offset, avctx, size);
#endif

#if 0
    memcpy((uint8_t*)s->vaddr + offset, &avctx->bit_rate, sizeof(int));
    size = sizeof(int);
    memcpy((uint8_t*)s->vaddr + offset + size, &avctx->sub_id, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + offset + size, &avctx->frame_size, sizeof(int));
    size += sizeof(int);
    memcpy((uint8_t*)s->vaddr + offset + size, &avctx->frame_number, sizeof(int));
    size += sizeof(int);
#endif
    memcpy((uint8_t*)s->vaddr + offset + size, samples, outbuf_size);
    size += outbuf_size;

    memcpy((uint8_t*)s->vaddr + offset + size, &frame_size_ptr, sizeof(int));
    size += sizeof(int);

    memcpy((uint8_t*)s->vaddr + offset + size, &ret, sizeof(int));

    TRACE("before free input buffer and output buffer!\n");
    av_free(samples);

    pthread_mutex_unlock(&s->codec_mutex);

    TRACE("[%s] Leave\n", __func__);

    return ret;
}

static int qemu_avcodec_encode_audio (SVCodecState *s, int ctxIndex)
{
    INFO("[%s] Does not support audio encoder using FFmpeg\n", __func__);
    return 0;
}

/* void av_picture_copy (AVPicture *dst, const AVPicture *src,
 *                      enum PixelFormat pix_fmt, int width, int height)
 */
#ifdef CODEC_HOST
static void qemu_av_picture_copy (SVCodecState* s, int ctxIndex)
{
    AVCodecContext* avctx;
    AVPicture dst;
    AVPicture *src;
    int numBytes;
    uint8_t *buffer = NULL;
    int ret;

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    src = (AVPicture*)s->ctxArr[ctxIndex].pFrame;
    if (!avctx && !src) { 
        ERR("AVCodecContext or AVFrame is NULL\n");
        return;
    }

    numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill(&dst, buffer, avctx->pix_fmt, avctx->width, avctx->height);
    av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);

    cpu_synchronize_state(cpu_single_env);
    ret = cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[5],
                              (uint8_t*)dst.data[0], numBytes, 1);
    if (ret < 0) {
        TRACE("failed to copy decoded frame into guest!! ret:%d\n", ret);
    }

    av_free(buffer);

    TRACE("Leave :%s\n", __func__);
}
#else
static void qemu_av_picture_copy (SVCodecState* s, int ctxIndex)
{
    AVCodecContext* avctx;
    AVPicture dst;
    AVPicture *src;
    int numBytes;
    uint8_t *buffer = NULL;
    off_t offset;

    TRACE("Enter :%s\n", __func__);
    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    src = (AVPicture*)s->ctxArr[ctxIndex].pFrame;
    if (!avctx && !src) { 
        ERR("AVCodecContext or AVFrame is NULL\n");
        return;
    }

    offset = s->codecParam.mmapOffset;

    numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill(&dst, buffer, avctx->pix_fmt, avctx->width, avctx->height);
    av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);

    memcpy((uint8_t*)s->vaddr + offset, dst.data[0], numBytes);
    TRACE("After copy image buffer from host to guest\n");

    av_free(buffer);

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("Leave :%s\n", __func__);
}
#endif

/* AVCodecParserContext *av_parser_init (int codec_id) */
static void qemu_av_parser_init (SVCodecState* s, int ctxIndex)
{
    AVCodecParserContext *parserctx = NULL;
    AVCodecContext *avctx;

    TRACE("Enter :%s\n", __func__);
    pthread_mutex_lock(&s->codec_mutex);

    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!avctx) {
        ERR("[%s][%d] AVCodecContext is NULL!!\n", __func__, __LINE__);
        return;
    }

    INFO("before av_parser_init, codec_id:%d\n", avctx->codec_id);
    parserctx = av_parser_init(avctx->codec_id);
    if (!parserctx) {
        ERR("Failed to initialize AVCodecParserContext\n");
    }
    s->ctxArr[ctxIndex].pParserCtx = parserctx;
    s->ctxArr[ctxIndex].bParser = true;

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("Leave :%s\n", __func__);
}

/* int av_parser_parse(AVCodecParserContext *s, AVCodecContext *avctx,
 *                      uint8_t **poutbuf, int *poutbuf_size,
 *                      const uint8_t *buf, int buf_size,
 *                      int64_t pts, int64_t dts)
 */
#ifdef CODEC_HOST
static int qemu_av_parser_parse (SVCodecState* s, int ctxIndex)
{
    AVCodecParserContext *parserctx = NULL;
    AVCodecContext *avctx = NULL;
    AVCodecContext tmp_ctx;
    uint8_t *poutbuf = NULL;
    int poutbuf_size;
    uint8_t *inbuf = NULL;
    int inbuf_size;
    int64_t pts;
    int64_t dts;
    int ret;

    parserctx = s->ctxArr[ctxIndex].pParserCtx;
    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!parserctx && !avctx) {
        ERR("[%s][%d] AVCodecParserContext or AVCodecContext is NULL\n", __func__, __LINE__);
    }

    memcpy(&tmp_ctx, avctx, sizeof(AVCodecContext));

    cpu_synchronize_state(cpu_single_env);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[1],
                        (uint8_t*)avctx, sizeof(AVCodecContext), 0);
    qemu_restore_context(avctx, &tmp_ctx);

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[5],
                        (uint8_t*)&inbuf_size, sizeof(int), 0);
    if (inbuf_size > 0) {
        inbuf = av_malloc(sizeof(uint8_t) * inbuf_size);
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[4],
                            (uint8_t*)inbuf, inbuf_size, 0);
    }
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[6],
                        (uint8_t*)&pts, sizeof(int64_t), 0);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[7],
                        (uint8_t*)&dts, sizeof(int64_t), 0);
    
    ret = av_parser_parse2(parserctx, avctx, &poutbuf, &poutbuf_size,
                          inbuf, inbuf_size, pts, dts, AV_NOPTS_VALUE);
    s->ctxArr[ctxIndex].pParserBuffer = poutbuf;
    if (inbuf_size > 0 && inbuf) {
        av_free(inbuf);
        inbuf = NULL;
    }

    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[0],
                        (uint8_t*)parserctx, sizeof(AVCodecParserContext), 1);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[1],
                        (uint8_t*)avctx, sizeof(AVCodecContext), 1);
    cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[3],
                        (uint8_t*)&poutbuf_size, sizeof(int), 1);
    if (poutbuf_size != 0) {
        cpu_memory_rw_debug(cpu_single_env, s->codecParam.in_args[2],
                            (uint8_t*)poutbuf, poutbuf_size, 1);
    }

    TRACE("Leave %s\n", __func__);
    return ret;
}
#else
static int qemu_av_parser_parse (SVCodecState *s, int ctxIndex)
{
    AVCodecParserContext *parserctx = NULL;
    AVCodecContext *avctx = NULL;
    uint8_t *poutbuf;
    int poutbuf_size;
    uint8_t *inbuf = NULL;
    int inbuf_size;
    int64_t pts;
    int64_t dts;
    int size, ret;
    off_t offset;

    TRACE("Enter %s\n", __func__);
    pthread_mutex_lock(&s->codec_mutex);

    parserctx = s->ctxArr[ctxIndex].pParserCtx;
    avctx = s->ctxArr[ctxIndex].pAVCtx;
    if (!parserctx && !avctx) {
        ERR("[%s][%d] AVCodecParserContext or AVCodecContext is NULL\n", __func__, __LINE__);
    }

    offset = s->codecParam.mmapOffset;

    memcpy(&pts, (uint8_t*)s->vaddr + offset, sizeof(int64_t));
    size = sizeof(int64_t);
    memcpy(&dts, (uint8_t*)s->vaddr + offset + size, sizeof(int64_t));
    size += sizeof(int64_t);
    memcpy(&inbuf_size, (uint8_t*)s->vaddr + offset + size, sizeof(int));
    if (inbuf_size > 0) {
        size += sizeof(int);
        inbuf = (uint8_t*)s->vaddr + offset + size;
        size += inbuf_size;
    } else {
        inbuf = NULL;
    }

    TRACE("input buffer size :%d\n", inbuf_size);

    ret = av_parser_parse2(parserctx, avctx, &poutbuf, &poutbuf_size,
                           inbuf, inbuf_size, pts, dts, AV_NOPTS_VALUE);

    TRACE("after parsing, output buffer size :%d, ret:%d\n", poutbuf_size, ret);

    s->ctxArr[ctxIndex].pParserBuffer = poutbuf;

    memcpy((uint8_t*)s->vaddr + offset, &poutbuf_size, sizeof(int));
    size = sizeof(int);
    if (poutbuf_size != 0) {
        memcpy((uint8_t*)s->vaddr + offset + size, poutbuf, poutbuf_size);
        size += poutbuf_size;
    }
    memcpy((uint8_t*)s->vaddr + offset + size, &ret, sizeof(int));

    pthread_mutex_unlock(&s->codec_mutex);
    TRACE("[%s]Leave\n", __func__);
    return ret;
}
#endif

/* void av_parser_close (AVCodecParserContext *s) */
static void qemu_av_parser_close (SVCodecState *s, int ctxIndex)
{
    AVCodecParserContext *parserctx;

    TRACE("av_parser_close\n");
    pthread_mutex_lock(&s->codec_mutex);

    parserctx = s->ctxArr[ctxIndex].pParserCtx;
    if (!parserctx) {
        ERR("AVCodecParserContext is NULL\n");
        return ;
    }
    av_parser_close(parserctx);
    pthread_mutex_unlock(&s->codec_mutex);

}

static int codec_operate (uint32_t apiIndex, uint32_t ctxIndex, SVCodecState *state)
{
    int ret = -1;

    TRACE("[%s] context : %d\n", __func__, ctxIndex);

    switch (apiIndex) {
        /* FFMPEG API */
        case EMUL_AV_REGISTER_ALL:
            qemu_av_register_all();
            break;
        case EMUL_AVCODEC_OPEN:
            ret = qemu_avcodec_open(state, ctxIndex);
            break;
        case EMUL_AVCODEC_CLOSE:
            ret = qemu_avcodec_close(state, ctxIndex);
            break;
        case EMUL_AVCODEC_ALLOC_CONTEXT:
            qemu_avcodec_alloc_context(state);
            break;
        case EMUL_AVCODEC_ALLOC_FRAME:
            qemu_avcodec_alloc_frame(state);
            break;
        case EMUL_AV_FREE_CONTEXT:
            qemu_av_free_context(state, ctxIndex);
            break;
        case EMUL_AV_FREE_FRAME:
            qemu_av_free_picture(state, ctxIndex);
            break;
        case EMUL_AV_FREE_PALCTRL:
            qemu_av_free_palctrl(state, ctxIndex);
            break;
        case EMUL_AV_FREE_EXTRADATA:
            qemu_av_free_extradata(state, ctxIndex);
            break;
        case EMUL_AVCODEC_FLUSH_BUFFERS:
            qemu_avcodec_flush_buffers(state, ctxIndex);
            break;
        case EMUL_AVCODEC_DECODE_VIDEO:
            ret = qemu_avcodec_decode_video(state, ctxIndex);
            break;
        case EMUL_AVCODEC_ENCODE_VIDEO:
            ret = qemu_avcodec_encode_video(state, ctxIndex);
            break;
        case EMUL_AVCODEC_DECODE_AUDIO:
            ret = qemu_avcodec_decode_audio(state, ctxIndex);
            break;
        case EMUL_AVCODEC_ENCODE_AUDIO:
            ret = qemu_avcodec_encode_audio(state, ctxIndex);
            break;
        case EMUL_AV_PICTURE_COPY:
            qemu_av_picture_copy(state, ctxIndex);
            break;
        case EMUL_AV_PARSER_INIT:
            qemu_av_parser_init(state, ctxIndex);
            break;
        case EMUL_AV_PARSER_PARSE:
            ret = qemu_av_parser_parse(state, ctxIndex);
            break;
        case EMUL_AV_PARSER_CLOSE:
            qemu_av_parser_close(state, ctxIndex);
            break;
        default:
            WARN("The api index does not exsit!. api index:%d\n", apiIndex);
    }
    return ret;
}

/*
 *  Codec Device APIs
 */
uint64_t codec_read (void *opaque, target_phys_addr_t addr, unsigned size)
{
    switch (addr) {
        default:
            ERR("There is no avaiable command for %s\n", QEMU_DEV_NAME);
    }
    return 0;
}

void codec_write (void *opaque, target_phys_addr_t addr, uint64_t value, unsigned size)
{
    int ret = -1;
    SVCodecState *state = (SVCodecState*)opaque;

    switch (addr) {
        case CODEC_API_INDEX:
            ret = codec_operate(value, state->codecParam.ctxIndex, state);
#ifdef CODEC_HOST
            if (ret >= 0) {
                cpu_synchronize_state(cpu_single_env);
                cpu_memory_rw_debug(cpu_single_env, state->codecParam.ret_args,
                                    (uint8_t*)&ret, sizeof(int), 1);
            }
#endif
            paramCount = 0;
            break;
        case CODEC_IN_PARAM:
            state->codecParam.in_args[paramCount++] = value;
            break;
        case CODEC_RETURN_VALUE:
            state->codecParam.ret_args = value;
            break;
        case CODEC_CONTEXT_INDEX:
            state->codecParam.ctxIndex = value;
            TRACE("Context Index : %d\n", state->codecParam.ctxIndex);
            break;
        case CODEC_MMAP_OFFSET:
            state->codecParam.mmapOffset = value * MARU_CODEC_MMAP_MEM_SIZE;
            TRACE("MMAP Offset :%d\n", state->codecParam.mmapOffset);
            break;
        case CODEC_FILE_INDEX:
            state->codecParam.fileIndex = value;
            break;
        case CODEC_CLOSED:
            qemu_codec_close(state, value);
            break;
        default:
            ERR("There is no avaiable command for %s\n", QEMU_DEV_NAME);
    }
}

static const MemoryRegionOps codec_mmio_ops = {
    .read = codec_read,
    .write = codec_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static int codec_initfn (PCIDevice *dev)
{
    SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    INFO("[%s] device init\n", __func__);

    memset(&s->codecParam, 0x00, sizeof(SVCodecParam));
    pthread_mutex_init(&s->codec_mutex, NULL);
 
    pci_config_set_interrupt_pin(pci_conf, 2);

    memory_region_init_ram(&s->vram, NULL, "codec.ram", MARU_CODEC_MEM_SIZE);
    s->vaddr = memory_region_get_ram_ptr(&s->vram);

    memory_region_init_io (&s->mmio, &codec_mmio_ops, s, "codec-mmio", MARU_CODEC_REG_SIZE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->vram);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    return 0;
}

static int codec_exitfn (PCIDevice *dev)
{
    SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
    INFO("[%s] device exit\n", __func__);

    memory_region_destroy (&s->vram);
    memory_region_destroy (&s->mmio);
    return 0;
}

int codec_init (PCIBus *bus)
{
    INFO("[%s] device create\n", __func__);
    pci_create_simple (bus, -1, QEMU_DEV_NAME);
    return 0;
}

static PCIDeviceInfo codec_info = {
    .qdev.name      = QEMU_DEV_NAME,
    .qdev.desc      = "Virtual Codec device for Tizen emulator",
    .qdev.size      = sizeof (SVCodecState),
    .init           = codec_initfn,
    .exit           = codec_exitfn,
    .vendor_id      = PCI_VENDOR_ID_TIZEN,
    .device_id      = PCI_DEVICE_ID_VIRTUAL_CODEC,
    .class_id       = PCI_CLASS_MULTIMEDIA_AUDIO,
};

static void codec_register (void)
{
    pci_qdev_register(&codec_info);
}
device_init(codec_register);
