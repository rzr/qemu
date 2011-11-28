/* 
 * Virtual codec device
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

#include "codec_accel.h"

/* #define DEBUG_SLPCODEC

#ifdef DEBUG_SLPCODEC
#define DEBUG_PRINT(fmt, ...) \
	fprintf (stdout, "[qemu][%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);
#else
#define DEBUG_PRINT(fmt, ...) ((void)0);
#endif */

#define QEMU_DEV_NAME			"codec"
#define SVCODEC_MEM_SIZE		(4 * 1024 * 1024)
#define SVCODEC_REG_SIZE		(256)

/* define debug channel */
MULTI_DEBUG_CHANNEL(slp, svcodec);

enum {
	FUNC_NUM 	= 0x00,
	IN_ARGS 	= 0x04,
	RET_STR 	= 0x08,
};

static int codec_operate(uint32_t value, void *opaque);

/* pthread_mutex_t sync_mutex;
pthread_cond_t sync_cond; */

typedef struct _SVCodecParam {
	uint32_t func_num;
	uint32_t in_args[20];
	uint32_t ret_args;
} SVCodecParam;

SVCodecParam *target_param;

typedef struct _SVCodecState {
	PCIDevice		dev;

	int				svcodec_mmio;

	uint8_t*		vaddr;
	ram_addr_t		vram_offset;

	uint32_t		mem_addr;
	uint32_t		mmio_addr;
} SVCodecState;


/*
 *  Codec API Emulation
 */

struct SVCodecInfo {
//	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
//	AVPacket pkt;
};

AVCodecContext *gAVCtx = NULL;
AVFrame *gFrame = NULL;

/* void av_register_all() */
void qemu_av_register_all (void)
{
	av_register_all();
}

int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture)
{
	int ret;

	picture->reordered_opaque = context->reordered_opaque;
	picture->opaque = NULL;

	ret = avcodec_default_get_buffer(context, picture);
	TRACE("after avcodec_default_get_buffer, return value:%d\n", ret);

	return ret;
}

/* int avcodec_open (AVCodecContext *avctx, AVCodec *codec)	*/
int qemu_avcodec_open (void *opaque)
{
	AVCodecContext *avctx;
	AVCodecContext tempCtx;
	AVCodec *codec;
	AVCodec tempCodec;
	enum CodecID codec_id;
	bool bEncode;
	int ret;
	int size;

	SVCodecState* s = (SVCodecState*)opaque;

	/* guest to host */
	if (!gAVCtx) {
//		gAVCtx = avcodec_alloc_context();
		ERR("AVCodecContext is NULL!!\n");
	}
	avctx = gAVCtx;
	size = sizeof(AVCodecContext); 
	memcpy(&tempCtx, avctx, size);
	memcpy(avctx, s->vaddr, size);
	memcpy(&tempCodec, (uint8_t*)s->vaddr + size, sizeof(AVCodec));

	codec_id = tempCodec.id;
	TRACE("CODEC ID : %d\n", codec_id);	
	size += sizeof(AVCodec);

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], avctx, sizeof(AVCodecContext), 0);
	if (avctx->extradata_size > 0) {
		avctx->extradata = (uint8_t*)av_malloc(avctx->extradata_size);
//		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], avctx->extradata, avctx->extradata_size, 0);
		memcpy(avctx->extradata, (uint8_t*)s->vaddr + size, avctx->extradata_size);		
	} else {
		avctx->extradata = NULL;
	}
