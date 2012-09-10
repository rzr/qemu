#include <GL/gl.h>
#include "yagl_gles_ogl.h"
#include "yagl_gles_ogl_macros.h"
#include "yagl_log.h"
#include "yagl_dyn_lib.h"
#include "yagl_process.h"
#include <GL/glx.h>

struct yagl_gles_ogl
{
    YAGL_GLES_OGL_PROC1(glActiveTexture, GLenum, texture)
    YAGL_GLES_OGL_PROC2(glBindBuffer, GLenum, GLuint, target, buffer)
    YAGL_GLES_OGL_PROC2(glBindTexture, GLenum, GLuint, target, texture)
    YAGL_GLES_OGL_PROC2(glBlendFunc, GLenum, GLenum, sfactor, dfactor)
    YAGL_GLES_OGL_PROC4(glBufferData, GLenum, GLsizeiptr, const GLvoid*, GLenum, target, size, data, usage)
    YAGL_GLES_OGL_PROC4(glBufferSubData, GLenum, GLintptr, GLsizeiptr, const GLvoid*, target, offset, size, data)
    YAGL_GLES_OGL_PROC1(glClear, GLbitfield, mask)
    YAGL_GLES_OGL_PROC4(glClearColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_OGL_PROC1(glClearDepthf, GLclampf, depth)
    YAGL_GLES_OGL_PROC1(glClearStencil, GLint, s)
    YAGL_GLES_OGL_PROC4(glColorMask, GLboolean, GLboolean, GLboolean, GLboolean, red, green, blue, alpha)
    YAGL_GLES_OGL_PROC8(glCompressedTexImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*, target, level, internalformat, width, height, border, imageSize, data)
    YAGL_GLES_OGL_PROC9(glCompressedTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    YAGL_GLES_OGL_PROC8(glCopyTexImage2D, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, target, level, internalformat, x, y, width, height, border)
    YAGL_GLES_OGL_PROC8(glCopyTexSubImage2D, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, target, level, xoffset, yoffset, x, y, width, height)
    YAGL_GLES_OGL_PROC1(glCullFace, GLenum, mode)
    YAGL_GLES_OGL_PROC2(glDeleteBuffers, GLsizei, const GLuint*, n, buffers)
    YAGL_GLES_OGL_PROC2(glDeleteTextures, GLsizei, const GLuint*, n, textures)
    YAGL_GLES_OGL_PROC1(glDepthFunc, GLenum, func)
    YAGL_GLES_OGL_PROC1(glDepthMask, GLboolean, flag)
    YAGL_GLES_OGL_PROC2(glDepthRangef, GLclampf, GLclampf, zNear, zFar)
    YAGL_GLES_OGL_PROC1(glDisable, GLenum, cap)
    YAGL_GLES_OGL_PROC3(glDrawArrays, GLenum, GLint, GLsizei, mode, first, count)
    YAGL_GLES_OGL_PROC4(glDrawElements, GLenum, GLsizei, GLenum, const GLvoid*, mode, count, type, indices)
    YAGL_GLES_OGL_PROC1(glEnable, GLenum, cap)
    YAGL_GLES_OGL_PROC0(glFinish)
    YAGL_GLES_OGL_PROC0(glFlush)
    YAGL_GLES_OGL_PROC1(glFrontFace, GLenum, mode)
    YAGL_GLES_OGL_PROC2(glGenBuffers, GLsizei, GLuint*, n, buffers)
    YAGL_GLES_OGL_PROC2(glGenTextures, GLsizei, GLuint*, n, textures)
    YAGL_GLES_OGL_PROC2(glGetBooleanv, GLenum, GLboolean*, pname, params)
    YAGL_GLES_OGL_PROC3(glGetBufferParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_OGL_PROC_RET0(GLenum, glGetError)
    YAGL_GLES_OGL_PROC2(glGetFloatv, GLenum, GLfloat*, pname, params)
    YAGL_GLES_OGL_PROC2(glGetIntegerv, GLenum, GLint*, pname, params)
    YAGL_GLES_OGL_PROC3(glGetTexParameterfv, GLenum, GLenum, GLfloat*, target, pname, params)
    YAGL_GLES_OGL_PROC3(glGetTexParameteriv, GLenum, GLenum, GLint*, target, pname, params)
    YAGL_GLES_OGL_PROC2(glHint, GLenum, GLenum, target, mode)
    YAGL_GLES_OGL_PROC_RET1(GLboolean, glIsBuffer, GLuint, buffer)
    YAGL_GLES_OGL_PROC_RET1(GLboolean, glIsEnabled, GLenum, cap)
    YAGL_GLES_OGL_PROC_RET1(GLboolean, glIsTexture, GLuint, texture)
    YAGL_GLES_OGL_PROC1(glLineWidth, GLfloat, width)
    YAGL_GLES_OGL_PROC2(glPixelStorei, GLenum, GLint, pname, param)
    YAGL_GLES_OGL_PROC2(glPolygonOffset, GLfloat, GLfloat, factor, units)
    YAGL_GLES_OGL_PROC7(glReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, x, y, width, height, format, type, pixels)
    YAGL_GLES_OGL_PROC2(glSampleCoverage, GLclampf, GLboolean, value, invert)
    YAGL_GLES_OGL_PROC4(glScissor, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_OGL_PROC3(glStencilFunc, GLenum, GLint, GLuint, func, ref, mask)
    YAGL_GLES_OGL_PROC1(glStencilMask, GLuint, mask)
    YAGL_GLES_OGL_PROC3(glStencilOp, GLenum, GLenum, GLenum, fail, zfail, zpass)
    YAGL_GLES_OGL_PROC9(glTexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*, target, level, internalformat, width, height, border, format, type, pixels)
    YAGL_GLES_OGL_PROC3(glTexParameterf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_OGL_PROC3(glTexParameterfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_OGL_PROC3(glTexParameteri, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_OGL_PROC3(glTexParameteriv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_OGL_PROC9(glTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*, target, level, xoffset, yoffset, width, height, format, type, pixels)
    YAGL_GLES_OGL_PROC4(glViewport, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
    YAGL_GLES_OGL_PROC1(glPushClientAttrib, GLbitfield, mask)
    YAGL_GLES_OGL_PROC0(glPopClientAttrib)
    YAGL_GLES_OGL_PROC_RET2(void*, glMapBuffer, GLenum, GLenum, target, access)
    YAGL_GLES_OGL_PROC_RET1(GLboolean, glUnmapBuffer, GLenum, target)
    YAGL_GLES_OGL_PROC2(glGenFramebuffersEXT, GLsizei, GLuint*, n, framebuffers)
    YAGL_GLES_OGL_PROC2(glDeleteFramebuffersEXT, GLsizei, const GLuint*, n, framebuffers)
    YAGL_GLES_OGL_PROC2(glGenRenderbuffersEXT, GLsizei, GLuint*, n, renderbuffers)
    YAGL_GLES_OGL_PROC2(glDeleteRenderbuffersEXT, GLsizei, const GLuint*, n, renderbuffers)
    YAGL_GLES_OGL_PROC2(glBindFramebufferEXT, GLenum, GLuint, target, framebuffer)
    YAGL_GLES_OGL_PROC2(glBindRenderbufferEXT, GLenum, GLuint, target, renderbuffer)
    YAGL_GLES_OGL_PROC5(glFramebufferTexture2DEXT, GLenum, GLenum, GLenum, GLuint, GLint, target, attachment, textarget, texture, level)
    YAGL_GLES_OGL_PROC4(glRenderbufferStorageEXT, GLenum, GLenum, GLsizei, GLsizei, target, internalformat, width, height)
    YAGL_GLES_OGL_PROC4(glFramebufferRenderbufferEXT, GLenum, GLenum, GLenum, GLuint, target, attachment, renderbuffertarget, renderbuffer)
    YAGL_GLES_OGL_PROC_RET1(GLenum, glCheckFramebufferStatusEXT, GLenum, target)
    YAGL_GLES_OGL_PROC1(glGenerateMipmapEXT, GLenum, target)
};

YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, ActiveTexture, GLenum, texture)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BindBuffer, GLenum, GLuint, target, buffer)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BindTexture, GLenum, GLuint, target, texture)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BlendFunc, GLenum, GLenum, sfactor, dfactor)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BufferData, GLenum, GLsizeiptr, const GLvoid*, GLenum, target, size, data, usage)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BufferSubData, GLenum, GLintptr, GLsizeiptr, const GLvoid*, target, offset, size, data)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Clear, GLbitfield, mask)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, ClearColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, ClearDepthf, GLclampf, depth)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, ClearStencil, GLint, s)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, ColorMask, GLboolean, GLboolean, GLboolean, GLboolean, red, green, blue, alpha)
YAGL_GLES_OGL_PROC_IMPL8(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, CompressedTexImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*, target, level, internalformat, width, height, border, imageSize, data)
YAGL_GLES_OGL_PROC_IMPL9(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, CompressedTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*, target, level, xoffset, yoffset, width, height, format, imageSize, data)
YAGL_GLES_OGL_PROC_IMPL8(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, CopyTexImage2D, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, target, level, internalformat, x, y, width, height, border)
YAGL_GLES_OGL_PROC_IMPL8(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, CopyTexSubImage2D, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, target, level, xoffset, yoffset, x, y, width, height)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, CullFace, GLenum, mode)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DeleteBuffers, GLsizei, const GLuint*, n, buffers)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DeleteTextures, GLsizei, const GLuint*, n, textures)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DepthFunc, GLenum, func)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DepthMask, GLboolean, flag)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DepthRangef, GLclampf, GLclampf, zNear, zFar)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Disable, GLenum, cap)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DrawArrays, GLenum, GLint, GLsizei, mode, first, count)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DrawElements, GLenum, GLsizei, GLenum, const GLvoid*, mode, count, type, indices)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Enable, GLenum, cap)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Finish)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Flush)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, FrontFace, GLenum, mode)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GenBuffers, GLsizei, GLuint*, n, buffers)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GenTextures, GLsizei, GLuint*, n, textures)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GetBooleanv, GLenum, GLboolean*, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GetBufferParameteriv, GLenum, GLenum, GLint*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL_RET0(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GLenum, GetError)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GetFloatv, GLenum, GLfloat*, pname, params)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GetIntegerv, GLenum, GLint*, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GetTexParameterfv, GLenum, GLenum, GLfloat*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GetTexParameteriv, GLenum, GLenum, GLint*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Hint, GLenum, GLenum, target, mode)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GLboolean, IsBuffer, GLuint, buffer)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GLboolean, IsEnabled, GLenum, cap)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GLboolean, IsTexture, GLuint, texture)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, LineWidth, GLfloat, width)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, PixelStorei, GLenum, GLint, pname, param)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, PolygonOffset, GLfloat, GLfloat, factor, units)
YAGL_GLES_OGL_PROC_IMPL7(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, ReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, x, y, width, height, format, type, pixels)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, SampleCoverage, GLclampf, GLboolean, value, invert)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Scissor, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, StencilFunc, GLenum, GLint, GLuint, func, ref, mask)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, StencilMask, GLuint, mask)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, StencilOp, GLenum, GLenum, GLenum, fail, zfail, zpass)
YAGL_GLES_OGL_PROC_IMPL9(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, TexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*, target, level, internalformat, width, height, border, format, type, pixels)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, TexParameterf, GLenum, GLenum, GLfloat, target, pname, param)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, TexParameterfv, GLenum, GLenum, const GLfloat*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, TexParameteri, GLenum, GLenum, GLint, target, pname, param)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, TexParameteriv, GLenum, GLenum, const GLint*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL9(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, TexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*, target, level, xoffset, yoffset, width, height, format, type, pixels)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, Viewport, GLint, GLint, GLsizei, GLsizei, x, y, width, height)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, PushClientAttrib, GLbitfield, mask)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, PopClientAttrib)
YAGL_GLES_OGL_PROC_IMPL_RET2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, void*, MapBuffer, GLenum, GLenum, target, access)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GLboolean, UnmapBuffer, GLenum, target)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GenFramebuffersEXT, GLsizei, GLuint*, n, framebuffers)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DeleteFramebuffersEXT, GLsizei, const GLuint*, n, framebuffers)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GenRenderbuffersEXT, GLsizei, GLuint*, n, renderbuffers)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, DeleteRenderbuffersEXT, GLsizei, const GLuint*, n, renderbuffers)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BindFramebufferEXT, GLenum, GLuint, target, framebuffer)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, BindRenderbufferEXT, GLenum, GLuint, target, renderbuffer)
YAGL_GLES_OGL_PROC_IMPL5(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, FramebufferTexture2DEXT, GLenum, GLenum, GLenum, GLuint, GLint, target, attachment, textarget, texture, level)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, RenderbufferStorageEXT, GLenum, GLenum, GLsizei, GLsizei, target, internalformat, width, height)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, FramebufferRenderbufferEXT, GLenum, GLenum, GLenum, GLuint, target, attachment, renderbuffertarget, renderbuffer)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GLenum, CheckFramebufferStatusEXT, GLenum, target)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles_driver_ps, yagl_gles_ogl_ps, yagl_gles_ogl, GenerateMipmapEXT, GLenum, target)

