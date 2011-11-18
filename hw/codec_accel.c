/* 
 * Qemu brightness emulator
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * Authors:
 *  Kitae KIM kt920.kim@samsung.com
 *
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").  
 * You shall not disclose such Confidential Information and shall use it only in accordance with the terms of the license agreement 
 * you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations or warranties about the suitability 
 * of the software, either express or implied, including but not limited to the implied warranties of merchantability, fitness for 
 * a particular purpose, or non-infringement. SAMSUNG shall not be liable for any damages suffered by licensee as 
 * a result of using, modifying or distributing this software or its derivatives.
 */
#include <stdio.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "hw.h"
#include "pci.h"
#include "pci_ids.h"
#include <pthread.h>
#include <sys/types.h>

// #define DEBUG_SLPCODEC

#ifdef DEBUG_SLPCODEC
#define DEBUG_PRINT(fmt, ...) \
	fprintf (stdout, "[qemu][%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);
#else
#define DEBUG_PRINT(fmt, ...) ((void)0);
#endif

// #define PCI_VENDOR_ID_SAMSUNG			0x144d
// #define PCI_DEVICE_ID_VIRTUAL_CODEC		0x1018

#define QEMU_DEV_NAME		"codec"
#define CODEC_MEM_SIZE		0x1000	

enum {
	FUNC_NUM = 0,
	IN_ARGS = 4,
	IN_ARGS_SIZE = 8,
	RET_STR = 12,
};

/* static int testEncode(const char *filename);
static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame); */

static int codec_operate(uint32_t value);

/* pthread_mutex_t sync_mutex;
pthread_cond_t sync_cond; */

typedef struct _SlcCodec_Param {
	uint32_t func_num;
	uint32_t in_args_size;
	uint32_t in_args[20];
	uint32_t ret_args;
} Codec_Param;

Codec_Param *target_param;

typedef struct _SlpCodecState {
	PCIDevice dev;

	/* Need to implement */
	int codec_mmio_io_addr;

/* SlpCodecInfo *pCodecInfo;
	Codec_Param target_param; */
} SlpCodecState;


/*
 *  Codec API Emulation
 */

struct SlpCodecInfo {
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVPacket pkt;
};

AVCodecContext *gAVCtx;
AVFrame *gFrame;
// AVCodecContext *gAVCtx[10];
// AVFrame *gFrame[10];
// AVFormatContext *gFormatCtx = NULL;
// AVCodec *gCodec = NULL;
// AVFrame *gFrame[10];
// AVFrame *gFrameRGB = NULL;
// AVPacket gPkt;
// struct SwsContext *gImgConvertCtx;

// uint8_t *gBuffer = NULL;
static int cnt = 0;
static int iCnt = 0;

static int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture);
static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

/* void av_register_all() */
void qemu_av_register_all (void)
{
	DEBUG_PRINT("\n")
	av_register_all();
}

static int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture)
{
	DEBUG_PRINT("Enter\n")

	picture->reordered_opaque = context->reordered_opaque;
	picture->opaque = NULL;

	avcodec_default_get_buffer(context, picture);
	DEBUG_PRINT("Leave\n")
}