//	memcpy(&codec_id, (uint8_t*)s->vaddr + size, sizeof(int));
//	memcpy(&bEncode, s->vaddr + size + avctx->extradata_size + 4, sizeof(char));
#if 1	
	avctx->av_class = tempCtx.av_class;
	avctx->codec = tempCtx.codec;
	avctx->priv_data = tempCtx.priv_data;
	avctx->opaque = tempCtx.opaque;
	avctx->get_buffer = tempCtx.get_buffer;
	avctx->release_buffer = tempCtx.release_buffer;
	avctx->stats_out = tempCtx.stats_out;
	avctx->stats_in = tempCtx.stats_in;
	avctx->rc_override = tempCtx.rc_override;
	avctx->rc_eq = tempCtx.rc_eq;
	avctx->slice_offset = tempCtx.slice_offset;
	avctx->get_format = tempCtx.get_format;
	avctx->internal_buffer = tempCtx.internal_buffer;
	avctx->intra_matrix = tempCtx.intra_matrix;
	avctx->inter_matrix = tempCtx.inter_matrix;
	avctx->reget_buffer = tempCtx.reget_buffer;
	avctx->execute = tempCtx.execute;
	avctx->thread_opaque = tempCtx.thread_opaque;
	avctx->execute2 = tempCtx.execute2;
#endif	

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &codec_id, sizeof(enum CodecID), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &bEncode, sizeof(bool), 0);
	if (tempCodec.encode) {
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

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], avctx, sizeof(AVCodecContext), 1);
	memcpy(s->vaddr, avctx, sizeof(AVCodecContext));
	memcpy((uint8_t*)s->vaddr + sizeof(AVCodecContext), &ret, sizeof(int));
/*	if (!gAVCtx) {
		gAVCtx = avctx;
	}
	if (!gFrame) {
		gFrame = avcodec_alloc_frame();
	} */

	return ret;
}

/* int avcodec_close (AVCodecContext *avctx) */
int qemu_avcodec_close (void)
{
	AVCodecContext *avctx;
	int ret;
	
	avctx = gAVCtx;
	if (!avctx) {
		ERR("AVCodecContext is NULL\n");
		return -1;
	}
	ret = avcodec_close(avctx);
	TRACE("after avcodec_close. ret:%d\n", ret);

	return ret;
}

/* AVCodecContext* avcodec_alloc_context (void) */
void qemu_avcodec_alloc_context (void)
{
	gAVCtx = avcodec_alloc_context();
}

/* AVFrame *avcodec_alloc_frame (void) */
void qemu_avcodec_alloc_frame (void)
{
	gFrame = avcodec_alloc_frame();
}

/* void av_free (void *ptr) */
void qemu_av_free (void* opaque)
{
	int value;
	SVCodecState *s = (SVCodecState*)opaque;
	TRACE("Enter\n");

//	cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, &value, sizeof(int), 0);
	memcpy(&value, s->vaddr, sizeof(int));
	TRACE("free num :%d\n", value);

	if (value == 1) {
        if (gAVCtx)
            av_free(gAVCtx);
        gAVCtx = NULL;
    } else if (value == 2) {
        if (gFrame)
            av_free(gFrame);
        gFrame = NULL;
    } else if (value == 3) {
        if (gAVCtx->palctrl)
            av_free(gAVCtx->palctrl);
        gAVCtx->palctrl = NULL;
    } else {
        if (gAVCtx->extradata && gAVCtx->extradata_size > 0)
            av_free(gAVCtx->extradata);
        gAVCtx->extradata = NULL;
    }

	TRACE("Leave\n");
}

/* void avcodec_get_context_defaults (AVCodecContext *ctx) */
void qemu_avcodec_get_context_defaults (void)
{
	AVCodecContext *avctx;

	TRACE("Enter\n");
	avctx = gAVCtx;
	if (avctx) {
		avcodec_get_context_defaults(avctx);
	} else {
		ERR("AVCodecContext is NULL\n");
	}
	TRACE("Leave\n");
}

/* void avcodec_flush_buffers (AVCodecContext *avctx) */
void qemu_avcodec_flush_buffers (void)
{
	AVCodecContext *avctx;

	TRACE("Enter\n");

	avctx = gAVCtx;
	if (avctx) {
		avcodec_flush_buffers(avctx);
	} else {
		ERR("AVCodecContext is NULL\n");
	}
	TRACE("Leave\n");
}

