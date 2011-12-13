#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include "hw.h"
#include "pci.h"
#include "pci_ids.h"
#include "debug_ch.h"

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

// #define CODEC_THREAD
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

void qemu_av_parser_init (void);

int qemu_av_parser_parse (void);

void qemu_av_parser_close (void);

int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture);

void qemu_avcodec_release_buffer (AVCodecContext *context, AVFrame *picture);

