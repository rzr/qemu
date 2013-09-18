#ifndef _QEMU_YAGL_TRANSPORT_GL_H
#define _QEMU_YAGL_TRANSPORT_GL_H

#include "yagl_types.h"
#include "yagl_transport.h"

static __inline GLbitfield yagl_transport_get_out_GLbitfield(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLboolean yagl_transport_get_out_GLboolean(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint8_t(t);
}

static __inline GLubyte yagl_transport_get_out_GLubyte(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint8_t(t);
}

static __inline GLclampf yagl_transport_get_out_GLclampf(struct yagl_transport *t)
{
    return yagl_transport_get_out_float(t);
}

static __inline GLclampx yagl_transport_get_out_GLclampx(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLfixed yagl_transport_get_out_GLfixed(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLenum yagl_transport_get_out_GLenum(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLfloat yagl_transport_get_out_GLfloat(struct yagl_transport *t)
{
    return yagl_transport_get_out_float(t);
}

static __inline GLint yagl_transport_get_out_GLint(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLsizei yagl_transport_get_out_GLsizei(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline GLuint yagl_transport_get_out_GLuint(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

#endif
