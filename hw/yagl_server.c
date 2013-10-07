#include "yagl_server.h"
#include "yagl_api.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_version.h"
#include "yagl_log.h"
#include "yagl_transport.h"
#include "yagl_egl_backend.h"
#include "yagl_apis/egl/yagl_egl_api.h"
#include "yagl_apis/gles/yagl_gles_api.h"
#include <GL/gl.h>
#include "yagl_gles_driver.h"

static __inline void yagl_marshal_put_uint32_t(uint8_t** buff, uint32_t value)
{
    *(uint32_t*)(*buff) = value;
    *buff += 8;
}

static __inline uint32_t yagl_marshal_get_uint32_t(uint8_t** buff)
{
    uint32_t tmp = *(uint32_t*)*buff;
    *buff += 8;
    return tmp;
}

static struct yagl_thread_state
    *yagl_server_find_thread(struct yagl_server_state *ss,
                             yagl_pid target_pid,
                             yagl_tid target_tid)
{
    struct yagl_process_state *ps = NULL;

    QLIST_FOREACH(ps, &ss->processes, entry) {
        if (ps->id == target_pid) {
            return yagl_process_find_thread(ps, target_tid);
        }
    }

    return NULL;
}

struct yagl_server_state
    *yagl_server_state_create(struct yagl_egl_backend *egl_backend,
                              struct yagl_gles_driver *gles_driver)
{
    int i;
    struct yagl_server_state *ss =
        g_malloc0(sizeof(struct yagl_server_state));

    QLIST_INIT(&ss->processes);

    ss->t = yagl_transport_create();

    if (!ss->t) {
        egl_backend->destroy(egl_backend);
        gles_driver->destroy(gles_driver);

        goto fail;
    }

    ss->apis[yagl_api_id_egl - 1] = yagl_egl_api_create(egl_backend);

    if (!ss->apis[yagl_api_id_egl - 1]) {
        egl_backend->destroy(egl_backend);
        gles_driver->destroy(gles_driver);

        goto fail;
    }

    ss->apis[yagl_api_id_gles - 1] = yagl_gles_api_create(gles_driver);

    if (!ss->apis[yagl_api_id_gles - 1]) {
        gles_driver->destroy(gles_driver);

        goto fail;
    }

    ss->render_type = egl_backend->render_type;

    return ss;

fail:
    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ss->apis[i]) {
            ss->apis[i]->destroy(ss->apis[i]);
            ss->apis[i] = NULL;
        }
    }

    if (ss->t) {
        yagl_transport_destroy(ss->t);
    }

    g_free(ss);

    return NULL;
}

void yagl_server_state_destroy(struct yagl_server_state *ss)
{
    int i;

    yagl_server_reset(ss);

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ss->apis[i]) {
            ss->apis[i]->destroy(ss->apis[i]);
            ss->apis[i] = NULL;
        }
    }

    yagl_transport_destroy(ss->t);

    g_free(ss);
}

void yagl_server_reset(struct yagl_server_state *ss)
{
    struct yagl_process_state *ps, *next;

    QLIST_FOREACH_SAFE(ps, &ss->processes, entry, next) {
        QLIST_REMOVE(ps, entry);
        yagl_process_state_destroy(ps);
    }

    assert(QLIST_EMPTY(&ss->processes));
}

/*
 * buff is:
 *  IN (uint32_t) version
 *  IN (uint32_t) pid
 *  IN (uint32_t) tid
 *  OUT (uint32_t) res: 1 - init ok, 0 - init error
 *  OUT (uint32_t) render_type: in case of init ok
 */
bool yagl_server_dispatch_init(struct yagl_server_state *ss,
                               uint8_t *buff,
                               yagl_pid *target_pid,
                               yagl_tid *target_tid)
{
    uint8_t *orig_buff;
    uint32_t version;
    struct yagl_process_state *ps = NULL;
    struct yagl_thread_state *ts = NULL;
    uint8_t **pages;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch_init, NULL);

    orig_buff = buff;

    version = yagl_marshal_get_uint32_t(&buff);
    *target_pid = yagl_marshal_get_uint32_t(&buff);
    *target_tid = yagl_marshal_get_uint32_t(&buff);

    if (version != YAGL_VERSION) {
        YAGL_LOG_CRITICAL(
            "target-host version mismatch, target version is %u, host version is %u",
            version,
            YAGL_VERSION);

        yagl_marshal_put_uint32_t(&buff, 0);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    }

    QLIST_FOREACH(ps, &ss->processes, entry) {
        if (ps->id == *target_pid) {
            /*
             * Process already exists.
             */

            ts = yagl_process_find_thread(ps, *target_tid);

            if (ts) {
                YAGL_LOG_CRITICAL(
                    "thread %u already initialized in process %u",
                    *target_tid, *target_pid);

                yagl_marshal_put_uint32_t(&buff, 0);

                YAGL_LOG_FUNC_EXIT(NULL);

                return false;
            } else {
                ts = yagl_process_add_thread(ps, *target_tid);

                if (!ts) {
                    YAGL_LOG_CRITICAL(
                        "cannot add thread %u to process %u",
                        *target_tid, *target_pid);

                    yagl_marshal_put_uint32_t(&buff, 0);

                    YAGL_LOG_FUNC_EXIT(NULL);

                    return false;
                } else {
                    goto out;
                }
            }
        }
    }

    /*
     * No process found, create one.
     */

    ps = yagl_process_state_create(ss, *target_pid);

    if (!ps) {
        YAGL_LOG_CRITICAL("cannot create process %u", *target_pid);

        yagl_marshal_put_uint32_t(&buff, 0);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    }

    ts = yagl_process_add_thread(ps, *target_tid);

    if (!ts) {
        YAGL_LOG_CRITICAL(
            "cannot add thread %u to process %u",
            *target_tid, *target_pid);

        yagl_process_state_destroy(ps);

        yagl_marshal_put_uint32_t(&buff, 0);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    } else {
        QLIST_INSERT_HEAD(&ss->processes, ps, entry);

        goto out;
    }

