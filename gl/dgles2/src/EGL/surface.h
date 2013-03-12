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

#ifndef SURFACE_H_
#define SURFACE_H_

#include "common.h"
#if(CONFIG_OSMESA == 1)
#	include "GL/osmesa.h"
#endif // (CONFIG_OSMESA == 1)
#if(CONFIG_GLX == 1)
#	include "GL/glx.h"
#endif // (CONFIG_GLX == 1)
#if(CONFIG_COCOA == 1)
#	include "hcocoa.h"
#endif // (CONFIG_COCOA == 1)
#if(CONFIG_WGL == 1)
#	include "hwgl.h"
#endif // (CONFIG_WGL == 1)
#if(CONFIG_OFFSCREEN == 1)
#	include "EGL/eglext.h"
#endif // CONFIG_OFFSCREEN == 1
#include "config.h"

typedef struct DEGLSurface
{
	int bound;
	int destroy;
	enum
	{
		DEGLSurface_invalid = 0,
		DEGLSurface_window,
		DEGLSurface_pixmap,
		DEGLSurface_pbuffer,
#		if(CONFIG_OFFSCREEN == 1)
		DEGLSurface_offscreen,
#		endif // CONFIG_OFFSCREEN == 1
	} type;
	unsigned width;  /* width in pixels */
	unsigned height; /* height in pixels */
	unsigned depth;  /* color bit depth */
	unsigned bpp;    /* bytes per pixel */
	DEGLConfig *config;

#	if(CONFIG_OFFSCREEN == 1)
	DEGLDrawable* ddraw; /* Offscreen drawable handle. */
	GLuint pbo;
#	endif // (CONFIG_OFFSCREEN == 1)

#	if(CONFIG_GLX == 1)
	struct
	{
		GLXDrawable drawable; // GLX Drawable (pbuffer)
		GLXFBConfig *config; // GLX Framebuffer config.
		Drawable x_drawable; // Window/Pixmap, zero for pbuffers
	} glx;
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_COCOA == 1)
	struct
	{
		CGLPBufferObj pbuffer;
		void *nsview; // NSView*
	} cocoa;
#	endif // (CONFIG_COCOA == 1)

#	if(CONFIG_WGL == 1)
	struct
	{
		HPBUFFERARB pbuffer;
		HDC dc;
		int format;
	} wgl;
#	endif // (CONFIG_WGL == 1)
} DEGLSurface;

void deglRefSurface(DEGLSurface* surface);
void deglUnrefSurface(EGLDisplay dpy, DEGLSurface** surface);

#endif // SURFACE_H_
