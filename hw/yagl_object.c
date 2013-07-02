#include "yagl_object.h"

void yagl_object_init(struct yagl_object *obj,
                      yagl_ref_destroy_func destroy)
{
    yagl_ref_init(&obj->ref, destroy);
    obj->nodelete = false;
}

void yagl_object_cleanup(struct yagl_object *obj)
{
    obj->nodelete = false;
    yagl_ref_cleanup(&obj->ref);
}

void yagl_object_set_nodelete(struct yagl_object *obj)
{
    obj->nodelete = true;
}

void yagl_object_acquire(struct yagl_object *obj)
{
    if (obj) {
        yagl_ref_acquire(&obj->ref);
    }
}

void yagl_object_release(struct yagl_object *obj)
{
    if (obj) {
        yagl_ref_release(&obj->ref);
    }
}
