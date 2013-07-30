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

#include <string.h>
#include "hgl.h"

//#define SHADER_TEST_USE_VERSION

void dglShaderInit()
{
	HGL *h = &HGL_TLS;
	degl_tls *tls = deglGetTLS();
#	ifdef SHADER_TEST_USE_VERSION
	const GLubyte *sl_ver = h->GetString(GL_SHADING_LANGUAGE_VERSION);
	Dprintf("Host OpenGL supports shading language \'%s\'\n", sl_ver);
	int slv_major = 0, slv_minor = 0;
	if(sl_ver &&
		sscanf((const char *)sl_ver, "%d.%d", &slv_major, &slv_minor) == 2 &&
		(slv_major > 1 || (slv_major == 1 && slv_minor >= 30)))
	{
		tls->shader_strip_enabled = 0;
	}
	else
	{
		tls->shader_strip_enabled = 1;
	}
#	else
	GLuint shader = h->CreateShader(GL_FRAGMENT_SHADER);
	static const char* shader_test =
//		"#version 120\n"
		"varying lowp vec4 c;\n"
		"void main(void) { gl_FragColor=c; }\n";
	h->ShaderSource(shader, 1, &shader_test, 0);
	GLint status;
	h->CompileShader(shader);
	h->GetShaderiv(shader, GL_COMPILE_STATUS, &status);
	h->DeleteShader(shader);
	if(status == GL_FALSE)
	{
		Dprintf("WARNING: Host OpenGL implementation doesn't understand precision keyword!\n");
		tls->shader_strip_enabled = 1;
	}
	else
	{
		Dprintf("Host OpenGL implementation understands precision keyword.\n");
		tls->shader_strip_enabled = 0;
	}
#	endif
	tls->shader_strtok.buffer = 0;
	tls->shader_strtok.buffersize = -1;
	tls->shader_strtok.prev = 0;
}

GL_APICALL_BUILD void GL_APIENTRY glCompileShader(GLuint shader)
{
	HGL_TLS.CompileShader(shader);
}

GL_APICALL_BUILD GLuint GL_APIENTRY glCreateShader(GLenum type)
{
	return HGL_TLS.CreateShader(type);
}

GL_APICALL_BUILD void GL_APIENTRY glDeleteShader(GLuint shader)
{
	HGL_TLS.DeleteShader(shader);
}

GL_APICALL_BUILD void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
	HGL_TLS.GetShaderiv(shader, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
	if(bufsize > 0) *infolog = 0;
	HGL_TLS.GetShaderInfoLog(shader, bufsize, length, infolog);
}

GL_APICALL_BUILD void GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
	HGL *h = &HGL_TLS;
	if(h->GetShaderPrecisionFormat)
	{
		h->GetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
	}
	else
	{
		switch(precisiontype)
		{
			case GL_LOW_FLOAT:
			case GL_MEDIUM_FLOAT:
			case GL_HIGH_FLOAT:
				range[0] = 127;
				range[1] = 127;
				*precision = 23;
				break;
			case GL_LOW_INT:
			case GL_MEDIUM_INT:
			case GL_HIGH_INT:
				range[0] = 31;
				range[1] = 30;
				*precision = 0;
				break;
			default:
				Dprintf("unknown precisiontype 0x%x\n", precisiontype);
				range[0] = 0;
				range[1] = 0;
				*precision = 0;
				break;
		}
	}
}

GL_APICALL_BUILD void GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
	/* TODO: if the shader was stripped on input, we should return the non-stripped version */
	HGL_TLS.GetShaderSource(shader, bufsize, length, source);
}

GL_APICALL_BUILD GLboolean GL_APIENTRY glIsShader(GLuint shader)
{
	return HGL_TLS.IsShader(shader);
}

