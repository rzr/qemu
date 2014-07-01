/*
 * Windows resources file for Virtio 9p
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

#ifndef __resources_win32_h__
#define __resources_win32_h__

#include <errno.h>
#include <windows.h>

#ifndef uid_t
#define uid_t int32_t
#endif

#ifndef gid_t
#define gid_t int32_t
#endif

#ifndef UTIME_NOW
#define UTIME_NOW       ((1l << 30) - 1l)
#endif
#ifndef UTIME_OMIT
#define UTIME_OMIT      ((1l << 30) - 2l)
#endif

#ifndef HAVE_STRUCT_TIMESPEC
#define HAVE_STRUCT_TIMESPEC
#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
    time_t  tv_sec;
    long    tv_nsec;
};
#endif /* _TIMESPEC_DEFINED */
#endif /* HAVE_STRUCT_TIMESPEC */

typedef struct {
    WORD val[2];
} fsid_t;

struct statfs {
   DWORD  f_type;
   DWORD  f_bsize;
   DWORD  f_blocks;
   DWORD  f_bfree;
   DWORD  f_bavail;
   DWORD  f_files;
   DWORD  f_ffree;
   fsid_t f_fsid;
   DWORD  f_namelen;
   DWORD  f_spare[6];
};

#define O_NOCTTY           0400
#define O_NONBLOCK        04000
#define O_DSYNC          010000
#define O_DIRECTORY     0200000
#define O_NOFOLLOW      0400000
#define O_DIRECT         040000
#define O_NOATIME      01000000
#define O_SYNC         04010000
#define O_ASYNC          020000
#define O_FSYNC          O_SYNC
#define FASYNC          O_ASYNC

#ifndef EOPNOTSUPP
#define EOPNOTSUPP ENOSYS
#endif

#ifndef ENOBUFS
#define ENOBUFS EFAULT
#endif

#endif
