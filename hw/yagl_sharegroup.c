#include "yagl_sharegroup.h"
#include "yagl_object.h"

static void yagl_sharegroup_destroy(struct yagl_ref *ref)
{
    struct yagl_sharegroup *sg = (struct yagl_sharegroup*)ref;
    int i;

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
    yagl_object_name local_name;

    qemu_mutex_lock(&sg->mutex);

    local_name = yagl_namespace_add(&sg->namespaces[ns], obj);

    qemu_mutex_unlock(&sg->mutex);

    return local_name;
}

struct yagl_object *yagl_sharegroup_add_named(struct yagl_sharegroup *sg,
                                              int ns,
                                              yagl_object_name local_name,
                                              struct yagl_object *obj)
{
    struct yagl_object *ret;

    qemu_mutex_lock(&sg->mutex);

    ret = yagl_namespace_add_named(&sg->namespaces[ns], local_name, obj);

    qemu_mutex_unlock(&sg->mutex);

    return ret;
}

void yagl_sharegroup_remove(struct yagl_sharegroup *sg,
                            int ns,
                            yagl_object_name local_name)
{
    qemu_mutex_lock(&sg->mutex);

    yagl_namespace_remove(&sg->namespaces[ns], local_name);

    qemu_mutex_unlock(&sg->mutex);
}

void yagl_sharegroup_remove_check(struct yagl_sharegroup *sg,
                                  int ns,
                                  yagl_object_name local_name,
                                  struct yagl_object *obj)
{
    struct yagl_object *actual_obj;

    qemu_mutex_lock(&sg->mutex);

    actual_obj = yagl_namespace_acquire(&sg->namespaces[ns], local_name);

    if (actual_obj == obj) {
        yagl_namespace_remove(&sg->namespaces[ns], local_name);
    }

    yagl_object_release(actual_obj);

    qemu_mutex_unlock(&sg->mutex);
}

struct yagl_object *yagl_sharegroup_acquire_object(struct yagl_sharegroup *sg,
                                                   int ns,
                                                   yagl_object_name local_name)
{
    struct yagl_object *obj;

    qemu_mutex_lock(&sg->mutex);

    obj = yagl_namespace_acquire(&sg->namespaces[ns], local_name);

    qemu_mutex_unlock(&sg->mutex);

    return obj;
}
