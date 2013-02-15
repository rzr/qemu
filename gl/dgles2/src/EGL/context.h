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

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <pthread.h>
#include "common.h"
#if(CONFIG_OSMESA == 1)
#	include "hosmesa.h"
#endif // (CONFIG_OSMESA == 1)
#if(CONFIG_GLX == 1)
#	include "hglx.h"
#endif // (CONFIG_GLX == 1)
#if(CONFIG_COCOA == 1)
#	include "hcocoa.h"
#endif // (CONFIG_COCOA == 1)
#if(CONFIG_WGL == 1)
#	include "hwgl.h"
#endif // (CONFIG_WGL == 1)
#include "surface.h"

typedef struct GLESFuncs
{
	void (*dglReadPixels)(void *pixels, unsigned *pbo, unsigned x, unsigned y,
		unsigned width, unsigned height, unsigned format, unsigned type,
		unsigned flags);
	void (*glFlush)(void);
	void (*glFinish)(void);
	void (*dglBindTexImage)(unsigned width, unsigned height, char* pixels);
#	if(CONFIG_OFFSCREEN == 1)
	void (*dglDeletePBO)(unsigned pbo);
#	endif // CONFIG_OFFSCREEN == 1
} GLESFuncs;

struct degl_ebo_s;

typedef struct DEGLContext
{
	EGLDisplay display;
	EGLConfig config;
	int version;
	void* clientlib;
	void* clientdata;
	GLESFuncs* glesfuncs;
	int bound;
	int destroy;
	DEGLSurface* draw;
	DEGLSurface* read;
	pthread_mutex_t lock;
	struct degl_ebo_s *ebo_bound;

#	if(CONFIG_OSMESA == 1)
	struct
	{
		OSMesaContext ctx;
	} osmesa;
#	endif // (CONFIG_OSMESA == 1)

#	if(CONFIG_GLX == 1)
	struct 
	{
		GLXContext ctx;
		GLXFBConfig *config;
	} glx;
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_COCOA == 1)
	struct
	{
		CGLContextObj ctx;
		void *nsctx; // NSOpenGLContext*
		int pbuffer_mode;
	} cocoa;
#	endif // (CONFIG_COCOA == 1)

#	if(CONFIG_WGL == 1)
	struct
	{
		HGLRC ctx;
		HDC   haxdc;
		HPBUFFERARB haxbuf;
		int haxown;
		int config;
	} wgl;
#	endif // (CONFIG_WGL == 1)
} DEGLContext;

#endif // CONTEXT_H_
