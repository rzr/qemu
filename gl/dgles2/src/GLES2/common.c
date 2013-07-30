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
#include "common-gles.h"

//texture functions
GL_APICALL_BUILD void GL_APIENTRY glGenerateMipmap (GLenum target)
{
    HGL_TLS.GenerateMipmapEXT(target);
}

//blend functions
GL_APICALL_BUILD void GL_APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    HGL_TLS.BlendColor(red, green, blue, alpha);
}

GL_APICALL_BUILD void GL_APIENTRY glBlendEquation(GLenum mode)
{
    HGL_TLS.BlendEquation(mode);
}

GL_APICALL_BUILD void GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    HGL_TLS.BlendEquationSeparate(modeRGB, modeAlpha);
}

GL_APICALL_BUILD void GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    HGL_TLS.BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

//program functions
GL_APICALL_BUILD void GL_APIENTRY glAttachShader (GLuint program, GLuint shader)
{
    HGL_TLS.AttachShader(program, shader);
}

GL_APICALL_BUILD void GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
    HGL_TLS.BindAttribLocation(program, index, name);
}

GL_APICALL_BUILD GLuint GL_APIENTRY glCreateProgram(void)
{
    return HGL_TLS.CreateProgram();
}

GL_APICALL_BUILD void GL_APIENTRY glDeleteProgram(GLuint program)
{
    HGL_TLS.DeleteProgram(program);
}

GL_APICALL_BUILD void GL_APIENTRY glDetachShader(GLuint program, GLuint shader)
{
    HGL_TLS.DetachShader(program, shader);
}

GL_APICALL_BUILD void GL_APIENTRY glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    HGL_TLS.GetActiveAttrib(program, index, bufsize, length, size, type, name);
}

GL_APICALL_BUILD void GL_APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    HGL_TLS.GetActiveUniform(program, index, bufsize, length, size, type, name);
}

GL_APICALL_BUILD void GL_APIENTRY glGetAttachedShaders (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    HGL_TLS.GetAttachedShaders(program, maxcount, count, shaders);
}

GL_APICALL_BUILD int GL_APIENTRY glGetAttribLocation (GLuint program, const char* name)
{
    return HGL_TLS.GetAttribLocation(program, name);
}

#define GL_PROGRAM_BINARY_LENGTH_OES                            0x8741

GL_APICALL_BUILD void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
    if (pname == GL_PROGRAM_BINARY_LENGTH_OES)
    {
        *params=0;
        return;
    }
    HGL_TLS.GetProgramiv(program, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
    if(bufsize > 0) *infolog = 0;
    HGL_TLS.GetProgramInfoLog(program, bufsize, length, infolog);
}

GL_APICALL_BUILD void GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    HGL_TLS.GetUniformfv(program, location, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
    HGL_TLS.GetUniformiv(program, location, params);
}

GL_APICALL_BUILD int GL_APIENTRY glGetUniformLocation(GLuint program, const char* name)
{
    return HGL_TLS.GetUniformLocation(program, name);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsProgram(GLuint program)
{
    return HGL_TLS.IsProgram(program);
}

GL_APICALL_BUILD void GL_APIENTRY glLinkProgram(GLuint program)
{
    HGL_TLS.LinkProgram(program);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform1f (GLint location, GLfloat x)
{
    HGL_TLS.Uniform1f(location, x);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat* v)
{
    HGL_TLS.Uniform1fv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform1i (GLint location, GLint x)
{
    HGL_TLS.Uniform1i(location, x);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint* v)
{
    HGL_TLS.Uniform1iv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform2f (GLint location, GLfloat x, GLfloat y)
{
    HGL_TLS.Uniform2f(location, x, y);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat* v)
{
    HGL_TLS.Uniform2fv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform2i (GLint location, GLint x, GLint y)
{
    HGL_TLS.Uniform2i(location, x, y);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint* v)
{
    HGL_TLS.Uniform2iv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    HGL_TLS.Uniform3f(location, x, y, z);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat* v)
{
    HGL_TLS.Uniform3fv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform3i (GLint location, GLint x, GLint y, GLint z)
{
    HGL_TLS.Uniform3i(location, x, y, z);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint* v)
{
    HGL_TLS.Uniform3iv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    HGL_TLS.Uniform4f(location, x, y, z, w);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat* v)
{
    HGL_TLS.Uniform4fv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform4i (GLint location, GLint x, GLint y, GLint z, GLint w)
{
    HGL_TLS.Uniform4i(location, x, y, z, w);
}

GL_APICALL_BUILD void GL_APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint* v)
{
    HGL_TLS.Uniform4iv(location, count, v);
}

GL_APICALL_BUILD void GL_APIENTRY glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    HGL_TLS.UniformMatrix2fv(location, count, transpose, value);
}

GL_APICALL_BUILD void GL_APIENTRY glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    HGL_TLS.UniformMatrix3fv(location, count, transpose, value);
}

GL_APICALL_BUILD void GL_APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    HGL_TLS.UniformMatrix4fv(location, count, transpose, value);
}

GL_APICALL_BUILD void GL_APIENTRY glUseProgram (GLuint program)
{
    HGL_TLS.UseProgram(program);
}

GL_APICALL_BUILD void GL_APIENTRY glValidateProgram (GLuint program)
{
    HGL_TLS.ValidateProgram(program);
}

//fbo functions

GL_APICALL_BUILD void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	HGL_TLS.BindFramebufferEXT(target, framebuffer);
}

GL_APICALL_BUILD void GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	HGL_TLS.BindRenderbufferEXT(target, renderbuffer);
}

GL_APICALL_BUILD GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target)
{
	return HGL_TLS.CheckFramebufferStatusEXT(target);
}

GL_APICALL_BUILD void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
	HGL_TLS.DeleteFramebuffersEXT(n, framebuffers);
}

GL_APICALL_BUILD void GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
	HGL_TLS.DeleteRenderbuffersEXT(n, renderbuffers);
}

GL_APICALL_BUILD void GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	HGL_TLS.FramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);
}

GL_APICALL_BUILD void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	HGL_TLS.FramebufferTexture2DEXT(target, attachment, textarget, texture, level);
}

GL_APICALL_BUILD void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
	HGL_TLS.GenFramebuffersEXT(n, framebuffers);
}

GL_APICALL_BUILD void GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
	HGL_TLS.GenRenderbuffersEXT(n, renderbuffers);
}

GL_APICALL_BUILD void GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	HGL_TLS.GetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	HGL_TLS.GetRenderbufferParameterivEXT(target, pname, params);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsFramebuffer(GLuint framebuffer)
{
	return HGL_TLS.IsFramebufferEXT(framebuffer);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
	return HGL_TLS.IsRenderbufferEXT(renderbuffer);
}

GL_APICALL_BUILD void GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	HGL_TLS.RenderbufferStorageEXT(target, internalformat, width, height);
}

//stencil functions
GL_APICALL_BUILD void GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	HGL_TLS.StencilFuncSeparate(face, func, ref, mask);
}

GL_APICALL_BUILD void GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
	HGL_TLS.StencilMaskSeparate(face, mask);
}

GL_APICALL_BUILD void GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	HGL_TLS.StencilOpSeparate(face, fail, zfail, zpass);
}
