#ifndef _QEMU_YAGL_CLIENT_CONTEXT_H
#define _QEMU_YAGL_CLIENT_CONTEXT_H

#include "yagl_types.h"

struct yagl_thread_state;
struct yagl_sharegroup;

struct yagl_client_context
{
    yagl_client_api client_api;

    struct yagl_sharegroup *sg;

    /*
     * 'activate' will be called whenever this context becomes
     * current. This function is essential for
     * context-dependent resources set up, since all host GL calls
     * influence the current context only, this is the place where
     * you can actually do host GL calls.
     */
    void (*activate)(struct yagl_client_context */*ctx*/,
                     struct yagl_thread_state */*ts*/);

    void (*flush)(struct yagl_client_context */*ctx*/);

    void (*finish)(struct yagl_client_context */*ctx*/);

    /*
     * Read pixel data from framebuffer.
     */
    bool (*read_pixels)(struct yagl_client_context */*ctx*/,
                        uint32_t /*width*/,
                        uint32_t /*height*/,
                        uint32_t /*bpp*/,
                        void */*pixels*/);

    /*
     * 'deactivate' is called whenever this context resigns
     * current state for the thread. This is the last
     * place where you can still make GL calls.
     *
     * This function is always called if 'activate' was called, even
     * if the target process is terminating.
     */
    void (*deactivate)(struct yagl_client_context */*ctx*/);

    void (*destroy)(struct yagl_client_context */*ctx*/);
};

void yagl_client_context_init(struct yagl_client_context *ctx,
                              yagl_client_api client_api,
                              struct yagl_sharegroup *sg);

void yagl_client_context_cleanup(struct yagl_client_context *ctx);

#endif