/* int avcodec_default_get_buffer (AVCodecContext *s, AVFrame *pic) */
int qemu_avcodec_default_get_buffer (void)
{
	AVCodecContext *avctx;
	AVFrame *pic;
	int ret;

	TRACE("Enter\n");

	avctx = gAVCtx;
	pic = gFrame;
	if (avctx == NULL | pic == NULL) {
		ERR("AVCodecContext or AVFrame is NULL!!\n");
		return -1;
	} else {
		ret = avcodec_default_get_buffer(avctx, pic);
	}

	TRACE("Leave, return value:%d\n", ret);

	return ret;
}

/* void avcodec_default_release_buffer (AVCodecContext *ctx, AVFrame *frame) */
void qemu_avcodec_default_release_buffer (void)
{
	AVCodecContext *ctx;
	AVFrame *frame;

	TRACE("Enter\n");

	ctx = gAVCtx;
	frame = gFrame;
	if (ctx == NULL | frame == NULL) {
		ERR("AVCodecContext or AVFrame is NULL!!\n");
	} else {
		avcodec_default_release_buffer(ctx, frame);
	}

	TRACE("Leave\n");
}

/* int avcodec_decode_video (AVCodecContext *avctx, AVFrame *picture,
 * 							int *got_picture_ptr, const uint8_t *buf,
 * 							int buf_size)
 */
int qemu_avcodec_decode_video (void* opaque)
{
	AVCodecContext *avctx;
	AVCodecContext tempCtx;
	AVFrame *picture;
	int got_picture_ptr = 0;
	const uint8_t *buf = NULL;
	int buf_size = 0, size;
	int ret;

	SVCodecState *s = (SVCodecState*)opaque;

	avctx = gAVCtx;
	picture = gFrame;
	if (avctx == NULL | picture == NULL) {
		ERR("AVCodecContext or AVFrame is NULL!! avctx:0x%x, picture:0x%x\n", avctx, picture);
		return -1;
	}
	size = sizeof(AVCodecContext);

	memcpy(&tempCtx, avctx, size);
	memcpy(avctx, s->vaddr, size);
	memcpy(&buf_size, (uint8_t*)s->vaddr + size, sizeof(int));
	size += sizeof(int);

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], &buf_size, sizeof(int), 0);
	if (buf_size > 0) {
		buf = (uint8_t*)av_malloc(buf_size * sizeof(uint8_t));
//		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], buf, buf_size, 0);
		memcpy(buf, (uint8_t*)s->vaddr + size, buf_size);
	} else {
		ERR("There is no input buffer\n");
	}

#if 1   
    avctx->av_class = tempCtx.av_class;
    avctx->codec = tempCtx.codec;
	avctx->extradata = tempCtx.extradata;
    avctx->priv_data = tempCtx.priv_data;
    avctx->opaque = tempCtx.opaque;
    avctx->get_buffer = tempCtx.get_buffer;
    avctx->release_buffer = tempCtx.release_buffer;
    avctx->stats_out = tempCtx.stats_out;
    avctx->stats_in = tempCtx.stats_in;
    avctx->rc_override = tempCtx.rc_override;
    avctx->rc_eq = tempCtx.rc_eq;
    avctx->slice_offset = tempCtx.slice_offset;
    avctx->get_format = tempCtx.get_format;
    avctx->internal_buffer = tempCtx.internal_buffer;
    avctx->intra_matrix = tempCtx.intra_matrix;
    avctx->inter_matrix = tempCtx.inter_matrix;
    avctx->reget_buffer = tempCtx.reget_buffer;
    avctx->execute = tempCtx.execute;
    avctx->thread_opaque = tempCtx.thread_opaque;
    avctx->execute2 = tempCtx.execute2;
#endif

