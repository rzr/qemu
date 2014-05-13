/*
 * Virtio 9p backend for Maru
 * Based on hw/9pfs/virtio-9p-local.c:
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Sooyoung Ha <yoosah.ha@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "hw/virtio/virtio.h"
#include "virtio-9p.h"
#ifndef CONFIG_WIN32
#include "virtio-9p-xattr.h"
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "qemu/xattr.h"
#endif
#include <libgen.h>
#ifdef CONFIG_LINUX
#include <linux/fs.h>
#endif
#ifdef CONFIG_LINUX_MAGIC_H
#include <linux/magic.h>
#endif
#ifndef CONFIG_WIN32
#include <sys/ioctl.h>
#endif

#include "../../tizen/src/debug_ch.h"
MULTI_DEBUG_CHANNEL(tizen, qemu_9p_local);

#ifndef XFS_SUPER_MAGIC
#define XFS_SUPER_MAGIC  0x58465342
#endif
#ifndef EXT2_SUPER_MAGIC
#define EXT2_SUPER_MAGIC 0xEF53
#endif
#ifndef REISERFS_SUPER_MAGIC
#define REISERFS_SUPER_MAGIC 0x52654973
#endif
#ifndef BTRFS_SUPER_MAGIC
#define BTRFS_SUPER_MAGIC 0x9123683E
#endif

#ifdef CONFIG_WIN32
#ifndef MSDOS_SUPER_MAGIC
#define MSDOS_SUPER_MAGIC 0x4d44
#endif
#ifndef NTFS_SUPER_MAGIC
#define NTFS_SUPER_MAGIC 0x5346544e
#endif

uint64_t hostBytesPerSector = -1;
#endif

#define VIRTFS_META_DIR ".virtfs_metadata"

#ifndef CONFIG_LINUX
#define AT_REMOVEDIR 0x200
#endif

static char *local_mapped_attr_path(FsContext *ctx, const char *path)
{
    char *dir_name;
    char *tmp_path = g_strdup(path);
    char *base_name = basename(tmp_path);
    char *buffer;

    /* NULL terminate the directory */
    dir_name = tmp_path;
    *(base_name - 1) = '\0';
#ifndef CONFIG_WIN32
    buffer = g_strdup_printf("%s/%s/%s/%s",
             ctx->fs_root, dir_name, VIRTFS_META_DIR, base_name);
#else
    buffer = g_strdup_printf("%s\\%s\\%s\\%s",
             ctx->fs_root, dir_name, VIRTFS_META_DIR, base_name);
    while(buffer[strlen(buffer)-1] == '\\'){
        buffer[strlen(buffer)-1] = '\0';
    }
#endif
    g_free(tmp_path);
    return buffer;
}

static FILE *local_fopen(const char *path, const char *mode)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int fd;
#ifndef CONFIG_WIN32
    int o_mode = 0;
#endif
    FILE *fp;
    int flags = O_NOFOLLOW;
    /*
     * only supports two modes
     */
    if (mode[0] == 'r') {
        flags |= O_RDONLY;
    } else if (mode[0] == 'w') {
        flags |= O_WRONLY | O_TRUNC | O_CREAT;
#ifndef CONFIG_WIN32
        o_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif
    } else {
        return NULL;
    }
#ifndef CONFIG_WIN32
    fd = open(path, flags, o_mode);
#else
    fd = open(path, flags | O_BINARY);
#endif
    if (fd == -1) {
        return NULL;
    }
    fp = fdopen(fd, mode);
    if (!fp) {
        close(fd);
    }
    return fp;
}

#define ATTR_MAX 100
#ifndef CONFIG_WIN32
static void local_mapped_file_attr(FsContext *ctx, const char *path,
                                   struct stat *stbuf)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    FILE *fp;
    char buf[ATTR_MAX];
    char *attr_path;

    attr_path = local_mapped_attr_path(ctx, path);
    fp = local_fopen(attr_path, "r");
    g_free(attr_path);
    if (!fp) {
        return;
    }
    memset(buf, 0, ATTR_MAX);
    while (fgets(buf, ATTR_MAX, fp)) {
        if (!strncmp(buf, "virtfs.uid", 10)) {
            stbuf->st_uid = atoi(buf+11);
        } else if (!strncmp(buf, "virtfs.gid", 10)) {
            stbuf->st_gid = atoi(buf+11);
        } else if (!strncmp(buf, "virtfs.mode", 11)) {
            stbuf->st_mode = atoi(buf+12);
        } else if (!strncmp(buf, "virtfs.rdev", 11)) {
            stbuf->st_rdev = atoi(buf+12);
        }
        memset(buf, 0, ATTR_MAX);
    }
    fclose(fp);
}
#endif

static int local_lstat(FsContext *fs_ctx, V9fsPath *fs_path, struct stat *stbuf)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err;
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(fs_ctx, path);

#ifndef CONFIG_WIN32
    err =  lstat(buffer, stbuf);
    if (err) {
        return err;
    }
    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        /* Actual credentials are part of extended attrs */
        uid_t tmp_uid;
        gid_t tmp_gid;
        mode_t tmp_mode;
        dev_t tmp_dev;
#ifdef CONFIG_LINUX
        if (getxattr(buffer, "user.virtfs.uid", &tmp_uid,
                    sizeof(uid_t)) > 0) {
            stbuf->st_uid = tmp_uid;
        }
        if (getxattr(buffer, "user.virtfs.gid", &tmp_gid,
                    sizeof(gid_t)) > 0) {
            stbuf->st_gid = tmp_gid;
        }
        if (getxattr(buffer, "user.virtfs.mode",
                    &tmp_mode, sizeof(mode_t)) > 0) {
            stbuf->st_mode = tmp_mode;
        }
        if (getxattr(buffer, "user.virtfs.rdev", &tmp_dev,
                    sizeof(dev_t)) > 0) {
            stbuf->st_rdev = tmp_dev;
        }
#else
	/*
	 *  extra two parameters on Mac OS X.
	 *  first one is position which specifies an offset within the extended attribute.
	 *  i.e. extended attribute means "user.virtfs.uid". Currently, it is only used with
	 *  resource fork attribute and all other extended attributes, it is reserved and should be zero.
	 *  second one is options which specify option for retrieving extended attributes:
	 *  (XATTR_NOFOLLOW, XATTR_SHOWCOMPRESSION)
	 */
        if (getxattr(buffer, "user.virtfs.uid", &tmp_uid,
                    sizeof(uid_t), 0, 0) > 0) {
            stbuf->st_uid = tmp_uid;
        }
        if (getxattr(buffer, "user.virtfs.gid", &tmp_gid,
                    sizeof(gid_t), 0, 0) > 0) {
            stbuf->st_gid = tmp_gid;
        }
        if (getxattr(buffer, "user.virtfs.mode",
                    &tmp_mode, sizeof(mode_t), 0, 0) > 0) {
            stbuf->st_mode = tmp_mode;
        }
        if (getxattr(buffer, "user.virtfs.rdev", &tmp_dev,
                    sizeof(dev_t), 0, 0) > 0) {
            stbuf->st_rdev = tmp_dev;
        }
