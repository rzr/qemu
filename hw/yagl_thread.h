#ifndef _QEMU_YAGL_THREAD_H
#define _QEMU_YAGL_THREAD_H

#include "yagl_types.h"
#include "yagl_event.h"
#include "yagl_tls.h"
#include "qemu/queue.h"
#include "qemu/thread.h"

struct yagl_process_state;

struct yagl_thread_state
{
    struct yagl_process_state *ps;

    QLIST_ENTRY(yagl_thread_state) entry;

    yagl_tid id;

    QemuThread host_thread;

    bool destroying; /* true when thread is being destroyed */

    bool is_first; /* true when this is first thread of a process */
    bool is_last; /* true when this is last thread of a process */

    /*
     * These events are auto reset.
     * @{
     */
    struct yagl_event call_event; /* Set whenever target calls us */
    struct yagl_event call_processed_event; /* Set whenever host processed target call */
    /*
     * @}
     */

    uint8_t **pages;

    /*
     * Set by the caller of yagl_thread_call for
     * the time of call.
     */
    uint32_t offset;
    CPUState *current_env;
};

YAGL_DECLARE_TLS(struct yagl_thread_state*, cur_ts);

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id,
                              bool is_first);

void yagl_thread_state_destroy(struct yagl_thread_state *ts,
                               bool is_last);

void yagl_thread_set_buffer(struct yagl_thread_state *ts, uint8_t **pages);

void yagl_thread_call(struct yagl_thread_state *ts, uint32_t offset);

#endif
