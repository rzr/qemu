#ifndef _QEMU_YAGL_GLES_DRIVER_H
#define _QEMU_YAGL_GLES_DRIVER_H

#include "yagl_types.h"

#ifdef GL_APIENTRY
#define YAGL_GLES_APIENTRY GL_APIENTRY
#else
#define YAGL_GLES_APIENTRY GLAPIENTRY
#endif

#define YAGL_GLES_DRIVER_FUNC0(func) \
void (YAGL_GLES_APIENTRY *func)(void);

#define YAGL_GLES_DRIVER_FUNC_RET0(ret_type, func) \
ret_type (YAGL_GLES_APIENTRY *func)(void);

#define YAGL_GLES_DRIVER_FUNC1(func, arg0_type, arg0) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0);

#define YAGL_GLES_DRIVER_FUNC_RET1(ret_type, func, arg0_type, arg0) \
ret_type (YAGL_GLES_APIENTRY *func)(arg0_type arg0);

#define YAGL_GLES_DRIVER_FUNC2(func, arg0_type, arg1_type, arg0, arg1) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1);

#define YAGL_GLES_DRIVER_FUNC_RET2(ret_type, func, arg0_type, arg1_type, arg0, arg1) \
ret_type (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1);

#define YAGL_GLES_DRIVER_FUNC3(func, arg0_type, arg1_type, arg2_type, arg0, arg1, arg2) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2);

#define YAGL_GLES_DRIVER_FUNC4(func, arg0_type, arg1_type, arg2_type, arg3_type, arg0, arg1, arg2, arg3) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3);

#define YAGL_GLES_DRIVER_FUNC5(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg0, arg1, arg2, arg3, arg4) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4);

#define YAGL_GLES_DRIVER_FUNC6(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg0, arg1, arg2, arg3, arg4, arg5) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5);

#define YAGL_GLES_DRIVER_FUNC7(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6);

#define YAGL_GLES_DRIVER_FUNC8(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7);

#define YAGL_GLES_DRIVER_FUNC9(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7, \
     arg8_type arg8);

#define YAGL_GLES_DRIVER_FUNC10(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, arg9_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
void (YAGL_GLES_APIENTRY *func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7, \
     arg8_type arg8, arg9_type arg9);

/* We need this because we can't include <GL/gl.h> (which has GLdouble
 * definition) and <GLES/gl.h> (which has GLfixed definition) simultaneously */
typedef double yagl_GLdouble;

/*
 * YaGL GLES driver.
 * @{
 */

struct yagl_gles_driver
{
    /*
     * OpenGL 2.1
     * @{
     */

