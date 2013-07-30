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

#ifndef HGLX_H_
#define HGLX_H_

#include <GL/glx.h>

#define HGLX_FUNCS \
	HGLX_FUNC(GLXFBConfig*, ChooseFBConfig, (Display *dpy, int screen, const int *attribList, int *nitems)) \
	HGLX_FUNC(int, GetFBConfigAttrib, (Display *dpy, GLXFBConfig config, int attribute, int *value)) \
	HGLX_FUNC(GLXFBConfig*, GetFBConfigs, (Display *dpy, int screen, int *nelements)) \
	HGLX_FUNC(GLXWindow, CreateWindow, (Display *dpy, GLXFBConfig config, Window win, const int *attribList)) \
	HGLX_FUNC(void, DestroyWindow, (Display *dpy, GLXWindow win)) \
	HGLX_FUNC(GLXContext, CreateNewContext, (Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct)) \
	HGLX_FUNC(void, DestroyContext, (Display *dpy, GLXContext ctx)) \
	HGLX_FUNC(Bool, MakeContextCurrent, (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)) \
	HGLX_FUNC(void, SwapBuffers, (Display *dpy, GLXDrawable drawable)) \
	HGLX_FUNC(int, QueryDrawable, (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)) \
	HGLX_FUNC(const char*, QueryServerString, (Display *dpy, int screen, int name)) \
	HGLX_FUNC(const char*, GetClientString, (Display *dpy, int name)) \
	HGLX_FUNC(Bool, QueryExtension, (Display *dpy, int *error_base, int *event_base)) \
	HGLX_FUNC(Bool, QueryVersion, (Display *dpy, int *major, int *minor)) \
	HGLX_FUNC(GLXPbuffer, CreatePbuffer, (Display* dpy, GLXFBConfig config, const int* attrib_list)) \
	HGLX_FUNC(void, DestroyPbuffer, (Display* dpy, GLXPbuffer pbuf)) \
	HGLX_FUNC(GLXPixmap, CreatePixmap, (Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list)) \
	HGLX_FUNC(void, DestroyPixmap, (Display *dpy, GLXPixmap pixmap)) \
	HGLX_FUNC(XVisualInfo *, GetVisualFromFBConfig, (Display *dpy, GLXFBConfig config)) \
	HGLX_FUNC(void, WaitGL, (void)) \
	HGLX_FUNC(void, WaitX, (void)) \
	HGLX_FUNC(void, BindTexImageEXT, (Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list)) \
	HGLX_FUNC(void, ReleaseTexImageEXT, (Display *dpy, GLXDrawable drawable, int buffer))

typedef struct HGLX
{
#define HGLX_FUNC(ret, name, attr) ret (*name) attr;
	HGLX_FUNCS
#undef HGLX_FUNC
} HGLX;

extern HGLX hglX;

extern int hglXLoad();

#endif // HGLX_H_
