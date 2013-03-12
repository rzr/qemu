/* Copyright (C) 2011  Nokia Corporation All Rights Reserved.
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

#ifndef HCOCOA_H_
#define HCOCOA_H_

#include "common.h"

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>

#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT          0x84F5
#endif

#define HCOCOA_FUNCS \


typedef struct HCocoa
{
#define HCOCOA_FUNC(ret, name, attr) ret (*name) attr;
	HCOCOA_FUNCS
#undef HCOCOA_FUNC
	void *(*CreateContext)(CGLContextObj ctx);
	void (*DestroyContext)(void *nsctx);
	void *(*CreateView)(EGLSurface s, void *nsview, unsigned *w, unsigned *h);
	void (*DestroyView)(void *cview);
	void (*SetView)(void *nsctx, void *cview);
	void (*BindTexImageFromView)(void *nsctx, void *cview);
} HCocoa;

extern HCocoa hcocoa;

extern int hcocoaLoad(void);

#endif // HCOCOA_H_
