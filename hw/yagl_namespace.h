#ifndef _QEMU_YAGL_NAMESPACE_H
#define _QEMU_YAGL_NAMESPACE_H

#include "yagl_types.h"

struct yagl_object;
struct yagl_avl_table;

struct yagl_namespace
{
    struct yagl_avl_table *entries;

    yagl_object_name next_local_name;
};

void yagl_namespace_init(struct yagl_namespace *ns);

void yagl_namespace_cleanup(struct yagl_namespace *ns);

/*
 * Adds an object to namespace and returns its local name,
 * this acquires 'obj', so the
 * caller should release 'obj' if he doesn't want to use it and wants
 * it to belong to this namespace alone.
 */
yagl_object_name yagl_namespace_add(struct yagl_namespace *ns,
                                    struct yagl_object *obj);

/*
 * Same as the above, but adds an object with local name.
 * If an object with such local name already exists then 'obj' will be
 * released and the existing object will be acquired and returned.
 * Otherwise, 'obj' is acquired and returned.
 * Typical use-case for this function is:
 *
 * yagl_object *obj;
 * obj = yagl_namespace_acquire(ns, local_name);
 * if (!obj) {
 *     obj = yagl_xxx_create(...);
 *     obj = yagl_namespace_add_named(ns, local_name, obj);
 * }
 * // use 'obj'.
 * yagl_object_release(obj);
 */
struct yagl_object *yagl_namespace_add_named(struct yagl_namespace *ns,
                                             yagl_object_name local_name,
                                             struct yagl_object *obj);

/*
 * Removes an object from the namespace, this also releases the
 * object.
 */
void yagl_namespace_remove(struct yagl_namespace *ns,
                           yagl_object_name local_name);

/*
 * Acquires an object by its local name. Be sure to release the object when
 * you're done.
 */
struct yagl_object *yagl_namespace_acquire(struct yagl_namespace *ns,
                                           yagl_object_name local_name);

#endif
