#include "work_queue.h"
#include "qom/object_interfaces.h"

static void *work_queue_run(void *arg)
{
    struct work_queue *wq = arg;

    qemu_mutex_lock(&wq->mutex);

    while (true) {
        struct work_queue_item *wq_item;

        while (wq->num_items == 0) {
            if (wq->destroying) {
                goto out;
            }

            qemu_cond_signal(&wq->wait_cond);
            qemu_cond_wait(&wq->add_cond, &wq->mutex);
        }

        wq_item = QTAILQ_FIRST(&wq->items);
        QTAILQ_REMOVE(&wq->items, wq_item, entry);

        qemu_mutex_unlock(&wq->mutex);

        wq_item->func(wq_item);

        qemu_mutex_lock(&wq->mutex);

        --wq->num_items;
    }

out:
    qemu_mutex_unlock(&wq->mutex);

    return NULL;
}

void work_queue_item_init(struct work_queue_item *wq_item,
                          work_queue_func func)
{
    memset(wq_item, 0, sizeof(*wq_item));

    wq_item->func = func;
}

struct work_queue *work_queue_create(const char *name)
{
    struct work_queue *wq;

    wq = g_malloc0(sizeof(*wq));

    qemu_mutex_init(&wq->mutex);
    qemu_cond_init(&wq->add_cond);
    qemu_cond_init(&wq->wait_cond);

    QTAILQ_INIT(&wq->items);

    qemu_thread_create(&wq->thread, name,
                       work_queue_run,
                       wq,
                       QEMU_THREAD_JOINABLE);

    return wq;
}

void work_queue_add_item(struct work_queue *wq,
                         struct work_queue_item *wq_item)
{
    qemu_mutex_lock(&wq->mutex);

    QTAILQ_INSERT_TAIL(&wq->items, wq_item, entry);
    ++wq->num_items;

    qemu_mutex_unlock(&wq->mutex);

    qemu_cond_signal(&wq->add_cond);
}

void work_queue_wait(struct work_queue *wq)
{
    if (wq->num_items == 0) {
        return;
    }

    qemu_mutex_lock(&wq->mutex);

    while (wq->num_items > 0) {
        qemu_cond_wait(&wq->wait_cond, &wq->mutex);
    }

    qemu_mutex_unlock(&wq->mutex);
}

void work_queue_destroy(struct work_queue *wq)
{
    wq->destroying = true;
    qemu_cond_signal(&wq->add_cond);

    qemu_thread_join(&wq->thread);

    qemu_cond_destroy(&wq->wait_cond);
    qemu_cond_destroy(&wq->add_cond);
    qemu_mutex_destroy(&wq->mutex);

    g_free(wq);
}

static void workqueueobject_instance_finalize(Object *obj)
{
    WorkQueueObject *wqobj = WORKQUEUEOBJECT(obj);

    work_queue_destroy(wqobj->wq);

    wqobj->wq = NULL;
}

static void workqueueobject_complete(UserCreatable *obj, Error **errp)
{
    WorkQueueObject *wqobj = WORKQUEUEOBJECT(obj);

    wqobj->wq = work_queue_create("render_queue");
}

static void workqueueobject_class_init(ObjectClass *klass, void *class_data)
{
    UserCreatableClass *ucc = USER_CREATABLE_CLASS(klass);
    ucc->complete = workqueueobject_complete;
}

static const TypeInfo workqueueobject_info = {
    .name = TYPE_WORKQUEUEOBJECT,
    .parent = TYPE_OBJECT,
    .class_init = workqueueobject_class_init,
    .instance_size = sizeof(WorkQueueObject),
    .instance_finalize = workqueueobject_instance_finalize,
    .interfaces = (InterfaceInfo[]) {
        {TYPE_USER_CREATABLE},
        {}
    },
};

static void workqueueobject_register_types(void)
{
    type_register_static(&workqueueobject_info);
}

type_init(workqueueobject_register_types)

struct query_object_arg
{
    WorkQueueObject *wqobj;
    bool ambiguous;
};

static int query_object(Object *object, void *opaque)
{
    struct query_object_arg *arg = opaque;
    WorkQueueObject *wqobj;

    wqobj = (WorkQueueObject*)object_dynamic_cast(object, TYPE_WORKQUEUEOBJECT);

    if (wqobj) {
        if (arg->wqobj) {
            arg->ambiguous = true;
        }
        arg->wqobj = wqobj;
    }

    return 0;
}

WorkQueueObject *workqueueobject_create(bool *ambiguous)
{
    Object *container = container_get(object_get_root(), "/objects");
    struct query_object_arg arg = { NULL, false };

    object_child_foreach(container, query_object, &arg);

    *ambiguous = arg.ambiguous;

    if (!arg.wqobj) {
        Error *err = NULL;

        arg.wqobj = WORKQUEUEOBJECT(object_new(TYPE_WORKQUEUEOBJECT));

        user_creatable_complete(&arg.wqobj->base, &err);

        if (err) {
            error_free(err);
            return NULL;
        }

        object_property_add_child(container, "wq0", &arg.wqobj->base, &err);

        object_unref(&arg.wqobj->base);

        if (err) {
            error_free(err);
            return NULL;
        }
    }

    return arg.wqobj;
}

WorkQueueObject *workqueueobject_find(const char *id)
{
    Object *container = container_get(object_get_root(), "/objects");
    Object *child;

    child = object_property_get_link(container, id, NULL);

    if (!child) {
        return NULL;
    }

    return (WorkQueueObject*)object_dynamic_cast(child, TYPE_WORKQUEUEOBJECT);
}
