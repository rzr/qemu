#include "yagl_egl_surface_attribs.h"

void yagl_egl_pbuffer_attribs_init(struct yagl_egl_pbuffer_attribs *attribs)
{
    attribs->largest = EGL_FALSE;
    attribs->tex_format = EGL_NO_TEXTURE;
    attribs->tex_target = EGL_NO_TEXTURE;
    attribs->tex_mipmap = EGL_FALSE;
}

void yagl_egl_pixmap_attribs_init(struct yagl_egl_pixmap_attribs *attribs)
{
}

void yagl_egl_window_attribs_init(struct yagl_egl_window_attribs *attribs)
{
}
