#include "vigs_ref.h"

void vigs_ref_init(struct vigs_ref *ref, vigs_ref_destroy_func destroy)
{
    assert(ref);
    assert(destroy);

    memset(ref, 0, sizeof(*ref));

    ref->destroy = destroy;
    ref->count = 1;
}

void vigs_ref_cleanup(struct vigs_ref *ref)
{
    assert(ref);
    assert(!ref->count);
}

void vigs_ref_acquire(struct vigs_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    ++ref->count;
}

void vigs_ref_release(struct vigs_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    if (--ref->count == 0) {
        assert(ref->destroy);
        ref->destroy(ref);
    }
}
