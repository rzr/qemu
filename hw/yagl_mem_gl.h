#ifndef _QEMU_YAGL_MEM_GL_H
#define _QEMU_YAGL_MEM_GL_H

#include "yagl_mem.h"

#define yagl_mem_prepare_GLint(mt, va) yagl_mem_prepare_int32((mt), (va))
#define yagl_mem_put_GLint(mt, value) yagl_mem_put_int32((mt), (value))
#define yagl_mem_get_GLint(va, value) yagl_mem_get_int32((va), (value))
#define yagl_mem_prepare_GLuint(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_GLuint(mt, value) yagl_mem_put_uint32((mt), (value))
#define yagl_mem_get_GLuint(va, value) yagl_mem_get_uint32((va), (value))
#define yagl_mem_prepare_GLsizei(mt, va) yagl_mem_prepare_int32((mt), (va))
#define yagl_mem_put_GLsizei(mt, value) yagl_mem_put_int32((mt), (value))
#define yagl_mem_get_GLsizei(va, value) yagl_mem_get_int32((va), (value))
#define yagl_mem_prepare_GLenum(mt, va) yagl_mem_prepare_uint32((mt), (va))
#define yagl_mem_put_GLenum(mt, value) yagl_mem_put_uint32((mt), (value))
#define yagl_mem_get_GLenum(va, value) yagl_mem_get_uint32((va), (value))
#define yagl_mem_prepare_GLfloat(mt, va) yagl_mem_prepare_float((mt), (va))
#define yagl_mem_put_GLfloat(mt, value) yagl_mem_put_float((mt), (value))
#define yagl_mem_get_GLfloat(va, value) yagl_mem_get_float((va), (value))
#define yagl_mem_prepare_GLboolean(mt, va) yagl_mem_prepare_uint8((mt), (va))
#define yagl_mem_put_GLboolean(mt, value) yagl_mem_put_uint8((mt), (value))
#define yagl_mem_get_GLboolean(va, value) yagl_mem_get_uint8((va), (value))
#define yagl_mem_prepare_GLfixed(mt, va) yagl_mem_prepare_int32((mt), (va))
#define yagl_mem_put_GLfixed(mt, value) yagl_mem_put_int32((mt), (value))
#define yagl_mem_get_GLfixed(va, value) yagl_mem_get_int32((va), (value))

#endif