    YAGL_GLES_DRIVER_FUNC3(DrawArrays, GLenum, GLint, GLsizei, mode, first, count)
    YAGL_GLES_DRIVER_FUNC4(DrawElements, GLenum, GLsizei, GLenum, const GLvoid*, mode, count, type, indices)
    YAGL_GLES_DRIVER_FUNC7(ReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, x, y, width, height, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC1(DisableVertexAttribArray, GLuint, index)
    YAGL_GLES_DRIVER_FUNC1(EnableVertexAttribArray, GLuint, index)
    YAGL_GLES_DRIVER_FUNC6(VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*, indx, size, type, normalized, stride, ptr)
    YAGL_GLES_DRIVER_FUNC4(VertexPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC3(NormalPointer, GLenum, GLsizei, const GLvoid*, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC4(ColorPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC4(TexCoordPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC1(DisableClientState, GLenum, array)
    YAGL_GLES_DRIVER_FUNC1(EnableClientState, GLenum, array)
    YAGL_GLES_DRIVER_FUNC2(GenBuffers, GLsizei, GLuint*, n, buffers)
    YAGL_GLES_DRIVER_FUNC2(BindBuffer, GLenum, GLuint, target, buffer)
    YAGL_GLES_DRIVER_FUNC4(BufferData, GLenum, GLsizeiptr, const GLvoid*, GLenum, target, size, data, usage)
    YAGL_GLES_DRIVER_FUNC4(BufferSubData, GLenum, GLintptr, GLsizeiptr, const GLvoid*, target, offset, size, data)
    YAGL_GLES_DRIVER_FUNC2(DeleteBuffers, GLsizei, const GLuint*, n, buffers)
    YAGL_GLES_DRIVER_FUNC2(GenTextures, GLsizei, GLuint*, n, textures)
    YAGL_GLES_DRIVER_FUNC2(BindTexture, GLenum, GLuint, target, texture)
    YAGL_GLES_DRIVER_FUNC2(DeleteTextures, GLsizei, const GLuint*, n, textures)
    YAGL_GLES_DRIVER_FUNC1(ActiveTexture, GLenum, texture)
    YAGL_GLES_DRIVER_FUNC8(CompressedTexImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*, target, level, internalformat, width, height, border, imageSize, data)
    YAGL_GLES_DRIVER_FUNC9(CompressedTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    YAGL_GLES_DRIVER_FUNC8(CopyTexImage2D, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, target, level, internalformat, x, y, width, height, border)
    YAGL_GLES_DRIVER_FUNC8(CopyTexSubImage2D, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, target, level, xoffset, yoffset, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC3(GetTexParameterfv, GLenum, GLenum, GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC9(TexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*, target, level, internalformat, width, height, border, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC3(TexParameterf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexParameterfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(TexParameteri, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexParameteriv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC9(TexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*, target, level, xoffset, yoffset, width, height, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC1(ClientActiveTexture, GLenum, texture)
    YAGL_GLES_DRIVER_FUNC3(TexEnvi, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexEnvf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_DRIVER_FUNC5(MultiTexCoord4f, GLenum, GLfloat, GLfloat, GLfloat, GLfloat, target, s, t, r, q)
    YAGL_GLES_DRIVER_FUNC3(TexEnviv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(TexEnvfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexEnviv, GLenum, GLenum, GLint*, env, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexEnvfv, GLenum, GLenum, GLfloat*, env, pname, params)
    YAGL_GLES_DRIVER_FUNC2(GenFramebuffers, GLsizei, GLuint*, n, framebuffers)
    YAGL_GLES_DRIVER_FUNC2(BindFramebuffer, GLenum, GLuint, target, framebuffer)
    YAGL_GLES_DRIVER_FUNC5(FramebufferTexture2D, GLenum, GLenum, GLenum, GLuint, GLint, target, attachment, textarget, texture, level)
    YAGL_GLES_DRIVER_FUNC4(FramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint, target, attachment, renderbuffertarget, renderbuffer)
    YAGL_GLES_DRIVER_FUNC2(DeleteFramebuffers, GLsizei, const GLuint*, n, framebuffers)
    YAGL_GLES_DRIVER_FUNC10(BlitFramebuffer, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
    YAGL_GLES_DRIVER_FUNC2(DrawBuffers, GLsizei, const GLenum*, n, bufs)
    YAGL_GLES_DRIVER_FUNC2(GenRenderbuffers, GLsizei, GLuint*, n, renderbuffers)
    YAGL_GLES_DRIVER_FUNC2(BindRenderbuffer, GLenum, GLuint, target, renderbuffer)
    YAGL_GLES_DRIVER_FUNC4(RenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei, target, internalformat, width, height)
    YAGL_GLES_DRIVER_FUNC2(DeleteRenderbuffers, GLsizei, const GLuint*, n, renderbuffers)
    YAGL_GLES_DRIVER_FUNC3(GetRenderbufferParameteriv, GLenum, GLenum, GLint*, target, pname, params);
    YAGL_GLES_DRIVER_FUNC_RET0(GLuint, CreateProgram)
    YAGL_GLES_DRIVER_FUNC_RET1(GLuint, CreateShader, GLenum, type)
    YAGL_GLES_DRIVER_FUNC1(DeleteProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC1(DeleteShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC4(ShaderSource, GLuint, GLsizei, const GLchar**, const GLint*, shader, count, string, length)
    YAGL_GLES_DRIVER_FUNC2(AttachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_DRIVER_FUNC2(DetachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_DRIVER_FUNC1(CompileShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC3(BindAttribLocation, GLuint, GLuint, const GLchar*, program, index, name)
    YAGL_GLES_DRIVER_FUNC7(GetActiveAttrib, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_DRIVER_FUNC7(GetActiveUniform, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_DRIVER_FUNC_RET2(int, GetAttribLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_DRIVER_FUNC3(GetProgramiv, GLuint, GLenum, GLint*, program, pname, params)
    YAGL_GLES_DRIVER_FUNC4(GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, program, bufsize, length, infolog)
    YAGL_GLES_DRIVER_FUNC3(GetShaderiv, GLuint, GLenum, GLint*, shader, pname, params)
    YAGL_GLES_DRIVER_FUNC4(GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, infolog)
    YAGL_GLES_DRIVER_FUNC3(GetUniformfv, GLuint, GLint, GLfloat*, program, location, params)
    YAGL_GLES_DRIVER_FUNC3(GetUniformiv, GLuint, GLint, GLint*, program, location, params)
    YAGL_GLES_DRIVER_FUNC_RET2(int, GetUniformLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_DRIVER_FUNC3(GetVertexAttribfv, GLuint, GLenum, GLfloat*, index, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetVertexAttribiv, GLuint, GLenum, GLint*, index, pname, params)
    YAGL_GLES_DRIVER_FUNC1(LinkProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC2(Uniform1f, GLint, GLfloat, location, x)
    YAGL_GLES_DRIVER_FUNC3(Uniform1fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC2(Uniform1i, GLint, GLint, location, x)
    YAGL_GLES_DRIVER_FUNC3(Uniform1iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC3(Uniform2f, GLint, GLfloat, GLfloat, location, x, y)
    YAGL_GLES_DRIVER_FUNC3(Uniform2fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC3(Uniform2i, GLint, GLint, GLint, location, x, y)
    YAGL_GLES_DRIVER_FUNC3(Uniform2iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(Uniform3f, GLint, GLfloat, GLfloat, GLfloat, location, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Uniform3fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(Uniform3i, GLint, GLint, GLint, GLint, location, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Uniform3iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC5(Uniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat, location, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC3(Uniform4fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC5(Uniform4i, GLint, GLint, GLint, GLint, GLint, location, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC3(Uniform4iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(UniformMatrix2fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC4(UniformMatrix3fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC4(UniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC1(UseProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC1(ValidateProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib1f, GLuint, GLfloat, indx, x)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib1fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC3(VertexAttrib2f, GLuint, GLfloat, GLfloat, indx, x, y)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib2fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC4(VertexAttrib3f, GLuint, GLfloat, GLfloat, GLfloat, indx, x, y, z)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib3fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC5(VertexAttrib4f, GLuint, GLfloat, GLfloat, GLfloat, GLfloat, indx, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib4fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC5(GetActiveUniformsiv, GLuint, GLsizei, const GLuint*, GLenum, GLint*, program, uniformCount, uniformIndices, pname, params)
    YAGL_GLES_DRIVER_FUNC4(GetUniformIndices, GLuint, GLsizei, const GLchar* const*, GLuint*, program, uniformCount, uniformNames, uniformIndices)
    YAGL_GLES_DRIVER_FUNC2(GetIntegerv, GLenum, GLint*, pname, params)
    YAGL_GLES_DRIVER_FUNC2(GetFloatv, GLenum, GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC_RET1(const GLubyte*, GetString, GLenum, name)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, IsEnabled, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC1(BlendEquation, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(BlendEquationSeparate, GLenum, GLenum, modeRGB, modeAlpha)
    YAGL_GLES_DRIVER_FUNC2(BlendFunc, GLenum, GLenum, sfactor, dfactor)
    YAGL_GLES_DRIVER_FUNC4(BlendFuncSeparate, GLenum, GLenum, GLenum, GLenum, srcRGB, dstRGB, srcAlpha, dstAlpha)
    YAGL_GLES_DRIVER_FUNC4(BlendColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(Clear, GLbitfield, mask)
    YAGL_GLES_DRIVER_FUNC4(ClearColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(ClearDepth, yagl_GLdouble, depth)
    YAGL_GLES_DRIVER_FUNC1(ClearStencil, GLint, s)
    YAGL_GLES_DRIVER_FUNC4(ColorMask, GLboolean, GLboolean, GLboolean, GLboolean, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(CullFace, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC1(DepthFunc, GLenum, func)
    YAGL_GLES_DRIVER_FUNC1(DepthMask, GLboolean, flag)
    YAGL_GLES_DRIVER_FUNC2(DepthRange, yagl_GLdouble, yagl_GLdouble, zNear, zFar)
    YAGL_GLES_DRIVER_FUNC1(Enable, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC1(Disable, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC0(Flush)
    YAGL_GLES_DRIVER_FUNC1(FrontFace, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC1(GenerateMipmap, GLenum, target)
    YAGL_GLES_DRIVER_FUNC2(Hint, GLenum, GLenum, target, mode)
    YAGL_GLES_DRIVER_FUNC1(LineWidth, GLfloat, width)
    YAGL_GLES_DRIVER_FUNC2(PixelStorei, GLenum, GLint, pname, param)
    YAGL_GLES_DRIVER_FUNC2(PolygonOffset, GLfloat, GLfloat, factor, units)
    YAGL_GLES_DRIVER_FUNC4(Scissor, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC3(StencilFunc, GLenum, GLint, GLuint, func, ref, mask)
    YAGL_GLES_DRIVER_FUNC1(StencilMask, GLuint, mask)
    YAGL_GLES_DRIVER_FUNC3(StencilOp, GLenum, GLenum, GLenum, fail, zfail, zpass)
    YAGL_GLES_DRIVER_FUNC2(SampleCoverage, GLclampf, GLboolean, value, invert)
    YAGL_GLES_DRIVER_FUNC4(Viewport, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC4(StencilFuncSeparate, GLenum, GLenum, GLint, GLuint, face, func, ref, mask)
    YAGL_GLES_DRIVER_FUNC2(StencilMaskSeparate, GLenum, GLuint, face, mask)
    YAGL_GLES_DRIVER_FUNC4(StencilOpSeparate, GLenum, GLenum, GLenum, GLenum, face, fail, zfail, zpass)
    YAGL_GLES_DRIVER_FUNC1(PointSize, GLfloat, size)
    YAGL_GLES_DRIVER_FUNC2(AlphaFunc, GLenum, GLclampf, func, ref)
    YAGL_GLES_DRIVER_FUNC1(MatrixMode, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC0(LoadIdentity)
    YAGL_GLES_DRIVER_FUNC0(PopMatrix)
    YAGL_GLES_DRIVER_FUNC0(PushMatrix)
    YAGL_GLES_DRIVER_FUNC4(Rotatef, GLfloat, GLfloat, GLfloat, GLfloat, angle, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Translatef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Scalef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_DRIVER_FUNC6(Ortho, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, left, right, bottom, top, znear, zfar)
    YAGL_GLES_DRIVER_FUNC4(Color4f, GLfloat, GLfloat, GLfloat, GLfloat, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC4(Color4ub, GLubyte, GLubyte, GLubyte, GLubyte, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC3(Normal3f, GLfloat, GLfloat, GLfloat, nx, ny, nz)
    YAGL_GLES_DRIVER_FUNC2(PointParameterf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(PointParameterfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC2(Fogf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(Fogfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC6(Frustum, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, left, right, bottom, top, znear, zfar)
    YAGL_GLES_DRIVER_FUNC3(Lightf, GLenum, GLenum, GLfloat, light, pname, param)
    YAGL_GLES_DRIVER_FUNC3(Lightfv, GLenum, GLenum, const GLfloat*, light, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetLightfv, GLenum, GLenum, GLfloat*, light, pname, params)
    YAGL_GLES_DRIVER_FUNC2(LightModelf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(LightModelfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(Materialf, GLenum, GLenum, GLfloat, face, pname, param)
    YAGL_GLES_DRIVER_FUNC3(Materialfv, GLenum, GLenum, const GLfloat*, face, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetMaterialfv, GLenum, GLenum, GLfloat*, face, pname, params)
    YAGL_GLES_DRIVER_FUNC1(ShadeModel, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC1(LogicOp, GLenum, opcode)
    YAGL_GLES_DRIVER_FUNC1(MultMatrixf, const GLfloat*, m)
    YAGL_GLES_DRIVER_FUNC1(LoadMatrixf, const GLfloat*, m)
    YAGL_GLES_DRIVER_FUNC2(ClipPlane, GLenum, const yagl_GLdouble *, plane, equation)
    YAGL_GLES_DRIVER_FUNC2(GetClipPlane, GLenum, const yagl_GLdouble *, plane, equation)
    YAGL_GLES_DRIVER_FUNC_RET2(void*, MapBuffer, GLenum, GLenum, target, access)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, UnmapBuffer, GLenum, target)
    YAGL_GLES_DRIVER_FUNC0(Finish)

    /*
     * @}
     */

    /*
     * OpenGL 3.1+ core.
     */

    YAGL_GLES_DRIVER_FUNC_RET2(const GLubyte*, GetStringi, GLenum, GLuint, name, index)
    YAGL_GLES_DRIVER_FUNC2(GenVertexArrays, GLsizei, GLuint*, n, arrays)
    YAGL_GLES_DRIVER_FUNC1(BindVertexArray, GLuint, array)
    YAGL_GLES_DRIVER_FUNC2(DeleteVertexArrays, GLsizei, const GLuint*, n, arrays)

    /*
     * @}
     */

    yagl_gl_version gl_version;

    void (*destroy)(struct yagl_gles_driver */*driver*/);
};

void yagl_gles_driver_init(struct yagl_gles_driver *driver,
                           yagl_gl_version gl_version);
void yagl_gles_driver_cleanup(struct yagl_gles_driver *driver);

/*
 * @}
 */

/*
 * Helpers for ensuring context.
 * @{
 */

uint32_t yagl_get_ctx_id(void);
void yagl_ensure_ctx(uint32_t ctx_id);
void yagl_unensure_ctx(uint32_t ctx_id);

/*
 * @}
 */

#endif
