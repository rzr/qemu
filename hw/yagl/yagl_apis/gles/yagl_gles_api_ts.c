/*
 * yagl
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

#include <GL/gl.h>
#include "yagl_gles_api_ts.h"
#include "yagl_gles_api_ps.h"
#include "yagl_gles_api.h"
#include "yagl_gles_driver.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_log.h"

static GLuint yagl_gles_api_ts_create_shader(struct yagl_gles_driver *driver,
                                             const char *source,
                                             GLenum type)
{
    GLuint shader = driver->CreateShader(type);
    GLint tmp = 0;

    YAGL_LOG_FUNC_SET(yagl_gles_api_ts_create_shader);

    if (!shader) {
        YAGL_LOG_ERROR("Unable to create shader type %d", type);
        return 0;
    }

    driver->ShaderSource(shader, 1, &source, NULL);
    driver->CompileShader(shader);
    driver->GetShaderiv(shader, GL_COMPILE_STATUS, &tmp);

    if (!tmp) {
        char *buff;

        tmp = 0;

        driver->GetShaderiv(shader, GL_INFO_LOG_LENGTH, &tmp);

        buff = g_malloc0(tmp);

        driver->GetShaderInfoLog(shader, tmp, NULL, buff);

        YAGL_LOG_ERROR("Unable to compile shader (type = %d) - %s",
                       type, buff);

        driver->DeleteShader(shader);

        g_free(buff);

        return 0;
    }

    return shader;
}

/*
 * Some platforms (like Mac OS X 10.8) have broken UBO support.
 * The problem is that if an UB is unnamed it still requires a name, i.e.:
 *
 * uniform myUB
 * {
 *     mat4 myMatrix;
 * };
 *
 * API should be able to reference myMatrix as "myMatrix", but broken
 * UBO platform expects "myUB.myMatrix".
 * For named UBs there's no error however:
 *
 * uniform myUB
 * {
 *     mat4 myMatrix;
 * } myUBName;
 *
 * Here myMatrix is referenced as "myUB.myMatrix" (yes, myUB, not myUBName)
 * and this is correct.
 *
 * To (partially) work around the problem we must patch
 * index lookups by UB name.
 */
static bool yagl_gles_api_ts_broken_ubo_test(struct yagl_gles_driver *driver)
{
    static const char *vs_source_es3 =
        "#version 300 es\n\n"
        "uniform myUB\n"
        "{\n"
        "    mediump mat4 myMatrix;\n"
        "};\n"
        "in mediump vec4 vertCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = myMatrix * vertCoord;\n"
        "}\n";

    static const char *fs_source_es3 =
        "#version 300 es\n\n"
        "out mediump vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(0, 0, 0, 0);\n"
        "}\n";

    static const char *vs_source_3_2 =
        "#version 150\n\n"
        "uniform myUB\n"
        "{\n"
        "    mat4 myMatrix;\n"
        "};\n"
        "in vec4 vertCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = myMatrix * vertCoord;\n"
        "}\n";

    static const char *fs_source_3_2 =
        "#version 150\n\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(0, 0, 0, 0);\n"
        "}\n";

    static const GLchar *good_name = "myMatrix";
    static const GLchar *bad_name = "myUB.myMatrix";

    bool res = false;
    GLuint vs, fs;
    GLuint program;
    GLint tmp = 0;
    GLuint index = GL_INVALID_INDEX;

    YAGL_LOG_FUNC_SET(yagl_gles_api_ts_broken_ubo_test);

    vs = yagl_gles_api_ts_create_shader(driver,
        ((driver->gl_version == yagl_gl_3_1_es3) ? vs_source_es3 : vs_source_3_2),
        GL_VERTEX_SHADER);

    if (!vs) {
        goto out1;
    }

    fs = yagl_gles_api_ts_create_shader(driver,
        ((driver->gl_version == yagl_gl_3_1_es3) ? fs_source_es3 : fs_source_3_2),
        GL_FRAGMENT_SHADER);

    if (!fs) {
        goto out2;
    }

    program = driver->CreateProgram();

    if (!program) {
        YAGL_LOG_ERROR("Unable to create program");
        goto out3;
    }

    driver->AttachShader(program, vs);
    driver->AttachShader(program, fs);
    driver->LinkProgram(program);
    driver->GetProgramiv(program, GL_LINK_STATUS, &tmp);

    if (!tmp) {
        char *buff;

        tmp = 0;

        driver->GetProgramiv(program, GL_INFO_LOG_LENGTH, &tmp);

        buff = g_malloc0(tmp);

        driver->GetProgramInfoLog(program, tmp, NULL, buff);

        YAGL_LOG_ERROR("Unable to link program - %s", buff);

        driver->DeleteProgram(program);

        g_free(buff);

        goto out4;
    }

    driver->GetUniformIndices(program, 1, &good_name, &index);

    if (index == GL_INVALID_INDEX) {
        driver->GetUniformIndices(program, 1, &bad_name, &index);

        if (index == GL_INVALID_INDEX) {
            YAGL_LOG_ERROR("UBO support is broken in unusual way, unable to workaround. Using UBO may cause undefined behavior");
        } else {
            YAGL_LOG_WARN("UBO support is broken, applying workaround");
            res = true;
        }
    }

out4:
    driver->DetachShader(program, fs);
    driver->DetachShader(program, vs);
    driver->DeleteProgram(program);
out3:
    driver->DeleteShader(fs);
out2:
    driver->DeleteShader(vs);
out1:
    return res;
}

void yagl_gles_api_ts_init(struct yagl_gles_api_ts *gles_api_ts,
                           struct yagl_gles_driver *driver,
                           struct yagl_gles_api_ps *ps)
{
    YAGL_LOG_FUNC_SET(yagl_gles_api_ts_init);

    gles_api_ts->driver = driver;
    gles_api_ts->ps = ps;
    gles_api_ts->api = (struct yagl_gles_api*)ps->base.api;

    if (gles_api_ts->api->checked) {
        return;
    }

    yagl_ensure_ctx(0);

    if (driver->gl_version > yagl_gl_2) {
        gles_api_ts->api->use_map_buffer_range = true;
    } else {
        const char *tmp = (const char*)driver->GetString(GL_EXTENSIONS);

        gles_api_ts->api->use_map_buffer_range =
            (tmp && (strstr(tmp, "GL_ARB_map_buffer_range ") != NULL));

        if (!gles_api_ts->api->use_map_buffer_range) {
            YAGL_LOG_WARN("glMapBufferRange not supported, using glBufferSubData");
        }
    }

    if (driver->gl_version >= yagl_gl_3_1_es3) {
        gles_api_ts->api->broken_ubo = yagl_gles_api_ts_broken_ubo_test(driver);
    } else {
        gles_api_ts->api->broken_ubo = false;
    }

    yagl_unensure_ctx(0);

    gles_api_ts->api->checked = true;
}

void yagl_gles_api_ts_cleanup(struct yagl_gles_api_ts *gles_api_ts)
{
    if (gles_api_ts->num_arrays > 0) {
        uint32_t i;

        yagl_ensure_ctx(0);

        if (gles_api_ts->ebo) {
            gles_api_ts->driver->DeleteBuffers(1, &gles_api_ts->ebo);
        }

        for (i = 0; i < gles_api_ts->num_arrays; ++i) {
            gles_api_ts->driver->DeleteBuffers(1,
                                               &gles_api_ts->arrays[i].vbo);
        }

        yagl_unensure_ctx(0);
    }

    g_free(gles_api_ts->arrays);
}
