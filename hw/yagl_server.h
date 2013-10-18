#ifndef _QEMU_YAGL_SERVER_H
#define _QEMU_YAGL_SERVER_H

#include "yagl_types.h"
#include "qemu/queue.h"

struct yagl_api;
struct yagl_process_state;
struct yagl_egl_backend;
struct yagl_gles_driver;
struct yagl_transport;

struct yagl_server_state
{
    yagl_render_type render_type;

    struct yagl_api *apis[YAGL_NUM_APIS];

    QLIST_HEAD(, yagl_process_state) processes;

    /*
     * Single transport that's used by all threads.
     */
    struct yagl_transport *t;
};

/*
 * Create/destroy YaGL server state. Typically, there should be one and only
 * server state which is a part of YaGL device.
 * @{
 */

/*
 * 'egl_backend' and 'gles_driver' will be owned by
 * returned server state or destroyed in case of error.
 */
struct yagl_server_state
    *yagl_server_state_create(struct yagl_egl_backend *egl_backend,
                              struct yagl_gles_driver *gles_driver);

void yagl_server_state_destroy(struct yagl_server_state *ss);
/*
 * @}
 */

/*
 * Don't destroy the state, just drop all processes/threads.
 */
void yagl_server_reset(struct yagl_server_state *ss);

/*
 * This is called for first YaGL call.
 */
bool yagl_server_dispatch_init(struct yagl_server_state *ss,
                               uint8_t *buff,
                               yagl_pid *target_pid,
                               yagl_tid *target_tid);

/*
 * This is called for each YaGL transport buffer update.
 */
void yagl_server_dispatch_update(struct yagl_server_state *ss,
                                 yagl_pid target_pid,
                                 yagl_tid target_tid,
                                 uint8_t *buff);

/*
 * This is called for each YaGL batch.
 */
void yagl_server_dispatch_batch(struct yagl_server_state *ss,
                                yagl_pid target_pid,
                                yagl_tid target_tid,
                                uint32_t offset);

/*
 * This is called for last YaGL call.
 */
void yagl_server_dispatch_exit(struct yagl_server_state *ss,
                               yagl_pid target_pid,
                               yagl_tid target_tid);

#endif