#endif
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        local_mapped_file_attr(fs_ctx, path, stbuf);
    }
#else
    const char * pathname = buffer;
    if (hostBytesPerSector == -1) {
        DWORD BytesPerSector;
        TCHAR tmpName[PATH_MAX] = {0};

        strncpy(tmpName, pathname, 3 > PATH_MAX ? PATH_MAX : 3);
        tmpName[4 > PATH_MAX ? PATH_MAX : 4] = '\0';
        LPCTSTR RootPathName = (LPCTSTR)tmpName;

        GetDiskFreeSpace(RootPathName, NULL, &BytesPerSector, NULL, NULL);
        hostBytesPerSector = BytesPerSector;
    }
    err = _stat(pathname, stbuf);

    /* Modify the permission to 777 except the directories. */
    stbuf->st_mode = stbuf->st_mode | 0777;
#endif
    g_free(buffer);
    return err;
}

static int local_create_mapped_attr_dir(FsContext *ctx, const char *path)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err;
    char attr_dir[PATH_MAX];
    char *tmp_path = g_strdup(path);
#ifndef CONFIG_WIN32
    snprintf(attr_dir, PATH_MAX, "%s/%s/%s",
             ctx->fs_root, dirname(tmp_path), VIRTFS_META_DIR);
    err = mkdir(attr_dir, 0700);
#else
    snprintf(attr_dir, PATH_MAX, "%s\\%s\\%s",
             ctx->fs_root, dirname(tmp_path), VIRTFS_META_DIR);
    err = mkdir(attr_dir);
#endif

    if (err < 0 && errno == EEXIST) {
        err = 0;
    }
    g_free(tmp_path);
    return err;
}

static int local_set_mapped_file_attr(FsContext *ctx,
                                      const char *path, FsCred *credp)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    FILE *fp;
    int ret = 0;
    char buf[ATTR_MAX];
    char *attr_path;
    int uid = -1, gid = -1, mode = -1, rdev = -1;

    attr_path = local_mapped_attr_path(ctx, path);
    fp = local_fopen(attr_path, "r");
    g_free(attr_path);
    if (!fp) {
        goto create_map_file;
    }
    memset(buf, 0, ATTR_MAX);
    while (fgets(buf, ATTR_MAX, fp)) {
        if (!strncmp(buf, "virtfs.uid", 10)) {
            uid = atoi(buf+11);
        } else if (!strncmp(buf, "virtfs.gid", 10)) {
            gid = atoi(buf+11);
        } else if (!strncmp(buf, "virtfs.mode", 11)) {
            mode = atoi(buf+12);
        } else if (!strncmp(buf, "virtfs.rdev", 11)) {
            rdev = atoi(buf+12);
        }
        memset(buf, 0, ATTR_MAX);
    }
    fclose(fp);
    goto update_map_file;

create_map_file:
    ret = local_create_mapped_attr_dir(ctx, path);
    if (ret < 0) {
        goto err_out;
    }

update_map_file:
    fp = local_fopen(attr_path, "w");
    if (!fp) {
        ret = -1;
        goto err_out;
    }

    if (credp->fc_uid != -1) {
        uid = credp->fc_uid;
    }
    if (credp->fc_gid != -1) {
        gid = credp->fc_gid;
    }
    if (credp->fc_mode != -1) {
        mode = credp->fc_mode;
    }
    if (credp->fc_rdev != -1) {
        rdev = credp->fc_rdev;
    }

    if (uid != -1) {
        fprintf(fp, "virtfs.uid=%d\n", uid);
    }
    if (gid != -1) {
        fprintf(fp, "virtfs.gid=%d\n", gid);
    }
    if (mode != -1) {
        fprintf(fp, "virtfs.mode=%d\n", mode);
    }
    if (rdev != -1) {
        fprintf(fp, "virtfs.rdev=%d\n", rdev);
    }
    fclose(fp);

err_out:
    return ret;
}

#ifndef CONFIG_WIN32
static int local_set_xattr(const char *path, FsCred *credp)
{
    int err = 0;

#ifdef CONFIG_LINUX
    if (credp->fc_uid != -1) {
        err = setxattr(path, "user.virtfs.uid", &credp->fc_uid, sizeof(uid_t),
                0);
        if (err) {
            return err;
        }
    }
    if (credp->fc_gid != -1) {
        err = setxattr(path, "user.virtfs.gid", &credp->fc_gid, sizeof(gid_t),
                0);
        if (err) {
            return err;
        }
    }
    if (credp->fc_mode != -1) {
        err = setxattr(path, "user.virtfs.mode", &credp->fc_mode,
                sizeof(mode_t), 0);
        if (err) {
            return err;
        }
    }
    if (credp->fc_rdev != -1) {
        err = setxattr(path, "user.virtfs.rdev", &credp->fc_rdev,
                sizeof(dev_t), 0);
        if (err) {
            return err;
        }
    }
#else
    /*
     * In case of setxattr on OS X, position parameter has been added.
     * Its purpose is the same as getxattr. Last parameter options is the same as flags on Linux.
     * XATTR_NOFOLLOW / XATTR_CREATE / XATTR_REPLACE
     */
    if (credp->fc_uid != -1) {
        err = setxattr(path, "user.virtfs.uid", &credp->fc_uid, sizeof(uid_t),
                 0, 0);
         if (err) {
             return err;
         }
     }
     if (credp->fc_gid != -1) {
         err = setxattr(path, "user.virtfs.gid", &credp->fc_gid, sizeof(gid_t),
                 0, 0);
         if (err) {
             return err;
         }
     }
     if (credp->fc_mode != -1) {
         err = setxattr(path, "user.virtfs.mode", &credp->fc_mode,
                 sizeof(mode_t), 0, 0);
         if (err) {
             return err;
         }
     }
     if (credp->fc_rdev != -1) {
         err = setxattr(path, "user.virtfs.rdev", &credp->fc_rdev,
                 sizeof(dev_t), 0, 0);
         if (err) {
             return err;
         }
     }
#endif

    return 0;
}
#endif

