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

        /*
         * At this point we should have api id, func id and out buffer.
         */

        assert(ts->current_api);
        assert(ts->current_func);
        assert(ts->current_out_buff);

        YAGL_LOG_TRACE("calling (api = %u, func = %u)",
                       ts->current_api,
                       ts->current_func);

        api_ps = ts->ps->api_states[ts->current_api - 1];
        assert(api_ps);

        /*
         * Only here we can reset return events, since it's guaranteed that
         * the caller and host thread are in sync at this point.
         */

        yagl_event_reset(&ts->return_event);
        yagl_event_reset(&ts->return_processed_event);

        func = api_ps->get_func(api_ps, ts->current_func);

        ts->call_was_processed = false;

        if (!func) {
            api_ps->bad_call();
        } else {
            func(ts, ts->current_out_buff, ts->in_buff);
        }

        assert(ts->call_was_processed || !ts->can_return);

        YAGL_LOG_TRACE("returned");

        if (!yagl_event_is_set(&ts->return_processed_event)) {
            /*
             * 'YAGL_BARRIER_ARG_NORET' was not called, that's the most common
             * case.
             */
            if (ts->call_was_processed) {
                /*
                 * 'func' did call 'YAGL_BARRIER_ARG'. This is
                 * slow path.
                 */

                /*
                 * In case it didn't call 'YAGL_BARRIER_RET'.
                 */
                if (!yagl_thread_wait_return(ts)) {
                    break;
                }

                /*
                 * At this point caller is waiting for return.
                 */
                assert(ts->current_in_buff);

                memcpy(ts->current_in_buff,
                       ts->in_buff,
                       YAGL_MARSHAL_MAX_RESPONSE);

                ts->current_env = NULL;
                yagl_event_set(&ts->return_processed_event);
            } else {
                /*
                 * 'func' didn't call neither 'YAGL_BARRIER_ARG' nor
                 * 'YAGL_BARRIER_XXX_RET', this is fast path.
                 */
                assert(!ts->can_return);

                /*
                 * We can't be in destroying state cause we know for sure
                 * we're in 'yagl_thread_call'.
                 */
                assert(!ts->destroying);

                /*
                 * And we know for sure that buffer is available at
                 * this point.
                 */
                assert(ts->current_in_buff);

                memcpy(ts->current_in_buff,
                       ts->in_buff,
                       YAGL_MARSHAL_MAX_RESPONSE);

                ts->call_was_processed = true;
                ts->can_return = true;
                ts->current_env = NULL;
                yagl_event_set(&ts->call_processed_event);
                yagl_event_set(&ts->return_processed_event);
            }
        }
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

    yagl_event_init(&ts->return_event, 1, 0);
    yagl_event_init(&ts->return_processed_event, 1, 0);

    ts->in_buff = g_malloc0(TARGET_PAGE_SIZE);

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
    yagl_event_set(&ts->return_event);
    qemu_thread_join(&ts->host_thread);

    g_free(ts->in_buff);

    yagl_event_cleanup(&ts->return_processed_event);
    yagl_event_cleanup(&ts->return_event);

    yagl_event_cleanup(&ts->call_processed_event);
    yagl_event_cleanup(&ts->call_event);

    g_free(ts);
}

bool yagl_thread_call(struct yagl_thread_state *ts,
                      yagl_api_id api_id,
                      yagl_func_id func_id,
                      uint8_t *out_buff,
                      uint8_t *in_buff,
                      uint32_t* returned)
{
    YAGL_LOG_FUNC_ENTER_TS(ts,
                           yagl_thread_call,
                           "api = %u, func = %u",
                           api_id,
                           func_id);

    *returned = 0;

    if (ts->current_api || ts->current_func) {
        YAGL_LOG_CRITICAL(
            "another call (api = %u, func = %u) is already in progress, not scheduling (api = %u, func = %u)",
            ts->current_api, ts->current_func,
            api_id, func_id);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    }

    assert(cpu_single_env);

    ts->current_api = api_id;
    ts->current_func = func_id;
    ts->current_out_buff = out_buff;
    ts->current_in_buff = in_buff;
    ts->current_env = cpu_single_env;

    YAGL_LOG_TRACE("calling to (api = %u, func = %u)",
                   ts->current_api,
                   ts->current_func);

    yagl_cpu_synchronize_state(ts->ps);

    yagl_event_set(&ts->call_event);
    yagl_event_wait(&ts->call_processed_event);

    assert(!ts->current_env);

    if (ts->can_return) {
        /*
         * Fast path, we can return immediately.
         */

        /*
         * We need to set 'current_env' again for the case
         * when working thread has already called
         * 'YAGL_BARRIER_ARG/YAGL_BARRIER_RET' sequence.
         */
        ts->current_env = cpu_single_env;

        yagl_event_set(&ts->return_event);
        yagl_event_wait(&ts->return_processed_event);

        /*
         * We'll have to reset ourselves since the thread
         * might have issued 'ts->current_env = NULL' before
         * our 'ts->current_env = cpu_single_env'
         */
        ts->current_env = NULL;

        ts->current_api = 0;
        ts->current_func = 0;
        ts->can_return = false;

        *returned = 1;
    }

    ts->current_out_buff = NULL;
    ts->current_in_buff = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);

    return true;
}

bool yagl_thread_try_return(struct yagl_thread_state *ts,
                            uint8_t *in_buff,
                            uint32_t* returned)
{
    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_thread_try_return, NULL);

    *returned = 0;

    if (!ts->current_api || !ts->current_func) {
        YAGL_LOG_CRITICAL("no call in progress");

        YAGL_LOG_FUNC_EXIT("false");

        return false;
    }

    if (!ts->can_return) {
        YAGL_LOG_TRACE("still in progress (api = %u, func = %u)",
                       ts->current_api,
                       ts->current_func);

        YAGL_LOG_FUNC_EXIT("true");

        return true;
    }

    YAGL_LOG_TRACE("returning from (api = %u, func = %u)",
                   ts->current_api,
                   ts->current_func);

    assert(cpu_single_env);

    ts->current_in_buff = in_buff;
    ts->current_env = cpu_single_env;

    yagl_cpu_synchronize_state(ts->ps);

    yagl_event_set(&ts->return_event);
    yagl_event_wait(&ts->return_processed_event);

    /*
     * We'll have to reset ourselves since the thread
     * might have issued 'ts->current_env = NULL' before
     * our 'ts->current_env = cpu_single_env'
     */
    ts->current_env = NULL;

    ts->current_api = 0;
    ts->current_func = 0;
    ts->current_in_buff = NULL;
    ts->can_return = false;

    *returned = 1;

    YAGL_LOG_FUNC_EXIT("true");

    return true;
}

void yagl_thread_call_processed_willwait(struct yagl_thread_state *ts)
{
    ts->call_was_processed = true;
    ts->current_env = NULL;
    yagl_event_set(&ts->call_processed_event);
}

void yagl_thread_call_processed_nowait(struct yagl_thread_state *ts)
{
    ts->call_was_processed = true;
    ts->can_return = true;
    ts->current_env = NULL;
    yagl_event_set(&ts->call_processed_event);
    yagl_event_set(&ts->return_processed_event);
}

bool yagl_thread_wait_return(struct yagl_thread_state *ts)
{
    ts->can_return = true;
    yagl_event_wait(&ts->return_event);
    return !ts->destroying;
}
