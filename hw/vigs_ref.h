#ifndef _QEMU_VIGS_REF_H
#define _QEMU_VIGS_REF_H

#include "vigs_types.h"

struct vigs_ref;

typedef void (*vigs_ref_destroy_func)(struct vigs_ref */*ref*/);

struct vigs_ref
{
    vigs_ref_destroy_func destroy;

    uint32_t count;
};

/*
 * Initializes ref count to 1.
 */
void vigs_ref_init(struct vigs_ref *ref, vigs_ref_destroy_func destroy);

void vigs_ref_cleanup(struct vigs_ref *ref);

/*
 * Increments ref count.
 */
void vigs_ref_acquire(struct vigs_ref *ref);

/*
 * Decrements ref count and releases when 0.
 */
void vigs_ref_release(struct vigs_ref *ref);

#endif
