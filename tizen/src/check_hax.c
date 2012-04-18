/*
 * check if hax is available. reference:target-i386/hax-all.c
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * Hyunjun Son <hj79.son@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <windows.h>
#include <winioctl.h>

#define HAX_DEVICE_TYPE 0x4000
#define HAX_IOCTL_CAPABILITY    CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_MAX_VCPU 0x10

#define HAX_CAP_STATUS_NOTWORKING  0x0
#define HAX_CAP_WORKSTATUS_MASK 0x1
#define HAX_CAP_FAILREASON_VT   0x1
#define HAX_CAP_FAILREASON_NX   0x2

#define HAX_CAP_MEMQUOTA    0x2

typedef HANDLE hax_fd;

struct hax_vm {
    hax_fd fd;
    int id;
    struct hax_vcpu_state *vcpus[HAX_MAX_VCPU];
};

struct hax_state {
    hax_fd fd; /* the global hax device interface */
    uint32_t version;
    struct hax_vm *vm;
    uint64_t mem_quota;
};

struct hax_capabilityinfo {
    /* bit 0: 1 - working
     *        0 - not working, possibly because NT/NX disabled
     * bit 1: 1 - memory limitation working
     *        0 - no memory limitation
     */
    uint16_t wstatus;
    /* valid when not working
     * bit 0: VT not enabeld
     * bit 1: NX not enabled*/
    uint16_t winfo;
    uint32_t pad;
    uint64_t mem_quota;
};

static inline int hax_invalid_fd( hax_fd fd ) {
    return ( fd == INVALID_HANDLE_VALUE );
}

static hax_fd hax_mod_open( void );
static int hax_open_device( hax_fd *fd );
static int hax_get_capability( struct hax_state *hax );
static int hax_capability( struct hax_state *hax, struct hax_capabilityinfo *cap );

int check_hax( void ) {

    struct hax_state hax;
    memset( &hax, 0, sizeof( struct hax_state ) );

    hax.fd = hax_mod_open();

    int ret_fd = hax_invalid_fd( hax.fd );
    if ( ret_fd ) {
        fprintf( stderr, "Invalid fd:%d\n", ret_fd );
        return ret_fd;
    }

    int ret_cap = hax_get_capability( &hax );
    if ( ret_cap ) {
        fprintf( stderr, "Not capable:%d\n", ret_cap );
        return ret_cap;
    }

    return 0;

}

static hax_fd hax_mod_open( void ) {
    int ret;
    hax_fd fd;

    ret = hax_open_device( &fd );
    if ( ret != 0 ) {
        fprintf( stderr, "Open HAX device failed\n" );
    }

    return fd;
}

static int hax_open_device( hax_fd *fd ) {
    uint32_t errNum = 0;
    HANDLE hDevice;

    if ( !fd )
        return -2;

    hDevice = CreateFile( "\\\\.\\HAX", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        NULL );

    if ( hDevice == INVALID_HANDLE_VALUE ) {
        fprintf( stderr, "Failed to open the HAX device!\n" );
        errNum = GetLastError();
        if ( errNum == ERROR_FILE_NOT_FOUND )
            return -1;
        return -2;
    }
    *fd = hDevice;
    fprintf( stdout, "device fd:%d\n", *fd );
    return 0;
}

static int hax_get_capability( struct hax_state *hax ) {
    int ret;
    struct hax_capabilityinfo capinfo, *cap = &capinfo;

    ret = hax_capability( hax, cap );
    if ( ret )
        return ret;

    if ( ( ( cap->wstatus & HAX_CAP_WORKSTATUS_MASK ) == HAX_CAP_STATUS_NOTWORKING ) ) {
        if ( cap->winfo & HAX_CAP_FAILREASON_VT )
            fprintf( stderr, "VTX feature is not enabled. which will cause HAX driver not working.\n" );
        else if ( cap->winfo & HAX_CAP_FAILREASON_NX )
            fprintf( stderr, "NX feature is not enabled, which will cause HAX driver not working.\n" );
        return -ENXIO;
    }

    if ( cap->wstatus & HAX_CAP_MEMQUOTA ) {
        if ( cap->mem_quota < hax->mem_quota ) {
            fprintf( stderr, "The memory needed by this VM exceeds the driver limit.\n" );
            return -ENOSPC;
        }
    }
    return 0;
}

static int hax_capability( struct hax_state *hax, struct hax_capabilityinfo *cap ) {
    int ret;
    HANDLE hDevice = hax->fd; //handle to hax module
    DWORD dSize = 0;
    DWORD err = 0;

    if ( hax_invalid_fd( hDevice ) ) {
        fprintf( stderr, "Invalid fd for hax device!\n" );
        return -ENODEV;
    }

    ret = DeviceIoControl( hDevice, HAX_IOCTL_CAPABILITY, NULL, 0, cap, sizeof( *cap ), &dSize, ( LPOVERLAPPED ) NULL );

    if ( !ret ) {
        err = GetLastError();
        if ( err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA )
            fprintf( stderr, "hax capability is too long to hold.\n" );
        fprintf( stderr, "Failed to get Hax capability:%d\n", err );
        return -EFAULT;
    } else
        return 0;

}
