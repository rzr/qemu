#include "yagl_ref.h"
#include "yagl_stats.h"

void yagl_ref_init(struct yagl_ref *ref, yagl_ref_destroy_func destroy)
{
    assert(ref);
    assert(destroy);

    memset(ref, 0, sizeof(*ref));

    ref->destroy = destroy;
    qemu_mutex_init(&ref->mutex);
    ref->count = 1;

    yagl_stats_new_ref();
}

void yagl_ref_cleanup(struct yagl_ref *ref)
{
    assert(ref);
    assert(!ref->count);

    qemu_mutex_destroy(&ref->mutex);

    yagl_stats_delete_ref();
}

void yagl_ref_acquire(struct yagl_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    qemu_mutex_lock(&ref->mutex);
    ++ref->count;
    qemu_mutex_unlock(&ref->mutex);
}

void yagl_ref_release(struct yagl_ref *ref)
{
    bool call_destroy = false;

    assert(ref);
    assert(ref->count > 0);

    qemu_mutex_lock(&ref->mutex);
    call_destroy = (--ref->count == 0);
    qemu_mutex_unlock(&ref->mutex);

    if (call_destroy)
    {
        assert(ref->destroy);
        ref->destroy(ref);
    }
}
