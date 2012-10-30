#ifndef _QEMU_YAGL_MEM_GL_H
#define _QEMU_YAGL_MEM_GL_H

#include "yagl_mem.h"

#define yagl_mem_prepare_GLint(mt, va) yagl_mem_prepare_int32((mt), (va))
#define yagl_mem_put_GLint(mt, value) yagl_mem_put_int32((mt), (value))
#define yagl_mem_get_GLint(ts, va, value) yagl_mem_get_int32((ts), (va), (value))
#define yagl_mem_prepare_GLuint(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_GLuint(mt, value) yagl_mem_put_uint32((mt), (value))
#define yagl_mem_get_GLuint(ts, va, value) yagl_mem_get_uint32((ts), (va), (value))
#define yagl_mem_prepare_GLsizei(mt, va) yagl_mem_prepare_int32((mt), (va))
#define yagl_mem_put_GLsizei(mt, value) yagl_mem_put_int32((mt), (value))
#define yagl_mem_get_GLsizei(ts, va, value) yagl_mem_get_int32((ts), (va), (value))
#define yagl_mem_prepare_GLenum(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_GLenum(mt, value) yagl_mem_put_uint32((mt), (value))
#define yagl_mem_get_GLenum(ts, va, value) yagl_mem_get_uint32((ts), (va), (value))
#define yagl_mem_prepare_GLfloat(mt, va) yagl_mem_prepare_float((mt), (va))
#define yagl_mem_put_GLfloat(mt, value) yagl_mem_put_float((mt), (value))
#define yagl_mem_get_GLfloat(ts, va, value) yagl_mem_get_float((ts), (va), (value))
#define yagl_mem_prepare_GLboolean(mt, va) yagl_mem_prepare_uint8((mt), (va))
#define yagl_mem_put_GLboolean(mt, value) yagl_mem_put_uint8((mt), (value))
#define yagl_mem_get_GLboolean(ts, va, value) yagl_mem_get_uint8((ts), (va), (value))

#endif
