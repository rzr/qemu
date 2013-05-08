#include "yagl_ref.h"
#include "yagl_stats.h"

void yagl_ref_init(struct yagl_ref *ref, yagl_ref_destroy_func destroy)
{
    assert(ref);
    assert(destroy);

    memset(ref, 0, sizeof(*ref));

    ref->destroy = destroy;
    ref->count = 1;

    yagl_stats_new_ref();
}

void yagl_ref_cleanup(struct yagl_ref *ref)
{
    assert(ref);
    assert(!ref->count);

    yagl_stats_delete_ref();
}

void yagl_ref_acquire(struct yagl_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    ++ref->count;
}

void yagl_ref_release(struct yagl_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    if (--ref->count == 0)
    {
        assert(ref->destroy);
        ref->destroy(ref);
    }
}
