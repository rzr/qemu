/*
 * Emulator signal handler
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
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

#include "maru_signal.h"
#include "debug_ch.h"
#ifndef _WIN32
#include <sys/ipc.h>  
#include <sys/shm.h> 
#endif

MULTI_DEBUG_CHANNEL(qemu, emulsignal);


static sigset_t cur_sigset, old_sigset;

int sig_block(void) 
{
#ifndef _WIN32
    sigfillset (&cur_sigset);

    if (sigprocmask (SIG_BLOCK, &cur_sigset, &old_sigset) < 0) {
        ERR( "sigprocmask error \n");
    }
#endif
    return 0;

}

int sig_unblock(void) 
{
#ifndef _WIN32
    sigfillset (&cur_sigset);

    if (sigprocmask (SIG_SETMASK, &old_sigset, NULL) < 0) {
        ERR( "sigprocmask error \n");
    }
#endif
    return 0;
}

void sig_handler (int signo)
{
    sigset_t sigset, oldset;
    
    TRACE("signo %d happens\n", signo);
    switch (signo) {
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            sigfillset (&sigset);

            if (sigprocmask (SIG_BLOCK, &sigset, &oldset) < 0) {
                ERR( "sigprocmask %d error \n", signo);
                //exit_emulator();
            }

            if (sigprocmask (SIG_SETMASK, &oldset, NULL) < 0) {
                ERR( "sigprocmask error \n");
            }

            //exit_emulator();
            exit(0);
            break;

       default:
            break;
    }
}

int register_sig_handler(void)
{
#ifndef _WIN32
    signal (SIGINT, sig_handler);
    signal (SIGQUIT, sig_handler);
    signal (SIGTERM, sig_handler);
#endif
    TRACE( "resist sig handler\n");

    return 0;
}