GL_APICALL_BUILD void GL_APIENTRY glReleaseShaderCompiler(void)
{
	HGL *h = &HGL_TLS;
	if(h->ReleaseShaderCompiler)
	{
		h->ReleaseShaderCompiler();
	}
	else
	{
		Dprintf("no glReleaseShaderCompiler function found, call ignored\n");
	}
}

GL_APICALL_BUILD void GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length)
{
	Dprintf("binaryFormat = 0x%x\n", binaryformat);
	HGL *h = &HGL_TLS;
	if(h->ShaderBinary)
	{
		h->ShaderBinary(n, shaders, binaryformat, binary, length);
	}
	else
	{
		Dprintf("no glShaderBinary function found, call ignored\n");
	}
}

static const char *opengl_strtok(const char *s, int *n)
{
    static const char *delim = " \t\n\r()[]{},;?:/%*&|^!+-=<>";
    degl_tls *tls = deglGetTLS();
    if (!s) {
        if (!*(tls->shader_strtok.prev) || !*n) {
            if (tls->shader_strtok.buffer) {
                free(tls->shader_strtok.buffer);
                tls->shader_strtok.buffer = 0;
                tls->shader_strtok.buffersize = -1;
            }
            tls->shader_strtok.prev = 0;
            return 0;
        }
        s = tls->shader_strtok.prev;
    } else {
        if (tls->shader_strtok.buffer) {
            free(tls->shader_strtok.buffer);
            tls->shader_strtok.buffer = 0;
            tls->shader_strtok.buffersize = -1;
        }
        tls->shader_strtok.prev = s;
    }
    for (; *n && strchr(delim, *s); s++, (*n)--) {
        if (*s == '/' && *n > 1) {
            if (s[1] == '/') {
                do {
                    s++, (*n)--;
                } while (*n > 1 && s[1] != '\n' && s[1] != '\r');
            } else if (s[1] == '*') {
                do {
                    s++, (*n)--;
                } while (*n > 2 && (s[1] != '*' || s[2] != '/'));
                s++, (*n)--;
            }
        }
    }
    const char *e = s;
    if (s > tls->shader_strtok.prev) {
        s = tls->shader_strtok.prev;
    } else {
        for (; *n && *e && !strchr(delim, *e); e++, (*n)--);
    }
    tls->shader_strtok.prev = e;
    if (tls->shader_strtok.buffersize < e - s) {
        tls->shader_strtok.buffersize = e - s;
        if (tls->shader_strtok.buffer) {
            free(tls->shader_strtok.buffer);
        }
        tls->shader_strtok.buffer = malloc(tls->shader_strtok.buffersize + 1);
        if (!tls->shader_strtok.buffer) {
            Dprintf("out of memory!\n");
            tls->shader_strtok.buffersize = -1;
            tls->glError = GL_OUT_OF_MEMORY;
            return NULL;
        }
    }
    /* never return comment fields so caller does not need to handle them */
    char *p = tls->shader_strtok.buffer;
    int m = e - s;
    while (m > 0) {
        if (*s == '/' && m > 1) {
            if (s[1] == '/') {
                do {
                    s++, m--;
                } while (m > 1 && s[1] != '\n' && s[1] != '\r');
                s++, m--;
                continue;
            } else if (s[1] == '*') {
                do {
                    s++, m--;
                } while (m > 2 && (s[1] != '*' || s[2] != '/'));
                s += 3, m -= 3;
                continue;
            }
        }
        *(p++) = *(s++), m--;
    }
    *p = 0;
    return tls->shader_strtok.buffer;
}