static int local_post_create_passthrough(FsContext *fs_ctx, const char *path,
                                         FsCred *credp)
{
#ifndef CONFIG_WIN32
    char *buffer = rpath(fs_ctx, path);

    if (lchown(buffer, credp->fc_uid,
                credp->fc_gid) < 0) {
        /*
         * If we fail to change ownership and if we are
         * using security model none. Ignore the error
         */
        if ((fs_ctx->export_flags & V9FS_SEC_MASK) != V9FS_SM_NONE) {
            g_free(buffer);
            return -1;
        }
    }

    if (chmod(buffer, credp->fc_mode & 07777) < 0) {
        g_free(buffer);
        return -1;
    }
#endif
    return 0;
}

static ssize_t local_readlink(FsContext *fs_ctx, V9fsPath *fs_path,
                              char *buf, size_t bufsz)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    ssize_t tsize = -1;
#ifndef CONFIG_WIN32
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(fs_ctx, path);

    if ((fs_ctx->export_flags & V9FS_SM_MAPPED) ||
        (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE)) {
        int fd;
        fd = open(buffer, O_RDONLY | O_NOFOLLOW);
        g_free(buffer);
        if (fd == -1) {
            return -1;
        }
        do {
            tsize = read(fd, (void *)buf, bufsz);
        } while (tsize == -1 && errno == EINTR);
        close(fd);
        return tsize;
    } else if ((fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
               (fs_ctx->export_flags & V9FS_SM_NONE)) {
        tsize = readlink(buffer, buf, bufsz);
        g_free(buffer);
    }
#endif
    return tsize;
}

static int local_close(FsContext *ctx, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    return close(fs->fd);
}

static int local_closedir(FsContext *ctx, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    return closedir(fs->dir);
}

static int local_open(FsContext *ctx, V9fsPath *fs_path,
                      int flags, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(ctx, path);
#ifdef CONFIG_WIN32
    flags |= O_BINARY;
#endif

    fs->fd = open(buffer, flags | O_NOFOLLOW);

    g_free(buffer);
    return fs->fd;
}

static int local_opendir(FsContext *ctx,
                         V9fsPath *fs_path, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(ctx, path);

    fs->dir = opendir(buffer);
    g_free(buffer);
    if (!fs->dir) {
        return -1;
    }
    return 0;
}

static void local_rewinddir(FsContext *ctx, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    return rewinddir(fs->dir);
}

static off_t local_telldir(FsContext *ctx, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    return telldir(fs->dir);
}

static int local_readdir_r(FsContext *ctx, V9fsFidOpenState *fs,
                           struct dirent *entry,
                           struct dirent **result)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int ret;

again:
#ifndef CONFIG_WIN32
    ret = readdir_r(fs->dir, entry, result);
    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        if (!ret && *result != NULL &&
            !strcmp(entry->d_name, VIRTFS_META_DIR)) {
            /* skp the meta data directory */
            goto again;
        }
    }
#else
    ret = errno;
    *result = readdir(fs->dir);
    ret = (ret == errno ? 0 : errno);
    entry = *result;
    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        if (!ret && *result != NULL &&
            !strcmp(entry->d_name, VIRTFS_META_DIR)) {
            /* skp the meta data directory */
            goto again;
        }
    }
#endif
    return ret;
}

static void local_seekdir(FsContext *ctx, V9fsFidOpenState *fs, off_t off)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    return seekdir(fs->dir, off);
}

static ssize_t local_preadv(FsContext *ctx, V9fsFidOpenState *fs,
                            const struct iovec *iov,
                            int iovcnt, off_t offset)
{
#ifdef CONFIG_PREADV
    return preadv(fs->fd, iov, iovcnt, offset);
#else
    int err = lseek(fs->fd, offset, SEEK_SET);
    if (err == -1) {
        return err;
    } else {
        return readv(fs->fd, iov, iovcnt);
    }
#endif
}

static ssize_t local_pwritev(FsContext *ctx, V9fsFidOpenState *fs,
                             const struct iovec *iov,
                             int iovcnt, off_t offset)
{
    ssize_t ret;

#ifdef CONFIG_PREADV
    ret = pwritev(fs->fd, iov, iovcnt, offset);
#else
    int err = lseek(fs->fd, offset, SEEK_SET);
    if (err == -1) {
        return err;
    } else {
        ret = writev(fs->fd, iov, iovcnt);
    }
#endif
#ifdef CONFIG_SYNC_FILE_RANGE
    if (ret > 0 && ctx->export_flags & V9FS_IMMEDIATE_WRITEOUT) {
        /*
         * Initiate a writeback. This is not a data integrity sync.
         * We want to ensure that we don't leave dirty pages in the cache
         * after write when writeout=immediate is sepcified.
         */
        sync_file_range(fs->fd, offset, ret,
                        SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE);
    }
#endif
    return ret;
}

static int local_chmod(FsContext *fs_ctx, V9fsPath *fs_path, FsCred *credp)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int ret = -1;
#ifndef CONFIG_WIN32
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(fs_ctx, path);

    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        ret = local_set_xattr(buffer, credp);
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        g_free(buffer);
        return local_set_mapped_file_attr(fs_ctx, path, credp);
    } else if ((fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
               (fs_ctx->export_flags & V9FS_SM_NONE)) {
        ret = chmod(buffer, credp->fc_mode);
    }
    g_free(buffer);
#endif
    return ret;
}

static int local_mknod(FsContext *fs_ctx, V9fsPath *dir_path,
                       const char *name, FsCred *credp)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err = -1;