void qemu_frame_dump (AVCodecContext *ctx, AVFrame *pic)
{
	AVFrame *frame;
	int numBytes;
	uint8_t *buffer;
	struct SwsContext *imgConvertCtx;

	frame = avcodec_alloc_frame();
	numBytes = avpicture_get_size(PIX_FMT_YUV420P, ctx->width, ctx->height);
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill((AVPicture*)frame, buffer, PIX_FMT_YUV420P, ctx->width, ctx->height);
	
	imgConvertCtx = sws_getContext(ctx->width, ctx->height, ctx->pix_fmt,
								ctx->width, ctx->height, PIX_FMT_YUV420P,
								SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(imgConvertCtx, pic->data, pic->linesize, 0,
			ctx->height, frame->data, frame->linesize);

	SaveFrame(frame, ctx->width, ctx->height, ++iCnt);
	av_free(buffer);
	av_free(frame);
}

/* int avcodec_open (AVCodecContext *avctx, AVCodec *codec)	*/
int qemu_avcodec_open (void)
{
	AVCodecContext *avctx;
	AVCodec *codec;
	enum CodecID codec_id;
	bool bEncode;
	int ret, extradata_size;

	DEBUG_PRINT("\n")
	/* guest to host */
	avctx = avcodec_alloc_context();
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], &avctx->extradata_size, sizeof(int), 0);
	extradata_size = avctx->extradata_size;
	if (avctx->extradata_size > 0) {
		avctx->extradata = (uint8_t*)av_malloc(extradata_size);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], avctx->extradata, extradata_size, 0);
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], &avctx->time_base, sizeof(AVRational), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[7], &avctx->sample_rate, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[8], &avctx->channels, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[9], &avctx->width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[10], &avctx->height, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[11], &avctx->sample_fmt, sizeof(enum SampleFormat), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[12], &avctx->sample_aspect_ratio, sizeof(AVRational), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[13], &avctx->coded_width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[14], &avctx->coded_height, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[15], &avctx->ticks_per_frame, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[16], &avctx->chroma_sample_location, sizeof(enum AVChromaLocation), 0);

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &codec_id, sizeof(enum CodecID), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &bEncode, sizeof(bool), 0);
	if (bEncode) {
		codec = avcodec_find_encoder(codec_id);
	} else {
		codec = avcodec_find_decoder(codec_id);
	}

	avctx->get_buffer = qemu_avcodec_get_buffer;
	
	ret = avcodec_open(avctx, codec);
	if (ret != 0) {
		perror("Failed to open codec\n");
		return ret;
	}

	/* host to guest */
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], &avctx->time_base, sizeof(AVRational), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[11], &avctx->sample_fmt, sizeof(enum SampleFormat), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[13], &avctx->coded_width, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[14], &avctx->coded_height, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[15], &avctx->ticks_per_frame, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[16], &avctx->chroma_sample_location, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[17], avctx->priv_data, codec->priv_data_size, 1);
/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[13], &avctx->codec_type, sizeof(enum AVMediaType), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[14], &avctx->codec_id, sizeof(enum CodecID), 1); */
	
//	gAVCtx[cnt] = avctx;
//	gFrame[cnt++] = avcodec_alloc_frame();
	gAVCtx = avctx;
	gFrame = avcodec_alloc_frame();

	return ret;
}

/* int avcodec_close (AVCodecContext *avctx) */
int qemu_avcodec_close (void)
{
	AVCodecContext *avctx;
	int ret;
	
	DEBUG_PRINT("\n")

//	avctx = gAVCtx[0];
	avctx = gAVCtx;
	ret = avcodec_close(avctx);

	return ret;
}

/* AVCodecContext* avcodec_alloc_context (void) */
void qemu_avcodec_alloc_context (void)
{
	AVCodecContext* ctx;
	ctx = avcodec_alloc_context();
	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, ctx, sizeof(AVCodecContext), 1);
//	gAVCtx[cnt] = ctx;
	gAVCtx = ctx;
}

/* AVFrame *avcodec_alloc_frame (void) */
void qemu_avcodec_alloc_frame (void)
{
	AVFrame *pFrame;

	pFrame = avcodec_alloc_frame();
	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, pFrame, sizeof(AVFrame), 1);
	gFrame = pFrame;
//	gFrame[cnt++] = pFrame;
}

/* void av_free (void *ptr) */
void qemu_av_free (void)
{
	void *ptr;
	int ret;

	DEBUG_PRINT("Enter\n")

	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, &ret, sizeof(int), 0);
	printf("%s : %d\n", __func__, ret);
	if (ret == 1) {
		if (gAVCtx)
			av_free(gAVCtx);
		gAVCtx = NULL;
	} else if (ret == 2) {
		if (gFrame)
			av_free(gFrame);
		gFrame = NULL;
	} else if (ret == 3) {
		if (gAVCtx->palctrl)
			av_free(gAVCtx->palctrl);
		gAVCtx->palctrl = NULL;
	} else { 
		if (gAVCtx->extradata)
			av_free(gAVCtx->extradata);
		gAVCtx->extradata = NULL;
	}

