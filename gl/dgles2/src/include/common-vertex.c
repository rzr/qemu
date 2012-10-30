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

static void dglGetMinMaxIndices(GLsizei count, GLenum type,
	const void *indices, GLushort *min, GLushort *max)
{
	*min = (GLushort)-1;
	*max = 0;

	GLsizei esize;
	switch(type)
	{
		case GL_UNSIGNED_BYTE: esize = sizeof(GLubyte); break;
		case GL_UNSIGNED_SHORT: esize = sizeof(GLushort); break;
		default: Dprintf("ERROR: Invalid type %d!\n", type); return;
	}

	degl_tls *tls = deglGetTLS();
	DEGLContext *current_context = tls->current_context;
	degl_ebo *ebo_bound = (current_context ? current_context->ebo_bound : 0);
	if(ebo_bound)
	{
#		if(CACHE_ELEMENT_BUFFERS == 1)
		if(ebo_bound->cache)
		{
			if(count * esize + (intptr_t)indices > ebo_bound->size)
			{
				Dprintf("element array buffer index out of bounds!"
						" (count=%d, offset=%d, buffer size=%d)\n",
						(int)count, (int)(intptr_t)indices, ebo_bound->size);
				count = (ebo_bound->size - (intptr_t)indices) / esize;
			}
			indices += (intptr_t)(ebo_bound->cache);
		}
		else
		{
			Dprintf("no cache available for element array buffer\n");
			return;
		}
#		else // CACHE_ELEMENT_BUFFERS == 1
		const void *p = HGL_TLS.MapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
		if(!p)
		{
			Dprintf("failed to map element array buffer, error 0x%x\n",
				HGL_TLS.GetError());
			return;
		}
		indices += (intptr_t)p;
#		endif // CACHE_ELEMENT_BUFFERS
	}

	GLsizei i;
	for(i = 0; i < count; ++i)
	{
		switch(esize)
		{
			case sizeof(GLubyte):
			{
				GLubyte const value = ((const GLubyte*)indices)[i];
				if(value > *max) *max = value;
				if(value < *min) *min = value;
				break;
			}
			case sizeof(GLushort):
			{
				GLushort const value = ((const GLushort*)indices)[i];
				if(value > *max) *max = value;
				if(value < *min) *min = value;
				break;
			}
		}
	}
#	if(DEBUG_VERTEX == 1)
	Dprintf("%d indices, min = %u, max = %u\n", count, *min, *max);
#	endif // DEBUG_VERTEX == 1

#	if(CACHE_ELEMENT_BUFFERS == 0)
	if(tls->ebo.bound)
	{
		HGL_TLS.UnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}
#	endif // CACHE_ELEMENT_BUFFERS == 0
}

static void dglArrayPointer(GLint size, GLenum type, GLboolean normalized,
	GLsizei stride, const GLvoid *pointer, GLenum array, int write);

int dglConvertData(int count, int size, int stride, int first, GLenum from, GLenum to, const void *in, void *out)
{
	if(to != GL_FLOAT)
	{
		Dprintf("Only conversion to float is currently supported!\n");
		return 0;
	}

	for(GLsizei j = 0; j < count; ++j)
	{
		for(signed k = 0; k < size; ++k)
		{
			GLfloat value;
			if(from == GL_FIXED)
				value = ((GLfixed*)(((char*)in) + stride*(first + j)))[k] / 65536.0f;
			else
				value = (GLfloat)((GLbyte*)(((char*)in) + stride*(first + j)))[k];
			((float*)out)[j*size + k] = value;
#						if(DEBUG_VERTEX == 1)
			Dprintf("Got %f\n", value);
#						endif
		}
	}
	return 1;
}

// Function to convert fixed/byte arrays to float arrays.
static void dglConvertFixedAndByteArrays(GLint first, GLsizei count, int isgles1)
{
	degl_tls* tls = deglGetTLS();
	for(int i = 0; i < tls->max_vertex_arrays; ++i)
	{
		DGLVertexArray*const va = tls->vertex_arrays + i;
#		if(DEBUG_VERTEX == 1)
		Dprintf("#%d: enabled=%d, type=0x%04x\n", i, va->enabled, va->type);
#		endif
		//Byte conversion is only needed for ES1.1
		if(va->enabled &&
			(va->type == GL_FIXED || (va->type == GL_BYTE && isgles1)) &&
			(!va->buffer || (va->buffer && !deglFindVBO(va->buffer))))
		{
			GLint temp_size, temp_first, offset=0;
			GLsizei temp_stride, temp_count;
			GLenum error;
			GLuint esize;
			const void* temp_ptr = NULL;
			if(va->type == GL_FIXED)
				esize = sizeof(GLfixed);
			else
				esize = sizeof(GLbyte);

			if(va->buffer)
			{
				HGL_TLS.BindBuffer(GL_ARRAY_BUFFER, va->buffer);
				GLint buffSize;
				glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffSize);
				//temporarily change params so all buffer data is converted
				temp_size = 1;
				temp_first = 0;
				temp_count = buffSize/esize;
				temp_ptr = HGL_TLS.MapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
				temp_stride = esize;
			}
			else
			{
				temp_size = va->size;
				temp_first = first;
				temp_count = count;
				temp_ptr = va->ptr;
				temp_stride = va->stride;

				if(!temp_stride)
				{
					temp_stride = temp_size*esize;
				}
			}
#			if(DEBUG_VERTEX == 1)
			Dprintf("Converting %d %s indices to float.\n", count, va->type == GL_FIXED? "fixed" : "byte");
#			endif

			if(va->floatptr)
			{
				free(va->floatptr);
				va->floatptr = NULL;
			}
			if(temp_ptr)
			{
				va->floatptr = malloc(sizeof(GLfloat)*temp_count*temp_size);
				dglConvertData(temp_count, temp_size, temp_stride, temp_first, va->type, GL_FLOAT, temp_ptr, (void*)va->floatptr);
			}

			if(va->buffer)
			{
				temp_stride = va->stride*sizeof(GLfloat)/esize;
				HGL_TLS.UnmapBuffer(GL_ARRAY_BUFFER);
				HGL_TLS.BufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*temp_count*temp_size, va->floatptr, GL_STATIC_DRAW);
				//Add to list of converted buffers
				deglAddVBOToList(va->buffer, va->type);
				if(va->floatptr)
					free(va->floatptr);

				va->floatptr = (float*)va->ptr;
				offset = 0;
			}
			else
			{
				temp_stride*=sizeof(GLfloat)/esize;
				offset = first*temp_stride;
			}
#			if(DEBUG_VERTEX == 1)
			Dprintf("Binding from %d\n", first);
#			endif
			dglArrayPointer(va->size, GL_FLOAT, va->normalized, temp_stride,
				(GLfloat*)((void*)va->floatptr - offset), va->array, 0);
		}
	}
}
