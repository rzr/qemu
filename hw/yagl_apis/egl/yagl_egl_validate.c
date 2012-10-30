#include "yagl_egl_validate.h"

bool yagl_egl_is_attrib_list_empty(const EGLint *attrib_list)
{
    return !attrib_list || (attrib_list[0] == EGL_NONE);
}

bool yagl_egl_is_api_valid(EGLenum api)
{
    return (api == EGL_OPENGL_ES_API);
}
