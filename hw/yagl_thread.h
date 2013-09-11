#ifndef _QEMU_YAGL_THREAD_H
#define _QEMU_YAGL_THREAD_H

#include "yagl_types.h"
#include "yagl_event.h"
#include "yagl_tls.h"
#include "qemu/queue.h"
#include "qemu/thread.h"

struct yagl_process_state;
struct yagl_mem_transfer;

struct yagl_thread_state
{
    struct yagl_process_state *ps;

    QLIST_ENTRY(yagl_thread_state) entry;

    yagl_tid id;

    /*
     * Memory transfers cached for speed. To be used
     * in EGL/GL implementations.
     * @{
     */

    struct yagl_mem_transfer *mt1;
    struct yagl_mem_transfer *mt2;
    struct yagl_mem_transfer *mt3;
    struct yagl_mem_transfer *mt4;

    /*
     * @}
     */

    QemuThread host_thread;

    volatile bool destroying; /* true when thread is being destroyed */

    volatile bool is_first; /* true when this is first thread of a process */
    volatile bool is_last; /* true when this is last thread of a process */

    /*
     * These events are auto reset.
     * @{
     */
    struct yagl_event call_event; /* Set whenever target calls us */
    struct yagl_event call_processed_event; /* Set whenever host processed target call */
    /*
     * @}
     */

    /*
     * Allocated by the caller of yagl_thread_call,
     * invalidated after function return.
     */
    uint8_t * volatile current_out_buff;
    uint8_t * volatile current_in_buff;

    /*
     * Set by the caller of yagl_thread_call for
     * the time of call. This is the time when host thread can
     * read/write target memory.
     */
    CPUState * volatile current_env;
};

YAGL_DECLARE_TLS(struct yagl_thread_state*, cur_ts);

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id,
                              bool is_first);

void yagl_thread_state_destroy(struct yagl_thread_state *ts,
                               bool is_last);

void yagl_thread_call(struct yagl_thread_state *ts,
                      uint8_t *out_buff,
                      uint8_t *in_buff);

#endif
