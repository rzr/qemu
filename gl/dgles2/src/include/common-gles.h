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

// Clear functions
GL_APICALL_BUILD void GL_APIENTRY glClear(GLbitfield mask)
{
	HGL_TLS.Clear(mask);
}

GL_APICALL_BUILD void GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	HGL_TLS.ClearColor(red, green, blue, alpha);
}

// Depth functions
GL_APICALL_BUILD void GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
	HGL_TLS.DepthRange(zNear, zFar);
}

GL_APICALL_BUILD void GL_APIENTRY glClearDepthf(GLclampf depth)
{
	HGL_TLS.ClearDepth(depth);
}

GL_APICALL_BUILD void GL_APIENTRY glDepthFunc(GLenum func)
{
	HGL_TLS.DepthFunc(func);
}

GL_APICALL_BUILD void GL_APIENTRY glDepthMask(GLboolean flag)
{
	HGL_TLS.DepthMask(flag);
}

// Stencil functions
GL_APICALL_BUILD void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	HGL_TLS.StencilFunc(func, ref, mask);
}
GL_APICALL_BUILD void GL_APIENTRY glStencilMask(GLuint mask)
{
	HGL_TLS.StencilMask(mask);
}
GL_APICALL_BUILD void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	HGL_TLS.StencilOp(fail, zfail, zpass);
}
GL_APICALL_BUILD void GL_APIENTRY glClearStencil(GLint s)
{
	HGL_TLS.ClearStencil(s);
}

// Buffer functions
GL_APICALL_BUILD void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
	degl_tls *tls = deglGetTLS();
	DEGLContext *current_context = tls->current_context;
	degl_ebo *ebo_bound = (current_context ? current_context->ebo_bound : 0);
#	if(CONFIG_OFFSCREEN == 1)
	if(deglIsReservedBuffer(buffer))
	{
		GLsizei reserved_size = 0;
		GLuint reserved_buffer = 0;
		HGL_TLS.GetBufferParameteriv(GL_PIXEL_PACK_BUFFER_ARB, GL_BUFFER_SIZE, &reserved_size);
		HGL_TLS.GenBuffers(1, &reserved_buffer);
		if(!reserved_buffer)
		{
			Dprintf("unable to generate a new buffer object for reserved buffer %u\n", buffer);
			tls->glError = GL_INVALID_VALUE;
			return;
		}
		HGL_TLS.BindBuffer(GL_PIXEL_PACK_BUFFER_ARB, reserved_buffer);
		HGL_TLS.DeleteBuffers(1, &buffer);
		HGL_TLS.BufferData(GL_PIXEL_PACK_BUFFER_ARB, reserved_size, 0, GL_STREAM_READ);
		deglReplaceReservedBuffer(buffer, reserved_buffer);
		Dprintf("replaced reserved buffer %u with %u\n", buffer, reserved_buffer);
	}
#	endif // CONFIG_OFFSCREEN == 1
	if(target == GL_ELEMENT_ARRAY_BUFFER)
	{
#		if(CACHE_ELEMENT_BUFFERS == 1)
		if(!ebo_bound || ebo_bound->name != buffer)
		{
			degl_ebo *e = tls->ebo.list;
			for(; e && e->name != buffer; e = e->next);
			if(!e && buffer)
			{
				if((e = malloc(sizeof(*e))))
				{
					memset(e, 0, sizeof(*e));
					e->name = buffer;
					e->next = tls->ebo.list;
					tls->ebo.list = e;
				}
				else
				{
					Dprintf("failed to allocate element array buffer %u cache entry\n", buffer);
				}
			}
			if (current_context)
			{
				current_context->ebo_bound = e;
			}
#			if(DEBUG_VERTEX == 1)
			Dprintf("binding element array buffer %u\n", buffer);
#			endif // DEBUG_VERTEX == 1
		}
#		else // CACHE_ELEMENT_BUFFERS == 1
		if (current_context)
		{
			current_context->ebo_bound = buffer;
		}
#		endif // CACHE_ELEMENT_BUFFERS
	}
	else
	{
#		if(CACHE_ELEMENT_BUFFERS == 1)
		if(ebo_bound && ebo_bound->name == buffer)
		{
			if (current_context)
			{
				current_context->ebo_bound = NULL;
			}
		}
#		else // CACHE_ELEMENT_BUFFERS == 1
		if(buffer == tls->ebo.bound)
		{
			if (current_context)
			{
				current_context->ebo_bound = e;
			}
		}
#		endif // CACHE_ELEMENT_BUFFERS == 1
	}
	HGL_TLS.BindBuffer(target, buffer);
}

