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
 * YaGL GLES driver per-process state.
 * @{
 */

struct yagl_gles_driver
{
    YAGL_GLES_DRIVER_FUNC1(ActiveTexture, GLenum, texture)
    YAGL_GLES_DRIVER_FUNC2(BindBuffer, GLenum, GLuint, target, buffer)
    YAGL_GLES_DRIVER_FUNC2(BindTexture, GLenum, GLuint, target, texture)
    YAGL_GLES_DRIVER_FUNC1(BlendEquation, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(BlendEquationSeparate, GLenum, GLenum, modeRGB, modeAlpha)
    YAGL_GLES_DRIVER_FUNC2(BlendFunc, GLenum, GLenum, sfactor, dfactor)
    YAGL_GLES_DRIVER_FUNC4(BlendFuncSeparate, GLenum, GLenum, GLenum, GLenum, srcRGB, dstRGB, srcAlpha, dstAlpha)
    YAGL_GLES_DRIVER_FUNC4(BufferData, GLenum, GLsizeiptr, const GLvoid*, GLenum, target, size, data, usage)
    YAGL_GLES_DRIVER_FUNC4(BufferSubData, GLenum, GLintptr, GLsizeiptr, const GLvoid*, target, offset, size, data)
    YAGL_GLES_DRIVER_FUNC1(Clear, GLbitfield, mask)
    YAGL_GLES_DRIVER_FUNC4(ClearColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(ClearDepth, yagl_GLdouble, depth)
    YAGL_GLES_DRIVER_FUNC1(ClearStencil, GLint, s)
    YAGL_GLES_DRIVER_FUNC4(ColorMask, GLboolean, GLboolean, GLboolean, GLboolean, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC8(CompressedTexImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*, target, level, internalformat, width, height, border, imageSize, data)
    YAGL_GLES_DRIVER_FUNC9(CompressedTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    YAGL_GLES_DRIVER_FUNC8(CopyTexImage2D, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, target, level, internalformat, x, y, width, height, border)
    YAGL_GLES_DRIVER_FUNC8(CopyTexSubImage2D, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, target, level, xoffset, yoffset, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC1(CullFace, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(DeleteBuffers, GLsizei, const GLuint*, n, buffers)
    YAGL_GLES_DRIVER_FUNC2(DeleteTextures, GLsizei, const GLuint*, n, textures)
    YAGL_GLES_DRIVER_FUNC1(DepthFunc, GLenum, func)
    YAGL_GLES_DRIVER_FUNC1(DepthMask, GLboolean, flag)
    YAGL_GLES_DRIVER_FUNC2(DepthRange, yagl_GLdouble, yagl_GLdouble, zNear, zFar)
    YAGL_GLES_DRIVER_FUNC1(Disable, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC3(DrawArrays, GLenum, GLint, GLsizei, mode, first, count)
    YAGL_GLES_DRIVER_FUNC4(DrawElements, GLenum, GLsizei, GLenum, const GLvoid*, mode, count, type, indices)
    YAGL_GLES_DRIVER_FUNC1(Enable, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC0(Finish)
    YAGL_GLES_DRIVER_FUNC0(Flush)
    YAGL_GLES_DRIVER_FUNC1(FrontFace, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(GenBuffers, GLsizei, GLuint*, n, buffers)
    YAGL_GLES_DRIVER_FUNC2(GenTextures, GLsizei, GLuint*, n, textures)
    YAGL_GLES_DRIVER_FUNC2(GetBooleanv, GLenum, GLboolean*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetBufferParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC_RET0(GLenum, GetError)
    YAGL_GLES_DRIVER_FUNC2(GetFloatv, GLenum, GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC2(GetIntegerv, GLenum, GLint*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexParameterfv, GLenum, GLenum, GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC2(Hint, GLenum, GLenum, target, mode)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, IsBuffer, GLuint, buffer)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, IsEnabled, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, IsTexture, GLuint, texture)
    YAGL_GLES_DRIVER_FUNC1(LineWidth, GLfloat, width)
    YAGL_GLES_DRIVER_FUNC2(PixelStorei, GLenum, GLint, pname, param)
    YAGL_GLES_DRIVER_FUNC2(PolygonOffset, GLfloat, GLfloat, factor, units)
    YAGL_GLES_DRIVER_FUNC7(ReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, x, y, width, height, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC2(SampleCoverage, GLclampf, GLboolean, value, invert)
    YAGL_GLES_DRIVER_FUNC4(Scissor, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC3(StencilFunc, GLenum, GLint, GLuint, func, ref, mask)
    YAGL_GLES_DRIVER_FUNC1(StencilMask, GLuint, mask)
    YAGL_GLES_DRIVER_FUNC3(StencilOp, GLenum, GLenum, GLenum, fail, zfail, zpass)
    YAGL_GLES_DRIVER_FUNC9(TexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*, target, level, internalformat, width, height, border, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC3(TexParameterf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexParameterfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(TexParameteri, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexParameteriv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC9(TexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*, target, level, xoffset, yoffset, width, height, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC4(Viewport, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC1(PushClientAttrib, GLbitfield, mask)
    YAGL_GLES_DRIVER_FUNC0(PopClientAttrib)
    YAGL_GLES_DRIVER_FUNC_RET2(void*, MapBuffer, GLenum, GLenum, target, access)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, UnmapBuffer, GLenum, target)
    YAGL_GLES_DRIVER_FUNC2(GenFramebuffers, GLsizei, GLuint*, n, framebuffers)
    YAGL_GLES_DRIVER_FUNC2(DeleteFramebuffers, GLsizei, const GLuint*, n, framebuffers)
    YAGL_GLES_DRIVER_FUNC2(GenRenderbuffers, GLsizei, GLuint*, n, renderbuffers)
    YAGL_GLES_DRIVER_FUNC2(DeleteRenderbuffers, GLsizei, const GLuint*, n, renderbuffers)
    YAGL_GLES_DRIVER_FUNC2(BindFramebuffer, GLenum, GLuint, target, framebuffer)
    YAGL_GLES_DRIVER_FUNC2(BindRenderbuffer, GLenum, GLuint, target, renderbuffer)
    YAGL_GLES_DRIVER_FUNC5(FramebufferTexture2D, GLenum, GLenum, GLenum, GLuint, GLint, target, attachment, textarget, texture, level)
    YAGL_GLES_DRIVER_FUNC4(RenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei, target, internalformat, width, height)
    YAGL_GLES_DRIVER_FUNC4(FramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint, target, attachment, renderbuffertarget, renderbuffer)
    YAGL_GLES_DRIVER_FUNC_RET1(GLenum, CheckFramebufferStatus, GLenum, target)
    YAGL_GLES_DRIVER_FUNC1(GenerateMipmap, GLenum, target)
    YAGL_GLES_DRIVER_FUNC_RET1(const GLubyte*, GetString, GLenum, name)
    YAGL_GLES_DRIVER_FUNC4(GetFramebufferAttachmentParameteriv, GLenum, GLenum, GLenum, GLint*, target, attachment, pname, params);
    YAGL_GLES_DRIVER_FUNC3(GetRenderbufferParameteriv, GLenum, GLenum, GLint*, target, pname, params);
    YAGL_GLES_DRIVER_FUNC10(BlitFramebuffer, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
};

void yagl_gles_driver_init(struct yagl_gles_driver *driver);
void yagl_gles_driver_cleanup(struct yagl_gles_driver *driver);

/*
 * @}
 */

/*
 * Helpers for ensuring context.
 * @{
 */

void yagl_ensure_ctx(void);
void yagl_unensure_ctx(void);

/*
 * @}
 */

#endif
