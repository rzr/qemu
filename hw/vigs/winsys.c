#include "winsys.h"
#include "qom/object_interfaces.h"

static const TypeInfo wsiobject_info = {
    .name = TYPE_WSIOBJECT,
    .parent = TYPE_OBJECT,
    .instance_size = sizeof(WSIObject)
};

static void wsiobject_register_types(void)
{
    type_register_static(&wsiobject_info);
}

type_init(wsiobject_register_types)

WSIObject *wsiobject_find(const char *id)
{
    Object *container = container_get(object_get_root(), "/objects");
    Object *child;

    child = object_property_get_link(container, id, NULL);

    if (!child) {
        return NULL;
    }

    return (WSIObject*)object_dynamic_cast(child, TYPE_WSIOBJECT);
}
