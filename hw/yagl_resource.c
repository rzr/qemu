#include "yagl_resource.h"
#include "yagl_handle_gen.h"

void yagl_resource_init(struct yagl_resource *res,
                        yagl_ref_destroy_func destroy)
{
    yagl_ref_init(&res->ref, destroy);
    res->handle = yagl_handle_gen();
}

void yagl_resource_cleanup(struct yagl_resource *res)
{
    res->handle = 0;
    yagl_ref_cleanup(&res->ref);
}

void yagl_resource_acquire(struct yagl_resource *res)
{
    if (res) {
        yagl_ref_acquire(&res->ref);
    }
}

void yagl_resource_release(struct yagl_resource *res)
{
    if (res) {
        yagl_ref_release(&res->ref);
    }
}
