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

#ifndef CONFIG_H_
#define CONFIG_H_

#include "common.h"

typedef struct DEGLConfig
{
	EGLint buffer_bits;
	EGLint red_bits;
	EGLint green_bits;
	EGLint blue_bits;
	// luminance
	EGLint alpha_bits;
	// alpha mask
	// bind to texture rgb
	// bind to texture rgba
	// color buffer type
	EGLint caveat;
	EGLint id;
	// conformant
	EGLint depth_bits;
	EGLint level;
	// max swap interval
	// min swap interval
	EGLint native_renderable;
	// native visual type
	// renderable type
	EGLint sample_buffers;
	EGLint samples;
	EGLint stencil_bits;
	EGLint surface_type;
	EGLint transparency_type;
	EGLint transparent_red;
	EGLint transparent_green;
	EGLint transparent_blue;
} DEGLConfig;

#if(CONFIG_OFFSCREEN == 1)
typedef struct DEGLOffscreenConfig
{
	DEGLConfig config;
	EGLint invert_y;
} DEGLOffscreenConfig;
#endif // CONFIG_OFFSCREEN == 1

#if(CONFIG_GLX == 1)
#include "hglx.h"
typedef struct DEGLX11Config
{
	DEGLConfig config;
	XID visual_type;
	VisualID visual_id;
	GLXFBConfig *fb_config;
} DEGLX11Config;
#endif // CONFIG_GLX == 1

#if(CONFIG_COCOA == 1)
#include "hcocoa.h"
typedef struct DEGLCocoaConfig
{
	DEGLConfig config;
	GLint renderer_id;
	CGLPixelFormatObj pf;
} DEGLCocoaConfig;
#endif // CONFIG_COCOA == 1

#endif // CONFIG_H_
