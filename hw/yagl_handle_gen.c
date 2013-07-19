#include "yagl_handle_gen.h"

static yagl_host_handle g_handle_gen_next = 0;

void yagl_handle_gen_init(void)
{
}

void yagl_handle_gen_reset(void)
{
    g_handle_gen_next = 0;
}

void yagl_handle_gen_cleanup(void)
{
}

yagl_host_handle yagl_handle_gen(void)
{
    yagl_host_handle ret;

    if (!g_handle_gen_next)
    {
        /*
         * 0 handles are invalid.
         */

        ++g_handle_gen_next;
    }
    ret = g_handle_gen_next++;

    return ret;
}
