#ifndef _QEMU_YAGL_MEM_EGL_H
#define _QEMU_YAGL_MEM_EGL_H

#include "yagl_mem.h"
#include <EGL/egl.h>

#define yagl_mem_prepare_EGLint(mt, va) yagl_mem_prepare_int32((mt), (va))
#define yagl_mem_put_EGLint(mt, value) yagl_mem_put_int32((mt), (value))
#define yagl_mem_get_EGLint(ts, va, value) yagl_mem_get_int32((ts), (va), (value))

EGLint *yagl_mem_get_attrib_list(struct yagl_thread_state *ts,
                                 target_ulong va);

#endif