/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], &avctx->frame_number, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], &avctx->pix_fmt, sizeof(enum PixelFormat), 0);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[7], &avctx->coded_frame, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[8], &avctx->sample_aspect_ratio, sizeof(AVRational), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[9], &avctx->reordered_opaque, sizeof(int), 0); */

	ret = avcodec_decode_video(avctx, picture, &got_picture_ptr, buf, buf_size);
	TRACE("after decode video, ret:%d\n", ret);
	if (got_picture_ptr == 0) {
		TRACE("There is no frame\n");
	}

	if (buf_size > 0) {
		size = sizeof(AVCodecContext);
		memcpy(s->vaddr, avctx, size);
		memcpy((uint8_t*)s->vaddr + size, picture, sizeof(AVFrame));
		size += sizeof(AVFrame);
	
		memcpy((uint8_t*)s->vaddr + size, &got_picture_ptr, sizeof(int));
		size += sizeof(int);
		memcpy((uint8_t*)s->vaddr + size, &ret, sizeof(int));
		av_free(buf);
	} else {
		memcpy(s->vaddr, &ret, sizeof(int));
	}

/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], picture, sizeof(AVFrame), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &got_picture_ptr, sizeof(int), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], &avctx->pix_fmt, sizeof(enum PixelFormat), 1); */
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[7], avctx->coded_frame, sizeof(AVFrame), 1);

	return ret;
}

/* int avcodec_decode_audio (AVCodecContext *avctx, AVFrame *picture,
								int *got_picture_ptr, const uint8_t *buf,
								int buf_size) */


/* int avcodec_encode_video (AVCodecContext *avctx, uint8_t *buf,
 * 							int buf_size, const AVFrame *pict)
 */
int qemu_avcodec_encode_video (void* opaque)
{
	AVCodecContext *avctx;
	uint8_t *buf, *buffer;
	int buf_size, size;
	AVFrame *pict;
//	AVFrame tempFrame;
	int numBytes;
	int ret, i;

	SVCodecState *s = (SVCodecState*)opaque;

	if (gAVCtx) {
		avctx = gAVCtx;
		pict = gFrame;
	} else {
		ERR("AVCodecContext is NULL\n");
		return -1;
	}

//	memcpy(&tempFrame, pict, sizeof(AVFrame));

/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &buf_size, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], pict, sizeof(AVFrame), 0); */

	memcpy(&buf_size, s->vaddr, sizeof(int));
	if (buf_size > 0) {
		buf = (uint8_t*)av_malloc(buf_size * sizeof(uint8_t));
		memcpy(buf, (uint8_t*)s->vaddr + 4, buf_size);
//		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], buf, buf_size, 0);
	} else {
		ERR("input buffer size is 0\n");
		return -1;
	}

	size = buf_size + 4;
	numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	if (!buffer) {
		ERR("failed to allocate decoded frame buffer\n");	
		return -1;
	}

	memcpy(pict, (uint8_t*)s->vaddr + size, sizeof(AVFrame));
	size += sizeof(AVFrame);
	memcpy(buffer, (uint8_t*)s->vaddr + size, numBytes);	

//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], buffer, numBytes, 0);
	
    avpicture_fill(pict, buffer, avctx->pix_fmt, avctx->width, avctx->height);

	ret = avcodec_encode_video (avctx, buf, buf_size, pict);
	TRACE("after encode video, ret:%d\n", ret);

	if (ret > 0) {
//		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], avctx->coded_frame, sizeof(AVFrame), 1);
//		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], buf, buf_size, 1);

/*		memcpy(s->vaddr, avctx->coded_frame, sizeof(AVFrame));
		memcpy((uint8_t*)s->vaddr + sizeof(AVFrame), buf, buf_size);
		size = sizeof(AVFrame) + buf_size; 
		memcpy((uint8_t*)s->vaddr + size, &ret, sizeof(int)); */

		memcpy(s->vaddr, buf, buf_size);
		memcpy((uint8_t*)s->vaddr + buf_size, &ret, sizeof(int));
	}

	TRACE("after encode video, ret:%d\n", ret);

	av_free(buffer);
//	av_free(buf);

	return ret;
}


/* int avcodec_encode_audio (AVCodecContext *avctx, uint8_t *buf,
 * 							int buf_size, const AVFrame *pict)
 */