static char *do_eglShaderPatch(const char *source, int length, int *patched_len)
{
    /* DISCLAIMER: this is not a full-blown shader parser but a simple
     * implementation which tries to remove the OpenGL ES shader
     * "precision" statements and precision qualifiers "lowp", "mediump"
     * and "highp" from the specified shader source. It also replaces
     * OpenGL ES shading language built-in constants gl_MaxVertexUniformVectors,
     * gl_MaxFragmentUniformVectors and gl_MaxVaryingVectors with corresponding
     * values from OpenGL shading language. */
    if (!length) {
        length = strlen(source);
    }
    *patched_len = 0;
    int patched_size = length;
    char *patched = malloc(patched_size + 1);
    if (!patched) {
        Dprintf("out of memory!\n");
        deglGetTLS()->glError = GL_OUT_OF_MEMORY;
        return NULL;
    }
    const char *p = opengl_strtok(source, &length);
    for (; p; p = opengl_strtok(0, &length)) {
        if (!strcmp(p, "lowp") || !strcmp(p, "mediump") || !strcmp(p, "highp")) {
            continue;
        } else if (!strcmp(p, "precision")) {
            while ((p = opengl_strtok(0, &length)) && !strchr(p, ';'));
        } else {
            if (!strcmp(p, "gl_MaxVertexUniformVectors")) {
                p = "(gl_MaxVertexUniformComponents / 4)";
            } else if (!strcmp(p, "gl_MaxFragmentUniformVectors")) {
                p = "(gl_MaxFragmentUniformComponents / 4)";
            } else if (!strcmp(p, "gl_MaxVaryingVectors")) {
                p = "(gl_MaxVaryingFloats / 4)";
            }
            int new_len = strlen(p);
            if (*patched_len + new_len > patched_size) {
                patched_size *= 2;
                patched = realloc(patched, patched_size + 1);
                if (!patched) {
                    Dprintf("out of memory!\n");
                    deglGetTLS()->glError = GL_OUT_OF_MEMORY;
                    return NULL;
                }
            }
            memcpy(patched + *patched_len, p, new_len);
            *patched_len += new_len;
        }     
    }
    patched[*patched_len] = 0;
    /* check that we don't leave dummy preprocessor lines */
    for (char *sp = patched; *sp;) {
        for (; *sp == ' ' || *sp == '\t'; sp++);
        if (!strncmp(sp, "#define", 7)) {
            for (p = sp + 7; *p == ' ' || *p == '\t'; p++);
            if (*p == '\n' || *p == '\r' || *p == '/') {
                memset(sp, 0x20, 7);
            }
        }
        for (; *sp && *sp != '\n' && *sp != '\r'; sp++);
        for (; *sp == '\n' || *sp == '\r'; sp++);
    }
    return patched;
}

GL_APICALL_BUILD void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
	degl_tls *tls = deglGetTLS();
	if(string && tls->shader_strip_enabled)
	{
		char **s = malloc(count * sizeof(char *));
		if(!s)
		{
			Dprintf("out of memory!\n");
			tls->glError = GL_OUT_OF_MEMORY;
			return;
		}
		GLint *l = malloc(count * sizeof(GLint));
		if(!l)
		{
			Dprintf("out of memory!\n");
			tls->glError = GL_OUT_OF_MEMORY;
			free(s);
			return;
		}
		for(int i = 0; i < count; ++i)
		{
			GLint len;
			if(length) {
				len = length[i];
				if (len < 0) {
					len = string[i]?strlen(string[i]):0;
				}
			}
			else
				len = string[i]?strlen(string[i]):0;
			if(string[i])
			{
				s[i] = do_eglShaderPatch(string[i], len, &l[i]);
				if(!s[i])
				{
					while(i)
					{
						free(s[--i]);
					}
					free(l);
					free(s);
					return;
				}
			}
			else
			{
				s[i] = NULL;
				l[i] = 0;
			}
			Dprintf("%d(len=%d):\nBEFORE:'%s'\nAFTER :'%s'\n", i, (int)len,
				string[i] ? string[i] : "(null)", s[i] ? s[i] : "(null)");
		}
		HGL_TLS.ShaderSource(shader, count, (const char **)s, l);
		for(int i = 0; i < count; i++)
			free(s[i]);
		free(l);
		free(s);
	}
	else
	{
		HGL_TLS.ShaderSource(shader, count, string, length);
	}
}
