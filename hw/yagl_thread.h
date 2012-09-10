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

    /*
     * And these are manual reset.
     * @{
     */
    struct yagl_event return_event; /* Set whenever target wants return */
    struct yagl_event return_processed_event; /* Set whenever host processed target return */
    /*
     * @}
     */

    /*
     * Indicates that host thread processed the call. This is needed in order to
     * detect functions that do not set 'call_processed_event' due to errors for
     * example and set it ourselves in thread body.
     */
    volatile bool call_was_processed;

    volatile bool can_return; /* Indicates that host thread is ready to return the value */

    volatile yagl_api_id current_api;

    volatile yagl_func_id current_func;

    /*
     * TARGET_PAGE_SIZE buffers, enough to marshal/unmarshal arguments.
     */

    /*
     * Allocated by the caller of yagl_thread_call/yagl_thread_try_return,
     * invalidated after function return.
     */
    uint8_t * volatile current_out_buff;
    uint8_t * volatile current_in_buff;

    /*
     * Set by the caller of yagl_thread_call/yagl_thread_try_return for
     * the time of call. This is the time when host thread can
     * read/write target memory.
     */
    CPUArchState * volatile current_env;

    /*
     * Allocated here, copied to 'current_in_buff' when needed.
     */
    uint8_t *in_buff;
};

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id);

void yagl_thread_state_destroy(struct yagl_thread_state *ts);

bool yagl_thread_call(struct yagl_thread_state *ts,
                      yagl_api_id api_id,
                      yagl_func_id func_id,
                      uint8_t *out_buff,
                      uint8_t *in_buff,
                      uint32_t* returned);

bool yagl_thread_try_return(struct yagl_thread_state *ts,
                            uint8_t *in_buff,
                            uint32_t* returned);

/*
 * Function below should only be called by API dispatch functions.
 * @{
 */

/*
 * Specifies that a call was processed and the caller can return.
 * You should call 'yagl_thread_wait_return' at some point later.
 * If you don't call 'yagl_thread_wait_return' then it'll be called
 * automatically for you.
 */
void yagl_thread_call_processed_willwait(struct yagl_thread_state *ts);

/*
 * Specifies that a call was processed and the caller can return.
 * You should not call 'yagl_thread_wait_return'.
 */
void yagl_thread_call_processed_nowait(struct yagl_thread_state *ts);

/*
 * Waits for the caller to query for return. If return value is false then
 * the target thread terminated and you should return as soon as possible.
 */
bool yagl_thread_wait_return(struct yagl_thread_state *ts);

/*
 * @}
 */

#define YAGL_BARRIER_ARG(ts) yagl_thread_call_processed_willwait(ts)
#define YAGL_BARRIER_RET(ts) yagl_thread_wait_return(ts)
#define YAGL_BARRIER_ARG_NORET(ts) yagl_thread_call_processed_nowait(ts)

#endif