/* void av_picture_copy (AVPicture *dst, const AVPicture *src,
 * 						enum PixelFormat pix_fmt, int width, int height)
 */
void qemu_av_picture_copy (void* opaque)
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

	SVCodecState *s = (SVCodecState*)opaque;

	avctx = gAVCtx;
	src = (AVPicture*)gFrame;
/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &pix_fmt, sizeof(enum PixelFormat), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], &width, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &height, sizeof(int), 0); */

	numBytes = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);
//	cpu_memory_request_page_debug(cpu_single_env, target_param->in_args[4], numBytes, 0);
   	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill(&dst, buffer, avctx->pix_fmt, avctx->width, avctx->height);
	av_picture_copy(&dst, src, avctx->pix_fmt, avctx->width, avctx->height);

//	ret = cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], dst.data[0], numBytes, 1);
	memcpy(s->vaddr, dst.data[0], numBytes);
	TRACE("After copy image buffer from host to guest, ret:%d\n", ret);
	
/*	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[5], dst.data[1], (width * height)/4, 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[6], dst.data[2], (width * height)/4, 1); */

	av_free(buffer);
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

/* int av_parser_parse(AVCodecParserContext *s, AVCodecContext *avctx,
 *						uint8_t **poutbuf, int *poutbuf_size,
 *						const uint8_t *buf, int buf_size,
 * 						int64_t pts, int64_t dts)
 */
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


#if 0

/*
 * THEORA API
 */

th_dec_ctx * _gDec;
th_info* _gInfo;
th_setup_info* _gSetup;
th_comment* _gTC;

/* int th_decode_ctl (th_dec_ctx* _dec, int _req,
 *						void* _buf, size_t _buf_sz)
 */
int qemu_th_decode_ctl (void)

{
	th_dec_ctx* dec;
	int req;
	void* buf;
	size_t buf_size;
	int ret;

	TRACE("Enter\n");

	if (_gDec) {
		dec = _gDec;
	} else {
		ERR("Failed to get th_dec_ctx pointer\n");
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &req, sizeof(int), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &buf_size, sizeof(size_t), 0);
	if (buf_size > 0) {
		buf = malloc(buf_size);
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], buf, buf_size, 0);
	}
	ret = th_decode_ctl(dec, req, buf, buf_size);

//	cpu_meory_rw_debug(cpu_single_env, target_param->in_args[0], dec, sizeof(th_dec_ctx), 1);

	_gDec = dec;

	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* th_dec_ctx* th_decode_alloc (const th_info* _info,
 *								const th_setup_info* _setup)
 */
th_dec_ctx* qemu_th_decode_alloc (void)
{
	th_info info;
	th_setup_info* setup;
	th_dec_ctx *ctx;
	
	TRACE("Enter\n");

	if (_gSetup) {
		setup = _gSetup;
	}

	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &info, sizeof(th_info), 0);

	ctx = th_decode_alloc(&info, setup);
	if (ctx) {
		_gDec = ctx;
	} else {
		ERR("Failed to alloc theora decoder\n");
	}

	TRACE("Leave\n");

	return ctx;
}

/* int th_decode_headerin (th_info* _info, th_comment* _tc,
 *						   th_setup_info** _setup, ogg_packet* _op)
 */
int qemu_th_decode_headerin (void)
{
	th_info info;
	th_comment tc;
	th_setup_info *setup;
	ogg_packet op;
	long bytes;
	char *packet;
	int ret;

	TRACE("Enter\n");

//	setup = _gSetup;
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &info, sizeof(th_info), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &tc, sizeof(th_comment), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &op, sizeof(ogg_packet), 0);
	bytes = op.bytes;
	if (bytes > 0) {
		packet = (char*)malloc(bytes * sizeof(char));
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[4], packet, bytes, 0);
	}
	op.packet = packet;
	ret = th_decode_headerin(&info, &tc, &setup, &op);
	_gSetup = setup;
	
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[0], &info, sizeof(th_info), 1);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &tc, sizeof(th_comment), 1);
//	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], setup, sizeof(th_setup_info), 1);

	free(packet);

	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* int th_decode_packetin (th_dec_ctx* _dec,
 *						   const ogg_packet* _op,
 *						   ogg_int64_t* _granpos)
 */
