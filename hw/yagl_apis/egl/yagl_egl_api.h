#ifndef _QEMU_YAGL_EGL_API_H
#define _QEMU_YAGL_EGL_API_H

#include "yagl_api.h"

struct yagl_egl_backend;

struct yagl_egl_api
{
    struct yagl_api base;

    struct yagl_egl_backend *backend;
};

/*
 * Takes ownership of 'backend'
 */
struct yagl_api *yagl_egl_api_create(struct yagl_egl_backend *backend);

#endif