#ifndef CONFIG_WIN32
    char *path;
    int serrno = 0;
    V9fsString fullname;
    char *buffer;

    v9fs_string_init(&fullname);
    v9fs_string_sprintf(&fullname, "%s/%s", dir_path->data, name);
    path = fullname.data;
    buffer = rpath(fs_ctx, path);

    /* Determine the security model */
    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        err = mknod(buffer,
                SM_LOCAL_MODE_BITS|S_IFREG, 0);
        if (err == -1) {
            goto out;
        }
        err = local_set_xattr(buffer, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {

        err = mknod(buffer,
                    SM_LOCAL_MODE_BITS|S_IFREG, 0);
        if (err == -1) {
            goto out;
        }
        err = local_set_mapped_file_attr(fs_ctx, path, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if ((fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
               (fs_ctx->export_flags & V9FS_SM_NONE)) {
        err = mknod(buffer, credp->fc_mode,
                credp->fc_rdev);
        if (err == -1) {
            goto out;
        }
        err = local_post_create_passthrough(fs_ctx, path, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    }
    goto out;

err_end:
    remove(buffer);
    errno = serrno;
    g_free(buffer);
out:
    v9fs_string_free(&fullname);
#endif
    return err;
}

static int local_mkdir(FsContext *fs_ctx, V9fsPath *dir_path,
                       const char *name, FsCred *credp)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *path;
    int err = -1;
    int serrno = 0;
    V9fsString fullname;
    char *buffer;

    v9fs_string_init(&fullname);
#ifndef CONFIG_WIN32
    v9fs_string_sprintf(&fullname, "%s/%s", dir_path->data, name);
#else
    v9fs_string_sprintf(&fullname, "%s\\%s", dir_path->data, name);
    while((fullname.data)[strlen(fullname.data)-1] == '\\'){
        (fullname.data)[strlen(fullname.data)-1] = '\0';
    }
#endif
    path = fullname.data;
    buffer = rpath(fs_ctx, path);

#ifndef CONFIG_WIN32
    /* Determine the security model */
    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        err = mkdir(buffer, SM_LOCAL_DIR_MODE_BITS);
        if (err == -1) {
            goto out;
        }
        credp->fc_mode = credp->fc_mode|S_IFDIR;
        err = local_set_xattr(buffer, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        err = mkdir(buffer, SM_LOCAL_DIR_MODE_BITS);
        if (err == -1) {
            goto out;
        }
        credp->fc_mode = credp->fc_mode|S_IFDIR;
        err = local_set_mapped_file_attr(fs_ctx, path, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if ((fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
               (fs_ctx->export_flags & V9FS_SM_NONE)) {
        err = mkdir(buffer, credp->fc_mode);
        if (err == -1) {
            goto out;
        }
        err = local_post_create_passthrough(fs_ctx, path, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    }
#else
    err = mkdir(buffer);
#endif
    goto out;

err_end:
    remove(buffer);
    errno = serrno;
    g_free(buffer);
out:
    v9fs_string_free(&fullname);
    return err;
}

static int local_fstat(FsContext *fs_ctx, int fid_type,
                       V9fsFidOpenState *fs, struct stat *stbuf)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err, fd;

    if (fid_type == P9_FID_DIR) {
#ifndef CONFIG_WIN32
        fd = dirfd(fs->dir);
#else
        fd = fs->fd;
#endif
    } else {
        fd = fs->fd;
    }

    err = fstat(fd, stbuf);
    if (err) {
        return err;
    }
    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
#ifndef CONFIG_WIN32
        /* Actual credentials are part of extended attrs */
        uid_t tmp_uid;
        gid_t tmp_gid;
        mode_t tmp_mode;
        dev_t tmp_dev;

#ifdef CONFIG_LINUX
        if (fgetxattr(fd, "user.virtfs.uid",
                      &tmp_uid, sizeof(uid_t)) > 0) {
            stbuf->st_uid = tmp_uid;
        }
        if (fgetxattr(fd, "user.virtfs.gid",
                      &tmp_gid, sizeof(gid_t)) > 0) {
            stbuf->st_gid = tmp_gid;
        }
        if (fgetxattr(fd, "user.virtfs.mode",
                      &tmp_mode, sizeof(mode_t)) > 0) {
            stbuf->st_mode = tmp_mode;
        }
        if (fgetxattr(fd, "user.virtfs.rdev",
                      &tmp_dev, sizeof(dev_t)) > 0) {
                stbuf->st_rdev = tmp_dev;
        }
#else
        if (fgetxattr(fd, "user.virtfs.uid",
                      &tmp_uid, sizeof(uid_t), 0, 0) > 0) {
            stbuf->st_uid = tmp_uid;
        }
        if (fgetxattr(fd, "user.virtfs.gid",
                      &tmp_gid, sizeof(gid_t), 0, 0) > 0) {
            stbuf->st_gid = tmp_gid;
        }
        if (fgetxattr(fd, "user.virtfs.mode",
                      &tmp_mode, sizeof(mode_t), 0, 0) > 0) {
            stbuf->st_mode = tmp_mode;
        }
        if (fgetxattr(fd, "user.virtfs.rdev",
                      &tmp_dev, sizeof(dev_t), 0, 0) > 0) {
                stbuf->st_rdev = tmp_dev;
        }
#endif
#endif
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        errno = EOPNOTSUPP;
        return -1;
    }
    return err;
}

static int local_open2(FsContext *fs_ctx, V9fsPath *dir_path, const char *name,
                       int flags, FsCred *credp, V9fsFidOpenState *fs)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *path;
    int fd = -1;
    int err = -1;
    int serrno = 0;
    V9fsString fullname;
    char *buffer;

    /*
     * Mark all the open to not follow symlinks
     */
    flags |= O_NOFOLLOW;

    v9fs_string_init(&fullname);
#ifndef CONFIG_WIN32
    v9fs_string_sprintf(&fullname, "%s/%s", dir_path->data, name);
#else
    v9fs_string_sprintf(&fullname, "%s\\%s", dir_path->data, name);
    flags |= O_BINARY;
#endif
    path = fullname.data;
    buffer = rpath(fs_ctx, path);

    /* Determine the security model */
    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        fd = open(buffer, flags, SM_LOCAL_MODE_BITS);
        if (fd == -1) {
            err = fd;
            goto out;
        }
#ifndef CONFIG_WIN32
        credp->fc_mode = credp->fc_mode|S_IFREG;
        /* Set cleint credentials in xattr */
        err = local_set_xattr(buffer, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
#endif
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        fd = open(buffer, flags, SM_LOCAL_MODE_BITS);
        if (fd == -1) {
            err = fd;
            goto out;
        }
        credp->fc_mode = credp->fc_mode|S_IFREG;
        /* Set client credentials in .virtfs_metadata directory files */
        err = local_set_mapped_file_attr(fs_ctx, path, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if ((fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
               (fs_ctx->export_flags & V9FS_SM_NONE)) {
        fd = open(buffer, flags, credp->fc_mode);
        if (fd == -1) {
            err = fd;
            goto out;
        }
        err = local_post_create_passthrough(fs_ctx, path, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    }
    err = fd;
    fs->fd = fd;
    goto out;

err_end:
    close(fd);
    remove(buffer);
    errno = serrno;
    g_free(buffer);
out:
    v9fs_string_free(&fullname);
    return err;
}

static int local_symlink(FsContext *fs_ctx, const char *oldpath,
                         V9fsPath *dir_path, const char *name, FsCred *credp)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err = -1;
#ifndef CONFIG_WIN32
    int serrno = 0;
    char *newpath;
    V9fsString fullname;
    char *buffer;

    int flags = O_CREAT|O_EXCL|O_RDWR|O_NOFOLLOW;

    v9fs_string_init(&fullname);
    v9fs_string_sprintf(&fullname, "%s/%s", dir_path->data, name);
    newpath = fullname.data;
    buffer = rpath(fs_ctx, newpath);

    /* Determine the security model */
    if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        int fd;
        ssize_t oldpath_size, write_size;
        fd = open(buffer, flags, SM_LOCAL_MODE_BITS);
        if (fd == -1) {
            err = fd;
            goto out;
        }
        /* Write the oldpath (target) to the file. */
        oldpath_size = strlen(oldpath);
        do {
            write_size = write(fd, (void *)oldpath, oldpath_size);
        } while (write_size == -1 && errno == EINTR);

        if (write_size != oldpath_size) {
            serrno = errno;
            close(fd);
            err = -1;
            goto err_end;
        }
        close(fd);
        /* Set cleint credentials in symlink's xattr */
        credp->fc_mode = credp->fc_mode|S_IFLNK;
        err = local_set_xattr(buffer, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        int fd;
        ssize_t oldpath_size, write_size;
        fd = open(buffer, flags, SM_LOCAL_MODE_BITS);
        if (fd == -1) {
            err = fd;
            goto out;
        }
        /* Write the oldpath (target) to the file. */
        oldpath_size = strlen(oldpath);
        do {
            write_size = write(fd, (void *)oldpath, oldpath_size);
        } while (write_size == -1 && errno == EINTR);

        if (write_size != oldpath_size) {
            serrno = errno;
            close(fd);
            err = -1;
            goto err_end;
        }
        close(fd);
        /* Set cleint credentials in symlink's xattr */
        credp->fc_mode = credp->fc_mode|S_IFLNK;
        err = local_set_mapped_file_attr(fs_ctx, newpath, credp);
        if (err == -1) {
            serrno = errno;
            goto err_end;
        }
    } else if ((fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
               (fs_ctx->export_flags & V9FS_SM_NONE)) {
        err = symlink(oldpath, buffer);
        if (err) {
            goto out;
        }
        err = lchown(buffer, credp->fc_uid,
                     credp->fc_gid);
        if (err == -1) {
            /*
             * If we fail to change ownership and if we are
             * using security model none. Ignore the error
             */
            if ((fs_ctx->export_flags & V9FS_SEC_MASK) != V9FS_SM_NONE) {
                serrno = errno;
                goto err_end;
            } else
                err = 0;
        }
    }
    goto out;

err_end:
    remove(buffer);
    errno = serrno;
    g_free(buffer);
out:
    v9fs_string_free(&fullname);
#endif
    return err;
}

static int local_link(FsContext *ctx, V9fsPath *oldpath,
                      V9fsPath *dirpath, const char *name)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int ret = 0;
#ifndef CONFIG_WIN32
    V9fsString newpath;
    char *buffer, *buffer1;

    v9fs_string_init(&newpath);
    v9fs_string_sprintf(&newpath, "%s/%s", dirpath->data, name);

    buffer = rpath(ctx, oldpath->data);
    buffer1 = rpath(ctx, newpath.data);
    ret = link(buffer, buffer1);
    g_free(buffer);
    g_free(buffer1);

    /* now link the virtfs_metadata files */
    if (!ret && (ctx->export_flags & V9FS_SM_MAPPED_FILE)) {
        /* Link the .virtfs_metadata files. Create the metada directory */
        ret = local_create_mapped_attr_dir(ctx, newpath.data);
        if (ret < 0) {
            goto err_out;
        }
        buffer = local_mapped_attr_path(ctx, oldpath->data);
        buffer1 = local_mapped_attr_path(ctx, newpath.data);
        ret = link(buffer, buffer1);
        g_free(buffer);
        g_free(buffer1);
        if (ret < 0 && errno != ENOENT) {
            goto err_out;
        }
    }
err_out:
    v9fs_string_free(&newpath);
#endif
    return ret;
}

static int local_truncate(FsContext *ctx, V9fsPath *fs_path, off_t size)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *buffer;
    int ret = -1;
    char *path = fs_path->data;
    buffer = rpath(ctx, path);
#ifndef CONFIG_WIN32
    ret = truncate(buffer, size);
#else
    int fd = open(buffer, O_RDWR | O_BINARY | O_NOFOLLOW);
    if(fd < 0) {
        ret = -1;
    } else {
        ret = ftruncate(fd, size);
        close(fd);
    }
#endif
    g_free(buffer);
    return ret;
}

static int local_rename(FsContext *ctx, const char *oldpath,
        const char *newpath)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err;
    char *buffer, *buffer1;

    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        err = local_create_mapped_attr_dir(ctx, newpath);
        if (err < 0) {
            return err;
        }
        /* rename the .virtfs_metadata files */
        buffer = local_mapped_attr_path(ctx, oldpath);
        buffer1 = local_mapped_attr_path(ctx, newpath);
        err = rename(buffer, buffer1);
        g_free(buffer);
        g_free(buffer1);
        if (err < 0 && errno != ENOENT) {
            return err;
        }
    }
    buffer = rpath(ctx, oldpath);
    buffer1 = rpath(ctx, newpath);
#ifndef CONFIG_WIN32
    err = rename(buffer, buffer1);
    g_free(buffer);
    g_free(buffer1);
    return err;
#else
    if (!MoveFileEx((LPCTSTR)buffer,
                    (LPCTSTR)buffer1, MOVEFILE_REPLACE_EXISTING)){
        err = -(GetLastError());
        ERR("[%d][ >> %s] err: %d\n", __LINE__, __func__, err);
    } else {
        err = 0;
    }
    g_free(buffer);
    g_free(buffer1);
    return err;
#endif
}

static int local_chown(FsContext *fs_ctx, V9fsPath *fs_path, FsCred *credp)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
#ifndef CONFIG_WIN32
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(fs_ctx, path);

    if ((credp->fc_uid == -1 && credp->fc_gid == -1) ||
        (fs_ctx->export_flags & V9FS_SM_PASSTHROUGH) ||
        (fs_ctx->export_flags & V9FS_SM_NONE)) {
        return lchown(buffer, credp->fc_uid, credp->fc_gid);
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED) {
        return local_set_xattr(buffer, credp);
    } else if (fs_ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        return local_set_mapped_file_attr(fs_ctx, path, credp);
    }
    g_free(buffer);
#endif
    return -1;
}

#ifdef CONFIG_WIN32
/* The function that change file last access and modification times for WIN32 */
static int win_utimes (const char *filename, const struct timeval tv[2])
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);

    FILETIME LastAccessTime, LastWriteTime;

    ULARGE_INTEGER tmpTime;

    ULONGLONG WindowsUnixTimeGap = 11644473600; // between 1601.01.01 and 1970.01.01
    ULONGLONG WindowsSecond = 10000000; // 1sec / 100nanosec

    int res = 0;

    HANDLE hFile = CreateFile(filename, FILE_WRITE_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY) {
            res = 0;
        }
        else {
            res = -1;
        }
    } else {
        if (!tv) { // case of NULL
            GetSystemTimeAsFileTime(&LastAccessTime);
            LastWriteTime = LastAccessTime;
        } else {
            tmpTime.QuadPart = UInt32x32To64(WindowsUnixTimeGap + tv[0].tv_sec, WindowsSecond)
                               + tv[0].tv_usec * 10;
            LastAccessTime.dwHighDateTime = tmpTime.HighPart;
            LastAccessTime.dwLowDateTime = tmpTime.LowPart;

            tmpTime.QuadPart = UInt32x32To64(WindowsUnixTimeGap + tv[1].tv_sec, WindowsSecond)
                               + tv[1].tv_usec * 10;
            LastWriteTime.dwHighDateTime = tmpTime.HighPart;
            LastWriteTime.dwLowDateTime = tmpTime.LowPart;
        }
        if (!SetFileTime(hFile, NULL, &LastAccessTime, &LastWriteTime)) {
            res = -1;
        } else {
            res = 0;
        }
    }

    if (hFile) {
        CloseHandle(hFile);
    }
    return res;
}
#endif