/*	if (ret == 1) {
		if (gAVCtx[0])
			av_free(gAVCtx[0]);
		gAVCtx[0] = NULL;
	} else if (ret == 2) {
		if (gFrame[0])
			av_free(gFrame[0]);
		gFrame[0] = NULL;
	} else if (ret == 3) {
		if ((gAVCtx[0])->palctrl)
			av_free((gAVCtx[0])->palctrl);
		(gAVCtx[0])->palctrl = NULL;
	} else { 
		if ((gAVCtx[0])->extradata)
			av_free((gAVCtx[0])->extradata);
		(gAVCtx[0])->extradata = NULL;
	} */
	DEBUG_PRINT("Leave\n")
}

/* void avcodec_get_context_defaults (AVCodecContext *ctx) */
void qemu_avcodec_get_context_defaults (void)
{
	AVCodecContext *avctx;

	DEBUG_PRINT("\n")
	avctx = gAVCtx;
/*	avctx = gAVCtx[0];
	if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		avctx = gAVCtx[1];
	} */
	avcodec_get_context_defaults(avctx);
}

/* void avcodec_flush_buffers (AVCodecContext *avctx) */
void qemu_avcodec_flush_buffers (void)
{
	AVCodecContext *avctx;

	DEBUG_PRINT("\n")
	avctx = gAVCtx;
/*	avctx = gAVCtx[0];
	if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		avctx = gAVCtx[1];
	} */
	avcodec_flush_buffers(avctx);
}

/* int avcodec_default_get_buffer (AVCodecContext *s, AVFrame *pic) */
int qemu_avcodec_default_get_buffer (void)
{
	AVCodecContext *avctx;
	AVFrame *pic;
	int ret;

	DEBUG_PRINT("\n")
	ret = avcodec_default_get_buffer(avctx, pic);

	return ret;
}

/* void avcodec_default_release_buffer (AVCodecContext *ctx, AVFrame *frame) */
void qemu_avcodec_default_release_buffer (void)
{
	AVCodecContext *ctx;
	AVFrame *frame;

	DEBUG_PRINT("\n")
	avcodec_default_release_buffer(ctx, frame);
}

/* int avcodec_decode_video (AVCodecContext *avctx, AVFrame *picture,
 * 							int *got_picture_ptr, const uint8_t *buf,
 * 							int buf_size) */
int qemu_avcodec_decode_video (void)
{
	AVCodecContext *avctx;
	AVFrame *picture;
	int got_picture_ptr;
	const uint8_t *buf;
	int buf_size;
	int ret;

	DEBUG_PRINT("\n")
	avctx = gAVCtx;
	picture = gFrame;
/*	avctx = gAVCtx[0];
	picture = gFrame[0];
	if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		avctx = gAVCtx[1];
		picture = gFrame[1];
	} */
	if (avctx == NULL | picture == NULL) {
		printf("AVCodecContext or AVFrame is NULL!! avctx:0x%x, picture:0x%x\n", avctx, picture);
		return -1;
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], &buf_size, sizeof(int), 0);
	if (buf_size > 0) {
		buf = (uint8_t*)av_malloc(buf_size * sizeof(uint8_t));
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], buf, buf_size, 0);
	} else {
		buf = NULL;
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], &avctx->frame_number, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], &avctx->pix_fmt, sizeof(enum PixelFormat), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[7], &avctx->coded_frame, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[8], &avctx->sample_aspect_ratio, sizeof(AVRational), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[9], &avctx->reordered_opaque, sizeof(int), 0);

	ret = avcodec_decode_video(avctx, picture, &got_picture_ptr, buf, buf_size);
	if (got_picture_ptr) {
//		qemu_frame_dump(avctx, picture);
	}

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], picture, sizeof(AVFrame), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &got_picture_ptr, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], &avctx->pix_fmt, sizeof(enum PixelFormat), 1);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[7], avctx->coded_frame, sizeof(AVFrame), 1);

	return ret;
}

