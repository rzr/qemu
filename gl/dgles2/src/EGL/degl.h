/* Copyright (C) 2010  Nokia Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DEGL_H_
#define DEGL_H_

#include "common.h"
#include "common-vertex.h"

// Available rendering backends.
typedef enum DEGLBackend
{
	DEGLBackend_invalid,
#	if(CONFIG_OSMESA == 1)
	DEGLBackend_osmesa,
#	endif // (CONFIG_OSMESA == 1)
#	if(CONFIG_GLX == 1)
	DEGLBackend_glx,
#	endif // (CONFIG_GLX == 1)
#	if(CONFIG_WGL == 1)
	DEGLBackend_wgl,
#	endif // (CONFIG_WGL == 1)
#	if(CONFIG_COCOA == 1)
	DEGLBackend_cocoa,
#	endif // (CONFIG_COCOA == 1)
} DEGLBackend;

// Available system frontends.
typedef enum DEGLFrontend
{
	DEGLFrontend_invalid,
#	if(CONFIG_GLX == 1)
	DEGLFrontend_x11,
#	endif // (CONFIG_GLX == 1)
#	if(CONFIG_COCOA == 1)
	DEGLFrontend_cocoa,
#	endif
#	if(CONFIG_OFFSCREEN == 1)
	DEGLFrontend_offscreen,
#	endif // (CONFIG_GLX == 1)
} DEGLFrontend;

extern DEGLBackend degl_backend;
extern DEGLFrontend degl_frontend;
extern int degl_no_alpha;
#if(CONFIG_GLX == 1)
extern int degl_glx_direct;
#endif
#if(CONFIG_STATIC == 1)
extern void* degl_handle;
#endif

//used to keep track of converted buffers
typedef struct degl_vbo_s degl_vbo;

struct degl_vbo_s
{
	GLuint name;
	GLenum from; //data type converted from
	GLenum to; //data type converted to
	degl_vbo* next;
};

#if(CONFIG_OFFSCREEN == 1)
typedef struct degl_internal_bo_s
{
	GLuint name;
	EGLSurface surface;
	struct degl_internal_bo_s *next;
} degl_internal_bo;
#endif // CONFIG_OFFSCREEN == 1

#if(CACHE_ELEMENT_BUFFERS == 1)
typedef struct degl_ebo_s
{
	GLuint name;
	GLsizei size;
	void *cache;
	struct degl_ebo_s *next;
} degl_ebo;
#endif // CACHE_ELEMENT_BUFFERS == 1

typedef struct degl_tls_s
{
	EGLint error;
	EGLContext current_context;
	void *hgl; // host GL function pointer array
	char *extensions_string;
	EGLint max_vertex_arrays;
	DGLVertexArray* vertex_arrays;
	void (*vertex_free)(void);
	int shader_strip_enabled;
	struct shader_strtok_s
	{
		char *buffer;
		int buffersize;
		const char *prev;
	} shader_strtok;
	degl_vbo* vbo_list;
	struct ebo_s
	{
		degl_ebo *list;
	} ebo;
#	if(CONFIG_OFFSCREEN == 1)
	degl_internal_bo *reserved_buffers;
#	endif // CONFIG_OFFSCREEN
	GLenum glError;
} degl_tls;
extern degl_tls * EGLAPIENTRY deglGetTLS(void);
extern void deglFreeTLS(void);

extern void* EGLAPIENTRY deglGetHostProcAddress(char const* path);
extern void* EGLAPIENTRY dglGetHostProcAddress(char const* path);

struct DEGLContext;
extern void deglActivateClientAPI(struct DEGLContext *context);
extern void deglDeactivateClientAPI(struct DEGLContext *context);
extern void deglSetError(EGLint error);
extern void dglSetError(GLenum error);
extern degl_vbo* deglFindVBO(GLuint name);
extern int deglAddVBOToList(GLuint name, GLenum from);
extern int deglRemoveVBOFromList(GLuint name);
extern int deglFreeVBOList(degl_tls* tls);
extern int deglUnbindVBOFromArrays(GLuint name);
#if(CONFIG_OFFSCREEN == 1)
extern void deglAddReservedBuffer(GLuint name);
extern void deglRemoveReservedBuffer(GLuint name);
extern void deglReplaceReservedBuffer(GLuint old_name, GLuint new_name);
extern int deglIsReservedBuffer(GLuint name);
#endif // CONFIG_OFFSCREEN == 1
#endif // DEGL_H_
