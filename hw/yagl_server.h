#ifndef _QEMU_YAGL_SERVER_H
#define _QEMU_YAGL_SERVER_H

#include "yagl_types.h"
#include "qemu-queue.h"

struct yagl_api;
struct yagl_process_state;

struct yagl_server_state
{
    struct yagl_api *apis[YAGL_NUM_APIS];

    QLIST_HEAD(, yagl_process_state) processes;
};

/*
 * Create/destroy YaGL server state. Typically, there should be one and only
 * server state which is a part of YaGL device.
 * @{
 */
struct yagl_server_state *yagl_server_state_create(void);

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
                               yagl_pid target_pid,
                               yagl_tid target_tid,
                               uint8_t *out_buff,
                               uint8_t *in_buff);

/*
 * This is called for each host YaGL call. Returns new
 * position of 'out_buff' on success and NULL on failure.
 */
uint8_t *yagl_server_dispatch(struct yagl_server_state *ss,
                              yagl_pid target_pid,
                              yagl_tid target_tid,
                              uint8_t *out_buff,
                              uint8_t *in_buff);

/*
 * This is called for last YaGL call.
 */
void yagl_server_dispatch_exit(struct yagl_server_state *ss,
                               yagl_pid target_pid,
                               yagl_tid target_tid);

#endif