int qemu_th_decode_packetin (void)
{
	th_dec_ctx* dec;
	ogg_packet op;
	ogg_int64_t granpos;
	char *packet;
	long bytes;
	int ret;

	TRACE("Enter\n");

	if (_gDec) {
		dec = _gDec;
	}
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[1], &op, sizeof(ogg_packet), 0);
	cpu_memory_rw_debug(cpu_single_env, target_param->in_args[3], &granpos, sizeof(ogg_int64_t), 0);
	bytes = op.bytes;
	if (bytes > 0) {
		packet = (char*)malloc(bytes * sizeof(char));
		cpu_memory_rw_debug(cpu_single_env, target_param->in_args[2], packet, bytes, 0);
	}
	op.packet = packet;

	ret = th_decode_packetin(dec, &op, &granpos);

	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* int th_decode_ycbcr_out (th_dec_ctx* _dec,
 *							th_ycbcr_buffer _ycbcr)
 */
int qemu_th_decode_ycbcr_out (void)
{
	th_dec_ctx* dec;
	th_ycbcr_buffer ycbcr;
	int ret, size;

	TRACE("Enter\n");

	if (_gDec) {
		dec = _gDec;
	}
	size = sizeof(th_dec_ctx);
	ret = th_decode_ycbcr_out(dec, ycbcr);
	
	TRACE("Leave : ret(%d)\n", ret);
	return ret;
}

/* void th_info_clear (th_info* _info) */
void qemu_th_info_clear (void)
{
	th_info* info;
	
	TRACE("Enter\n");

	th_info_clear(info);

	TRACE("Leave\n");
}

/* void th_comment_clear (th_comment* _tc) */
void qemu_th_comment_clear (void)
{
	th_comment* tc;

	TRACE("Enter\n");

	th_comment_clear(tc);

	TRACE("Leave\n");
}

/* void th_setup_free (th_setup_info* _setup) */
void qemu_th_setup_free (void)
{
	th_setup_info* setup;

	TRACE("Enter\n");

	th_setup_free(setup);

	TRACE("Leave\n");
}

/* void th_decode_free (th_dec_ctx* _dec) */
void qemu_th_decode_free (void)
{
	th_dec_ctx* dec;

	TRACE("Enter\n");

	th_decode_free(dec);

	TRACE("Leave\n");
}
#endif

static int codec_operate (uint32_t apiIndex, void *opaque)
{
	int ret = -1;
	
	switch (apiIndex) {
		/* FFMPEG API */
		case 1:
			qemu_av_register_all();
			break;
		case 2:
			ret = qemu_avcodec_open(opaque);
			break;
		case 3:
			ret = qemu_avcodec_close();
			break;
		case 4:
			qemu_avcodec_alloc_context();
			break;
		case 5:
			qemu_avcodec_alloc_frame();
			break;
		case 6:
			qemu_av_free(opaque);
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
			ret = qemu_avcodec_decode_video(opaque);
			break;
		case 21:
//			qemu_avcodec_decode_audio();
			break;
		case 22:
			ret = qemu_avcodec_encode_video(opaque);
			break;
		case 23:
//			qemu_avcodec_encode_audio();
			break;
		case 24:
			qemu_av_picture_copy(opaque);
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
		/* THEORA API */
/*		case 40:
			ret = qemu_th_decode_ctl();
			break;
		case 41:
			ret = qemu_th_decode_alloc();
			break;
		case 42:
			ret = qemu_th_decode_headerin();
			break;
		case 43:
			ret = qemu_th_decode_packetin();
			break;
		case 44:
			ret = qemu_th_decode_ycbcr_out();
			break;
		case 45:
			qemu_th_info_clear();
			break;
		case 46:
			qemu_th_comment_clear();
			break;
		case 47:
			qemu_th_setup_free();
			break;
		case 48:
			qemu_th_decode_free();
			break; */
		default:
			printf("The api index does not exsit!. api index:%d\n", apiIndex);
	}
	return ret;
}

