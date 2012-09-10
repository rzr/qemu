#ifndef _QEMU_YAGL_MEM_GL_H
#define _QEMU_YAGL_MEM_GL_H

#include "yagl_mem.h"

#define yagl_mem_put_GLint(ts, va, value) yagl_mem_put_int32((ts), (va), (value))
#define yagl_mem_get_GLint(ts, va, value) yagl_mem_get_int32((ts), (va), (value))
#define yagl_mem_put_GLuint(ts, va, value) yagl_mem_put_uint32((ts), (va), (value))
#define yagl_mem_get_GLuint(ts, va, value) yagl_mem_get_uint32((ts), (va), (value))
#define yagl_mem_put_GLsizei(ts, va, value) yagl_mem_put_int32((ts), (va), (value))
#define yagl_mem_get_GLsizei(ts, va, value) yagl_mem_get_int32((ts), (va), (value))
#define yagl_mem_put_GLenum(ts, va, value) yagl_mem_put_uint32((ts), (va), (value))
#define yagl_mem_get_GLenum(ts, va, value) yagl_mem_get_uint32((ts), (va), (value))

#endif
