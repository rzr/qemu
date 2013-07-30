#include "yagl_namespace.h"
#include "yagl_avl.h"
#include "yagl_object.h"

struct yagl_namespace_entry
{
    yagl_object_name local_name;

    struct yagl_object *obj;
};

static int yagl_namespace_entry_comparison_func(const void *avl_a,
                                                const void *avl_b,
                                                void *avl_param)
{
    const struct yagl_namespace_entry *a = avl_a;
    const struct yagl_namespace_entry *b = avl_b;

    if (a->local_name < b->local_name) {
        return -1;
    } else if (a->local_name > b->local_name) {
        return 1;
    } else {
        return 0;
    }
}

static void yagl_namespace_entry_destroy_func(void *avl_item, void *avl_param)
{
    struct yagl_namespace_entry *item = avl_item;

    yagl_object_release(item->obj);

    g_free(item);
}

static void yagl_namespace_entry_destroy_nodelete_func(void *avl_item, void *avl_param)
{
    struct yagl_namespace_entry *item = avl_item;

    yagl_object_set_nodelete(item->obj);

    yagl_object_release(item->obj);

    g_free(item);
}

void yagl_namespace_init(struct yagl_namespace *ns)
{
    ns->entries = yagl_avl_create(&yagl_namespace_entry_comparison_func,
                                  ns,
                                  NULL);
    assert(ns->entries);
    ns->next_local_name = 1;
}

void yagl_namespace_cleanup(struct yagl_namespace *ns)
{
    /*
     * We're cleaning up the namespace, this means that
     * the last context which used this namespace has been
     * destroyed. This means that all of the objects will be
     * automatically deleted by host GL implementation and we don't
     * have to do it. More than that, we MUSTN'T. The context destruction
     * might have been triggered while another context is current, that means
     * that if we delete object now, we'll actually delete some other
     * objects from another context with same name!
     */
    yagl_avl_destroy(ns->entries, &yagl_namespace_entry_destroy_nodelete_func);
    ns->entries = NULL;
    ns->next_local_name = 0;
}

yagl_object_name yagl_namespace_add(struct yagl_namespace *ns,
                                    struct yagl_object *obj)
{
    struct yagl_namespace_entry *item =
        g_malloc0(sizeof(struct yagl_namespace_entry));

    if (!ns->next_local_name) {
        /*
         * 0 names are invalid.
         */

        ++ns->next_local_name;
    }

    item->local_name = ns->next_local_name++;
    item->obj = obj;

    yagl_object_acquire(obj);

    yagl_avl_assert_insert(ns->entries, item);

    return item->local_name;
}

struct yagl_object *yagl_namespace_add_named(struct yagl_namespace *ns,
                                             yagl_object_name local_name,
                                             struct yagl_object *obj)
{
    struct yagl_namespace_entry *dup_item;
    struct yagl_namespace_entry *item =
        g_malloc0(sizeof(struct yagl_namespace_entry));

    item->local_name = local_name;
    item->obj = obj;

    dup_item = yagl_avl_insert(ns->entries, item);

    if (dup_item) {
        yagl_namespace_entry_destroy_func(item, ns->entries->avl_param);

        yagl_object_acquire(dup_item->obj);

        return dup_item->obj;
    } else {
        yagl_object_acquire(obj);

        return obj;
    }
}

void yagl_namespace_remove(struct yagl_namespace *ns,
                           yagl_object_name local_name)
{
    void *item;
    struct yagl_namespace_entry dummy;

    dummy.local_name = local_name;

    item = yagl_avl_delete(ns->entries, &dummy);

    if (item) {
        yagl_namespace_entry_destroy_func(item, ns->entries->avl_param);
    }
}

struct yagl_object *yagl_namespace_acquire(struct yagl_namespace *ns,
                                           yagl_object_name local_name)
{
    struct yagl_namespace_entry *item;
    struct yagl_namespace_entry dummy;

    dummy.local_name = local_name;

    item = yagl_avl_find(ns->entries, &dummy);

    if (item) {
        yagl_object_acquire(item->obj);
        return item->obj;
    } else {
        return NULL;
    }
}