/* int avcodec_decode_audio (AVCodecContext *avctx, AVFrame *picture,
								int *got_picture_ptr, const uint8_t *buf,
								int buf_size) */

/* int avcodec_encode_video (AVCodecContext *avctx, uint8_t *buf,
 * 							int buf_size, const AVFrame *pict) */
int qemu_avcodec_encode_video (void)
{
	AVCodecContext *avctx;
	uint8_t *buf;
	int buf_size;
	const AVFrame *pict;
	int ret;

	ret = avcodec_encode_video (avctx, buf, buf_size, pict);
	return ret;
}


/* int avcodec_encode_audio (AVCodecContext *avctx, uint8_t *buf,
 * 							int buf_size, const AVFrame *pict) */


/* void av_picture_copy (AVPicture *dst, const AVPicture *src,
 * 						enum PixelFormat pix_fmt, int width, int height) */
void qemu_av_picture_copy (void)
{
	AVCodecContext* avctx;
	AVPicture dst;
	AVPicture *src;
	enum PixelFormat pix_fmt;
	int width;
	int height;
	int numBytes;
	uint8_t *buffer = NULL;
	int ret;

	DEBUG_PRINT("Enter\n")
	avctx = gAVCtx;
	src = (AVPicture*)gFrame;
/*	avctx = gAVCtx[0];
	src = (AVPicture*)gFrame[0];
	if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		avctx = gAVCtx[1];
		src = (AVPicture*)gFrame[1];
	} */
/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &pix_fmt, sizeof(enum PixelFormat), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &height, sizeof(int), 0); */

	numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
//	cpu_memory_request_page_debug(cpu_single_env, target_param->in_args[4], numBytes, 0);
   	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill(&dst, buffer, avctx->pix_fmt, avctx->width, avctx->height);
	av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);

	ret = cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], dst.data[0], numBytes, 1);
	if (ret < 0) {
		printf("Failed to copy data ret:%d\n", ret);
	}
/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], dst.data[1], (width * height)/4, 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], dst.data[2], (width * height)/4, 1); */

	av_free(buffer);
	DEBUG_PRINT("Leave\n") 
}

/* AVCodecParserContext *av_parser_init (int codec_id) */
void qemu_av_parser_init (void)
{
	AVCodecParserContext *s;
	int codec_id;

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &codec_id, sizeof(int), 0);

	s = av_parser_init(codec_id);

	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, s, sizeof(AVCodecParserContext), 1);
}
/* int av_parser_parse(AVCodecParserContext *s, AVCodecContext *avctx, uint8_t **poutbuf,
 * 						int *poutbuf_size, const uint8_t *buf, int buf_size,
 * 						int64_t pts, int64_t dts) */
int qemu_av_parser_parse (void)
{
	AVCodecParserContext *s;
	AVCodecContext *avctx;
	uint8_t *poutbuf;
	int poutbuf_size;
	const uint8_t *buf;
	int buf_size;
	int64_t pts;
	int64_t pos;
	int ret;

	ret = av_parser_parse(s, avctx, &poutbuf, &poutbuf_size, buf, buf_size, pts, pos);

	return ret;
}

/* void av_parser_close (AVCodecParserContext *s) */
void qemu_av_parser_close (void)
{
	AVCodecParserContext *s;

	av_parser_close(s);
}


/* int avcodec_decode_video2 (AVCodecContext *avctx, AVFrame *picture,
 * 							int *got_picture_ptr, AVPacket *avpkt) */
