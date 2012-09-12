#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_server.h"
#include "yagl_api.h"
#include "yagl_log.h"
#include "yagl_marshal.h"
#include "kvm.h"
#include "hax.h"

#ifdef TARGET_I386
static __inline void yagl_cpu_synchronize_state(struct yagl_process_state *ps)
{
    if (kvm_enabled() || hax_enabled()) {
        memcpy(&((CPUX86State*)cpu_single_env)->cr[0], &ps->cr[0], sizeof(ps->cr));
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
    int i;

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_thread_func, NULL);

    /*
     * Init APIs.
     */

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->thread_init(ts->ps->api_states[i], ts);
        }
    }

    while (true) {
        struct yagl_api_ps *api_ps;
        yagl_api_func func;

        yagl_event_wait(&ts->call_event);

        if (ts->destroying) {
            break;
        }

        YAGL_LOG_TRACE("calling (api = %u, func = %u)",
                       ts->current_api,
                       ts->current_func);

        api_ps = ts->ps->api_states[ts->current_api - 1];
        assert(api_ps);

        func = api_ps->get_func(api_ps, ts->current_func);

        if (!func) {
            api_ps->bad_call();
            ts->current_out_buff = NULL;
        } else {
            ts->current_out_buff = func(ts,
                                        ts->current_out_buff,
                                        ts->current_in_buff);
        }

        yagl_event_set(&ts->call_processed_event);
    }

    /*
     * Fini APIs.
     */

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->thread_fini(ts->ps->api_states[i]);
        }
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id)
{
    struct yagl_thread_state *ts =
        g_malloc0(sizeof(struct yagl_thread_state));

    ts->ps = ps;
    ts->id = id;

    yagl_event_init(&ts->call_event, 0, 0);
    yagl_event_init(&ts->call_processed_event, 0, 0);

    qemu_thread_create(&ts->host_thread,
                       yagl_thread_func,
                       ts,
                       QEMU_THREAD_JOINABLE);

    return ts;
}

void yagl_thread_state_destroy(struct yagl_thread_state *ts)
{
    ts->destroying = true;
    yagl_event_set(&ts->call_event);
    qemu_thread_join(&ts->host_thread);

    yagl_event_cleanup(&ts->call_processed_event);
    yagl_event_cleanup(&ts->call_event);

    g_free(ts);
}

uint8_t *yagl_thread_call(struct yagl_thread_state *ts,
                          yagl_api_id api_id,
                          yagl_func_id func_id,
                          uint8_t *out_buff,
                          uint8_t *in_buff)
{
    YAGL_LOG_FUNC_ENTER_TS(ts,
                           yagl_thread_call,
                           "api = %u, func = %u",
                           api_id,
                           func_id);

    assert(cpu_single_env);

    ts->current_api = api_id;
    ts->current_func = func_id;
    ts->current_out_buff = out_buff;
    ts->current_in_buff = in_buff;
    ts->current_env = cpu_single_env;

    YAGL_LOG_TRACE("calling (api = %u, func = %u)",
                   ts->current_api,
                   ts->current_func);

    yagl_cpu_synchronize_state(ts->ps);

    yagl_event_set(&ts->call_event);
    yagl_event_wait(&ts->call_processed_event);

    YAGL_LOG_FUNC_EXIT(NULL);

    return ts->current_out_buff;
}
