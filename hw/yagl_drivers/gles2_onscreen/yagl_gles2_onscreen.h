#ifndef _QEMU_YAGL_GLES2_ONSCREEN_H
#define _QEMU_YAGL_GLES2_ONSCREEN_H

#include "yagl_types.h"

struct yagl_gles2_driver;

/*
 * Takes ownership of 'orig_driver'.
 */
struct yagl_gles2_driver
    *yagl_gles2_onscreen_create(struct yagl_gles2_driver *orig_driver);

#endif
