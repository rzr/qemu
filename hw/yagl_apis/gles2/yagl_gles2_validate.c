#include <GLES2/gl2.h>
#include "yagl_gles2_validate.h"

bool yagl_gles2_get_array_param_count(GLenum pname, int *count)
{
    switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING: *count = 1; break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED: *count = 1; break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE: *count = 1; break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE: *count = 1; break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE: *count = 1; break;
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED: *count = 1; break;
    case GL_CURRENT_VERTEX_ATTRIB: *count = 4; break;
    default: return false;
    }
    return true;
}

bool yagl_gles2_get_uniform_type_count(GLenum uniform_type, int *count)
{
    switch (uniform_type) {
    case GL_FLOAT: *count = 1; break;
    case GL_FLOAT_VEC2: *count = 2; break;
    case GL_FLOAT_VEC3: *count = 3; break;
    case GL_FLOAT_VEC4: *count = 4; break;
    case GL_FLOAT_MAT2: *count = 2*2; break;
    case GL_FLOAT_MAT3: *count = 3*3; break;
    case GL_FLOAT_MAT4: *count = 4*4; break;
    case GL_INT: *count = 1; break;
    case GL_INT_VEC2: *count = 2; break;
    case GL_INT_VEC3: *count = 3; break;
    case GL_INT_VEC4: *count = 4; break;
    case GL_BOOL: *count = 1; break;
    case GL_BOOL_VEC2: *count = 2; break;
    case GL_BOOL_VEC3: *count = 3; break;
    case GL_BOOL_VEC4: *count = 4; break;
    case GL_SAMPLER_2D: *count = 1; break;
    case GL_SAMPLER_CUBE: *count = 1; break;
    default: return false;
    }
    return true;
}
