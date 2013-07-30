#ifndef _QEMU_YAGL_RESOURCE_H
#define _QEMU_YAGL_RESOURCE_H

#include "yagl_types.h"
#include "yagl_ref.h"
#include "qemu-queue.h"

struct yagl_resource
{
    struct yagl_ref ref;

    QTAILQ_ENTRY(yagl_resource) entry;

    yagl_host_handle handle;
};

/*
 * For implementations.
 * @{
 */

void yagl_resource_init(struct yagl_resource *res,
                        yagl_ref_destroy_func destroy);
void yagl_resource_cleanup(struct yagl_resource *res);

/*
 * @}
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_resource_acquire(struct yagl_resource *res);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_resource_release(struct yagl_resource *res);

#endif