static int local_utimensat(FsContext *s, V9fsPath *fs_path,
                           const struct timespec *buf)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *buffer;
    int ret;
    char *path = fs_path->data;
    buffer = rpath(s, path);
#ifndef CONFIG_WIN32
    ret = qemu_utimens(buffer, buf);
    g_free(buffer);
    return ret;
#else
    const char * r_path = buffer;
    struct timeval tv[2], tv_now;
    struct stat st;
    int i;

    /* happy if special cases */
    if (buf[0].tv_nsec == UTIME_OMIT && buf[1].tv_nsec == UTIME_OMIT) {
        return 0;
    }
    if (buf[0].tv_nsec == UTIME_NOW && buf[1].tv_nsec == UTIME_NOW) {
        return win_utimes(r_path, NULL);
    }

    /* prepare for hard cases */
    if (buf[0].tv_nsec == UTIME_NOW || buf[1].tv_nsec == UTIME_NOW) {
        gettimeofday(&tv_now, NULL);
    }
    if (buf[0].tv_nsec == UTIME_OMIT || buf[1].tv_nsec == UTIME_OMIT) {
        _stat(r_path, &st);
    }

    for (i = 0; i < 2; i++) {
        if (buf[i].tv_nsec == UTIME_NOW) {
            tv[i].tv_sec = tv_now.tv_sec;
            tv[i].tv_usec = tv_now.tv_usec;
        } else if (buf[i].tv_nsec == UTIME_OMIT) {
            tv[i].tv_sec = (i == 0) ? st.st_atime : st.st_mtime;
            tv[i].tv_usec = 0;
        } else {
            tv[i].tv_sec = buf[i].tv_sec;
            tv[i].tv_usec = buf[i].tv_nsec / 1000;
        }
    }

    return win_utimes(r_path, &tv[0]);
