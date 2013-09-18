#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_server.h"
#include "yagl_api.h"
#include "yagl_log.h"
#include "yagl_stats.h"
#include "yagl_transport.h"
#include "sysemu/kvm.h"
#include "sysemu/hax.h"

YAGL_DEFINE_TLS(struct yagl_thread_state*, cur_ts);

#ifdef CONFIG_KVM
static __inline void yagl_cpu_synchronize_state(struct yagl_process_state *ps)
{
    if (kvm_enabled()) {
        memcpy(&((CPUX86State*)current_cpu->env_ptr)->cr[0], &ps->cr[0], sizeof(ps->cr));
    }
}
#else
static __inline void yagl_cpu_synchronize_state(struct yagl_process_state *ps)
{
}
#endif

static void *yagl_thread_func(void* arg)
{
    struct yagl_thread_state *ts = arg;
    struct yagl_transport *t = ts->ps->ss->t;
    int i;

    cur_ts = ts;

    YAGL_LOG_FUNC_ENTER(yagl_thread_func, NULL);

    if (ts->is_first) {
        /*
         * Init APIs (process).
         */

        for (i = 0; i < YAGL_NUM_APIS; ++i) {
            if (ts->ps->ss->apis[i]) {
                ts->ps->api_states[i] = ts->ps->ss->apis[i]->process_init(ts->ps->ss->apis[i]);
                assert(ts->ps->api_states[i]);
            }
        }

        yagl_event_set(&ts->call_processed_event);
    }

    /*
     * Init APIs (thread).
     */

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->thread_init(ts->ps->api_states[i]);
        }
    }

    while (true) {
        uint32_t num_calls = 0;

        yagl_event_wait(&ts->call_event);

        if (ts->destroying) {
            break;
        }

        YAGL_LOG_TRACE("batch started");

        yagl_transport_begin(t, ts->pages, ts->offset);

        while (true) {
            yagl_api_id api_id;
            yagl_func_id func_id;
            struct yagl_api_ps *api_ps;
            yagl_api_func func;

            yagl_transport_begin_call(t, &api_id, &func_id);

            if (api_id == 0) {
                /*
                 * Batch ended.
                 */
                break;
            }

            if ((api_id <= 0) || (api_id > YAGL_NUM_APIS)) {
                YAGL_LOG_CRITICAL("target-host protocol error, bad api_id - %u", api_id);
                break;
            }

            api_ps = ts->ps->api_states[api_id - 1];

            if (!api_ps) {
                YAGL_LOG_CRITICAL("uninitialized api - %u. host logic error", api_id);
                break;
            }

            func = api_ps->get_func(api_ps, func_id);

            if (func) {
                if (func(t)) {
                    yagl_transport_end_call(t);
                } else {
                    /*
                     * Retry is requested.
                     */
                    break;
                }
            } else {
                YAGL_LOG_CRITICAL("bad function call (api = %u, func = %u)",
                                  api_id,
                                  func_id);
                break;
            }

            ++num_calls;
        }

        YAGL_LOG_TRACE("batch ended");

        yagl_stats_batch(num_calls, yagl_transport_bytes_processed(t));

        yagl_event_set(&ts->call_processed_event);
    }

    /*
     * Fini APIs (thread).
     */

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->thread_fini(ts->ps->api_states[i]);
        }
    }

    if (ts->is_last) {
        /*
         * Fini APIs (process).
         */

        for (i = 0; i < YAGL_NUM_APIS; ++i) {
            if (ts->ps->api_states[i]) {
                ts->ps->api_states[i]->fini(ts->ps->api_states[i]);
            }
        }

        /*
         * Destroy APIs (process).
         */

        for (i = 0; i < YAGL_NUM_APIS; ++i) {
            if (ts->ps->api_states[i]) {
                ts->ps->api_states[i]->destroy(ts->ps->api_states[i]);
                ts->ps->api_states[i] = NULL;
            }
        }
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    cur_ts = NULL;

    return NULL;
}

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id,
                              bool is_first)
{
    struct yagl_thread_state *ts =
        g_malloc0(sizeof(struct yagl_thread_state));

    ts->ps = ps;
    ts->id = id;
    ts->is_first = is_first;

    yagl_event_init(&ts->call_event, 0, 0);
    yagl_event_init(&ts->call_processed_event, 0, 0);

    qemu_thread_create(&ts->host_thread,
                       yagl_thread_func,
                       ts,
                       QEMU_THREAD_JOINABLE);

    if (is_first) {
        /*
         * If this is the first thread wait until API process states are
         * initialized.
         */

        yagl_event_wait(&ts->call_processed_event);
    }

    return ts;
}

void yagl_thread_state_destroy(struct yagl_thread_state *ts,
                               bool is_last)
{
    ts->is_last = is_last;
    ts->destroying = true;
    yagl_event_set(&ts->call_event);
    qemu_thread_join(&ts->host_thread);

    yagl_event_cleanup(&ts->call_processed_event);
    yagl_event_cleanup(&ts->call_event);

    yagl_thread_set_buffer(ts, NULL);

    g_free(ts);
}

void yagl_thread_set_buffer(struct yagl_thread_state *ts, uint8_t **pages)
{
    if (ts->pages) {
        uint8_t **tmp;

        for (tmp = ts->pages; *tmp; ++tmp) {
            cpu_physical_memory_unmap(*tmp,
                                      TARGET_PAGE_SIZE,
                                      false,
                                      TARGET_PAGE_SIZE);
        }

        g_free(ts->pages);
        ts->pages = NULL;
    }

    ts->pages = pages;
}

void yagl_thread_call(struct yagl_thread_state *ts, uint32_t offset)
{
    assert(current_cpu);

    ts->current_env = current_cpu;
    ts->offset = offset;

    yagl_cpu_synchronize_state(ts->ps);

    yagl_event_set(&ts->call_event);
    yagl_event_wait(&ts->call_processed_event);

    ts->current_env = NULL;
    ts->offset = 0;
}
