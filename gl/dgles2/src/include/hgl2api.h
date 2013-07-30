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

#ifndef HGL2API_H_
#define HGL2API_H_

#include "GLES2/gl2.h"
// Things that GL/gl.h has, but GLES2/gl2.h doesn't.

/* ClientAttribMask */
#define GL_CLIENT_PIXEL_STORE_BIT         0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT        0x00000002
#if (CONFIG_COCOA != 1)
#define GL_CLIENT_ALL_ATTRIB_BITS         0xFFFFFFFF
#endif

/* OpenGL 1.2 */
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1

typedef double GLdouble;
typedef double GLclampd; /* double precision float in [0,1] */

// Things that GL/glext.h has.

/* OpenGL 1.5 */
#define GL_BUFFER_SIZE                    0x8764
#define GL_BUFFER_USAGE                   0x8765
#define GL_QUERY_COUNTER_BITS             0x8864
#define GL_CURRENT_QUERY                  0x8865
#define GL_QUERY_RESULT                   0x8866
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ARRAY_BUFFER_BINDING           0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING   0x8895
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_BUFFER_ACCESS                  0x88BB
#define GL_BUFFER_MAPPED                  0x88BC
#define GL_BUFFER_MAP_POINTER             0x88BD
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_SAMPLES_PASSED                 0x8914

/* OpenGL 2.0 */
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#define GL_MAX_VARYING_FLOATS             0x8B4B

/* GL_ARB_pixel_buffer_object */
#define GL_PIXEL_PACK_BUFFER_ARB          0x88EB
#define GL_PIXEL_UNPACK_BUFFER_ARB        0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB  0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB 0x88EF

