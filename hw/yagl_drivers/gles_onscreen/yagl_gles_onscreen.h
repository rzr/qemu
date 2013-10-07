#ifndef _QEMU_YAGL_GLES_ONSCREEN_H
#define _QEMU_YAGL_GLES_ONSCREEN_H

#include "yagl_types.h"

struct yagl_gles_driver;

/*
 * Takes ownership of 'orig_driver'.
 */
struct yagl_gles_driver
    *yagl_gles_onscreen_create(struct yagl_gles_driver *orig_driver);

#endif
