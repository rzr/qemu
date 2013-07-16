#ifndef VIRTIO_GL_H_
#define VIRTIO_GL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/hw.h"
#include "maru_device_ids.h"
#include "hw/virtio/virtio.h"
#include "exec/hwaddr.h"

#include "opengl_process.h"
#include "opengl_exec.h"
#include <sys/time.h>

#include "tizen/src/debug_ch.h"
MULTI_DEBUG_CHANNEL(qemu, virtio-gl);

#define TYPE_VIRTIO_GL	"virtio-gl"
#define VIRTIO_GL(obj) \
        OBJECT_CHECK(VirtIOGL, (obj), TYPE_VIRTIO_GL)

typedef hwaddr arg_t;
int decode_call_int(ProcessStruct *p, char *in_args, int args_len, char *r_buffer);

/* Uncomment to enable debugging - WARNING!!! changes ABI! */
//#define DEBUG_GLIO

typedef struct VirtIOGL
{
	VirtIODevice vdev;
	VirtQueue *vq;
} VirtIOGL;

struct d_hdr
{
	int pid;
	int rq_l;
	int rrq_l;
#ifdef DEBUG_GLIO
	int sum;
#endif
};

#define SIZE_OUT_HEADER sizeof(struct d_hdr)

#ifdef DEBUG_GLIO
#define SIZE_IN_HEADER (4*2)
#else
#define SIZE_IN_HEADER 4
#endif

#ifdef __cplusplus
}
#endif

#endif /* VIRTIO_GL_H_ */
