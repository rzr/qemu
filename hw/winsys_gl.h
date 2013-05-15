#ifndef _QEMU_WINSYS_GL_H
#define _QEMU_WINSYS_GL_H

#include "winsys.h"

struct winsys_gl_surface
{
    struct winsys_surface base;

    GLuint (*get_texture)(struct winsys_gl_surface */*sfc*/);
};

struct winsys_gl_info
{
    struct winsys_info base;

    /*
     * OpenGL context that holds all winsys texture objects.
     */
    void *context;
};

#endif