out:
    pages = g_malloc0(2 * sizeof(*pages));

    pages[0] = orig_buff;

    yagl_thread_set_buffer(ts, pages);

    yagl_marshal_put_uint32_t(&buff, 1);
    yagl_marshal_put_uint32_t(&buff, ss->render_type);

    YAGL_LOG_FUNC_EXIT(NULL);

    return true;
}

/*
 * buff is:
 *  IN (uint32_t) count
 *  IN (uint32_t) phys_addr
 *  ...
 *  IN (uint32_t) phys_addr
 *  OUT (uint32_t) res: 1 - update ok, no change - update error
 */
void yagl_server_dispatch_update(struct yagl_server_state *ss,
                                 yagl_pid target_pid,
                                 yagl_tid target_tid,
                                 uint8_t *buff)
{
    struct yagl_thread_state *ts;
    uint32_t i, count = 0;
    uint8_t **pages = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch_update, NULL);

    ts = yagl_server_find_thread(ss, target_pid, target_tid);

    if (!ts) {
        YAGL_LOG_CRITICAL("process/thread %u/%u not found",
                          target_pid, target_tid);
        goto fail;
    }

    count = yagl_marshal_get_uint32_t(&buff);

    if (count > ((TARGET_PAGE_SIZE / 8) - 2)) {
        YAGL_LOG_CRITICAL("bad count = %u", count);
        goto fail;
    }

    pages = g_malloc0((count + 1) * sizeof(*pages));

    for (i = 0; i < count; ++i) {
        hwaddr page_pa = yagl_marshal_get_uint32_t(&buff);
        hwaddr len = TARGET_PAGE_SIZE;

        pages[i] = cpu_physical_memory_map(page_pa, &len, false);

        if (!pages[i] || (len != TARGET_PAGE_SIZE)) {
            YAGL_LOG_CRITICAL("cpu_physical_memory_map(read) failed for page_pa = 0x%X",
                              (uint32_t)page_pa);
            goto fail;
        }
    }

    yagl_thread_set_buffer(ts, pages);

    yagl_marshal_put_uint32_t(&buff, 1);

    goto out;

fail:
    for (i = count; i > 0; --i) {
        if (pages[i - 1]) {
            cpu_physical_memory_unmap(pages[i - 1],
                                      TARGET_PAGE_SIZE,
                                      false,
                                      TARGET_PAGE_SIZE);
        }
    }
    g_free(pages);

out:
    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_server_dispatch_batch(struct yagl_server_state *ss,
                                yagl_pid target_pid,
                                yagl_tid target_tid,
                                uint32_t offset)
{
    struct yagl_thread_state *ts;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch_batch, NULL);

    ts = yagl_server_find_thread(ss, target_pid, target_tid);

    if (ts) {
        yagl_thread_call(ts, offset);
    } else {
        YAGL_LOG_CRITICAL("process/thread %u/%u not found",
                          target_pid, target_tid);
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_server_dispatch_exit(struct yagl_server_state *ss,
                               yagl_pid target_pid,
                               yagl_tid target_tid)
{
    struct yagl_thread_state *ts =
        yagl_server_find_thread(ss, target_pid, target_tid);
    struct yagl_process_state *ps = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch_exit, NULL);

    if (!ts) {
        YAGL_LOG_CRITICAL("process/thread %u/%u not found",
                          target_pid, target_tid);

        YAGL_LOG_FUNC_EXIT(NULL);

        return;
    }

    ps = ts->ps;

    yagl_process_remove_thread(ps, target_tid);

    if (!yagl_process_has_threads(ps)) {
        QLIST_REMOVE(ps, entry);

        yagl_process_state_destroy(ps);
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}