/* Needed OpenGL 2.1 functions */
#define HGL_FUNCS \
	HGL_FUNC(GLenum, GetError, (void)) \
	HGL_FUNC(GLuint, CreateShader, (GLenum)) \
	HGL_FUNC(void, ShaderSource, (GLuint, GLsizei, const GLchar* *, const GLint *)) \
	HGL_FUNC(void, CompileShader, (GLuint)) \
	HGL_FUNC(void, GetShaderiv, (GLuint, GLenum, GLint *)) \
	HGL_FUNC(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
	HGL_FUNC(GLuint, CreateProgram, (void)) \
	HGL_FUNC(void, AttachShader, (GLuint, GLuint)) \
	HGL_FUNC(void, BindAttribLocation, (GLuint, GLuint, const GLchar *)) \
	HGL_FUNC(void, LinkProgram, (GLuint)) \
	HGL_FUNC(void, UseProgram, (GLuint)) \
	HGL_FUNC(void, ValidateProgram, (GLuint)) \
	HGL_FUNC(void, GetProgramiv, (GLuint, GLenum, GLint *)) \
	HGL_FUNC(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
	HGL_FUNC(GLint, GetUniformLocation, (GLuint, const GLchar *)) \
	HGL_FUNC(void, GetUniformfv, (GLuint, GLint, GLfloat *)) \
	HGL_FUNC(void, GetUniformiv, (GLuint, GLint, GLint *)) \
	HGL_FUNC(void, Uniform1f, (GLint, GLfloat)) \
	HGL_FUNC(void, Uniform2f, (GLint, GLfloat, GLfloat)) \
	HGL_FUNC(void, Uniform3f, (GLint, GLfloat, GLfloat, GLfloat)) \
	HGL_FUNC(void, Uniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
	HGL_FUNC(void, Uniform1i, (GLint, GLint)) \
	HGL_FUNC(void, Uniform2i, (GLint, GLint, GLint)) \
	HGL_FUNC(void, Uniform3i, (GLint, GLint, GLint, GLint)) \
	HGL_FUNC(void, Uniform4i, (GLint, GLint, GLint, GLint, GLint)) \
	HGL_FUNC(void, Uniform1fv, (GLint, GLsizei, const GLfloat *)) \
	HGL_FUNC(void, Uniform2fv, (GLint, GLsizei, const GLfloat *)) \
	HGL_FUNC(void, Uniform3fv, (GLint, GLsizei, const GLfloat *)) \
	HGL_FUNC(void, Uniform4fv, (GLint, GLsizei, const GLfloat *)) \
	HGL_FUNC(void, Uniform1iv, (GLint, GLsizei, const GLint *)) \
	HGL_FUNC(void, Uniform2iv, (GLint, GLsizei, const GLint *)) \
	HGL_FUNC(void, Uniform3iv, (GLint, GLsizei, const GLint *)) \
	HGL_FUNC(void, Uniform4iv, (GLint, GLsizei, const GLint *)) \
	HGL_FUNC(void, UniformMatrix2fv, (GLint, GLsizei, GLboolean, const GLfloat *)) \
	HGL_FUNC(void, UniformMatrix3fv, (GLint, GLsizei, GLboolean, const GLfloat *)) \
	HGL_FUNC(void, UniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *)) \
	HGL_FUNC(void, GetActiveAttrib, (GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, char *)) \
	HGL_FUNC(void, GetActiveUniform, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)) \
	HGL_FUNC(void, GetAttachedShaders, (GLuint, GLsizei, GLsizei *, GLuint *)) \
	HGL_FUNC(void, Clear, (GLbitfield mask )) \
	HGL_FUNC(void, ClearColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )) \
	HGL_FUNC(void, EnableVertexAttribArray, (GLuint)) \
	HGL_FUNC(void, DisableVertexAttribArray, (GLuint)) \
	HGL_FUNC(void, VertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)) \
	HGL_FUNC(void, DrawArrays, (GLenum mode, GLint first, GLsizei count )) \
	HGL_FUNC(void, GenBuffers, (GLsizei, GLuint *)) \
	HGL_FUNC(void, BindBuffer, (GLenum, GLuint)) \
	HGL_FUNC(void, GetBufferParameteriv, (GLenum, GLenum, GLint *)) \
	HGL_FUNC(void, BufferData, (GLenum, GLsizeiptr, const GLvoid *, GLenum)) \
	HGL_FUNC(GLvoid*, MapBuffer, (GLenum target, GLenum access)) \
	HGL_FUNC(GLboolean, UnmapBuffer, (GLenum target)) \
	HGL_FUNC(void, GenTextures, (GLsizei n, GLuint *textures)) \
	HGL_FUNC(void, DeleteTextures, (GLsizei n, const GLuint *textures)) \
	HGL_FUNC(void, BindTexture, (GLenum target, GLuint texture)) \
	HGL_FUNC(void, TexImage2D, (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) \
	HGL_FUNC(void, TexParameterf, (GLenum target, GLenum pname, GLfloat param)) \
	HGL_FUNC(void, TexParameteri, (GLenum target, GLenum pname, GLint param)) \
	HGL_FUNC(void, TexParameterfv, (GLenum target, GLenum pname, const GLfloat *params)) \
	HGL_FUNC(void, TexParameteriv, (GLenum target, GLenum pname, const GLint *params)) \
	HGL_FUNC(const GLubyte*, GetString, (GLenum name)) \
	HGL_FUNC(void, GetBooleanv, (GLenum pname, GLboolean *params)) \
	HGL_FUNC(void, GetFloatv, (GLenum pname, GLfloat *params)) \
	HGL_FUNC(void, GetIntegerv, (GLenum pname, GLint *params)) \
	HGL_FUNC(void, BlendFunc, (GLenum sfactor, GLenum dfactor)) \
	HGL_FUNC(void, ColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
	HGL_FUNC(void, CullFace, (GLenum mode)) \
	HGL_FUNC(void, Enable, (GLenum cap)) \
	HGL_FUNC(void, Disable, (GLenum cap)) \
	HGL_FUNC(void, Viewport, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	HGL_FUNC(void, PolygonOffset, (GLfloat factor, GLfloat units)) \
	HGL_FUNC(void, PixelStorei, (GLenum pname, GLint param)) \
	HGL_FUNC(void, TexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)) \
	HGL_FUNC(void, VertexAttrib1f, (GLuint, GLfloat)) \
	HGL_FUNC(void, VertexAttrib1fv, (GLuint, const GLfloat *)) \
	HGL_FUNC(void, VertexAttrib2f, (GLuint, GLfloat, GLfloat)) \
	HGL_FUNC(void, VertexAttrib2fv, (GLuint, const GLfloat *)) \
	HGL_FUNC(void, VertexAttrib3f, (GLuint, GLfloat, GLfloat, GLfloat)) \
	HGL_FUNC(void, VertexAttrib3fv, (GLuint, const GLfloat *)) \
	HGL_FUNC(void, VertexAttrib4f, (GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) \
	HGL_FUNC(void, VertexAttrib4fv, (GLuint, const GLfloat *)) \
	HGL_FUNC(void, GetVertexAttribfv, (GLuint, GLenum, GLfloat *)) \
	HGL_FUNC(void, GetVertexAttribiv, (GLuint, GLenum, GLint *)) \
	HGL_FUNC(void, GetVertexAttribPointerv, (GLuint, GLenum, void **)) \
	HGL_FUNC(void, DeleteProgram, (GLuint)) \
	HGL_FUNC(void, DeleteShader, (GLuint)) \
	HGL_FUNC(void, DetachShader, (GLuint, GLuint)) \
	HGL_FUNC(void, Hint, (GLenum target, GLenum mode)) \
	HGL_FUNC(void, Scissor, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	HGL_FUNC(void, ReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)) \
	HGL_FUNC(void, GenerateMipmapEXT, (GLenum)) \
	HGL_FUNC(void, BlendEquation, (GLenum mode)) \
	HGL_FUNC(void, BlendColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
	HGL_FUNC(void, BlendFuncSeparate, (GLenum, GLenum, GLenum, GLenum)) \
	HGL_FUNC(void, BlendEquationSeparate, (GLenum, GLenum)) \
	HGL_FUNC(void, BindFramebufferEXT, (GLenum, GLuint)) \
	HGL_FUNC(void, DeleteFramebuffersEXT, (GLsizei, const GLuint *)) \
	HGL_FUNC(void, GenFramebuffersEXT, (GLsizei, GLuint *)) \
	HGL_FUNC(void, BindRenderbufferEXT, (GLenum, GLuint)) \
	HGL_FUNC(void, DeleteRenderbuffersEXT, (GLsizei, const GLuint *)) \
	HGL_FUNC(void, GenRenderbuffersEXT, (GLsizei, GLuint *)) \
	HGL_FUNC(void, GetFramebufferAttachmentParameterivEXT, (GLenum, GLenum, GLenum, GLint *)) \
	HGL_FUNC(void, GetRenderbufferParameterivEXT, (GLenum, GLenum, GLint *)) \
	HGL_FUNC(GLboolean, IsEnabled, (GLenum cap)) \
	HGL_FUNC(void, ActiveTexture, (GLenum texture)) \
	HGL_FUNC(void, GetTexParameterfv, (GLenum target, GLenum pname, const GLfloat *params)) \
	HGL_FUNC(void, GetTexParameteriv, (GLenum target, GLenum pname, const GLint *params)) \
	HGL_FUNC(GLboolean, IsTexture, (GLuint texture)) \
	HGL_FUNC(GLboolean, IsProgram, (GLuint program)) \
	HGL_FUNC(GLboolean, IsShader, (GLuint shader)) \
	HGL_FUNC(void, BufferSubData, (GLenum, GLintptr, GLsizeiptr, const GLvoid *)) \
	HGL_FUNC(void, DeleteBuffers, (GLsizei, const GLuint *)) \
	HGL_FUNC(GLboolean, IsBuffer, (GLuint)) \
	HGL_FUNC(GLboolean, IsFramebufferEXT, (GLuint)) \
	HGL_FUNC(GLboolean, IsRenderbufferEXT, (GLuint)) \
	HGL_FUNC(void, DrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)) \
	HGL_FUNC(GLint, GetAttribLocation, (GLuint, const GLchar *)) \
	HGL_FUNC(void, StencilFunc, (GLenum func, GLint ref, GLuint mask)) \
	HGL_FUNC(void, StencilFuncSeparate, (GLenum face, GLenum func, GLint ref, GLuint mask)) \
	HGL_FUNC(void, StencilMask, (GLuint mask)) \
	HGL_FUNC(void, StencilMaskSeparate, (GLenum face, GLuint mask)) \
	HGL_FUNC(void, StencilOp, (GLenum fail, GLenum zfail, GLenum zpass)) \
	HGL_FUNC(void, StencilOpSeparate, (GLenum face, GLenum fail, GLenum zfail, GLenum zpass)) \
	HGL_FUNC(void, ClearStencil, (GLint s)) \
	HGL_FUNC(void, FramebufferTexture2DEXT, (GLenum, GLenum, GLenum, GLuint, GLint)) \
	HGL_FUNC(void, RenderbufferStorageEXT, (GLenum, GLenum, GLsizei, GLsizei)) \
	HGL_FUNC(void, FramebufferRenderbufferEXT, (GLenum, GLenum, GLenum, GLuint)) \
	HGL_FUNC(GLenum, CheckFramebufferStatusEXT, (GLenum)) \
	HGL_FUNC(void, Finish, (void)) \
	HGL_FUNC(void, Flush, (void)) \
	HGL_FUNC(void, DepthFunc, (GLenum func)) \
	HGL_FUNC(void, DepthMask, (GLboolean flag)) \
	HGL_FUNC(void, DepthRange, (GLclampd zNear, GLclampd zFar)) \
	HGL_FUNC(void, ClearDepth, (GLclampd depth)) \
	HGL_FUNC(void, PushClientAttrib, (GLbitfield mask)) \
	HGL_FUNC(void, PopClientAttrib, (void)) \
	HGL_FUNC(void, CopyTexImage2D, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)) \
	HGL_FUNC(void, CopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)) \
	HGL_FUNC(void, FrontFace, (GLenum mode)) \
	HGL_FUNC(void, SampleCoverage, (GLclampf value, GLboolean invert)) \
	HGL_FUNC(void, LineWidth, (GLfloat width)) \
	HGL_FUNC(void, CompressedTexImage2D, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)) \
	HGL_FUNC(void, CompressedTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)) \
	HGL_FUNC(void, GetShaderSource, (GLuint, GLsizei, GLsizei *, char *))

/* These OpenGL 4.1 functions are also used when found but we don't complain if they are missing */
#define HGL_OPTIONAL_FUNCS \
	HGL_FUNC(void, GetShaderPrecisionFormat, (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)) \
	HGL_FUNC(void, ReleaseShaderCompiler, (void)) \
	HGL_FUNC(void, ShaderBinary, (GLsizei, const GLuint *, GLenum, const void *, GLsizei))

typedef struct HGL
{
#define HGL_FUNC(ret, name, attr) ret GL_APIENTRY (*name)attr;
	HGL_FUNCS
	HGL_OPTIONAL_FUNCS
#undef HGL_FUNC
} HGL;

#include "../EGL/degl.h"
#define HGL_TLS (*(HGL *)(deglGetTLS()->hgl))

#endif // HGL2API_H_