#endif
}

static int local_remove(FsContext *ctx, const char *path)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err;
    struct stat stbuf;
    char *buffer = rpath(ctx, path);

    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
#ifndef CONFIG_WIN32
        err =  lstat(buffer, &stbuf);
#else
        err = _stat(buffer, &stbuf);
#endif
        g_free(buffer);
        if (err) {
            goto err_out;
        }
        /*
         * If directory remove .virtfs_metadata contained in the
         * directory
         */
        if (S_ISDIR(stbuf.st_mode)) {
#ifndef CONFIG_WIN32
            sprintf(buffer, "%s/%s/%s", ctx->fs_root, path, VIRTFS_META_DIR);
#else
            sprintf(buffer, "%s\\%s\\%s", ctx->fs_root, path, VIRTFS_META_DIR);
#endif
            err = remove(buffer);
            if (err < 0 && errno != ENOENT) {
                /*
                 * We didn't had the .virtfs_metadata file. May be file created
                 * in non-mapped mode ?. Ignore ENOENT.
                 */
                goto err_out;
            }
        }
        /*
         * Now remove the name from parent directory
         * .virtfs_metadata directory
         */
        buffer = local_mapped_attr_path(ctx, path);
        err = remove(buffer);
        g_free(buffer);
        if (err < 0 && errno != ENOENT) {
            /*
             * We didn't had the .virtfs_metadata file. May be file created
             * in non-mapped mode ?. Ignore ENOENT.
             */
            goto err_out;
        }
    }
    buffer = rpath(ctx, path);
    err = remove(buffer);
    g_free(buffer);
err_out:
    return err;
}

static int local_fsync(FsContext *ctx, int fid_type,
                       V9fsFidOpenState *fs, int datasync)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int fd;

    if (fid_type == P9_FID_DIR) {
#ifndef CONFIG_WIN32
        fd = dirfd(fs->dir);
#else
        fd = fs->fd;
#endif
    } else {
        fd = fs->fd;
    }
#ifndef CONFIG_WIN32
    if (datasync) {
        return qemu_fdatasync(fd);
    } else {
        return fsync(fd);
    }
#else
    return _commit(fd);
#endif
}

static int local_statfs(FsContext *s, V9fsPath *fs_path, struct statfs *stbuf)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    char *buffer;
    char *path = fs_path->data;
    buffer = rpath(s, path);

#ifndef CONFIG_WIN32
    return statfs(buffer, stbuf);
