#ifndef _QEMU_YAGL_HOST_GLES2_CALLS_H
#define _QEMU_YAGL_HOST_GLES2_CALLS_H

#include "yagl_api.h"
#include <GLES2/gl2.h>
#include "yagl_apis/gles/yagl_host_gles_calls.h"

struct yagl_api_ps *yagl_host_gles2_process_init(struct yagl_api *api);

void yagl_host_glAttachShader(GLuint program,
    GLuint shader);
void yagl_host_glBindAttribLocation(GLuint program,
    GLuint index,
    const GLchar *name, int32_t name_count);
void yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha);
void yagl_host_glCompileShader(GLuint shader);
GLuint yagl_host_glCreateProgram(void);
GLuint yagl_host_glCreateShader(GLenum type);
void yagl_host_glDeleteProgram(GLuint program);
void yagl_host_glDeleteShader(GLuint shader);
void yagl_host_glDetachShader(GLuint program,
    GLuint shader);
void yagl_host_glDisableVertexAttribArray(GLuint index);
void yagl_host_glEnableVertexAttribArray(GLuint index);
void yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count);
void yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count);
void yagl_host_glGetAttachedShaders(GLuint program,
    GLuint *shaders, int32_t shaders_maxcount, int32_t *shaders_count);
int yagl_host_glGetAttribLocation(GLuint program,
    const GLchar *name, int32_t name_count);
void yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    GLint *param);
void yagl_host_glGetProgramInfoLog(GLuint program,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count);
void yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    GLint *param);
void yagl_host_glGetShaderInfoLog(GLuint shader,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count);
void yagl_host_glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype,
    GLint *range, int32_t range_maxcount, int32_t *range_count,
    GLint *precision);
void yagl_host_glGetShaderSource(GLuint shader,
    GLchar *source, int32_t source_maxcount, int32_t *source_count);
void yagl_host_glGetUniformfv(GLuint program,
    GLint location,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetUniformiv(GLuint program,
    GLint location,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
int yagl_host_glGetUniformLocation(GLuint program,
    const GLchar *name, int32_t name_count);
void yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetVertexAttribPointerv(GLuint index,
    GLenum pname,
    target_ulong *pointer);
GLboolean yagl_host_glIsProgram(GLuint program);
GLboolean yagl_host_glIsShader(GLuint shader);
void yagl_host_glLinkProgram(GLuint program);
void yagl_host_glReleaseShaderCompiler(void);
void yagl_host_glShaderBinary(const GLuint *shaders, int32_t shaders_count,
    GLenum binaryformat,
    const GLvoid *binary, int32_t binary_count);
void yagl_host_glShaderSource(GLuint shader,
    const GLchar *string, int32_t string_count);
void yagl_host_glStencilFuncSeparate(GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask);
void yagl_host_glStencilMaskSeparate(GLenum face,
    GLuint mask);
void yagl_host_glStencilOpSeparate(GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass);
void yagl_host_glUniform1f(GLint location,
    GLfloat x);
void yagl_host_glUniform1fv(GLint location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform1i(GLint location,
    GLint x);
void yagl_host_glUniform1iv(GLint location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniform2f(GLint location,
    GLfloat x,
    GLfloat y);
void yagl_host_glUniform2fv(GLint location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform2i(GLint location,
    GLint x,
    GLint y);
void yagl_host_glUniform2iv(GLint location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniform3f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glUniform3fv(GLint location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform3i(GLint location,
    GLint x,
    GLint y,
    GLint z);
void yagl_host_glUniform3iv(GLint location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniform4f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
void yagl_host_glUniform4fv(GLint location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform4i(GLint location,
    GLint x,
    GLint y,
    GLint z,
    GLint w);
void yagl_host_glUniform4iv(GLint location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniformMatrix2fv(GLint location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count);
void yagl_host_glUniformMatrix3fv(GLint location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count);
void yagl_host_glUniformMatrix4fv(GLint location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count);
void yagl_host_glUseProgram(GLuint program);
void yagl_host_glValidateProgram(GLuint program);
void yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x);
void yagl_host_glVertexAttrib1fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y);
void yagl_host_glVertexAttrib2fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glVertexAttrib3fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
void yagl_host_glVertexAttrib4fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttribPointer(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    target_ulong ptr);

#endif