/* int qemu_avcodec_decode_video2 (void)
{
	int ret;

    AVCodecContext *avctx;

    avctx = gFormatCtx->streams[0]->codec;

	ret = avcodec_decode_video2(avctx, gFrame, &frameFinished, &gPkt);

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], avctx, sizeof(AVCodecContext), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], gFrame, sizeof(AVFrame), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &frameFinished, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &gPkt, sizeof(AVPacket), 1);

	return ret;
} */

/* void av_free_packet (AVPacket *pkt) */
/* void qemu_av_free_packet (void)
{
	av_free_packet(&gPkt);
} */

/*
 * Not used function
 */

/* int avpicture_get_size (enum PixelFormat pix_fmt, int width, int height)	 */
/* int qemu_avpicture_get_size (void)
{
	int width;
	int height;
	int ret;
	enum PixelFormat pix_fmt;

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &pix_fmt, sizeof(enum PixelFormat), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &height, sizeof(int), 0);

	ret = avpicture_get_size(pix_fmt, width, height);
	if (ret != 0)
		perror("Failed to get size of avpicture\n");

	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, &ret, sizeof(int), 1);
	return ret;
} */

/* void *av_malloc(size_t size) */
/* void qemu_av_malloc (void)
{
	uint8_t *pBuf;
	uint32_t value, size;

	value = target_param->in_args[0];
	cpu_memory_rw_debug(cpu_single_env, value, &size, sizeof(uint32_t), 0);

	pBuf = (uint8_t*)av_malloc(size * sizeof(uint8_t));
	if (!pBuf)
		perror("Failed to allocate memory\n");
	
	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, pBuf, size, 1);

	gBuffer = pBuf;
} */

/* int avpicture_fill (AVPicture *picture, uint8_t *ptr, enum PixelFormat pix_fmt, int width, int height)  */
/* int qemu_avpicture_fill (void)
{
	int width;
	int height;
	enum PixelFormat pix_fmt;
	uint32_t size;
	int ret;

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &pix_fmt, sizeof(enum PixelFormat), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], &height, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], &size, sizeof(uint32_t), 0);

	ret = avpicture_fill((AVPicture*)gFrameRGB, gBuffer, pix_fmt, width, height);
	
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], gFrameRGB, sizeof(AVFrame), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], gBuffer, size, 1);

	return ret;
} */

/* int av_read_frame (AVFormatContext *s, AVPacket *pkt) */
/* int qemu_av_read_frame (void)
{
	int ret;

	ret = av_read_frame(gFormatCtx, &gPkt);

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], gFormatCtx, sizeof(AVFormatContext), 1);
//  cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], ic_ptr->iformat, sizeof(struct AVInputFormat), 1);
//  cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], ic_ptr->pb, sizeof(ByteIOContext), 1);
//  cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], ic_ptr->streams[0], sizeof(AVStream), 1);
//  cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], ic_ptr->streams[0]->codec, sizeof(AVCodecContext), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &gPkt, sizeof(AVPacket), 1);

	return ret;
} */

/* struct SwsContext *sws_getContext(int srcW, int srcH, enum PixelFormat srcFormat,
	                                  int dstW, int dstH, enum PixelFormat dstFormat,
    	                              int flags, SwsFilter *srcFilter,
        	                          SwsFilter *dstFilter, const double *param)  */
/* void qemu_sws_getContext (void)
{
	enum PixelFormat fmt1, fmt2;
	int width;
	int height;

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &height, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &fmt1, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &fmt2, sizeof(int), 0);

	gImgConvertCtx = sws_getContext(width, height, fmt1, width, height, fmt2, SWS_BICUBIC, NULL, NULL, NULL);

//	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, gImgConvertCtx, sizeof(SwsContext), 1);
} */

/* int sws_scale(struct SwsContext *context, const uint8_t* const srcSlice[], const int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* const dst[], const int dstStride[]) */
/* int qemu_sws_scale (void)
{
	int height;
	int ret;

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], &height, sizeof(int), 0);

	ret = sws_scale(gImgConvertCtx, gFrame->data, gFrame->linesize, 0, height,
					gFrameRGB->data, gFrameRGB->linesize);

	return ret;
} */

