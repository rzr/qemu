#ifndef _QEMU_YAGL_THREAD_H
#define _QEMU_YAGL_THREAD_H

#include "yagl_types.h"
#include "yagl_event.h"
#include "qemu-queue.h"
#include "qemu-thread.h"

struct yagl_process_state;

struct yagl_thread_state
{
    struct yagl_process_state *ps;

    QLIST_ENTRY(yagl_thread_state) entry;

    yagl_tid id;

    QemuThread host_thread;

    volatile bool destroying; /* true when thread is being destroyed */

    /*
     * These events are auto reset.
     * @{
     */
    struct yagl_event call_event; /* Set whenever target calls us */
    struct yagl_event call_processed_event; /* Set whenever host processed target call */
    /*
     * @}
     */

    volatile yagl_api_id current_api;

    volatile yagl_func_id current_func;

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
    CPUArchState * volatile current_env;
};

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id);

void yagl_thread_state_destroy(struct yagl_thread_state *ts);

/*
 * Returns new position of 'out_buff' on success and NULL on failure.
 */
uint8_t *yagl_thread_call(struct yagl_thread_state *ts,
                          yagl_api_id api_id,
                          yagl_func_id func_id,
                          uint8_t *out_buff,
                          uint8_t *in_buff);

#define YAGL_BARRIER_ARG(ts)
#define YAGL_BARRIER_RET(ts) 1
#define YAGL_BARRIER_ARG_NORET(ts)

#endif
