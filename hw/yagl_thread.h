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

    uint8_t **pages;

    /*
     * Fake TLS.
     * @{
     */
    void *egl_api_ts;
    void *gles_api_ts;
    void *egl_offscreen_ts;
    void *egl_onscreen_ts;
    /*
     * @}
     */
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