/* int av_open_input_file (AVFormatContext **ic_ptr, const char *filename,
						   AVInputFormat *fmt, int buf_size, AVFormatParameters *ap) */
/* int qemu_av_open_input_file (void)
{
	AVFormatContext *ic_ptr;
//	AVFormatContext formatctx;
//	AVClass avclass;
//	AVStream stream;
//	AVCodecContext avcodecctx;
//	AVFrame avframe;
//	AVCodec avcodec;

	const char filename[10];
	uint32_t ret;

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &formatctx, sizeof(AVFormatContext), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], formatctx->av_class, sizeof(AVClass), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], formatctx->iformat, sizeof(AVInputFormat), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], formatctx->pb, sizeof(ByteIOContext), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], formatctx->streams[0], sizeof(AVStream), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], filename, 10, 0);
	ret = av_open_input_file(&ic_ptr, filename, NULL, 0, NULL);
	if (ret != 0)
		perror("Failed to get AVFormatContext\n");

	gFormatCtx = ic_ptr;

	// copy to guest buffer - AVFormatContext structure
	{
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], ic_ptr, sizeof(AVFormatContext), 1);
//		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], ic_ptr->av_class, sizeof(AVClass), 1);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], ic_ptr->iformat, sizeof(struct AVInputFormat), 1);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], ic_ptr->pb, sizeof(ByteIOContext), 1);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], ic_ptr->streams[0], sizeof(AVStream), 1);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], ic_ptr->streams[0]->codec, sizeof(AVCodecContext), 1);

	}
	//cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], ic_ptr, sizeof(AVFormatContext), 1);
	//size = sizeof(target_param->in_args[0]);

	return ret;
} */

/* int av_find_stream_info (AVFormatContext *ic)  */
/* int qemu_av_find_stream_info (void)
{
//	AVFormatContext ic;
	AVFormatContext *ic;
	uint32_t value;
	int ret;

	ic = gFormatCtx;

//	value = target_param->in_args[0];
//	cpu_memory_rw_debug(cpu_single_env, value, &ic, sizeof(AVFormatContext), 0);
	ret = av_find_stream_info(ic);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], ic->streams[0], sizeof(AVStream), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], ic->streams[0]->codec, sizeof(AVCodecContext), 1);

	return ret;
} */

/* AVCodec *avcodec_find_decoder (enum CodecID id) */
/* void qemu_avcodec_find_decoder (void)
{
//	AVCodec *pCodec;
	uint32_t value;
	enum CodecID codec_id;

	value = target_param->in_args[0];
	cpu_memory_rw_debug(cpu_single_env, value, &codec_id, sizeof(uint32_t), 0 );

	gCodec = avcodec_find_decoder(codec_id);
	if (!gCodec)
		perror("Failed to get codec\n");
//	gFormatCtx->streams[0]->codec->codec = pCodec;

	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, gCodec, sizeof(AVCodec), 1);
} */

/* void av_close_input_file (AVFormatContext *s) */
/* void qemu_av_close_input_file (void)
{
	av_close_input_file(gFormatCtx);
} */


