#ifndef _QEMU_YAGL_PROCESS_H
#define _QEMU_YAGL_PROCESS_H

#include "yagl_types.h"
#include "qemu-queue.h"

struct yagl_server_state;
struct yagl_thread_state;
struct yagl_egl_interface;
struct yagl_client_interface;
struct yagl_api_ps;

struct yagl_process_state
{
    struct yagl_server_state *ss;

    QLIST_ENTRY(yagl_process_state) entry;

    yagl_pid id;

    struct yagl_api_ps *api_states[YAGL_NUM_APIS];

    struct yagl_egl_interface *egl_iface;
    struct yagl_client_interface *client_ifaces[YAGL_NUM_CLIENT_APIS];

    QLIST_HEAD(, yagl_thread_state) threads;

#ifdef TARGET_I386
    target_ulong cr[5];
#endif
};

struct yagl_process_state
    *yagl_process_state_create(struct yagl_server_state *ss,
                               yagl_pid id);

void yagl_process_state_destroy(struct yagl_process_state *ps);

void yagl_process_register_egl_interface(struct yagl_process_state *ps,
                                         struct yagl_egl_interface *egl_iface);

void yagl_process_unregister_egl_interface(struct yagl_process_state *ps);

void yagl_process_register_client_interface(struct yagl_process_state *ps,
                                            yagl_client_api client_api,
                                            struct yagl_client_interface *client_iface);

void yagl_process_unregister_client_interface(struct yagl_process_state *ps,
                                              yagl_client_api client_api);

struct yagl_thread_state*
    yagl_process_find_thread(struct yagl_process_state *ps,
                            yagl_tid id);

struct yagl_thread_state*
    yagl_process_add_thread(struct yagl_process_state *ps,
                            yagl_tid id);

void yagl_process_remove_thread(struct yagl_process_state *ps,
                                yagl_tid id);

bool yagl_process_has_threads(struct yagl_process_state *ps);

#endif
