#include "display.h"
#include "qom/object_interfaces.h"
#if defined(CONFIG_LINUX)
#include <X11/Xlib.h>

static int x_error_handler(Display *dpy, XErrorEvent *e)
{
    return 0;
}
#endif

static void displayobject_instance_finalize(Object *obj)
{
    DisplayObject *dobj = DISPLAYOBJECT(obj);

    dobj->dpy = NULL;
}

static void displayobject_complete(UserCreatable *obj, Error **errp)
{
    DisplayObject *dobj = DISPLAYOBJECT(obj);

#if defined(CONFIG_LINUX)
    XSetErrorHandler(x_error_handler);
    XInitThreads();

    dobj->dpy = XOpenDisplay(0);

    if (!dobj->dpy) {
        error_setg(errp, "Cannot open X display");
    }
#else
    dobj->dpy = NULL;
#endif
}

static void displayobject_class_init(ObjectClass *klass, void *class_data)
{
    UserCreatableClass *ucc = USER_CREATABLE_CLASS(klass);
    ucc->complete = displayobject_complete;
}

static const TypeInfo displayobject_info = {
    .name = TYPE_DISPLAYOBJECT,
    .parent = TYPE_OBJECT,
    .class_init = displayobject_class_init,
    .instance_size = sizeof(DisplayObject),
    .instance_finalize = displayobject_instance_finalize,
    .interfaces = (InterfaceInfo[]) {
        {TYPE_USER_CREATABLE},
        {}
    },
};

static void displayobject_register_types(void)
{
    type_register_static(&displayobject_info);
}

type_init(displayobject_register_types)

struct query_object_arg
{
    DisplayObject *dobj;
    bool ambiguous;
};

static int query_object(Object *object, void *opaque)
{
    struct query_object_arg *arg = opaque;
    DisplayObject *dobj;

    dobj = (DisplayObject*)object_dynamic_cast(object, TYPE_DISPLAYOBJECT);

    if (dobj) {
        if (arg->dobj) {
            arg->ambiguous = true;
        }
        arg->dobj = dobj;
    }

    return 0;
}

DisplayObject *displayobject_create(bool *ambiguous)
{
    Object *container = container_get(object_get_root(), "/objects");
    struct query_object_arg arg = { NULL, false };

    object_child_foreach(container, query_object, &arg);

    *ambiguous = arg.ambiguous;

    if (!arg.dobj) {
        Error *err = NULL;

        arg.dobj = DISPLAYOBJECT(object_new(TYPE_DISPLAYOBJECT));

        user_creatable_complete(&arg.dobj->base, &err);

        if (err) {
            error_free(err);
            return NULL;
        }

        object_property_add_child(container, "dpy0", &arg.dobj->base, &err);

        object_unref(&arg.dobj->base);

        if (err) {
            error_free(err);
            return NULL;
        }
    }

    return arg.dobj;
}

DisplayObject *displayobject_find(const char *id)
{
    Object *container = container_get(object_get_root(), "/objects");
    Object *child;

    child = object_property_get_link(container, id, NULL);

    if (!child) {
        return NULL;
    }

    return (DisplayObject*)object_dynamic_cast(child, TYPE_DISPLAYOBJECT);
}
