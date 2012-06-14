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

void dglVertexFinish(void)
{
	degl_tls *tls = deglGetTLS();
	DGLVertexArray *v = tls->vertex_arrays;
	if(v)
	{
		for(int i = 0; i < tls->max_vertex_arrays; i++)
		{
			if(v[i].floatptr)
			{
				free(v[i].floatptr);
				v[i].floatptr = NULL;
			}
		}
		free(v);
	}
}

GL_APICALL_BUILD void dglDeletePBO(GLuint pbo)
{
	HGL_TLS.DeleteBuffers(1, &pbo);
}

GL_APICALL_BUILD void dglBindTexImage(unsigned width, unsigned height, char* pixels)
{
    HGL *h = &HGL_TLS;
	if(pixels)
	{
		h->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height,
			0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
	}
	else
	{
		GLuint saved_pack_buffer = 0, buffer = 0;
		GLint saved_pack_size = 0, saved_pack_usage = 0;
		h->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, (GLint *)&saved_pack_buffer);
		if(saved_pack_buffer)
		{
			h->GetBufferParameteriv(saved_pack_buffer, GL_BUFFER_SIZE, &saved_pack_size);
			h->GetBufferParameteriv(saved_pack_buffer, GL_BUFFER_USAGE, &saved_pack_usage);
		}
		h->GenBuffers(1, &buffer);
		h->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, buffer);
		h->BufferData(GL_PIXEL_PACK_BUFFER_ARB, width * height * 4, 0, GL_STREAM_COPY);
		h->PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
		h->PixelStorei(GL_PACK_ALIGNMENT, 4);
		h->ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		h->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, saved_pack_buffer);
		if(saved_pack_buffer) h->BufferData(GL_PIXEL_PACK_BUFFER_ARB, saved_pack_size, 0, saved_pack_usage);
		h->BindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, buffer);
		h->PixelStorei(GL_UNPACK_ALIGNMENT, 4);
		h->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		h->BindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		h->PopClientAttrib();
		h->DeleteBuffers(1, &buffer);
	}
}

/* internal_flags:
 * b0 ... if 0, other bits must be zero and this is called from glReadPixels
 *        if 1, other bits have valid values and this is called from eglSwapBuffers
 * b1 ... surface has been resized (false/true)
 */
GL_APICALL_BUILD void dglReadPixels(void *pixels_, GLuint *pbo,
                                    GLint x, GLint y, GLint width, GLint height,
                                    GLenum format, GLenum type, GLuint internal_flags)
{
	HGL *h = &HGL_TLS;
	GLuint current_pbo = 0;
	if(!internal_flags && !pbo)
	{
		h->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, (GLint *)&current_pbo);
		pbo = &current_pbo;
	}

	unsigned bpp = 1;
	switch(format)
	{
		case GL_RGB:
			bpp = (type == GL_UNSIGNED_BYTE) ? 3 : 2;
			break;
		case GL_RGBA:
		case GL_BGRA:
			bpp = (type == GL_UNSIGNED_BYTE) ? 4 : 2;
			break;
		case GL_LUMINANCE_ALPHA:
			bpp = 2;
			break;
		case GL_ALPHA:
		case GL_LUMINANCE:
		default:
			break;
	}

	if(internal_flags && pbo)
	{
		if(!*pbo)
		{
			h->GenBuffers(1, pbo);
			if(*pbo)
			{
#				if(CONFIG_OFFSCREEN == 1)
				deglAddReservedBuffer(*pbo);
#				endif // CONFIG_OFFSCREEN == 1
				h->BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, *pbo);
				internal_flags |= 2;
			}
		}
        if(internal_flags & 2) {
			h->BufferData(GL_PIXEL_PACK_BUFFER_ARB,
				width * height * bpp, 0, GL_STREAM_READ);
		}
	}

    const unsigned linew = width * bpp;
	if(internal_flags || (pbo && *pbo))
	{
		h->PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
		h->PixelStorei(GL_PACK_ALIGNMENT, (bpp == 4) ? 4 : 1);
	}
	if(pbo && *pbo)
	{
		h->ReadPixels(x, y, width, height, format, type, 0 + 0);
		void *pixels = h->MapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
		h->Flush();
		pixels_ += (height - 1) * linew;
		for(unsigned i = height; i--;)
		{
			memcpy(pixels_, pixels, linew);
			pixels_ -= linew;
			pixels += linew;
		}
		h->UnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
	}
	else
	{
		h->ReadPixels(x, y, width, height, format, type, pixels_);
		if(internal_flags)
		{
			void *line = malloc(linew);
			for(EGLint i = 0, j = height - 1; i < height / 2; ++i, --j)
			{
				memcpy(line, pixels_ + i*linew, linew);
				memcpy(pixels_ + i*linew, pixels_ + j*linew, linew);
				memcpy(pixels_ + j*linew, line, linew);
			}
			free(line);
		}
	}
	if(internal_flags || (pbo && *pbo))
	{
		h->PopClientAttrib();
	}
}