#else
    DWORD VolumeSerialNumber,
          MaximumComponentLength,
          FileSystemFlags;

    DWORD SectorsPerCluster,
          BytesPerSector,
          NumberOfFreeClusters,
          TotalNumberOfClusters,
          BytesPerCluster;

    ULARGE_INTEGER TotalNumberOfBytes,
                   TotalNumberOfFreeBytes,
                   FreeBytesAvailableToCaller;

    TCHAR tmpName[PATH_MAX] = {0};
    TCHAR VolumeNameBuffer[PATH_MAX] = {0};
    TCHAR FileSystemNameBuffer[PATH_MAX] = {0};

    strncpy(tmpName, buffer, 3 > PATH_MAX ? PATH_MAX : 3);
    tmpName[4 > PATH_MAX ? PATH_MAX : 4] = '\0';
    LPCTSTR RootPathName = (LPCTSTR)tmpName;

    if (RootPathName == NULL) {
        ERR("[%d][%s] >> err = %d\n", __LINE__, __func__, GetLastError());
        stbuf = NULL;
        return -1;
    }

    if (!GetVolumeInformation(RootPathName, (LPTSTR)&VolumeNameBuffer, PATH_MAX,
                        &VolumeSerialNumber, &MaximumComponentLength, &FileSystemFlags,
                        (LPTSTR)&FileSystemNameBuffer, PATH_MAX)) {
        ERR("[%d][%s] >> err = %d\n", __LINE__, __func__, GetLastError());
        return -1;
    }

    if (!GetDiskFreeSpace(RootPathName, &SectorsPerCluster, &BytesPerSector,
                        &NumberOfFreeClusters, &TotalNumberOfClusters)) {
        SectorsPerCluster = 1;
        BytesPerSector = 1;
        NumberOfFreeClusters = 0;
        TotalNumberOfClusters = 0;
    }

    BytesPerCluster = SectorsPerCluster * BytesPerSector;
    TotalNumberOfBytes.QuadPart = Int32x32To64(TotalNumberOfClusters, BytesPerCluster);
    TotalNumberOfFreeBytes.QuadPart = Int32x32To64(NumberOfFreeClusters, BytesPerCluster);
    FreeBytesAvailableToCaller=TotalNumberOfFreeBytes;

    if (!strcmp(FileSystemNameBuffer, "NTFS")) {
        stbuf->f_type = NTFS_SUPER_MAGIC;
    }
    /* need to check more about other filesystems (ex CD) */
    else {
        stbuf->f_type = MSDOS_SUPER_MAGIC;
    }
    stbuf->f_bsize       = BytesPerSector;
    stbuf->f_blocks      = TotalNumberOfBytes.QuadPart / BytesPerSector;
    stbuf->f_bfree       = TotalNumberOfFreeBytes.QuadPart / BytesPerSector;
    stbuf->f_bavail      = FreeBytesAvailableToCaller.QuadPart / BytesPerSector;
    stbuf->f_files       = stbuf->f_blocks / SectorsPerCluster;
    stbuf->f_ffree       = stbuf->f_bfree / SectorsPerCluster;
    stbuf->f_fsid.val[0] = HIWORD(VolumeSerialNumber);
    stbuf->f_fsid.val[1] = LOWORD(VolumeSerialNumber);
    stbuf->f_namelen     = MaximumComponentLength;
    stbuf->f_spare[0]    = 0;
    stbuf->f_spare[1]    = 0;
    stbuf->f_spare[2]    = 0;
    stbuf->f_spare[3]    = 0;
    stbuf->f_spare[4]    = 0;
    stbuf->f_spare[5]    = 0;

    return 0;
#endif
}

static ssize_t local_lgetxattr(FsContext *ctx, V9fsPath *fs_path,
                               const char *name, void *value, size_t size)
{
#ifndef CONFIG_WIN32
    char *path = fs_path->data;

    return v9fs_get_xattr(ctx, path, name, value, size);
#else
    /* xattr doesn't support on Windows */
    return -1;
#endif
}

static ssize_t local_llistxattr(FsContext *ctx, V9fsPath *fs_path,
                                void *value, size_t size)
{
#ifndef CONFIG_WIN32
    char *path = fs_path->data;

    return v9fs_list_xattr(ctx, path, value, size);
#else
    /* xattr doesn't support on Windows */
    return -1;
#endif
}

static int local_lsetxattr(FsContext *ctx, V9fsPath *fs_path, const char *name,
                           void *value, size_t size, int flags)
{
#ifndef CONFIG_WIN32
    char *path = fs_path->data;

    return v9fs_set_xattr(ctx, path, name, value, size, flags);
#else
    /* xattr doesn't support on Windows */
    return -1;
#endif
}

static int local_lremovexattr(FsContext *ctx, V9fsPath *fs_path,
                              const char *name)
{
#ifndef CONFIG_WIN32
    char *path = fs_path->data;

    return v9fs_remove_xattr(ctx, path, name);
#else
    /* xattr doesn't support on Windows */
    return -1;
#endif
}

static int local_name_to_path(FsContext *ctx, V9fsPath *dir_path,
                              const char *name, V9fsPath *target)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    if (dir_path) {
#ifndef CONFIG_WIN32
        v9fs_string_sprintf((V9fsString *)target, "%s/%s",
                            dir_path->data, name);
#else
        v9fs_string_sprintf((V9fsString *)target, "%s\\%s",
                            dir_path->data, name);
    while((target->data)[strlen(target->data)-1] == '\\'){
        (target->data)[strlen(target->data)-1] = '\0';
    }
#endif
    } else {
        v9fs_string_sprintf((V9fsString *)target, "%s", name);
    }
    /* Bump the size for including terminating NULL */
    target->size++;
    return 0;
}

static int local_renameat(FsContext *ctx, V9fsPath *olddir,
                          const char *old_name, V9fsPath *newdir,
                          const char *new_name)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int ret;
    V9fsString old_full_name, new_full_name;

    v9fs_string_init(&old_full_name);
    v9fs_string_init(&new_full_name);

#ifndef CONFIG_WIN32
    v9fs_string_sprintf(&old_full_name, "%s/%s", olddir->data, old_name);
    v9fs_string_sprintf(&new_full_name, "%s/%s", newdir->data, new_name);
#else
    v9fs_string_sprintf(&old_full_name, "%s\\%s", olddir->data, old_name);
    v9fs_string_sprintf(&new_full_name, "%s\\%s", newdir->data, new_name);
#endif

    ret = local_rename(ctx, old_full_name.data, new_full_name.data);
    v9fs_string_free(&old_full_name);
    v9fs_string_free(&new_full_name);
    return ret;
}

static int local_unlinkat(FsContext *ctx, V9fsPath *dir,
                          const char *name, int flags)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int ret;
    V9fsString fullname;
    char *buffer;

    v9fs_string_init(&fullname);

#ifndef CONFIG_WIN32
    v9fs_string_sprintf(&fullname, "%s/%s", dir->data, name);
#else
    v9fs_string_sprintf(&fullname, "%s\\%s", dir->data, name);
    while(fullname.data[strlen(fullname.data)-1] == '\\'){
        fullname.data[strlen(fullname.data)-1] = '\0';
    }
