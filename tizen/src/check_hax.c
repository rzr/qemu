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

#include "target-i386/hax-i386.h"
#include "debug_ch.h"
#include "check_hax.h"

MULTI_DEBUG_CHANNEL(qemu, check_hax);

/* Current version */
static uint32_t hax_cur_version = 0x1;
/* Least HAX kernel version */
static uint32_t hax_lest_version = 0x1;

static int hax_get_capability( struct hax_state *hax );
static int hax_version_support( struct hax_state *hax );

int check_hax( void ) {

    struct hax_state hax_stat;
    memset(&hax_stat, 0, sizeof(struct hax_state));

    struct hax_state* hax = &hax_stat;

    hax->fd = hax_mod_open();

    int ret_fd = hax_invalid_fd( hax->fd );
    if ( hax_invalid_fd( hax->fd ) ) {
        ERR( "Invalid fd:%d\n", ret_fd );
        return ret_fd;
    } else {
        TRACE( "Succcess hax_mod_open.\n" );
    }

    int ret_cap = hax_get_capability( hax );
    if ( ret_cap ) {
        ERR( "Not capable:%d\n", ret_cap );
        return ret_cap;
    } else {
        TRACE( "Succcess hax_get_capability.\n" );
    }

    int ret_ver = hax_version_support( hax );
    if ( !ret_ver ) {
        ERR( "Incompat Hax version. Qemu current version %x ", hax_cur_version );
        ERR( "requires least HAX version %x\n", hax_lest_version );
        return !ret_ver;
    } else {
        TRACE( "Succcess hax_version_support.\n" );
    }

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
        dprint( "VTX feature is not enabled. which will cause HAX driver not working.\n" );
        else if ( cap->winfo & HAX_CAP_FAILREASON_NX )
        dprint( "NX feature is not enabled, which will cause HAX driver not working.\n" );
        return -ENXIO;
    }

    if ( cap->wstatus & HAX_CAP_MEMQUOTA ) {
        if ( cap->mem_quota < hax->mem_quota ) {
            dprint( "The memory needed by this VM exceeds the driver limit.\n" );
            return -ENOSPC;
        }
    }
    return 0;
}

static int hax_version_support( struct hax_state *hax ) {
    int ret;
    struct hax_module_version version;

    ret = hax_mod_version( hax, &version );
    if ( ret < 0 )
        return 0;

    if ( ( hax_lest_version > version.cur_version ) || ( hax_cur_version < version.compat_version ) )
        return 0;

    return 1;
}