static int codec_operate (uint32_t value)
{
	int ret = -1;
	
	switch (value) {
		case 1:
			qemu_av_register_all();
			break;
/*		case 2:
			ret = qemu_av_open_input_file();
			break;
		case 3:
			ret = qemu_av_find_stream_info();
			break;
		case 4:
			qemu_avcodec_find_decoder();
			break; */
		case 2:
			ret = qemu_avcodec_open();
			break;
		case 3:
			qemu_avcodec_close();
			break;
		case 4:
			qemu_avcodec_alloc_context();
			break;
		case 5:
			qemu_avcodec_alloc_frame();
			break;
		case 6:
			qemu_av_free();
			break;
		case 10:
			qemu_avcodec_get_context_defaults();
			break;
		case 11:
			qemu_avcodec_flush_buffers();
			break;
		case 12:
			ret = qemu_avcodec_default_get_buffer();
			break;
		case 13:
			qemu_avcodec_default_release_buffer();
			break;
		case 20:
			ret = qemu_avcodec_decode_video();
			break;
		case 21:
//			qemu_avcodec_decode_audio();
			break;
		case 22:
			ret = qemu_avcodec_encode_video();
			break;
		case 23:
//			qemu_avcodec_encode_audio();
			break;
		case 24:
			qemu_av_picture_copy();
			break;
		case 30:
			qemu_av_parser_init();
			break;
		case 31:
			ret= qemu_av_parser_parse();
			break;
		case 32:
			qemu_av_parser_close();
			break;

/*		case 7:
			ret = qemu_avpicture_get_size();	
			break;
		case 8:
			qemu_av_malloc();
			break;
		case 9:
			ret = qemu_avpicture_fill();
			break;
		case 10:
			ret = qemu_av_read_frame();
			break;
		case 11:
			ret = qemu_avcodec_decode_video2();
			break;
		case 12:
			qemu_sws_getContext();
			break;
		case 13:
			qemu_sws_scale();
			break;
		case 14:
			qemu_av_free_packet();
			break;
		case 17:
			qemu_av_close_input_file();
			break; */
	}
	return ret;
}
/*
static void codec_decode_prepare (void)
{
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	
	pFrame = av_alloc_frame();

	pFrameRGB = av_alloc_frame();

	avcodec_open(pCodecCtx, pCodec);
}

static void codec_encode_prepare (void)
{
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pPicture;

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
		
	pCodecCtx = avcodec_alloc_context();

	pPicture = avcodec_alloc_frame();

	avcodec_open(pCodecCtx, pCodec);

}

static void codec_decode (void)
{
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;
	AVPacket packet;
	int nFrameSize = 0;
	
	
	avcodec_decode_video2(pCodecCtx, pFrame, &nFrameSize, &packet);
}

static void codec_encode (void)
{
	uint8_t *outbuf;
	int outbuf_size;
	avcodec_encode_video(pCodecCtx, outbuf, outbuf_size, pPicture);
}


static void thread_func (void *arg)
{
	while(1) {
		pthread_mutex_lock(&sync_mutex);
		
		pthread_cond_wait(&sync_cond, &sync_mutex);
		{
		}
		pthread_mutex_unlock(&sync_mutex);
	}
}

static void thread_job (void)
{
	pthread_t work_thread;
	pthread_mutex_init(&sync_mutex, NULL);
	phtread_cond_init(&sync_cond, NULL);

	pthread_mutex_lock(&sync_mutex);
	if (pthread_create(&work_thread, NULL, thread_func, (void*)NULL) < 0) {
		perror("Failed to create thread\n");
		exit(0);
	}

	// pthread_mutex_trylock(&sync_mutex);
	if (0) {
		pthread_cond_signal(&sync_cond);
	}
	
	pthread_mutex_unlock(&sync_mutex);
} */


static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y = 0; y < height; y++)
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

    // Close file
    fclose(pFile);
}

/*
static int testEncode(const char *filename) {
	AVFormatContext *pFormatCtx;
	int             i, videoStream;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame;
	AVFrame         *pFrameRGB;
	AVPacket        packet;
	int             frameFinished;
	int             numBytes;
	uint8_t         *buffer;
	struct SwsContext *img_convert_ctx;
	int ret;

	if (!filename) {
		return 0;
	}
	// Register all formats and codecs
	av_register_all();

	// Open video file
	if ((ret = av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL)) != 0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	dump_format(pFormatCtx, 0, filename, 0);

	// Find the first video stream
	videoStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
	if(videoStream==-1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec
	if(avcodec_open(pCodecCtx, pCodec)<0)
		return -1; // Could not open codec

	// Allocate video frame
	pFrame=avcodec_alloc_frame();

	// Allocate an AVFrame structure
	pFrameRGB=avcodec_alloc_frame();
	if(pFrameRGB==NULL)
		return -1;

	// Determine required buffer size and allocate buffer
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			pCodecCtx->height);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);

	// Read frames and save first five frames to disk
	i=0;
	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if(frameFinished) {
				// Convert the image from its native format to RGB
				img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
						pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24,
						SWS_BICUBIC, NULL, NULL, NULL);
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				// Save the frame to disk
				if(++i<=5)
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	av_close_input_file(pFormatCtx);

	return 0;
} */

