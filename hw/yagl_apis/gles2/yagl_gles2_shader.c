#include <GLES2/gl2.h>
#include "yagl_gles2_shader.h"
#include "yagl_gles2_driver.h"

struct yagl_gles2_shader_strtok
{
    char *buffer;
    int buffersize;
    const char *prev;
};

static void yagl_gles2_shader_destroy(struct yagl_ref *ref)
{
    struct yagl_gles2_shader *shader = (struct yagl_gles2_shader*)ref;

    yagl_ensure_ctx();
    shader->driver->DeleteShader(shader->global_name);
    yagl_unensure_ctx();

    yagl_object_cleanup(&shader->base);

    g_free(shader);
}

struct yagl_gles2_shader
    *yagl_gles2_shader_create(struct yagl_gles2_driver *driver,
                              GLenum type)
{
    GLuint global_name;
    struct yagl_gles2_shader *shader;

    global_name = driver->CreateShader(type);

    if (global_name == 0) {
        return NULL;
    }

    shader = g_malloc0(sizeof(*shader));

    yagl_object_init(&shader->base, &yagl_gles2_shader_destroy);

    shader->is_shader = true;
    shader->driver = driver;
    shader->global_name = global_name;
    shader->type = type;

    return shader;
}

static const char *g_delim = " \t\n\r()[]{},;?:/%*&|^!+-=<>";

static const char *yagl_gles2_shader_opengl_strtok(struct yagl_gles2_shader_strtok *st,
                                                   const char *s,
                                                   int *n)
{
    if (!s) {
        if (!*(st->prev) || !*n) {
            if (st->buffer) {
                g_free(st->buffer);
                st->buffer = 0;
                st->buffersize = -1;
            }
            st->prev = 0;
            return 0;
        }
        s = st->prev;
    } else {
        if (st->buffer) {
            g_free(st->buffer);
            st->buffer = 0;
            st->buffersize = -1;
        }
        st->prev = s;
    }
    for (; *n && strchr(g_delim, *s); s++, (*n)--) {
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
    if (s > st->prev) {
        s = st->prev;
    } else {
        for (; *n && *e && !strchr(g_delim, *e); e++, (*n)--);
    }
    st->prev = e;
    if (st->buffersize < e - s) {
        st->buffersize = e - s;
        if (st->buffer) {
            g_free(st->buffer);
        }
        st->buffer = g_malloc(st->buffersize + 1);
    }
    /* never return comment fields so caller does not need to handle them */
    char *p = st->buffer;
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
    return st->buffer;
}

static char *yagl_gles2_shader_patch(const char *source,
                                     int length,
                                     int *patched_len)
{
    /* DISCLAIMER: this is not a full-blown shader parser but a simple
     * implementation which tries to remove the OpenGL ES shader
     * "precision" statements and precision qualifiers "lowp", "mediump"
     * and "highp" from the specified shader source. It also replaces
     * OpenGL ES shading language built-in constants gl_MaxVertexUniformVectors,
     * gl_MaxFragmentUniformVectors and gl_MaxVaryingVectors with corresponding
     * values from OpenGL shading language. */

    struct yagl_gles2_shader_strtok st;
    char *sp;

    st.buffer = NULL;
    st.buffersize = -1;
    st.prev = NULL;

    if (!length) {
        length = strlen(source);
    }
    *patched_len = 0;
    int patched_size = length;
    char *patched = g_malloc(patched_size + 1);
    const char *p = yagl_gles2_shader_opengl_strtok(&st, source, &length);
    for (; p; p = yagl_gles2_shader_opengl_strtok(&st, 0, &length)) {
        if (!strcmp(p, "lowp") || !strcmp(p, "mediump") || !strcmp(p, "highp")) {
            continue;
        } else if (!strcmp(p, "precision")) {
            while ((p = yagl_gles2_shader_opengl_strtok(&st, 0, &length)) && !strchr(p, ';'));
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
                patched = g_realloc(patched, patched_size + 1);
            }
            memcpy(patched + *patched_len, p, new_len);
            *patched_len += new_len;
        }
    }
    patched[*patched_len] = 0;
    /* check that we don't leave dummy preprocessor lines */
    for (sp = patched; *sp;) {
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

    g_free(st.buffer);

    return patched;
}

void yagl_gles2_shader_source(struct yagl_gles2_shader *shader,
                              const GLchar *string)
{
    const GLchar *strings[1];
    GLint patched_len = 0;
    GLchar *patched_string = yagl_gles2_shader_patch(string,
                                                     strlen(string),
                                                     &patched_len);

    /*
     * On some GPUs (like Ivybridge Desktop) it's necessary to add
     * "#version" directive as the first line of the shader, otherwise
     * some of the features might not be available to the shader.
     *
     * For example, on Ivybridge Desktop, if we don't add the "#version"
     * line to the fragment shader then "gl_PointCoord"
     * won't be available.
     */

    if (strstr(patched_string, "#version") == NULL) {
        patched_len += sizeof("#version 120\n\n");
        char *tmp = g_malloc(patched_len);
        strcpy(tmp, "#version 120\n\n");
        strcat(tmp, patched_string);
        g_free(patched_string);
        patched_string = tmp;
    }

    strings[0] = patched_string;

    shader->driver->ShaderSource(shader->global_name,
                                 1,
                                 strings,
                                 &patched_len);

    g_free(patched_string);
}

void yagl_gles2_shader_compile(struct yagl_gles2_shader *shader)
{
    shader->driver->CompileShader(shader->global_name);
}

void yagl_gles2_shader_get_param(struct yagl_gles2_shader *shader,
                                 GLenum pname,
                                 GLint *param)
{
    shader->driver->GetShaderiv(shader->global_name,
                                pname,
                                param);
}

void yagl_gles2_shader_get_source(struct yagl_gles2_shader *shader,
                                  GLsizei bufsize,
                                  GLsizei *length,
                                  GLchar *source)
{
    shader->driver->GetShaderSource(shader->global_name,
                                    bufsize,
                                    length,
                                    source);
}

void yagl_gles2_shader_get_info_log(struct yagl_gles2_shader *shader,
                                    GLsizei bufsize,
                                    GLsizei *length,
                                    GLchar *infolog)
{
    shader->driver->GetShaderInfoLog(shader->global_name,
                                     bufsize,
                                     length,
                                     infolog);
}

void yagl_gles2_shader_acquire(struct yagl_gles2_shader *shader)
{
    if (shader) {
        yagl_object_acquire(&shader->base);
    }
}

void yagl_gles2_shader_release(struct yagl_gles2_shader *shader)
{
    if (shader) {
        yagl_object_release(&shader->base);
    }
}
