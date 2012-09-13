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
