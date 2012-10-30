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

#ifndef COMMON_VERTEX_H_
#define COMMON_VERTEX_H_

#if(defined BUILD_GLES2)
#	include "hgl2api.h"
#else
#	include "hglapi.h"
#endif

// set to 1 to enable extra vertex debug info
// configure with --enable-debug is also needed
#define DEBUG_VERTEX 0

// set to 0 to make no local copies of element array buffers
// -- potentially slower drawing, but does not need to allocate extra memory
// set to 1 to make local copies of element array buffers
// -- faster drawing, but all related buffer objects are cached in process heap

//
// Currently broken, use "1"
//
#define CACHE_ELEMENT_BUFFERS 1

// GLES Vertex array data holder.
typedef struct DGLVertexArray
{
	GLenum array;         // Which array (e.g. GL_COLOR_ARRAY).
	GLint size;           // Function call parameter.
	GLboolean normalized; // --''--
	GLenum type;          // --''--
	GLsizei stride;       // --''--
	const void* ptr;      // --''--
	GLboolean enabled;    // State.
	GLfloat* floatptr;    // Buffer for fixed->float conversion.
	GLuint buffer;         //binded vbo
} DGLVertexArray;

extern void dglVertexInit(void);
extern void dglVertexFinish(void);

#endif // COMMON_VERTEX_H_
