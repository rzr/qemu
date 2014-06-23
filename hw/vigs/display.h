#ifndef _QEMU_DISPLAY_H
#define _QEMU_DISPLAY_H

#include "qemu-common.h"

#define TYPE_DISPLAYOBJECT "display"

typedef struct {
    Object base;
    void *dpy;
} DisplayObject;

#define DISPLAYOBJECT(obj) \
   OBJECT_CHECK(DisplayObject, obj, TYPE_DISPLAYOBJECT)

DisplayObject *displayobject_create(bool *ambiguous);

DisplayObject *displayobject_find(const char *id);

#endif