/*
 *	Virtual Codec Device Define
 */

static uint32_t codec_read (void *opaque, target_phys_addr_t addr)
{
	DEBUG_PRINT("[QEMU]codec_read\n");
	return 0;
}

static int count = 0;

static void codec_write (void *opaque, target_phys_addr_t addr, uint32_t value)
{
	uint32_t offset;
	int ret = -1;

	offset = addr - 512;
	switch (offset) {
		case FUNC_NUM:
			ret = codec_operate(value);
			if (ret >= 0) {
				cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, &ret, sizeof(int), 1);
			}
			count = 0;
			break;
		case IN_ARGS:
			target_param->in_args[count++] = value;
			break;
		case IN_ARGS_SIZE:
			target_param->in_args_size = value;
			break;
		case RET_STR:
			target_param->ret_args = value;
			break;
	}
}

static CPUReadMemoryFunc * const codec_io_readfn[3] = {
	codec_read,
	codec_read,
	codec_read,
};

static CPUWriteMemoryFunc * const codec_io_writefn[3] = {
	codec_write,
	codec_write,
	codec_write,
};

/* static void codec_ioport_map (PCIDevice *dev, int region_num,
								pcibus_t addr, pcibus_t size, int type)
{
	SlpCodecState *s = DO_UPCAST(SlpCodecState, dev, dev);
	
	register_ioport_write(addr, 0x100, 1, slpcodec_ioport_write, s);
	register_ioport_read(addr, 0x100, 1, slpcodec_ioport_read, s);
} */

static void codec_mmio_map (PCIDevice *dev, int region_num,
								pcibus_t addr, pcibus_t size, int type)
{
	SlpCodecState *s = DO_UPCAST(SlpCodecState, dev, dev);
	cpu_register_physical_memory(addr, CODEC_MEM_SIZE, s->codec_mmio_io_addr);
	
	// s->mmio_addrr = addr;
}

static int codec_initfn (PCIDevice *dev)
{
	SlpCodecState *s = DO_UPCAST(SlpCodecState, dev, dev);
	uint8_t *pci_conf = s->dev.config;

	target_param = (Codec_Param*)qemu_malloc(sizeof(Codec_Param));
	
	pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_SAMSUNG);
	pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_CODEC);
	pci_config_set_class(pci_conf, PCI_CLASS_MULTIMEDIA_OTHER);
	pci_conf[PCI_INTERRUPT_PIN] = 1;

	s->codec_mmio_io_addr = cpu_register_io_memory(codec_io_readfn, codec_io_writefn,
													s, DEVICE_LITTLE_ENDIAN);

	// pci_register_bar(&s->dev, 0, 256, PCI_BASE_ADDRESS_SPACE_IO, slpcodec_ioport_map);
	pci_register_bar(&s->dev, 1, 256, PCI_BASE_ADDRESS_SPACE_MEMORY, codec_mmio_map);

	return 0;
}

int pci_codec_init (PCIBus *bus)
{
	pci_create_simple (bus, -1, QEMU_DEV_NAME);
	return 0;
}

static PCIDeviceInfo codec_info = {
	.qdev.name 		= QEMU_DEV_NAME,
	.qdev.desc 		= "Virtual Codec for SLP x86 Emulator",
	.qdev.size 		= sizeof (SlpCodecState),	
	.init			= codec_initfn,
};

static void codec_register (void)
{
	pci_qdev_register(&codec_info);
}
device_init(codec_register);
