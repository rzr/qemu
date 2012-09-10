#ifndef _QEMU_YAGL_GLES_DRIVER_H
#define _QEMU_YAGL_GLES_DRIVER_H

#include "yagl_types.h"

#define YAGL_GLES_DRIVER_FUNC0(obj, func) \
void (*func)(obj);

#define YAGL_GLES_DRIVER_FUNC_RET0(obj, ret_type, func) \
ret_type (*func)(obj);

#define YAGL_GLES_DRIVER_FUNC1(obj, func, arg0_type, arg0) \
void (*func)(obj, \
    arg0_type arg0);

#define YAGL_GLES_DRIVER_FUNC_RET1(obj, ret_type, func, arg0_type, arg0) \
ret_type (*func)(obj, \
    arg0_type arg0);

#define YAGL_GLES_DRIVER_FUNC2(obj, func, arg0_type, arg1_type, arg0, arg1) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1);

#define YAGL_GLES_DRIVER_FUNC_RET2(obj, ret_type, func, arg0_type, arg1_type, arg0, arg1) \
ret_type (*func)(obj, \
     arg0_type arg0, arg1_type arg1);

#define YAGL_GLES_DRIVER_FUNC3(obj, func, arg0_type, arg1_type, arg2_type, arg0, arg1, arg2) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2);

#define YAGL_GLES_DRIVER_FUNC4(obj, func, arg0_type, arg1_type, arg2_type, arg3_type, arg0, arg1, arg2, arg3) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3);

#define YAGL_GLES_DRIVER_FUNC5(obj, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg0, arg1, arg2, arg3, arg4) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4);

#define YAGL_GLES_DRIVER_FUNC6(obj, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg0, arg1, arg2, arg3, arg4, arg5) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5);

#define YAGL_GLES_DRIVER_FUNC7(obj, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6);

#define YAGL_GLES_DRIVER_FUNC8(obj, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7);

#define YAGL_GLES_DRIVER_FUNC9(obj, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
void (*func)(obj, \
     arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7, \
     arg8_type arg8);

struct yagl_thread_state;
struct yagl_process_state;

/*
 * YaGL GLES driver per-process state.
 * @{
 */

