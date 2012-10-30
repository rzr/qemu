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

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "common.h"
#if(CONFIG_OSMESA == 1)
#	include "hosmesa.h"
#endif // (CONFIG_OSMESA == 1)
#if(CONFIG_GLX == 1)
#	include "hglx.h"
#endif // (CONFIG_GLX == 1)
#if(CONFIG_WGL == 1)
#	include "hwgl.h"
#endif // (CONFIG_WGL == 1)
#if(CONFIG_COCOA == 1)
#	include "hcocoa.h"
#endif // (CONFIG_COCOA == 1)

#include "config.h"

// EGL display.
typedef struct DEGLDisplay
{
	int initialized;
	int major, minor; // backend version info
	int config_count;

#	if(CONFIG_GLX == 1)
	Display *dpy;
	Screen *scr;
	DEGLX11Config *x11_config;
#	endif // CONFIG_GLX == 1

#	if(CONFIG_COCOA == 1)
	CGDirectDisplayID id;
	DEGLCocoaConfig *cocoa_config;
#	endif // (CONFIG_COCOA == 1)
} DEGLDisplay;

#endif // DISPLAY_H_
