#ifndef _QEMU_YAGL_OBJECT_H
#define _QEMU_YAGL_OBJECT_H

#include "yagl_types.h"
#include "yagl_ref.h"

struct yagl_object
{
    struct yagl_ref ref;
};

/*
 * For implementations.
 * @{
 */

void yagl_object_init(struct yagl_object *obj,
                      yagl_ref_destroy_func destroy);
void yagl_object_cleanup(struct yagl_object *obj);

/*
 * @}
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_object_acquire(struct yagl_object *obj);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_object_release(struct yagl_object *obj);

#endif
