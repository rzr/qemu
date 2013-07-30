#include "yagl_resource_list.h"
#include "yagl_resource.h"

void yagl_resource_list_init(struct yagl_resource_list *list)
{
    QTAILQ_INIT(&list->resources);
    list->count = 0;
}

void yagl_resource_list_cleanup(struct yagl_resource_list *list)
{
    struct yagl_resource *res, *next;

    QTAILQ_FOREACH_SAFE(res, &list->resources, entry, next) {
        QTAILQ_REMOVE(&list->resources, res, entry);
        yagl_resource_release(res);
    }

    assert(QTAILQ_EMPTY(&list->resources));
    list->count = 0;
}

int yagl_resource_list_get_count(struct yagl_resource_list *list)
{
    return list->count;
}

void yagl_resource_list_add(struct yagl_resource_list *list,
                            struct yagl_resource *res)
{
    yagl_resource_acquire(res);
    QTAILQ_INSERT_TAIL(&list->resources, res, entry);
    ++list->count;
}

void yagl_resource_list_move(struct yagl_resource_list *from_list,
                             struct yagl_resource_list *to_list)
{
    struct yagl_resource *res, *next;

    QTAILQ_FOREACH_SAFE(res, &from_list->resources, entry, next) {
        QTAILQ_REMOVE(&from_list->resources, res, entry);
        QTAILQ_INSERT_TAIL(&to_list->resources, res, entry);
        --from_list->count;
        ++to_list->count;
    }

    assert(QTAILQ_EMPTY(&from_list->resources));
}

struct yagl_resource
    *yagl_resource_list_acquire(struct yagl_resource_list *list,
                                yagl_host_handle handle)
{
    struct yagl_resource *res;

    QTAILQ_FOREACH(res, &list->resources, entry) {
        if (res->handle == handle) {
            yagl_resource_acquire(res);
            return res;
        }
    }

    return NULL;
}

bool yagl_resource_list_remove(struct yagl_resource_list *list,
                               yagl_host_handle handle)
{
    struct yagl_resource *res;

    QTAILQ_FOREACH(res, &list->resources, entry) {
        if (res->handle == handle) {
            QTAILQ_REMOVE(&list->resources, res, entry);
            yagl_resource_release(res);
            return true;
        }
    }

    return false;
}