GL_APICALL_BUILD void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
	if(target == GL_ARRAY_BUFFER)
	{
		GLint buffer;
		HGL_TLS.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
		//if needed vbo would need to be reconverted
		deglRemoveVBOFromList(buffer);
	}
#	if(CACHE_ELEMENT_BUFFERS == 1)
	if(target == GL_ELEMENT_ARRAY_BUFFER)
	{
		degl_tls *tls = deglGetTLS();
		DEGLContext *current_context = tls->current_context;
		degl_ebo *b = (current_context ? current_context->ebo_bound : 0);
		if(b)
		{
			if(b->cache)
			{
#				if(DEBUG_VERTEX == 1)
				Dprintf("releasing old cache at %p for element array buffer %u\n", b->cache, b->name);
#				endif // DEBUG_VERTEX
				free(b->cache);
			}
			if ((b->cache = malloc(size)))
			{
#				if(DEBUG_VERTEX == 1)
				Dprintf("allocated new cache at %p for element array buffer %u\n", b->cache, b->name);
#				endif // DEBUG_VERTEX == 1
				b->size = size;
				if(data)
				{
					memcpy(b->cache, data, size);
				}
			}
			else
			{
				b->size = 0;
				Dprintf("failed to allocate %d bytes for element array buffer %u\n", (int)size, b->name);
				tls->glError = GL_OUT_OF_MEMORY;
				//TODO: should we return here, not completing the GL call?
			}
		}
		else
		{
			Dprintf("GL_ELEMENT_ARRAY_BUFFER not bound!\n");
		}
	}
#	endif // CACHE_ELEMENT_BUFFERS == 1
	HGL_TLS.BufferData(target, size, data, usage);
}

int dglConvertData(int count, int size, int stride, int first, GLenum from, GLenum to, const void *in, void *out);
GL_APICALL_BUILD void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
	if(target == GL_ARRAY_BUFFER)
	{
		GLint buffer;
		HGL_TLS.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
		//if vbo has been already converted then new data will need to be converted too.
		//Offset and size also need to be recalculated!
		degl_vbo* vbo = deglFindVBO(buffer);
		if(vbo)
		{
			int osize = (vbo->from==GL_FIXED)?sizeof(GLfixed):sizeof(GLbyte);
			int dsize = sizeof(GLfloat);
			void* newdata = malloc(size*dsize/osize);
			if(!dglConvertData(size/osize, 1, osize, 0, vbo->from, vbo->to, data, newdata))
			{
				degl_tls *tls = deglGetTLS();
				Dprintf("Buffer Data conversion failed!\n");
				tls->glError = GL_INVALID_OPERATION;
				return;
			}
			HGL_TLS.BufferSubData(target, offset*dsize/osize, size*dsize/osize, newdata);
			free(newdata);
			return;
		}
	}
#	if(CACHE_ELEMENT_BUFFERS == 1)
	if(target == GL_ELEMENT_ARRAY_BUFFER)
	{
		degl_tls *tls = deglGetTLS();
		DEGLContext *current_context = tls->current_context;
		degl_ebo *b = (current_context ? current_context->ebo_bound : 0);
		if(b)
		{
			if(b->cache)
			{
				if(offset < 0 || size < 0 || b->size < offset + size)
				{
					Dprintf("invalid offset (%d) and/or size (%d) for buffer %u\n", (int)offset, (int)size, b->name);
				}
				else if(data && size > 0)
				{
					memcpy(b->cache + offset, data, size);
				}
			}
			else
			{
				Dprintf("element array buffer %u not yet initialized!\n", b->name);
			}
		}
		else
		{
			Dprintf("GL_ELEMENT_ARRAY_BUFFER not bound!\n");
		}
	}
#	endif // CACHE_ELEMENT_BUFFERS
	HGL_TLS.BufferSubData(target, offset, size, data);
}

