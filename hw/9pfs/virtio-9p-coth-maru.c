/*
 * Virtio 9p backend for Maru
 * Based on hw/9pfs/virtio-9p-coth.c:
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

#include "fsdev/qemu-fsdev.h"
#include "qemu/thread.h"
#include "block/coroutine.h"
#include "virtio-9p-coth.h"

#ifdef CONFIG_WIN32
#include <winsock2.h>
#endif

#include "../../tizen/src/debug_ch.h"
MULTI_DEBUG_CHANNEL(tizen, qemu_9p_coth);

/* v9fs glib thread pool */
static V9fsThPool v9fs_pool;

void co_run_in_worker_bh(void *opaque)
{
    Coroutine *co = opaque;
    g_thread_pool_push(v9fs_pool.pool, co, NULL);
}

static void v9fs_qemu_process_req_done(void *arg)
{
    char byte;
    ssize_t len;
    Coroutine *co;

    do {
#ifndef CONFIG_WIN32
        len = read(v9fs_pool.rfd, &byte, sizeof(byte));
#else
        struct sockaddr_in saddr;
        int recv_size = sizeof(saddr);
        len = recvfrom((SOCKET)v9fs_pool.rfd, &byte, sizeof(byte), 0, (struct sockaddr *)&saddr, &recv_size);
#endif
    } while (len == -1 &&  errno == EINTR);

    while ((co = g_async_queue_try_pop(v9fs_pool.completed)) != NULL) {
        qemu_coroutine_enter(co, NULL);
    }
}

static void v9fs_thread_routine(gpointer data, gpointer user_data)
{
    ssize_t len = 0;
    char byte = 0;
    Coroutine *co = data;

    qemu_coroutine_enter(co, NULL);

    g_async_queue_push(v9fs_pool.completed, co);
    do {
#ifndef CONFIG_WIN32
        len = write(v9fs_pool.wfd, &byte, sizeof(byte));
#else
        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        saddr.sin_port=htons(9190);
        len = sendto((SOCKET)v9fs_pool.wfd, &byte, sizeof(byte), 0, (const struct sockaddr *)&saddr, sizeof(saddr));
#endif
    } while (len == -1 && errno == EINTR);
}

int v9fs_init_worker_threads(void)
{
    TRACE("[%d][ Enter >> %s]\n", __LINE__, __func__);
    int ret = 0;
    V9fsThPool *p = &v9fs_pool;
#ifndef CONFIG_WIN32
    int notifier_fds[2];
    sigset_t set, oldset;

    sigfillset(&set);
    /* Leave signal handling to the iothread.  */
    pthread_sigmask(SIG_SETMASK, &set, &oldset);

    if (qemu_pipe(notifier_fds) == -1) {
        ret = -1;
        goto err_out;
    }
#else
    WSADATA wsadata;
    struct sockaddr_in saddr;

    WSAStartup(MAKEWORD(2,2),&wsadata);

    SOCKET pSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port=htons(9190);

    if(bind(pSock, (const struct sockaddr *)&saddr, sizeof(saddr)) == -1){
        ERR("[%d][%s] >> bind err: %d \n", __LINE__, __func__, WSAGetLastError());
    } else {
        TRACE("[%d][%s] >> bind ok\n", __LINE__, __func__);
    }
#endif
    p->pool = g_thread_pool_new(v9fs_thread_routine, p, -1, FALSE, NULL);
    if (!p->pool) {
        ret = -1;
        goto err_out;
    }
    p->completed = g_async_queue_new();
    if (!p->completed) {
        /*
         * We are going to terminate.
         * So don't worry about cleanup
         */
        ret = -1;
        goto err_out;
    }
#ifndef CONFIG_WIN32
    p->rfd = notifier_fds[0];
    p->wfd = notifier_fds[1];

    fcntl(p->rfd, F_SETFL, O_NONBLOCK);
    fcntl(p->wfd, F_SETFL, O_NONBLOCK);
#else
    p->rfd = (int)pSock;
    p->wfd = (int)pSock;
#endif

    qemu_set_fd_handler(p->rfd, v9fs_qemu_process_req_done, NULL, NULL);
err_out:
#ifndef CONFIG_WIN32
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
#endif
    TRACE("[%d][ Leave >> %s]\n", __LINE__, __func__);
    return ret;
}
