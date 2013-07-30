#ifndef _QEMU_YAGL_OBJECT_H
#define _QEMU_YAGL_OBJECT_H

#include "yagl_types.h"
#include "yagl_ref.h"

struct yagl_object
{
    struct yagl_ref ref;

    /*
     * An implementation should not attempt to delete
     * a name when 'nodelete' is true.
     */
    volatile bool nodelete;
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

void yagl_object_set_nodelete(struct yagl_object *obj);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_object_acquire(struct yagl_object *obj);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_object_release(struct yagl_object *obj);

#endif