GL_APICALL_BUILD void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	if(n > 0 && buffers)
	{
		degl_tls *tls = deglGetTLS();
		DEGLContext *current_context = tls->current_context;
		degl_ebo *ebo_bound = (current_context ? current_context->ebo_bound : 0);
#		if(CONFIG_OFFSCREEN == 1)
		GLuint *buffers_mod = NULL;
		int j = 0;
#		endif // CONFIG_OFFSCREEN == 1
		int i = 0;
		for(; i<n; i++)
		{
#			if(CONFIG_OFFSCREEN == 1)
			if(deglIsReservedBuffer(buffers[i]))
			{
				if(!buffers_mod)
				{
					buffers_mod = malloc(n * sizeof(GLuint));
					if(!buffers_mod)
					{
						Dprintf("out of memory!\n");
						deglGetTLS()->glError = GL_OUT_OF_MEMORY;
						return;
					}
					memcpy(buffers_mod, buffers, i * sizeof(GLuint));
					j = i;
				}
				continue;
			}
			if(buffers_mod)
			{
				buffers_mod[j++] = buffers[i];
			}
#			endif // CONFIG_OFFSCREEN == 1
			//if vbos were binded to an array unbind them
			deglUnbindVBOFromArrays(buffers[i]);
			//and if they were translated remove from list
			deglRemoveVBOFromList(buffers[i]);
#			if(CACHE_ELEMENT_BUFFERS == 1)
			if(ebo_bound && ebo_bound->name == buffers[i])
			{
				if (current_context)
				{
					current_context->ebo_bound = 0;
				}
			}
			degl_ebo *e = tls->ebo.list, *ep = NULL;
			while(e)
			{
				if(e->name == buffers[i])
				{
					if(e->cache)
					{
#						if(DEBUG_VERTEX == 1)
						Dprintf("releasing cache at %p for element array buffer %u\n", e->cache, e->name);
#						endif // DEBUG_VERTEX == 1
						free(e->cache);
					}
#					if(DEBUG_VERTEX == 1)
					Dprintf("releasing cache object for element array buffer %u\n", e->name);
#					endif // DEBUG_VERTEX == 1
					if(ep)
					{
						ep->next = e->next;
					}
					else
					{
						tls->ebo.list = e->next;
					}
					free(e);
					e = NULL;
				}
				else
				{
					ep = e;
					e = e->next;
				}
			}
#			else // CACHE_ELEMENT_BUFFERS == 1
			if(ebo_bound == buffers[i])
			{
				if (current_context)
				{
					current_context->ebo_bound = 0;
				}
			}
#			endif // CACHE_ELEMENT_BUFFERS
		}
#		if(CONFIG_OFFSCREEN == 1)
		if(buffers_mod)
		{
			HGL_TLS.DeleteBuffers(j, buffers_mod);
			free(buffers_mod);
			return;
		}
#		endif // CONFIG_OFFSCREEN
	}
	HGL_TLS.DeleteBuffers(n, buffers);
}

GL_APICALL_BUILD void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
	HGL_TLS.GenBuffers(n, buffers);
}

GL_APICALL_BUILD void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	HGL_TLS.GetBufferParameteriv(target, pname, params);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
#	if(CONFIG_OFFSCREEN == 1)
	if(deglIsReservedBuffer(buffer))
	{
		return GL_FALSE;
	}
#	endif // CONFIG_OFFSCREEN == 1
	return HGL_TLS.IsBuffer(buffer);
}

// Pixel functions
GL_APICALL_BUILD void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{
	HGL_TLS.PixelStorei(pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
{
	extern void dglReadPixels(void *, void *, unsigned, unsigned, unsigned, unsigned, int, int, unsigned);
	dglReadPixels(pixels, 0, x, y, width, height, format, type, 0);
	//HGL_TLS.ReadPixels(x, y, width, height, format, type, pixels);
}

// Texture functions
GL_APICALL_BUILD void GL_APIENTRY glActiveTexture(GLenum texture)
{
	HGL_TLS.ActiveTexture(texture);
}

GL_APICALL_BUILD void GL_APIENTRY glBindTexture (GLenum target, GLuint texture)
{
	HGL_TLS.BindTexture(target, texture);
}

GL_APICALL_BUILD void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
	HGL_TLS.CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

GL_APICALL_BUILD void GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
	HGL_TLS.CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

GL_APICALL_BUILD void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	HGL_TLS.CopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

GL_APICALL_BUILD void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	HGL_TLS.CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

GL_APICALL_BUILD void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
	HGL_TLS.DeleteTextures(n, textures);
}

GL_APICALL_BUILD void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
	HGL_TLS.GenTextures(n, textures);
}