#endif
    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
#ifdef CONFIG_VIRTFS_DARWIN
        // TODO: find something to replace AT_REMOVEDIR feature.
        if (flags == AT_REMOVEDIR) {
            /*
             * If directory remove .virtfs_metadata contained in the
             * directory
             */
#ifndef CONFIG_WIN32
            sprintf(buffer, "%s/%s/%s", ctx->fs_root,
                    fullname.data, VIRTFS_META_DIR);
#else
            sprintf(buffer, "%s\\%s\\%s", ctx->fs_root,
                    fullname.data, VIRTFS_META_DIR);
#endif
            ret = remove(buffer);
            if (ret < 0 && errno != ENOENT) {
                /*
                 * We didn't had the .virtfs_metadata file. May be file created
                 * in non-mapped mode ?. Ignore ENOENT.
                 */
                goto err_out;
            }
        }
#endif
        /*
         * Now remove the name from parent directory
         * .virtfs_metadata directory.
         */
        buffer = local_mapped_attr_path(ctx, fullname.data);
        ret = remove(buffer);
        g_free(buffer);
        if (ret < 0 && errno != ENOENT) {
            /*
             * We didn't had the .virtfs_metadata file. May be file created
             * in non-mapped mode ?. Ignore ENOENT.
             */
            goto err_out;
        }
    }
    /* Remove the name finally */
    buffer = rpath(ctx, fullname.data);
#ifndef CONFIG_WIN32
    ret = remove(buffer);
#else
    if (flags == AT_REMOVEDIR) { // is dir
        ret = rmdir(buffer);
    } else { //is file
        ret = remove(buffer);
    }
#endif
    g_free(buffer);
    v9fs_string_free(&fullname);

err_out:
    return ret;
}

#ifndef CONFIG_WIN32
static int local_ioc_getversion(FsContext *ctx, V9fsPath *path,
                                mode_t st_mode, uint64_t *st_gen)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err;

#ifdef FS_IOC_GETVERSION
    V9fsFidOpenState fid_open;

    /*
     * Do not try to open special files like device nodes, fifos etc
     * We can get fd for regular files and directories only
     */
    if (!S_ISREG(st_mode) && !S_ISDIR(st_mode)) {
            return 0;
    }
    err = local_open(ctx, path, O_RDONLY, &fid_open);
    if (err < 0) {
        return err;
    }
    err = ioctl(fid_open.fd, FS_IOC_GETVERSION, st_gen);
    local_close(ctx, &fid_open);
#else
    err = -ENOTTY;
#endif
    return err;
}
#endif

static int local_init(FsContext *ctx)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int err = 0;
#ifdef FS_IOC_GETVERSION
    struct statfs stbuf;
#endif

#ifndef CONFIG_WIN32
    if (ctx->export_flags & V9FS_SM_PASSTHROUGH) {
        ctx->xops = passthrough_xattr_ops;
    } else if (ctx->export_flags & V9FS_SM_MAPPED) {
        ctx->xops = mapped_xattr_ops;
    } else if (ctx->export_flags & V9FS_SM_NONE) {
        ctx->xops = none_xattr_ops;
    } else if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        /*
         * xattr operation for mapped-file and passthrough
         * remain same.
         */
        ctx->xops = passthrough_xattr_ops;
    }
#else
    ctx->xops = NULL;
#endif
    ctx->export_flags |= V9FS_PATHNAME_FSCONTEXT;
#ifdef FS_IOC_GETVERSION
    /*
     * use ioc_getversion only if the iocl is definied
     */
    err = statfs(ctx->fs_root, &stbuf);
    if (!err) {
        switch (stbuf.f_type) {
        case EXT2_SUPER_MAGIC:
        case BTRFS_SUPER_MAGIC:
        case REISERFS_SUPER_MAGIC:
        case XFS_SUPER_MAGIC:
            ctx->exops.get_st_gen = local_ioc_getversion;
            break;
        }
    }
#endif
    return err;
}

static int local_parse_opts(QemuOpts *opts, struct FsDriverEntry *fse)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    const char *sec_model = qemu_opt_get(opts, "security_model");
    const char *path = qemu_opt_get(opts, "path");

    if (!sec_model) {
        fprintf(stderr, "security model not specified, "
                "local fs needs security model\nvalid options are:"
                "\tsecurity_model=[passthrough|mapped|none]\n");
        return -1;
    }

    if (!strcmp(sec_model, "passthrough")) {
        fse->export_flags |= V9FS_SM_PASSTHROUGH;
    } else if (!strcmp(sec_model, "mapped") ||
               !strcmp(sec_model, "mapped-xattr")) {
        fse->export_flags |= V9FS_SM_MAPPED;
    } else if (!strcmp(sec_model, "none")) {
        fse->export_flags |= V9FS_SM_NONE;
    } else if (!strcmp(sec_model, "mapped-file")) {
        fse->export_flags |= V9FS_SM_MAPPED_FILE;
    } else {
        fprintf(stderr, "Invalid security model %s specified, valid options are"
                "\n\t [passthrough|mapped-xattr|mapped-file|none]\n",
                sec_model);
        return -1;
    }

    if (!path) {
        fprintf(stderr, "fsdev: No path specified.\n");
        return -1;
    }
    fse->path = g_strdup(path);

    return 0;
}

FileOperations local_ops = {
    .parse_opts = local_parse_opts,
    .init  = local_init,
    .lstat = local_lstat,
    .readlink = local_readlink,
    .close = local_close,
    .closedir = local_closedir,
    .open = local_open,
    .opendir = local_opendir,
    .rewinddir = local_rewinddir,
    .telldir = local_telldir,
    .readdir_r = local_readdir_r,
    .seekdir = local_seekdir,
    .statfs = local_statfs,
    .preadv = local_preadv,
    .pwritev = local_pwritev,
    .truncate = local_truncate,
    .utimensat = local_utimensat,
    .open2 = local_open2,
    .lgetxattr = local_lgetxattr,
    .llistxattr = local_llistxattr,
    .lsetxattr = local_lsetxattr,
    .lremovexattr = local_lremovexattr,
    .mkdir = local_mkdir,
    .symlink = local_symlink,
    .remove = local_remove,
    .fstat = local_fstat,
    .chmod = local_chmod,
    .mknod = local_mknod,
    .link = local_link,
    .rename = local_rename,
    .chown = local_chown,
    .fsync = local_fsync,
    .name_to_path = local_name_to_path,
    .unlinkat = local_unlinkat,
    .renameat  = local_renameat,
};
