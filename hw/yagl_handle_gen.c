#include "yagl_handle_gen.h"
#include "qemu-thread.h"

static yagl_host_handle g_handle_gen_next = 0;
static QemuMutex g_handle_gen_mutex;

void yagl_handle_gen_init(void)
{
    qemu_mutex_init(&g_handle_gen_mutex);
}

void yagl_handle_gen_reset(void)
{
    qemu_mutex_lock(&g_handle_gen_mutex);
    g_handle_gen_next = 0;
    qemu_mutex_unlock(&g_handle_gen_mutex);
}

void yagl_handle_gen_cleanup(void)
{
    qemu_mutex_destroy(&g_handle_gen_mutex);
}

yagl_host_handle yagl_handle_gen(void)
{
    yagl_host_handle ret;

    qemu_mutex_lock(&g_handle_gen_mutex);
    if (!g_handle_gen_next)
    {
        /*
         * 0 handles are invalid.
         */

        ++g_handle_gen_next;
    }
    ret = g_handle_gen_next++;
    qemu_mutex_unlock(&g_handle_gen_mutex);

    return ret;
}
