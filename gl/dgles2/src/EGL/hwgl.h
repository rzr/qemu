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

#ifndef HWGL_H_
#define HWGL_H_

#include "common.h"
#if(defined BUILD_GLES2)
#	include "hgl2api.h"
#else
#	include "hglapi.h"
#endif
#include <GL/wglext.h>
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif // !def WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <pthread.h>

#define HWGL_FUNCS \
	HWGL_FUNC(PROC, GetProcAddress, (LPCSTR lpszProc)) \
	HWGL_FUNC(HGLRC, CreateContext, (HDC hDC)) \
	HWGL_FUNC(BOOL, MakeCurrent, (HDC hDC, HGLRC hRC)) \
	HWGL_FUNC(BOOL, DeleteContext, (HGLRC hRC)) \
	HWGL_FUNC(HGLRC, GetCurrentContext, (void)) \
	HWGL_FUNC(HDC, GetCurrentDC, (void)) \
	HWGL_FUNC(BOOL, ShareLists, (HGLRC hglrc1, HGLRC hglrc2))

#define HWGL_MANDATORY_EXT_FUNCS \
	HWGL_FUNC(HPBUFFERARB, CreatePbufferARB, (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)) \
	HWGL_FUNC(HDC, GetPbufferDCARB, (HPBUFFERARB hPbuffer)) \
	HWGL_FUNC(int, ReleasePbufferDCARB, (HPBUFFERARB hPbuffer, HDC hDC)) \
	HWGL_FUNC(BOOL, DestroyPbufferARB, (HPBUFFERARB hPbuffer)) \

#define HWGL_OPTIONAL_EXT_FUNCS \
	HWGL_FUNC(BOOL, GetPixelFormatAttribivARB, (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues)) \
	HWGL_FUNC(BOOL, GetPixelFormatAttribfvARB, (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues)) \
	HWGL_FUNC(BOOL, ChoosePixelFormatARB, (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)) \
	HWGL_FUNC(BOOL, QueryPbufferARB, (HPBUFFERARB hPbuffer, int iAttribute, int *piValue))

typedef struct HWGL
{
#define HWGL_FUNC(ret, name, attr) ret (WINAPI *name) attr;
	HWGL_FUNCS
	HWGL_MANDATORY_EXT_FUNCS
	HWGL_OPTIONAL_EXT_FUNCS
#undef HWGL_FUNC
	struct 
	{
		WNDCLASSEX haxclass;
		HDC        haxdc;
		HWND       haxwnd;
		HGLRC      haxctx;
		PIXELFORMATDESCRIPTOR haxpfd;
		int        haxpf;
	} window;
	struct
	{
		HPBUFFERARB haxbuf;
		HDC         haxdc;
		HGLRC       haxctx;
		PIXELFORMATDESCRIPTOR haxpfd;
		int haxpf;
	} pbuffer;
	pthread_mutex_t haxlock;
} HWGL;

extern HWGL hwgl;

extern int hwglLoad();

#endif // HWGL_H_
