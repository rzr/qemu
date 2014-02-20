/*
 * vigs
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#ifndef _QEMU_VIGS_GL_BACKEND_H
#define _QEMU_VIGS_GL_BACKEND_H

#include "vigs_types.h"
#include "vigs_backend.h"
#include "vigs_vector.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include "winsys_gl.h"

struct work_queue;

struct vigs_gl_backend
{
    struct vigs_backend base;

    struct winsys_gl_info ws_info;

    bool (*has_current)(struct vigs_gl_backend */*gl_backend*/);

    bool (*make_current)(struct vigs_gl_backend */*gl_backend*/,
                         bool /*enable*/);

    bool (*read_pixels_make_current)(struct vigs_gl_backend */*gl_backend*/,
                                     bool /*enable*/);

    /*
     * Mandatory GL functions and extensions.
     * @{
     */

    void (GLAPIENTRY *GenTextures)(GLsizei n, GLuint *textures);
    void (GLAPIENTRY *DeleteTextures)(GLsizei n, const GLuint *textures);
    void (GLAPIENTRY *BindTexture)(GLenum target, GLuint texture);
    void (GLAPIENTRY *CullFace)(GLenum mode);
    void (GLAPIENTRY *TexParameterf)(GLenum target, GLenum pname, GLfloat param);
    void (GLAPIENTRY *TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
    void (GLAPIENTRY *TexParameteri)(GLenum target, GLenum pname, GLint param);
    void (GLAPIENTRY *TexParameteriv)(GLenum target, GLenum pname, const GLint *params);
    void (GLAPIENTRY *TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (GLAPIENTRY *TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (GLAPIENTRY *Clear)(GLbitfield mask);
    void (GLAPIENTRY *ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (GLAPIENTRY *Disable)(GLenum cap);
    void (GLAPIENTRY *Enable)(GLenum cap);
    void (GLAPIENTRY *Finish)(void);
    void (GLAPIENTRY *Flush)(void);
    void (GLAPIENTRY *PixelStorei)(GLenum pname, GLint param);
    void (GLAPIENTRY *ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
    void (GLAPIENTRY *Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (GLAPIENTRY *GenFramebuffers)(GLsizei n, GLuint *framebuffers);
    void (GLAPIENTRY *GenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
    void (GLAPIENTRY *DeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
    void (GLAPIENTRY *DeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
    void (GLAPIENTRY *BindFramebuffer)(GLenum target, GLuint framebuffer);
    void (GLAPIENTRY *BindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (GLAPIENTRY *RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void (GLAPIENTRY *FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (GLAPIENTRY *FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (GLAPIENTRY *GetIntegerv)(GLenum pname, GLint *params);
    void (GLAPIENTRY *DrawArrays)(GLenum mode, GLint first, GLsizei count);
    void (GLAPIENTRY *GenBuffers)(GLsizei n, GLuint *buffers);
    void (GLAPIENTRY *DeleteBuffers)(GLsizei n, const GLuint *buffers);
    void (GLAPIENTRY *BindBuffer)(GLenum target, GLuint buffer);
    void (GLAPIENTRY *BufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
    void (GLAPIENTRY *BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
    GLvoid *(GLAPIENTRY *MapBuffer)(GLenum target, GLenum access);
    GLboolean (GLAPIENTRY *UnmapBuffer)(GLenum target);
    GLuint (GLAPIENTRY* CreateProgram)(void);
    GLuint (GLAPIENTRY* CreateShader)(GLenum type);
    void (GLAPIENTRY* CompileShader)(GLuint shader);
    void (GLAPIENTRY* AttachShader)(GLuint program, GLuint shader);
    void (GLAPIENTRY* LinkProgram)(GLuint program);
    void (GLAPIENTRY* GetProgramiv)(GLuint program, GLenum pname, GLint* params);
    void (GLAPIENTRY* GetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    void (GLAPIENTRY* GetShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void (GLAPIENTRY* GetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    void (GLAPIENTRY* DetachShader)(GLuint program, GLuint shader);
    void (GLAPIENTRY* DeleteProgram)(GLuint program);
    void (GLAPIENTRY* DeleteShader)(GLuint shader);
    void (GLAPIENTRY* DisableVertexAttribArray)(GLuint index);
    void (GLAPIENTRY* EnableVertexAttribArray)(GLuint index);
    void (GLAPIENTRY* ShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
    void (GLAPIENTRY* UseProgram)(GLuint program);
    GLint (GLAPIENTRY* GetAttribLocation)(GLuint program, const GLchar* name);
    GLint (GLAPIENTRY* GetUniformLocation)(GLuint program, const GLchar* name);
    void (GLAPIENTRY* VertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
    void (GLAPIENTRY* Uniform4fv)(GLint location, GLsizei count, const GLfloat* value);
    void (GLAPIENTRY* UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

    /*
     * @}
     */

    /*
     * General purpose vectors.
     * @{
     */

    struct vigs_vector v1;
    struct vigs_vector v2;

    /*
     * @}
     */

    /*
     * Other.
     */

    GLuint tex_prog_vs_id;
    GLuint tex_prog_fs_id;
    GLuint tex_prog_id;
    GLint tex_prog_proj_loc;
    GLint tex_prog_vertCoord_loc;
    GLint tex_prog_texCoord_loc;

    GLuint color_prog_vs_id;
    GLuint color_prog_fs_id;
    GLuint color_prog_id;
    GLint color_prog_proj_loc;
    GLint color_prog_vertCoord_loc;
    GLint color_prog_color_loc;

    GLuint vbo;
    uint32_t vbo_size;

    GLuint cur_prog_id;

    struct work_queue *read_pixels_queue;

    GLuint pbo;
    uint32_t pbo_size;
};

bool vigs_gl_backend_init(struct vigs_gl_backend *gl_backend);

void vigs_gl_backend_cleanup(struct vigs_gl_backend *gl_backend);

#endif
