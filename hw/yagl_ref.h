#ifndef _QEMU_YAGL_REF_H
#define _QEMU_YAGL_REF_H

#include "yagl_types.h"
#include "qemu-thread.h"

struct yagl_ref;

typedef void (*yagl_ref_destroy_func)(struct yagl_ref */*ref*/);

struct yagl_ref
{
    yagl_ref_destroy_func destroy;

    QemuMutex mutex;
    volatile uint32_t count;
};

/*
 * Initializes ref count to 1.
 */
void yagl_ref_init(struct yagl_ref *ref, yagl_ref_destroy_func destroy);

void yagl_ref_cleanup(struct yagl_ref *ref);

/*
 * Increments ref count.
 */
void yagl_ref_acquire(struct yagl_ref *ref);

/*
 * Decrements ref count and releases when 0.
 */
void yagl_ref_release(struct yagl_ref *ref);

#endif
