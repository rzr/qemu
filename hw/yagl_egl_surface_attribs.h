#ifndef _QEMU_YAGL_EGL_SURFACE_ATTRIBS_H
#define _QEMU_YAGL_EGL_SURFACE_ATTRIBS_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_egl_pbuffer_attribs
{
    EGLint largest;
    EGLint tex_format;
    EGLint tex_target;
    EGLint tex_mipmap;
};

/*
 * Initialize to default values.
 */
void yagl_egl_pbuffer_attribs_init(struct yagl_egl_pbuffer_attribs *attribs);

struct yagl_egl_pixmap_attribs
{
};

/*
 * Initialize to default values.
 */
void yagl_egl_pixmap_attribs_init(struct yagl_egl_pixmap_attribs *attribs);

struct yagl_egl_window_attribs
{
};

/*
 * Initialize to default values.
 */
void yagl_egl_window_attribs_init(struct yagl_egl_window_attribs *attribs);

#endif
