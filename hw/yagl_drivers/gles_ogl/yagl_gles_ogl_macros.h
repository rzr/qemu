#ifndef _QEMU_YAGL_GLES_OGL_MACROS_H
#define _QEMU_YAGL_GLES_OGL_MACROS_H

#include <GL/gl.h>

/*
 * GLES OpenGL function loading/assigning macros.
 * @{
 */

#define YAGL_GLES_OGL_GET_PROC(driver, func, sym) \
    do { \
        *(void**)(&driver->func) = yagl_dyn_lib_get_ogl_procaddr(dyn_lib, #sym); \
        if (!driver->func) { \
            YAGL_LOG_ERROR("Unable to get symbol: %s", \
                           yagl_dyn_lib_get_error(dyn_lib)); \
            goto fail; \
        } \
    } while (0)

/*
 * @}
 */

#endif
