#ifndef _QEMU_WINSYS_GL_H
#define _QEMU_WINSYS_GL_H

#include "winsys.h"

struct winsys_gl_surface
{
    struct winsys_surface base;

    GLuint (*get_front_texture)(struct winsys_gl_surface */*sfc*/);
    GLuint (*get_back_texture)(struct winsys_gl_surface */*sfc*/);

    void (*swap_buffers)(struct winsys_gl_surface */*sfc*/);

    void (*copy_buffers)(uint32_t /*width*/,
                         uint32_t /*height*/,
                         struct winsys_gl_surface */*target*/);
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