GL_APICALL_BUILD void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
	HGL_TLS.GetTexParameterfv(target, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	HGL_TLS.GetTexParameteriv(target, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
	HGL_TLS.TexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

GL_APICALL_BUILD void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	HGL_TLS.TexParameterf(target, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
	HGL_TLS.TexParameterfv(target, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	HGL_TLS.TexParameteri(target, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	HGL_TLS.TexParameteriv(target, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
	HGL_TLS.TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
	HGL_TLS.IsTexture(texture);
}

// Draw functions
GL_APICALL_BUILD void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	HGL_TLS.ColorMask(red, green, blue, alpha);
}

GL_APICALL_BUILD void GL_APIENTRY glCullFace(GLenum mode)
{
	HGL_TLS.CullFace(mode);
}

GL_APICALL_BUILD void GL_APIENTRY glDisable(GLenum cap)
{
	HGL_TLS.Disable(cap);
}

GL_APICALL_BUILD void GL_APIENTRY glEnable(GLenum cap)
{
	HGL_TLS.Enable(cap);
}

GL_APICALL_BUILD void GL_APIENTRY glFinish(void)
{
	HGL_TLS.Finish();
}

GL_APICALL_BUILD void GL_APIENTRY glFlush(void)
{
	HGL_TLS.Flush();
}

GL_APICALL_BUILD void GL_APIENTRY glFrontFace(GLenum mode)
{
	HGL_TLS.FrontFace(mode);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
	return HGL_TLS.IsEnabled(cap);
}

GL_APICALL_BUILD void GL_APIENTRY glHint(GLenum target, GLenum mode)
{
	HGL_TLS.Hint(target, mode);
}

GL_APICALL_BUILD void GL_APIENTRY glLineWidth (GLfloat width)
{
	HGL_TLS.LineWidth(width);
}

GL_APICALL_BUILD void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
	HGL_TLS.PolygonOffset(factor, units);
}

GL_APICALL_BUILD void GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
	HGL_TLS.SampleCoverage(value, invert);
}

GL_APICALL_BUILD void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	HGL_TLS.Scissor(x, y, width, height);
}

GL_APICALL_BUILD void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	HGL_TLS.Viewport(x, y, width, height);
}

// Blend functions
GL_APICALL_BUILD void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	HGL_TLS.BlendFunc(sfactor, dfactor);
}

// Get functions
static GLenum glGet_pname_translate(GLenum pname)
{
	switch(pname)
	{
		case GL_MAX_FRAGMENT_UNIFORM_VECTORS: pname = GL_MAX_FRAGMENT_UNIFORM_COMPONENTS; break;
		case GL_MAX_VARYING_VECTORS: pname = GL_MAX_VARYING_FLOATS; break;
		case GL_MAX_VERTEX_UNIFORM_VECTORS: pname = GL_MAX_VERTEX_UNIFORM_COMPONENTS; break;
		default: break;
	}
	return pname;
}

GL_APICALL_BUILD void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params)
{
	if(pname == GL_SHADER_COMPILER)
	{
		*params = GL_TRUE;
	}
	else
	{
		HGL_TLS.GetBooleanv(glGet_pname_translate(pname), params);
	}
}

GL_APICALL_BUILD GLenum GL_APIENTRY glGetError(void)
{
	degl_tls *tls = deglGetTLS();
	GLenum error = tls->glError;
	if(error == GL_NO_ERROR)
	{
		error = HGL_TLS.GetError();
	}
	else
	{
		tls->glError = GL_NO_ERROR;
	}
	if(error != GL_NO_ERROR)
	{
		Dprintf("WARNING: GL error: 0x%x!\n", error);
	}
	return error;
}

GL_APICALL_BUILD void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
	HGL_TLS.GetFloatv(glGet_pname_translate(pname), params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
	switch(pname)
	{
		case GL_IMPLEMENTATION_COLOR_READ_TYPE:
			*params = GL_UNSIGNED_BYTE;
			break;
		case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
			*params = GL_RGBA;
			break;
		case GL_NUM_SHADER_BINARY_FORMATS:
			*params = 0;
			break;
		case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
			*params = 0;
			break;
		default:
			HGL_TLS.GetIntegerv(glGet_pname_translate(pname), params);
			break;
	}
}

static const GLubyte *translateExtensions()
{
    degl_tls *tls = deglGetTLS();
    if(!tls)
    {
        return (GLubyte *)"";
    }
    if(!tls->extensions_string)
    {
        const char *translationTable[] =
        {
            "GL_EXT_packed_depth_stencil", "GL_OES_packed_depth_stencil"
        };

        char *extensions = (char *)HGL_TLS.GetString(GL_EXTENSIONS);
        int len = 1;
        char *result = malloc(len);
        *result = 0;
        unsigned i;
        for(i = 0; i < sizeof(translationTable)/sizeof(translationTable[0]); i += 2)
        {
            if(strstr(extensions, translationTable[i]))
            {
                int new_len = len + strlen(translationTable[i+1]) + 1;
                result = realloc(result, new_len);
                strcat(result, translationTable[i+1]);
                result[new_len-2] = ' ';
                result[new_len-1] = 0;
                len = new_len;
            }
        }
        tls->extensions_string = result;
    }
    return (GLubyte *)tls->extensions_string;
}

GL_APICALL_BUILD const GLubyte* GL_APIENTRY glGetString(GLenum name)
{
	switch(name)
	{
		case GL_EXTENSIONS:
			return translateExtensions();
		default:
			break;
	}
	return HGL_TLS.GetString(name);
}