struct yagl_gles_driver_ps
{
    struct yagl_process_state *ps;

    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, ActiveTexture, GLenum, texture)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, BindBuffer, GLenum, GLuint, target, buffer)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, BindTexture, GLenum, GLuint, target, texture)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, BlendFunc, GLenum, GLenum, sfactor, dfactor)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, BufferData, GLenum, GLsizeiptr, const GLvoid*, GLenum, target, size, data, usage)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, BufferSubData, GLenum, GLintptr, GLsizeiptr, const GLvoid*, target, offset, size, data)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, Clear, GLbitfield, mask)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, ClearColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, ClearDepthf, GLclampf, depth)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, ClearStencil, GLint, s)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, ColorMask, GLboolean, GLboolean, GLboolean, GLboolean, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC8(struct yagl_gles_driver_ps *driver_ps, CompressedTexImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*, target, level, internalformat, width, height, border, imageSize, data)
    YAGL_GLES_DRIVER_FUNC9(struct yagl_gles_driver_ps *driver_ps, CompressedTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    YAGL_GLES_DRIVER_FUNC8(struct yagl_gles_driver_ps *driver_ps, CopyTexImage2D, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, target, level, internalformat, x, y, width, height, border)
    YAGL_GLES_DRIVER_FUNC8(struct yagl_gles_driver_ps *driver_ps, CopyTexSubImage2D, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, target, level, xoffset, yoffset, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, CullFace, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, DeleteBuffers, GLsizei, const GLuint*, n, buffers)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, DeleteTextures, GLsizei, const GLuint*, n, textures)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, DepthFunc, GLenum, func)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, DepthMask, GLboolean, flag)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, DepthRangef, GLclampf, GLclampf, zNear, zFar)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, Disable, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, DrawArrays, GLenum, GLint, GLsizei, mode, first, count)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, DrawElements, GLenum, GLsizei, GLenum, const GLvoid*, mode, count, type, indices)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, Enable, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles_driver_ps *driver_ps, Finish)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles_driver_ps *driver_ps, Flush)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, FrontFace, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GenBuffers, GLsizei, GLuint*, n, buffers)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GenTextures, GLsizei, GLuint*, n, textures)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GetBooleanv, GLenum, GLboolean*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, GetBufferParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC_RET0(struct yagl_gles_driver_ps *driver_ps, GLenum, GetError)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GetFloatv, GLenum, GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GetIntegerv, GLenum, GLint*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, GetTexParameterfv, GLenum, GLenum, GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, GetTexParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, Hint, GLenum, GLenum, target, mode)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles_driver_ps *driver_ps, GLboolean, IsBuffer, GLuint, buffer)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles_driver_ps *driver_ps, GLboolean, IsEnabled, GLenum, cap)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles_driver_ps *driver_ps, GLboolean, IsTexture, GLuint, texture)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, LineWidth, GLfloat, width)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, PixelStorei, GLenum, GLint, pname, param)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, PolygonOffset, GLfloat, GLfloat, factor, units)
    YAGL_GLES_DRIVER_FUNC7(struct yagl_gles_driver_ps *driver_ps, ReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, x, y, width, height, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, SampleCoverage, GLclampf, GLboolean, value, invert)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, Scissor, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, StencilFunc, GLenum, GLint, GLuint, func, ref, mask)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, StencilMask, GLuint, mask)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, StencilOp, GLenum, GLenum, GLenum, fail, zfail, zpass)
    YAGL_GLES_DRIVER_FUNC9(struct yagl_gles_driver_ps *driver_ps, TexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*, target, level, internalformat, width, height, border, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, TexParameterf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, TexParameterfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, TexParameteri, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles_driver_ps *driver_ps, TexParameteriv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC9(struct yagl_gles_driver_ps *driver_ps, TexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*, target, level, xoffset, yoffset, width, height, format, type, pixels)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, Viewport, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, PushClientAttrib, GLbitfield, mask)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles_driver_ps *driver_ps, PopClientAttrib)
    YAGL_GLES_DRIVER_FUNC_RET2(struct yagl_gles_driver_ps *driver_ps, void*, MapBuffer, GLenum, GLenum, target, access)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles_driver_ps *driver_ps, GLboolean, UnmapBuffer, GLenum, target)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GenFramebuffers, GLsizei, GLuint*, n, framebuffers)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, DeleteFramebuffers, GLsizei, const GLuint*, n, framebuffers)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, GenRenderbuffers, GLsizei, GLuint*, n, renderbuffers)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, DeleteRenderbuffers, GLsizei, const GLuint*, n, renderbuffers)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, BindFramebuffer, GLenum, GLuint, target, framebuffer)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles_driver_ps *driver_ps, BindRenderbuffer, GLenum, GLuint, target, renderbuffer)
    YAGL_GLES_DRIVER_FUNC5(struct yagl_gles_driver_ps *driver_ps, FramebufferTexture2D, GLenum, GLenum, GLenum, GLuint, GLint, target, attachment, textarget, texture, level)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, RenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei, target, internalformat, width, height)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles_driver_ps *driver_ps, FramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint, target, attachment, renderbuffertarget, renderbuffer)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles_driver_ps *driver_ps, GLenum, CheckFramebufferStatus, GLenum, target)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles_driver_ps *driver_ps, GenerateMipmap, GLenum, target)
};

void yagl_gles_driver_ps_init(struct yagl_gles_driver_ps *driver_ps,
                              struct yagl_process_state *ps);
void yagl_gles_driver_ps_cleanup(struct yagl_gles_driver_ps *driver_ps);

/*
 * @}
 */

#endif