/*
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


/*
 *	Virtual Codec Device Define
 */
static uint32_t codec_read (void *opaque, target_phys_addr_t addr)
{
	TRACE("empty function\n");
	return 0;
}

static int count = 0;
static void codec_write (void *opaque, target_phys_addr_t addr, uint32_t value)
{
	uint32_t offset;
	int ret = -1;
	SVCodecState *state = (SVCodecState*)opaque;

	offset = addr;
	switch (offset) {
		case FUNC_NUM:
			ret = codec_operate(value, state);
			if (ret >= 0) {
				cpu_memory_rw_debug(cpu_single_env, target_param->ret_args, &ret, sizeof(int), 1);
//				memcpy(state->mmio_addr, &ret, sizeof(int));
			}
			count = 0;
			break;
		case IN_ARGS:
			target_param->in_args[count++] = value;
			break;
		case RET_STR:
			target_param->ret_args = value;
			break;
	}
}

static CPUReadMemoryFunc * const svcodec_io_readfn[3] = {
	codec_read,
	codec_read,
	codec_read,
};

static CPUWriteMemoryFunc * const svcodec_io_writefn[3] = {
	codec_write,
	codec_write,
	codec_write,
};

static void codec_mem_map (PCIDevice *dev, int region_num,
							pcibus_t addr, pcibus_t size, int type)
{
	SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
	cpu_register_physical_memory(addr, size, s->vram_offset);
	s->mem_addr = addr;
}

static void codec_mmio_map (PCIDevice *dev, int region_num,
							pcibus_t addr, pcibus_t size, int type)
{
	SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
	cpu_register_physical_memory(addr, size, s->svcodec_mmio);
	s->mmio_addr = addr;
}

static int codec_initfn (PCIDevice *dev)
{
	SVCodecState *s = DO_UPCAST(SVCodecState, dev, dev);
	uint8_t *pci_conf = s->dev.config;

	target_param = (SVCodecParam*)qemu_malloc(sizeof(SVCodecParam));
	memset(target_param, 0x00, sizeof(SVCodecParam));
	
	pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_SAMSUNG);
	pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_CODEC);
	pci_config_set_class(pci_conf, PCI_CLASS_MULTIMEDIA_OTHER);
	pci_config_set_interrupt_pin(pci_conf, 0x01);

	s->vram_offset = qemu_ram_alloc(NULL, "codec.ram", SVCODEC_MEM_SIZE);
	s->vaddr = qemu_get_ram_ptr(s->vram_offset);

	s->svcodec_mmio = cpu_register_io_memory(svcodec_io_readfn, svcodec_io_writefn,
											s, DEVICE_LITTLE_ENDIAN);

    /* setup memory space */
    /* memory #0 device memory (overlay surface) */
    /* memory #1 memory-mapped I/O */
	pci_register_bar(&s->dev, 0, SVCODEC_MEM_SIZE,
					PCI_BASE_ADDRESS_MEM_PREFETCH, codec_mem_map);
	pci_register_bar(&s->dev, 1, SVCODEC_REG_SIZE,
					PCI_BASE_ADDRESS_SPACE_MEMORY, codec_mmio_map);

	return 0;
}

int pci_codec_init (PCIBus *bus)
{
	pci_create_simple (bus, -1, QEMU_DEV_NAME);
	return 0;
}

static PCIDeviceInfo codec_info = {
	.qdev.name 		= QEMU_DEV_NAME,
	.qdev.desc 		= "Virtual codec device for emulator",
	.qdev.size 		= sizeof (SVCodecState),	
	.init			= codec_initfn,
};

static void codec_register (void)
{
	pci_qdev_register(&codec_info);
}
device_init(codec_register);