struct yagl_gles_ogl_ps
    *yagl_gles_ogl_ps_create(struct yagl_gles_ogl *gles_ogl,
                             struct yagl_process_state *ps)
{
    struct yagl_gles_ogl_ps *gles_ogl_ps;

    YAGL_LOG_FUNC_ENTER(ps->id, 0, yagl_gles_ogl_ps_create, NULL);

    gles_ogl_ps = g_malloc0(sizeof(*gles_ogl_ps));

    yagl_gles_driver_ps_init(&gles_ogl_ps->base, ps);
    gles_ogl_ps->driver = gles_ogl;

    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, ActiveTexture);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, BindBuffer);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, BindTexture);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, BlendFunc);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, BufferData);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, BufferSubData);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Clear);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, ClearColor);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, ClearDepthf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, ClearStencil);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, ColorMask);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, CompressedTexImage2D);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, CompressedTexSubImage2D);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, CopyTexImage2D);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, CopyTexSubImage2D);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, CullFace);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DeleteBuffers);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DeleteTextures);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DepthFunc);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DepthMask);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DepthRangef);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Disable);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DrawArrays);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, DrawElements);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Enable);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Finish);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Flush);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, FrontFace);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GenBuffers);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GenTextures);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetBooleanv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetBufferParameteriv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetError);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetFloatv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetIntegerv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetTexParameterfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, GetTexParameteriv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Hint);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, IsBuffer);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, IsEnabled);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, IsTexture);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, LineWidth);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, PixelStorei);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, PolygonOffset);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, ReadPixels);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, SampleCoverage);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Scissor);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, StencilFunc);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, StencilMask);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, StencilOp);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, TexImage2D);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, TexParameterf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, TexParameterfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, TexParameteri);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, TexParameteriv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, TexSubImage2D);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, Viewport);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, PushClientAttrib);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, PopClientAttrib);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, MapBuffer);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles_ogl, gles_ogl_ps, UnmapBuffer);
    gles_ogl_ps->base.GenFramebuffers = &yagl_gles_ogl_GenFramebuffersEXT;
    gles_ogl_ps->base.DeleteFramebuffers = &yagl_gles_ogl_DeleteFramebuffersEXT;
    gles_ogl_ps->base.GenRenderbuffers = &yagl_gles_ogl_GenRenderbuffersEXT;
    gles_ogl_ps->base.DeleteRenderbuffers = &yagl_gles_ogl_DeleteRenderbuffersEXT;
    gles_ogl_ps->base.BindFramebuffer = &yagl_gles_ogl_BindFramebufferEXT;
    gles_ogl_ps->base.BindRenderbuffer = &yagl_gles_ogl_BindRenderbufferEXT;
    gles_ogl_ps->base.FramebufferTexture2D = &yagl_gles_ogl_FramebufferTexture2DEXT;
    gles_ogl_ps->base.RenderbufferStorage = &yagl_gles_ogl_RenderbufferStorageEXT;
    gles_ogl_ps->base.FramebufferRenderbuffer = &yagl_gles_ogl_FramebufferRenderbufferEXT;
    gles_ogl_ps->base.CheckFramebufferStatus = &yagl_gles_ogl_CheckFramebufferStatusEXT;
    gles_ogl_ps->base.GenerateMipmap = &yagl_gles_ogl_GenerateMipmapEXT;

    YAGL_LOG_FUNC_EXIT(NULL);

    return gles_ogl_ps;
}

