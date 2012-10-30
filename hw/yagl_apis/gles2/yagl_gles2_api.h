#ifndef _QEMU_YAGL_GLES2_API_H
#define _QEMU_YAGL_GLES2_API_H

#include "yagl_api.h"

struct yagl_gles2_driver;

struct yagl_gles2_api
{
    struct yagl_api base;

    struct yagl_gles2_driver *driver;
};

/*
 * Takes ownership of 'driver'
 */
struct yagl_api *yagl_gles2_api_create(struct yagl_gles2_driver *driver);

#endif
