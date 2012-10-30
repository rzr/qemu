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

#ifndef HOSMESA_H_
#define HOSMESA_H_

#include "common.h"
#if(CONFIG_GLX == 1)
#	include <X11/Xlib.h>
#endif // (CONFIG_GLX == 1)
#include "GL/osmesa.h"

#define HOSMESA_FUNCS \
	HOSMESA_FUNC(OSMesaContext, CreateContextExt, (GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)) \
	HOSMESA_FUNC(GLboolean, MakeCurrent, (OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height)) \
	HOSMESA_FUNC(void, PixelStore, (GLint pname, GLint value)) \
	HOSMESA_FUNC(OSMesaContext, GetCurrentContext, (void)) \
	HOSMESA_FUNC(void, DestroyContext, (OSMesaContext ctx))

#define HOSMESA_FUNC(ret, name, attr) typedef ret (GL_APIENTRY *HOSMesa##name##_t) attr;
	HOSMESA_FUNCS
#undef HOSMESA_FUNC

typedef struct HOSMesa
{
#define HOSMESA_FUNC(ret, name, attr) HOSMesa##name##_t name;
	HOSMESA_FUNCS
#undef HOSMESA_FUNC
} HOSMesa;

extern HOSMesa hOSMesa;

extern int hOSMesaLoad();

#endif // HOSMESA_H_
