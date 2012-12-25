#ifndef _QEMU_YAGL_GLES1_ONSCREEN_H
#define _QEMU_YAGL_GLES1_ONSCREEN_H

#include "yagl_types.h"

struct yagl_gles1_driver;

/*
 * Takes ownership of 'orig_driver'.
 */
struct yagl_gles1_driver
    *yagl_gles1_onscreen_create(struct yagl_gles1_driver *orig_driver);

#endif
