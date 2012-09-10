#ifndef _QEMU_YAGL_SHAREGROUP_H
#define _QEMU_YAGL_SHAREGROUP_H

#include "yagl_types.h"
#include "yagl_ref.h"
#include "yagl_namespace.h"
#include "qemu-thread.h"
#include "qemu-queue.h"

#define YAGL_NUM_NAMESPACES 6

struct yagl_sharegroup_reap_entry;

struct yagl_sharegroup
{
    struct yagl_ref ref;

    QemuMutex mutex;

    struct yagl_namespace namespaces[YAGL_NUM_NAMESPACES];

    /*
     * This is a list of objects to be released as soon as possible.
     * 'yagl_sharegroup_reap_object' is used to add objects to this list.
     * 'reap_list' is walked when other functions of 'yagl_sharegroup' are
     * called and objects are released.
     *
     * 'reap_list' is necessary because objects cannot always be
     * immediately released, one such case is client context destroy
     * method. it's not safe to release objects bound to context bind points
     * in context destroy method because context might be destroying
     * while another context is active, in that case calling 'glDeleteXXX'
     * will destroy some object in the active context instead of in the context
     * being destroyed. 'reap_list' will keep the objects until it's safe to
     * release them.
     */
    QLIST_HEAD(, yagl_sharegroup_reap_entry) reap_list;
};

struct yagl_sharegroup *yagl_sharegroup_create(void);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_sharegroup_acquire(struct yagl_sharegroup *sg);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_sharegroup_release(struct yagl_sharegroup *sg);

/*
 * Adds an object to share group and returns its local name,
 * this acquires 'obj', so the
 * caller should release 'obj' if he doesn't want to use it and wants
 * it to belong to this share group alone.
 *
 * This function must be called only when the context using 'sg' is the current
 * context.
 */
yagl_object_name yagl_sharegroup_add(struct yagl_sharegroup *sg,
                                     int ns,
                                     struct yagl_object *obj);

/*
 * Same as the above, but adds an object with local name.
 * If an object with such local name already exists then 'obj' will be
 * released and the existing object will be acquired and returned.
 * Otherwise, 'obj' is acquired and returned.
 * Typical use-case for this function is:
 *
 * yagl_object *obj;
 * obj = yagl_sharegroup_acquire_object(sg, ns, local_name);
 * if (!obj) {
 *     obj = yagl_xxx_create(...);
 *     obj = yagl_sharegroup_add_named(sg, ns, local_name, obj);
 * }
 * // use 'obj'.
 * yagl_object_release(obj);
 *
 * This function must be called only when the context using 'sg' is the current
 * context.
 */
struct yagl_object *yagl_sharegroup_add_named(struct yagl_sharegroup *sg,
                                              int ns,
                                              yagl_object_name local_name,
                                              struct yagl_object *obj);

/*
 * Removes an object from the share group, this also releases the
 * object.
 *
 * This function must be called only when the context using 'sg' is the current
 * context.
 */
void yagl_sharegroup_remove(struct yagl_sharegroup *sg,
                            int ns,
                            yagl_object_name local_name);

/*
 * Acquires an object by its local name. Be sure to release the object when
 * you're done.
 *
 * This function must be called only when the context using 'sg' is the current
 * context.
 */
struct yagl_object *yagl_sharegroup_acquire_object(struct yagl_sharegroup *sg,
                                                   int ns,
                                                   yagl_object_name local_name);

/*
 * Adds an object to the reap list, so the object could be
 * released some time later when appropriate. Unlike many
 * other functions this function does not increase 'obj' reference
 * count, it simply takes ownership of 'obj', so after this call
 * you should not use 'obj' unless you have one extra reference to it.
 */
void yagl_sharegroup_reap_object(struct yagl_sharegroup *sg,
                                 struct yagl_object *obj);

#endif
