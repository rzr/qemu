#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_server.h"
#include "yagl_api.h"
#include "yagl_log.h"
#include "yagl_marshal.h"
#include "yagl_stats.h"
#include "yagl_mem_transfer.h"
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
        yagl_call_result res = yagl_call_result_ok;
        uint8_t *current_buff;
        uint8_t *tmp;
#ifdef CONFIG_YAGL_STATS
        uint32_t num_calls = 0;
#endif

        yagl_event_wait(&ts->call_event);

        if (ts->destroying) {
            break;
        }

        current_buff = ts->current_out_buff;

        /*
         * current_buff is:
         *  (yagl_api_id) api_id
         *  (yagl_func_id) func_id
         *  ... api_id/func_id dependent
         * current_in_buff must be:
         *  (uint32_t) 1/0 - call ok/failed
         *  ... api_id/func_id dependent
         */

        while (true) {
            yagl_api_id api_id;
            yagl_func_id func_id;
            struct yagl_api_ps *api_ps;
            yagl_api_func func;

            if (current_buff >= (ts->current_out_buff + YAGL_MARSHAL_SIZE)) {
                YAGL_LOG_CRITICAL("batch passes the end of buffer, protocol error");

                res = yagl_call_result_fail;

                break;
            }

            api_id = yagl_marshal_get_api_id(&current_buff);

            if (api_id == 0) {
                /*
                 * Batch ended.
                 */

                break;
            }

            func_id = yagl_marshal_get_func_id(&current_buff);

            YAGL_LOG_TRACE("calling (api = %u, func = %u)",
                           api_id,
                           func_id);

            if ((api_id <= 0) || (api_id > YAGL_NUM_APIS)) {
                YAGL_LOG_CRITICAL("target-host protocol error, bad api_id - %u", api_id);

                res = yagl_call_result_fail;

                break;
            }

            api_ps = ts->ps->api_states[api_id - 1];

            if (!api_ps) {
                YAGL_LOG_CRITICAL("uninitialized api - %u. host logic error", api_id);

                res = yagl_call_result_fail;

                break;
            }

            func = api_ps->get_func(api_ps, func_id);

            if (func) {
                tmp = ts->current_in_buff;

                yagl_marshal_skip(&tmp);

                if (!func(ts, &current_buff, tmp)) {
                    /*
                     * Retry is requested. Check if this is the last function
                     * in the batch, if it's not, then there's a logic error
                     * in target <-> host interaction.
                     */

                    if (((current_buff + 8) > (ts->current_out_buff + YAGL_MARSHAL_SIZE)) ||
                        (yagl_marshal_get_api_id(&current_buff) != 0)) {
                        YAGL_LOG_CRITICAL("retry request not at the end of the batch");

                        res = yagl_call_result_fail;
                    } else {
                        res = yagl_call_result_retry;
                    }

                    break;
                }
            } else {
                YAGL_LOG_CRITICAL("bad function call (api = %u, func = %u)",
                                  api_id,
                                  func_id);

                res = yagl_call_result_fail;

                break;
            }

#ifdef CONFIG_YAGL_STATS
            ++num_calls;
#endif
        }

        tmp = ts->current_in_buff;

        yagl_stats_batch(num_calls,
            (ts->current_out_buff + YAGL_MARSHAL_SIZE - current_buff));

        yagl_marshal_put_call_result(&tmp, res);

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

    ts->mt1 = yagl_mem_transfer_create(ts);
    ts->mt2 = yagl_mem_transfer_create(ts);
    ts->mt3 = yagl_mem_transfer_create(ts);
    ts->mt4 = yagl_mem_transfer_create(ts);

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

    yagl_mem_transfer_destroy(ts->mt1);
    yagl_mem_transfer_destroy(ts->mt2);
    yagl_mem_transfer_destroy(ts->mt3);
    yagl_mem_transfer_destroy(ts->mt4);

    g_free(ts);
}

void yagl_thread_call(struct yagl_thread_state *ts,
                      uint8_t *out_buff,
                      uint8_t *in_buff)
{
    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_thread_call, NULL);

    assert(cpu_single_env);

    ts->current_out_buff = out_buff;
    ts->current_in_buff = in_buff;
    ts->current_env = cpu_single_env;

    yagl_cpu_synchronize_state(ts->ps);

    yagl_event_set(&ts->call_event);
    yagl_event_wait(&ts->call_processed_event);

    YAGL_LOG_FUNC_EXIT(NULL);
}
