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

	HGL_TLS.GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tls->max_vertex_arrays);
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

GL_APICALL_BUILD void GL_APIENTRY glDisableVertexAttribArray(GLuint index)
{
	degl_tls *tls = deglGetTLS();
#	if(DEBUG_VERTEX == 1)
	Dprintf("Disabling array %d\n", index);
#	endif
	if(index < (unsigned)tls->max_vertex_arrays)
	{
		DGLVertexArray* va = tls->vertex_arrays + index;
	    va->enabled = 0;
	}
	HGL_TLS.DisableVertexAttribArray(index);
}

static void dglArrayPointer(GLint size, GLenum type, GLboolean normalized,
	GLsizei stride, const GLvoid *pointer, GLenum array, int write)
{
	int indx = (int)array;
	HGL_TLS.VertexAttribPointer(indx, size, type, normalized, stride, pointer);
	GLenum error;
	if((error = HGL_TLS.GetError()) != GL_NO_ERROR)
	{
		Dprintf("glVertexAttribPointer(%d, %d, GL_FLOAT, 0, %d, %p\n) errored 0x%x!\n",
			indx, size, normalized, pointer, error);
	}
}

GL_APICALL_BUILD void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	dglConvertFixedAndByteArrays(first, count, 0);
	HGL_TLS.DrawArrays(mode, first, count);
}

GL_APICALL_BUILD void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
	GLushort min, max;
	dglGetMinMaxIndices(count, type, indices, &min, &max);
	dglConvertFixedAndByteArrays(min, max - min + 1, 0);
	HGL_TLS.DrawElements(mode, count, type, indices);
}

GL_APICALL_BUILD void GL_APIENTRY glEnableVertexAttribArray(GLuint index)
{
#	if(DEBUG_VERTEX == 1)
	Dprintf("Enabling array %d\n", index);
#	endif
	degl_tls *tls = deglGetTLS();
	DGLVertexArray *arrays = tls->vertex_arrays;
	if (index < (unsigned)tls->max_vertex_arrays) {
		arrays[index].enabled = 1;
	}
	HGL_TLS.EnableVertexAttribArray(index);
}

GL_APICALL_BUILD void GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
	HGL_TLS.GetVertexAttribfv(index, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
	HGL_TLS.GetVertexAttribiv(index, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
	degl_tls *tls = deglGetTLS();
	DGLVertexArray *arrays = tls->vertex_arrays;

	if ((index < (unsigned)tls->max_vertex_arrays) &&
		(arrays[index].type == GL_FIXED))
	{
		*pointer = (void *)arrays[index].ptr;
	}
	else
	{
		HGL_TLS.GetVertexAttribPointerv(index, pname, pointer);
	}
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib1f(GLuint indx, GLfloat x)
{
	HGL_TLS.VertexAttrib1f(indx, x);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
	HGL_TLS.VertexAttrib1fv(indx, values);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
	HGL_TLS.VertexAttrib2f(indx, x, y);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
	HGL_TLS.VertexAttrib2fv(indx, values);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
	HGL_TLS.VertexAttrib3f(indx, x, y, z);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
	HGL_TLS.VertexAttrib3fv(indx, values);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	HGL_TLS.VertexAttrib4f(indx, x, y, z, w);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
	HGL_TLS.VertexAttrib4fv(indx, values);
}

GL_APICALL_BUILD void GL_APIENTRY glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
#	if(DEBUG_VERTEX == 1)
	Dprintf("Array %d at %p (%d elements every %d bytes)\n", indx, ptr, size, stride);
#	endif

	degl_tls *tls = deglGetTLS();
	DGLVertexArray *arrays = tls->vertex_arrays;
	GLint buffer;
	HGL_TLS.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);

	if (indx < (unsigned)tls->max_vertex_arrays) {
		arrays[indx].type = type;
	}

	if((indx < (unsigned)tls->max_vertex_arrays) && (type == GL_FIXED))
	{
		arrays[indx].array = indx;
		arrays[indx].size = size;
		arrays[indx].normalized = normalized;
		arrays[indx].stride = stride;
		arrays[indx].ptr = ptr;
		arrays[indx].buffer = buffer;
		//if a vbo is binded and its already converted then array needs to be binded
		if(buffer)
		{
			degl_vbo* vbo = deglFindVBO(buffer);
			if(vbo)
			{
				//stride would need to be converted but since fixed and float are the same size it doesn't matter
				HGL_TLS.VertexAttribPointer(indx, size, GL_FLOAT, normalized, stride, ptr);
			}
		}
	}
	else
	{
		HGL_TLS.VertexAttribPointer(indx, size, type, normalized, stride, ptr);
	}
}
