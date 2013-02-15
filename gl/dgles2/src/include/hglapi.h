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

#ifndef HGLAPI_H_
#define HGLAPI_H_

#include "GLES/gl.h"

// Things that GL/gl.h has, but GLES2/gl2.h doesn't.

/* ClientAttribMask */
#define GL_CLIENT_PIXEL_STORE_BIT         0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT        0x00000002
#if (CONFIG_COCOA != 1)
#define GL_CLIENT_ALL_ATTRIB_BITS         0xFFFFFFFF
#endif

/* OpenGL12 */
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1

typedef double GLdouble;
typedef double GLclampd; /* double precision float in [0,1] */


// Things that GL/glext.h has.

#define GL_READ_FRAMEBUFFER_BINDING       0x8CAA

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


#define HGL_FUNCS \
    HGL_FUNC(GLenum, GetError, (void)) \
    HGL_FUNC(GLuint, CreateShader, (GLenum)) \
    HGL_FUNC(void, ShaderSource, (GLuint, GLsizei, const GLchar* *, const GLint *)) \
    HGL_FUNC(void, CompileShader, (GLuint)) \
    HGL_FUNC(void, GetShaderiv, (GLuint, GLenum, GLint *)) \
    HGL_FUNC(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
    HGL_FUNC(void, GetShaderSource, (GLuint shader, GLsizei bufsize, GLsizei* length, char* source)) \
    HGL_FUNC(GLuint, CreateProgram, (void)) \
    HGL_FUNC(void, AttachShader, (GLuint, GLuint)) \
    HGL_FUNC(void, BindAttribLocation, (GLuint, GLuint, const GLchar *)) \
    HGL_FUNC(void, LinkProgram, (GLuint)) \
    HGL_FUNC(void, UseProgram, (GLuint)) \
    HGL_FUNC(void, GetProgramiv, (GLuint, GLenum, GLint *)) \
    HGL_FUNC(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
    HGL_FUNC(GLint, GetUniformLocation, (GLuint, const GLchar *)) \
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
    HGL_FUNC(void, GetActiveAttrib, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)) \
    HGL_FUNC(void, GetActiveUniform, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)) \
    HGL_FUNC(void, GetAttachedShaders, (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)) \
    HGL_FUNC(void, GetUniformfv, (GLuint program, GLint location, GLfloat* params)) \
    HGL_FUNC(void, GetUniformiv, (GLuint program, GLint location, GLint* params)) \
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
    HGL_FUNC(void, GetVertexAttribfv, (GLuint index, GLenum pname, GLfloat* params)) \
    HGL_FUNC(void, GetVertexAttribiv, (GLuint index, GLenum pname, GLint* params)) \
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
    HGL_FUNC(void, DrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)) \
    HGL_FUNC(GLint, GetAttribLocation, (GLuint, const GLchar *)) \
    HGL_FUNC(void, StencilFunc, (GLenum func, GLint ref, GLuint mask)) \
    HGL_FUNC(void, StencilMask, (GLuint mask)) \
    HGL_FUNC(void, StencilOp, (GLenum fail, GLenum zfail, GLenum zpass)) \
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
    HGL_FUNC(void, CompressedTexImage2D, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)) \
    HGL_FUNC(void, CompressedTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)) \
    HGL_FUNC(void, CopyTexImage2D, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)) \
    HGL_FUNC(void, CopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)) \
    HGL_FUNC(void, ValidateProgram, (GLuint program))\
    \
    HGL_FUNC(void, AlphaFunc, (GLenum func, GLclampf ref)) \
    HGL_FUNC(void, ClipPlane, (GLenum plane, const GLdouble *equation))\
    HGL_FUNC(void, Color4f, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    HGL_FUNC(void, Fogf, (GLenum pname, GLfloat param)) \
    HGL_FUNC(void, Fogfv, (GLenum pname, const GLfloat *params)) \
    HGL_FUNC(void, Frustum, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal)) \
    HGL_FUNC(void, GetClipPlane, (GLenum plane, GLdouble *equation)) \
    HGL_FUNC(void, GetLightfv, (GLenum light, GLenum pname, GLfloat *params)) \
    HGL_FUNC(void, GetMaterialfv, (GLenum face, GLenum pname, GLfloat *params)) \
    HGL_FUNC(void, GetTexEnvfv, (GLenum env, GLenum pname, GLfloat *params)) \
    HGL_FUNC(void, LightModelf, (GLenum pname, GLfloat param)) \
    HGL_FUNC(void, LightModelfv, (GLenum pname, const GLfloat *params)) \
    HGL_FUNC(void, Lightf, (GLenum light, GLenum pname, GLfloat param)) \
    HGL_FUNC(void, Lightfv, (GLenum light, GLenum pname, const GLfloat *params)) \
    HGL_FUNC(void, LoadMatrixf, (const GLfloat *m)) \
    HGL_FUNC(void, Materialf, (GLenum face, GLenum pname, GLfloat param)) \
    HGL_FUNC(void, Materialfv, (GLenum face, GLenum pname, const GLfloat* params)) \
    HGL_FUNC(void, MultMatrixf, (const GLfloat *m)) \
    HGL_FUNC(void, MultiTexCoord4f, (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)) \
    HGL_FUNC(void, Normal3f, (GLfloat nx, GLfloat ny, GLfloat nz)) \
    HGL_FUNC(void, Ortho, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far)) \
    HGL_FUNC(void, PointParameterf, (GLenum pname, GLfloat param)) \
    HGL_FUNC(void, PointParameterfv, (GLenum pname, const GLfloat *params)) \
    HGL_FUNC(void, PointSize, (GLfloat size)) \
    HGL_FUNC(void, Rotatef, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)) \
    HGL_FUNC(void, Scalef, (GLfloat x, GLfloat y, GLfloat z)) \
    HGL_FUNC(void, TexEnvf, (GLenum target, GLenum pname, GLfloat param)) \
    HGL_FUNC(void, TexEnvfv, (GLenum target, GLenum pname, const GLfloat *params)) \
    HGL_FUNC(void, Translatef, (GLfloat x, GLfloat y, GLfloat z)) \
    HGL_FUNC(void, ClientActiveTexture, (GLenum texture)) \
    HGL_FUNC(void, Color4ub, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)) \
    HGL_FUNC(void, ColorPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
    HGL_FUNC(void, DisableClientState, (GLenum array)) \
    HGL_FUNC(void, EnableClientState, (GLenum array)) \
    HGL_FUNC(void, GetDoublev, (GLenum pname, GLdouble *params)) \
    HGL_FUNC(void, GetPointerv, (GLenum pname, GLvoid **params)) \
    HGL_FUNC(void, GetTexEnviv, (GLenum env, GLenum pname, GLint *params)) \
    HGL_FUNC(void, LineWidth, (GLfloat width)) \
    HGL_FUNC(void, LoadIdentity, (void)) \
    HGL_FUNC(void, LogicOp, (GLenum opcode)) \
    HGL_FUNC(void, MatrixMode, (GLenum mode)) \
    HGL_FUNC(void, MultMatrixd, (const GLdouble *m)) \
    HGL_FUNC(void, MultiTexCoord4d, (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)) \
    HGL_FUNC(void, NormalPointer, (GLenum type, GLsizei stride, const GLvoid *pointer)) \
    HGL_FUNC(void, PolygonOffset, (GLfloat factor, GLfloat units)) \
    HGL_FUNC(void, PopMatrix, (void)) \
    HGL_FUNC(void, PushMatrix, (void)) \
    HGL_FUNC(void, SampleCoverage, (GLclampf value, GLboolean invert)) \
    HGL_FUNC(void, ShadeModel, (GLenum mode)) \
    HGL_FUNC(void, TexCoordPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
    HGL_FUNC(void, TexEnvi, (GLenum target, GLenum pname, GLint param)) \
    HGL_FUNC(void, TexEnviv, (GLenum target, GLenum pname, const GLint *params)) \
    HGL_FUNC(void, VertexPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
    HGL_FUNC(void, FrontFace, (GLenum mode))
    // the end

typedef struct HGL
{
#define HGL_FUNC(ret, name, attr) ret GL_APIENTRY (*name)attr;
    HGL_FUNCS
#undef HGL_FUNC
} HGL;

#include "../EGL/degl.h"
#define HGL_TLS (*(HGL *)(deglGetTLS()->hgl))

#endif // HGLAPI_H_

