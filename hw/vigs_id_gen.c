#include "vigs_id_gen.h"

static vigsp_surface_id g_id_gen_next = 0;

void vigs_id_gen_init(void)
{
}

void vigs_id_gen_reset(void)
{
    g_id_gen_next = 0;
}

void vigs_id_gen_cleanup(void)
{
}

vigsp_surface_id vigs_id_gen(void)
{
    if (!g_id_gen_next)
    {
        /*
         * 0 ids are invalid.
         */

        ++g_id_gen_next;
    }

    return g_id_gen_next++;
}
