#ifndef _QEMU_YAGL_API_H
#define _QEMU_YAGL_API_H

#include "yagl_types.h"

struct yagl_thread_state;
struct yagl_process_state;

/*
 * YaGL API per-process state.
 * @{
 */

struct yagl_api_ps
{
    struct yagl_api *api;

    struct yagl_process_state *ps;

    void (*thread_init)(struct yagl_api_ps */*api_ps*/,
                        struct yagl_thread_state */*ts*/);

    yagl_api_func (*get_func)(struct yagl_api_ps */*api_ps*/,
                              uint32_t /*func_id*/);

    void (*thread_fini)(struct yagl_api_ps */*api_ps*/);

    void (*fini)(struct yagl_api_ps */*api_ps*/);

    void (*destroy)(struct yagl_api_ps */*api_ps*/);
};

void yagl_api_ps_init(struct yagl_api_ps *api_ps,
                      struct yagl_api *api,
                      struct yagl_process_state *ps);
void yagl_api_ps_cleanup(struct yagl_api_ps *api_ps);

/*
 * @}
 */

/*
 * YaGL API.
 * @{
 */

struct yagl_api
{
    struct yagl_api_ps *(*process_init)(struct yagl_api */*api*/,
                                        struct yagl_process_state */*ps*/);

    void (*destroy)(struct yagl_api */*api*/);
};

void yagl_api_init(struct yagl_api *api);
void yagl_api_cleanup(struct yagl_api *api);

/*
 * @}
 */

#endif
