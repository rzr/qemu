#ifndef _QEMU_YAGL_GLES_ONSCREEN_H
#define _QEMU_YAGL_GLES_ONSCREEN_H

#include "yagl_types.h"

struct yagl_gles_driver;

void yagl_gles_onscreen_init(struct yagl_gles_driver *driver);
void yagl_gles_onscreen_cleanup(struct yagl_gles_driver *driver);

#endif
