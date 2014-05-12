/*
 * Virtio 9p
 *
 * Copyright IBM, Corp. 2010
 *
 * Authors:
 *  Aneesh Kumar K.V <aneesh.kumar@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */
#ifndef _QEMU_VIRTIO_9P_XATTR_H
#define _QEMU_VIRTIO_9P_XATTR_H

#include "qemu/xattr.h"

typedef struct xattr_operations
{
    const char *name;
    ssize_t (*getxattr)(FsContext *ctx, const char *path,
                        const char *name, void *value, size_t size);
    ssize_t (*listxattr)(FsContext *ctx, const char *path,
                         char *name, void *value, size_t size);
    int (*setxattr)(FsContext *ctx, const char *path, const char *name,
                    void *value, size_t size, int flags);
    int (*removexattr)(FsContext *ctx,
                       const char *path, const char *name);
} XattrOperations;


extern XattrOperations mapped_user_xattr;
extern XattrOperations passthrough_user_xattr;

extern XattrOperations mapped_pacl_xattr;
extern XattrOperations mapped_dacl_xattr;
extern XattrOperations passthrough_acl_xattr;
extern XattrOperations none_acl_xattr;

extern XattrOperations *mapped_xattr_ops[];
extern XattrOperations *passthrough_xattr_ops[];
extern XattrOperations *none_xattr_ops[];

ssize_t v9fs_get_xattr(FsContext *ctx, const char *path, const char *name,
                       void *value, size_t size);
ssize_t v9fs_list_xattr(FsContext *ctx, const char *path, void *value,
                        size_t vsize);
int v9fs_set_xattr(FsContext *ctx, const char *path, const char *name,
                          void *value, size_t size, int flags);
int v9fs_remove_xattr(FsContext *ctx, const char *path, const char *name);
ssize_t pt_listxattr(FsContext *ctx, const char *path, char *name, void *value,
                     size_t size);

static inline ssize_t pt_getxattr(FsContext *ctx, const char *path,
                                  const char *name, void *value, size_t size)
{
#ifdef CONFIG_MARU
#ifndef CONFIG_WIN32
    char buffer[PATH_MAX];
#ifdef CONFIG_LINUX
    return lgetxattr(rpath(ctx, path, buffer), name, value, size);
#else
    return getxattr(rpath(ctx, path, buffer), name, value, size, 0, XATTR_NOFOLLOW);
#endif
#else
    return 0;
#endif
#else
    char buffer[PATH_MAX];
    return lgetxattr(rpath(ctx, path, buffer), name, value, size);
#endif
}

static inline int pt_setxattr(FsContext *ctx, const char *path,
                              const char *name, void *value,
                              size_t size, int flags)
{
#ifdef CONFIG_MARU
#ifndef CONFIG_WIN32
    char buffer[PATH_MAX];
#ifdef CONFIG_LINUX
    return lsetxattr(rpath(ctx, path, buffer), name, value, size, flags);
#else
    return setxattr(rpath(ctx, path, buffer), name, value, size, 0, flags | XATTR_NOFOLLOW);
#endif
#else
    return 0;
#endif
#else
    char buffer[PATH_MAX];
    return lsetxattr(rpath(ctx, path, buffer), name, value, size, flags);
#endif
}

static inline int pt_removexattr(FsContext *ctx,
                                 const char *path, const char *name)
{
#ifdef CONFIG_MARU
#ifndef CONFIG_WIN32
    char buffer[PATH_MAX];
#ifdef CONFIG_LINUX
    return lremovexattr(rpath(ctx, path, buffer), name);
#else
    return removexattr(rpath(ctx, path, buffer), name, XATTR_NOFOLLOW);
#endif
#else
    return 0;
#endif
#else
    char buffer[PATH_MAX];
    return lremovexattr(rpath(ctx, path, buffer), name);
#endif
}

static inline ssize_t notsup_getxattr(FsContext *ctx, const char *path,
                                      const char *name, void *value,
                                      size_t size)
{
    errno = ENOTSUP;
    return -1;
}

static inline int notsup_setxattr(FsContext *ctx, const char *path,
                                  const char *name, void *value,
                                  size_t size, int flags)
{
    errno = ENOTSUP;
    return -1;
}

static inline ssize_t notsup_listxattr(FsContext *ctx, const char *path,
                                       char *name, void *value, size_t size)
{
    return 0;
}

static inline int notsup_removexattr(FsContext *ctx,
                                     const char *path, const char *name)
{
    errno = ENOTSUP;
    return -1;
}

#endif
