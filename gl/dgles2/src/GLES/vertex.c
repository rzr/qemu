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

#include "hgl.h"
#include "common-vertex.h"
#include "common-vertex.c"

void dglVertexInit(void)
{
	degl_tls* tls = deglGetTLS();

	tls->max_vertex_arrays = 5;
	Dprintf("Maximum number of vertex arrays: %d.\n", tls->max_vertex_arrays);

	tls->vertex_arrays = malloc(tls->max_vertex_arrays * sizeof(DGLVertexArray));
	for(int i = 0; i < tls->max_vertex_arrays; ++i)
	{
		DGLVertexArray*const va = tls->vertex_arrays + i;
		va->type = 0;
		va->enabled = 0;
		va->floatptr = 0;
		va->buffer = 0;
	}
}

static int dglGetArrayIndex(GLenum array)
{
	int index;
	switch(array)
	{
		case GL_COLOR_ARRAY:          index = 0; break;
		case GL_NORMAL_ARRAY:         index = 1; break;
		case GL_POINT_SIZE_ARRAY_OES: index = 2; break;
		case GL_TEXTURE_COORD_ARRAY:  index = 3; break;
		case GL_VERTEX_ARRAY:         index = 4; break;
		default:                      index = -1; break;
	}
#   if(DEBUG_VERTEX == 1)
	Dprintf("Array 0x%x has index %d.\n", array, index);
#   endif
	return index;
}

static void dglArrayPointer(GLint size, GLenum type, GLboolean normalized,
	GLsizei stride, const GLvoid *pointer, GLenum array, int write)
{
	(void)normalized;
#   if(DEBUG_VERTEX == 1)
	Dprintf("Array %d at %p (%d elements every %d bytes)\n",array, pointer, size, stride);
#   endif
	int indx = dglGetArrayIndex(array);
	DGLVertexArray *va = deglGetTLS()->vertex_arrays + indx;

	GLint buffer;
	HGL_TLS.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);

	if(write) va->type = type;
	if(type == GL_FIXED || type == GL_BYTE || array == GL_POINT_SIZE_ARRAY_OES)
	{
		if(write)
		{
			va->array = array;
			va->size = size;
			va->stride = stride;
			va->ptr = pointer;
			va->buffer = buffer;
		}
		//if a vbo is binded and its already converted then array needs to be binded
		if(buffer)
		{
			degl_vbo* vbo = deglFindVBO(va->buffer);
			if(vbo)
			{
				//type and stride need to be converted
				int from =(vbo->from==GL_FIXED)?sizeof(GLfixed):sizeof(GLbyte);
				int to = sizeof(GLfloat);
				GLsizei newstride = stride*to/from;
				switch(array)
				{
					case GL_COLOR_ARRAY:
						HGL_TLS.ColorPointer(size, GL_FLOAT, newstride, pointer);
						break;
					case GL_NORMAL_ARRAY:
						HGL_TLS.NormalPointer(GL_FLOAT, newstride, pointer);
						break;
					case GL_TEXTURE_COORD_ARRAY:
						HGL_TLS.TexCoordPointer(size, GL_FLOAT, newstride, pointer);
						break;
					case GL_VERTEX_ARRAY:
						HGL_TLS.VertexPointer(size, GL_FLOAT, newstride, pointer);
						break;
					default:
						break;
				}
			}
		}

	}
	else
	{
		switch(array)
		{
			case GL_COLOR_ARRAY:
				HGL_TLS.ColorPointer(size, type, stride, pointer);
				break;
			case GL_NORMAL_ARRAY:
				HGL_TLS.NormalPointer(type, stride, pointer);
				break;
			case GL_TEXTURE_COORD_ARRAY:
				HGL_TLS.TexCoordPointer(size, type, stride, pointer);
				break;
			case GL_VERTEX_ARRAY:
				HGL_TLS.VertexPointer(size, type, stride, pointer);
				break;
			default:
				break;
		}
		GLenum error;
		if((error = HGL_TLS.GetError()) != GL_NO_ERROR)
		{
			Dprintf("glXXXPointer, XXX=0x%x (%d, GL_FLOAT, 0, %p\n)"
					" errored 0x%x!\n", array, size, pointer, error);
		}
	}
}

GL_APICALL_BUILD void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	dglArrayPointer(size, type, 0, stride, pointer, GL_VERTEX_ARRAY, 1);
}

GL_APICALL_BUILD void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	dglArrayPointer(size, type, 0, stride, pointer, GL_TEXTURE_COORD_ARRAY, 1);
}

GL_APICALL_BUILD void GL_APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	dglArrayPointer(size, type, 0, stride, pointer, GL_COLOR_ARRAY, 1);
}

GL_APICALL_BUILD void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	dglArrayPointer(0, type, 0, stride, pointer, GL_NORMAL_ARRAY, 1);
}

GL_APICALL_BUILD void GL_APIENTRY glPointSizePointerOES (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	dglArrayPointer(0, type, 0, stride, pointer, GL_POINT_SIZE_ARRAY_OES, 1);
}

GL_APICALL_BUILD void GL_APIENTRY glDisableClientState (GLenum array)
{
	int index;
	if((index = dglGetArrayIndex(array)) < 0)
	{
		Dprintf("Invalid array\n");
	}
	else
	{
		DGLVertexArray *v = deglGetTLS()->vertex_arrays;
		v[index].enabled = 0;
	}
	HGL_TLS.DisableClientState(array);
}

GL_APICALL_BUILD void GL_APIENTRY glEnableClientState (GLenum array)
{
	int index;
	if((index = dglGetArrayIndex(array)) < 0)
	{
		Dprintf("Invalid array\n");
	}
	else
	{
		DGLVertexArray *v = deglGetTLS()->vertex_arrays;
		v[index].enabled = 1;
	}
	HGL_TLS.EnableClientState(array);
}

GL_APICALL_BUILD void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	dglConvertFixedAndByteArrays(first, count, 1);
	DGLVertexArray *arrays = deglGetTLS()->vertex_arrays;
	if(arrays[dglGetArrayIndex(GL_POINT_SIZE_ARRAY_OES)].enabled)
	{
		STUB("support for point size arrays not implemented!\n");
		HGL_TLS.DrawArrays(mode, first, count);
	}
	else
	{
		HGL_TLS.DrawArrays(mode, first, count);
	}
}

GL_APICALL_BUILD void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
	GLushort min, max;
	dglGetMinMaxIndices(count, type, indices, &min, &max);
	dglConvertFixedAndByteArrays(min, max - min + 1, 1);

	degl_tls* tls = deglGetTLS();
	if(tls->vertex_arrays[dglGetArrayIndex(GL_POINT_SIZE_ARRAY_OES)].enabled)
	{
		STUB("support for point size arrays not implemented!\n");
		HGL_TLS.DrawElements(mode, count, type, indices);
	}
	else
	{
		HGL_TLS.DrawElements(mode, count, type, indices);
	}
}
