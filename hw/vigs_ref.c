#include "vigs_ref.h"

void vigs_ref_init(struct vigs_ref *ref, vigs_ref_destroy_func destroy)
{
    assert(ref);
    assert(destroy);

    memset(ref, 0, sizeof(*ref));

    ref->destroy = destroy;
    qemu_mutex_init(&ref->mutex);
    ref->count = 1;
}

void vigs_ref_cleanup(struct vigs_ref *ref)
{
    assert(ref);
    assert(!ref->count);

    qemu_mutex_destroy(&ref->mutex);
}

void vigs_ref_acquire(struct vigs_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    qemu_mutex_lock(&ref->mutex);
    ++ref->count;
    qemu_mutex_unlock(&ref->mutex);
}

void vigs_ref_release(struct vigs_ref *ref)
{
    bool call_destroy = false;

    assert(ref);
    assert(ref->count > 0);

    qemu_mutex_lock(&ref->mutex);
    call_destroy = (--ref->count == 0);
    qemu_mutex_unlock(&ref->mutex);

    if (call_destroy) {
        assert(ref->destroy);
        ref->destroy(ref);
    }
}
