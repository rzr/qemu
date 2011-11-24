#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include "hw.h"
#include "pci.h"
#include "pci_ids.h"
#include "debug_ch.h"

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#if 0
#include <theora/codec.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include "decint.h"
#endif

/*
 *  FFMPEG APIs
 */

void qemu_av_register_all (void);

int qemu_avcodec_open (void);

int qemu_avcodec_close (void);

void qemu_avcodec_alloc_context (void);

void qemu_avcodec_alloc_frame (void);

void qemu_av_free (void);

void qemu_avcodec_get_context_defaults (void);

void qemu_avcodec_flush_buffers (void);

int qemu_avcodec_default_get_buffer (void);

void qemu_avcodec_default_release_buffer (void);

int qemu_avcodec_decode_video (void);

// int qemu_avcodec_decode_audio (void);

int qemu_avcodec_encode_video (void);

// int qemu_avcodec_encode_audio (void);

void qemu_av_picture_copy (void);

void qemu_av_parser_init (void);

int qemu_av_parser_parse (void);

void qemu_av_parser_close (void);

int qemu_avcodec_get_buffer (AVCodecContext *context, AVFrame *picture);
#if 0
/*
 * THEORA APIs
 */

int qemu_th_decode_ctl (void);

th_dec_ctx* qemu_th_decode_alloc (void);

int qemu_th_decode_headerin (void);

int qemu_th_decode_packetin (void);

int qemu_th_decode_ycbcr_out (void);

void qemu_th_info_clear (void);

void qemu_th_comment_clear (void);

void qemu_th_setup_free (void);

void qemu_th_decode_free (void);

#endif
