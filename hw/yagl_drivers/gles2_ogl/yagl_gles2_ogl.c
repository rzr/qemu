#include <GL/gl.h>
#include "yagl_gles2_ogl.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl_macros.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl.h"
#include "yagl_gles2_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_gles2_ogl_destroy(struct yagl_gles2_driver *driver)
{
    YAGL_LOG_FUNC_ENTER(yagl_gles2_ogl_destroy, NULL);

    yagl_gles2_driver_cleanup(driver);
    yagl_gles_ogl_cleanup(&driver->base);
    g_free(driver);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles2_driver *yagl_gles2_ogl_create(struct yagl_dyn_lib *dyn_lib)
{
    struct yagl_gles2_driver *gles2_ogl = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_gles2_ogl_create, NULL);

    gles2_ogl = g_malloc0(sizeof(*gles2_ogl));

    if (!yagl_gles_ogl_init(&gles2_ogl->base, dyn_lib)) {
        goto fail_free;
    }

    yagl_gles2_driver_init(gles2_ogl);

    YAGL_GLES_OGL_GET_PROC(gles2_ogl, AttachShader, glAttachShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, BindAttribLocation, glBindAttribLocation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, BlendColor, glBlendColor);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, CompileShader, glCompileShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, CreateProgram, glCreateProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, CreateShader, glCreateShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, DeleteProgram, glDeleteProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, DeleteShader, glDeleteShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, DetachShader, glDetachShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, DisableVertexAttribArray, glDisableVertexAttribArray);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, EnableVertexAttribArray, glEnableVertexAttribArray);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetActiveAttrib, glGetActiveAttrib);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetActiveUniform, glGetActiveUniform);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetAttachedShaders, glGetAttachedShaders);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetAttribLocation, glGetAttribLocation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetProgramiv, glGetProgramiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetProgramInfoLog, glGetProgramInfoLog);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetShaderiv, glGetShaderiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetShaderInfoLog, glGetShaderInfoLog);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetShaderPrecisionFormat, glGetShaderPrecisionFormat);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetShaderSource, glGetShaderSource);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetUniformfv, glGetUniformfv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetUniformiv, glGetUniformiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetUniformLocation, glGetUniformLocation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetVertexAttribfv, glGetVertexAttribfv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetVertexAttribiv, glGetVertexAttribiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, GetVertexAttribPointerv, glGetVertexAttribPointerv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, IsProgram, glIsProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, IsShader, glIsShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, LinkProgram, glLinkProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, ShaderSource, glShaderSource);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, StencilFuncSeparate, glStencilFuncSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, StencilMaskSeparate, glStencilMaskSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, StencilOpSeparate, glStencilOpSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform1f, glUniform1f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform1fv, glUniform1fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform1i, glUniform1i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform1iv, glUniform1iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform2f, glUniform2f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform2fv, glUniform2fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform2i, glUniform2i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform2iv, glUniform2iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform3f, glUniform3f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform3fv, glUniform3fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform3i, glUniform3i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform3iv, glUniform3iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform4f, glUniform4f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform4fv, glUniform4fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform4i, glUniform4i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, Uniform4iv, glUniform4iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, UniformMatrix2fv, glUniformMatrix2fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, UniformMatrix3fv, glUniformMatrix3fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, UniformMatrix4fv, glUniformMatrix4fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, UseProgram, glUseProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, ValidateProgram, glValidateProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib1f, glVertexAttrib1f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib1fv, glVertexAttrib1fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib2f, glVertexAttrib2f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib2fv, glVertexAttrib2fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib3f, glVertexAttrib3f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib3fv, glVertexAttrib3fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib4f, glVertexAttrib4f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttrib4fv, glVertexAttrib4fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, VertexAttribPointer, glVertexAttribPointer);

    gles2_ogl->destroy = &yagl_gles2_ogl_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return gles2_ogl;

fail:
    yagl_gles2_driver_cleanup(gles2_ogl);
    yagl_gles_ogl_cleanup(&gles2_ogl->base);
fail_free:
    g_free(gles2_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