void yagl_gles_ogl_ps_destroy(struct yagl_gles_ogl_ps *gles_ogl_ps)
{
    YAGL_LOG_FUNC_ENTER(gles_ogl_ps->base.ps->id, 0, yagl_gles_ogl_ps_destroy, NULL);

    yagl_gles_driver_ps_cleanup(&gles_ogl_ps->base);

    g_free(gles_ogl_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles_ogl
    *yagl_gles_ogl_create(struct yagl_dyn_lib *dyn_lib)
{
    struct yagl_gles_ogl *gles_ogl;
    PFNGLXGETPROCADDRESSPROC get_address = NULL;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_gles_ogl_create, NULL);

    gles_ogl = g_malloc0(sizeof(*gles_ogl));

    get_address = yagl_dyn_lib_get_sym(dyn_lib, "glXGetProcAddress");

    if (!get_address) {
        get_address = yagl_dyn_lib_get_sym(dyn_lib, "glXGetProcAddressARB");
    }

    YAGL_GLES_OGL_GET_PROC(gles_ogl, glActiveTexture);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBindBuffer);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBindTexture);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBlendFunc);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBufferData);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBufferSubData);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glClear);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glClearColor);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glClearDepthf);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glClearStencil);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glColorMask);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glCompressedTexImage2D);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glCompressedTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glCopyTexImage2D);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glCopyTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glCullFace);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDeleteBuffers);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDeleteTextures);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDepthFunc);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDepthMask);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDepthRangef);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDisable);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDrawArrays);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDrawElements);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glEnable);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glFinish);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glFlush);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glFrontFace);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGenBuffers);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGenTextures);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetBooleanv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetBufferParameteriv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetError);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetFloatv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetIntegerv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetTexParameterfv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGetTexParameteriv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glHint);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glIsBuffer);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glIsEnabled);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glIsTexture);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glLineWidth);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glPixelStorei);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glPolygonOffset);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glReadPixels);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glSampleCoverage);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glScissor);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glStencilFunc);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glStencilMask);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glStencilOp);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glTexImage2D);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glTexParameterf);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glTexParameterfv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glTexParameteri);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glTexParameteriv);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glViewport);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glPushClientAttrib);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glPopClientAttrib);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glMapBuffer);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glUnmapBuffer);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGenFramebuffersEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDeleteFramebuffersEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGenRenderbuffersEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glDeleteRenderbuffersEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBindFramebufferEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glBindRenderbufferEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glFramebufferTexture2DEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glRenderbufferStorageEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glFramebufferRenderbufferEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glCheckFramebufferStatusEXT);
    YAGL_GLES_OGL_GET_PROC(gles_ogl, glGenerateMipmapEXT);

    YAGL_LOG_FUNC_EXIT(NULL);

    return gles_ogl;

fail:
    g_free(gles_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

void yagl_gles_ogl_destroy(struct yagl_gles_ogl *gles_ogl)
{
    YAGL_LOG_FUNC_ENTER_NPT(yagl_gles_ogl_destroy, NULL);

    g_free(gles_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);
}
