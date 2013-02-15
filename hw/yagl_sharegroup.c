#include "yagl_sharegroup.h"
#include "yagl_object.h"

struct yagl_sharegroup_reap_entry
{
    QLIST_ENTRY(yagl_sharegroup_reap_entry) entry;

    struct yagl_object *obj;
};

QLIST_HEAD(yagl_object_list, yagl_sharegroup_reap_entry);

static void yagl_sharegroup_reap_list_move(struct yagl_sharegroup *sg,
                                           struct yagl_object_list *tmp)
{
    struct yagl_sharegroup_reap_entry *reap_entry, *next;

    QLIST_FOREACH_SAFE(reap_entry, &sg->reap_list, entry, next) {
        QLIST_REMOVE(reap_entry, entry);
        QLIST_INSERT_HEAD(tmp, reap_entry, entry);
    }

    assert(QLIST_EMPTY(&sg->reap_list));
}

static void yagl_sharegroup_release_objects(struct yagl_object_list *tmp)
{
    struct yagl_sharegroup_reap_entry *reap_entry, *next;

    QLIST_FOREACH_SAFE(reap_entry, tmp, entry, next) {
        QLIST_REMOVE(reap_entry, entry);
        yagl_object_release(reap_entry->obj);
        g_free(reap_entry);
    }

    assert(QLIST_EMPTY(tmp));
}

static void yagl_sharegroup_destroy(struct yagl_ref *ref)
{
    struct yagl_sharegroup *sg = (struct yagl_sharegroup*)ref;
    struct yagl_sharegroup_reap_entry *reap_entry, *next_reap_entry;
    int i;

    QLIST_FOREACH_SAFE(reap_entry, &sg->reap_list, entry, next_reap_entry) {
        QLIST_REMOVE(reap_entry, entry);
        yagl_object_set_nodelete(reap_entry->obj);
        yagl_object_release(reap_entry->obj);
        g_free(reap_entry);
    }

    assert(QLIST_EMPTY(&sg->reap_list));

    for (i = 0; i < YAGL_NUM_NAMESPACES; ++i) {
        yagl_namespace_cleanup(&sg->namespaces[i]);
    }

    qemu_mutex_destroy(&sg->mutex);

    yagl_ref_cleanup(&sg->ref);

    g_free(sg);
}

struct yagl_sharegroup *yagl_sharegroup_create(void)
{
    struct yagl_sharegroup *sg = g_malloc0(sizeof(struct yagl_sharegroup));
    int i;

    yagl_ref_init(&sg->ref, &yagl_sharegroup_destroy);

    qemu_mutex_init(&sg->mutex);

    for (i = 0; i < YAGL_NUM_NAMESPACES; ++i) {
        yagl_namespace_init(&sg->namespaces[i]);
    }

    QLIST_INIT(&sg->reap_list);

    return sg;
}

void yagl_sharegroup_acquire(struct yagl_sharegroup *sg)
{
    if (sg) {
        yagl_ref_acquire(&sg->ref);
    }
}

void yagl_sharegroup_release(struct yagl_sharegroup *sg)
{
    if (sg) {
        yagl_ref_release(&sg->ref);
    }
}

yagl_object_name yagl_sharegroup_add(struct yagl_sharegroup *sg,
                                     int ns,
                                     struct yagl_object *obj)
{
    struct yagl_object_list tmp;
    yagl_object_name local_name;

    QLIST_INIT(&tmp);

    qemu_mutex_lock(&sg->mutex);

    yagl_sharegroup_reap_list_move(sg, &tmp);

    local_name = yagl_namespace_add(&sg->namespaces[ns], obj);

    qemu_mutex_unlock(&sg->mutex);

    yagl_sharegroup_release_objects(&tmp);

    return local_name;
}

struct yagl_object *yagl_sharegroup_add_named(struct yagl_sharegroup *sg,
                                              int ns,
                                              yagl_object_name local_name,
                                              struct yagl_object *obj)
{
    struct yagl_object_list tmp;
    struct yagl_object *ret;

    QLIST_INIT(&tmp);

    qemu_mutex_lock(&sg->mutex);

    yagl_sharegroup_reap_list_move(sg, &tmp);

    ret = yagl_namespace_add_named(&sg->namespaces[ns], local_name, obj);

    qemu_mutex_unlock(&sg->mutex);

    yagl_sharegroup_release_objects(&tmp);

    return ret;
}

void yagl_sharegroup_remove(struct yagl_sharegroup *sg,
                            int ns,
                            yagl_object_name local_name)
{
    struct yagl_object_list tmp;

    QLIST_INIT(&tmp);

    qemu_mutex_lock(&sg->mutex);

    yagl_sharegroup_reap_list_move(sg, &tmp);

    yagl_namespace_remove(&sg->namespaces[ns], local_name);

    qemu_mutex_unlock(&sg->mutex);

    yagl_sharegroup_release_objects(&tmp);
}

struct yagl_object *yagl_sharegroup_acquire_object(struct yagl_sharegroup *sg,
                                                   int ns,
                                                   yagl_object_name local_name)
{
    struct yagl_object_list tmp;
    struct yagl_object *obj;

    QLIST_INIT(&tmp);

    qemu_mutex_lock(&sg->mutex);

    yagl_sharegroup_reap_list_move(sg, &tmp);

    obj = yagl_namespace_acquire(&sg->namespaces[ns], local_name);

    qemu_mutex_unlock(&sg->mutex);

    yagl_sharegroup_release_objects(&tmp);

    return obj;
}

void yagl_sharegroup_reap_object(struct yagl_sharegroup *sg,
                                 struct yagl_object *obj)
{
    struct yagl_sharegroup_reap_entry *reap_entry;

    reap_entry = g_malloc0(sizeof(*reap_entry));

    reap_entry->obj = obj;

    qemu_mutex_lock(&sg->mutex);

    QLIST_INSERT_HEAD(&sg->reap_list, reap_entry, entry);

    qemu_mutex_unlock(&sg->mutex);
}
