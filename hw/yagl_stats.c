#include "yagl_stats.h"
#include "yagl_log.h"
#include "qemu-thread.h"

#ifdef CONFIG_YAGL_STATS

static uint32_t g_num_refs = 0;
static QemuMutex g_stats_mutex;

void yagl_stats_init(void)
{
    qemu_mutex_init(&g_stats_mutex);
}

void yagl_stats_cleanup(void)
{
    qemu_mutex_destroy(&g_stats_mutex);
}

void yagl_stats_new_ref(void)
{
    qemu_mutex_lock(&g_stats_mutex);
    ++g_num_refs;
    qemu_mutex_unlock(&g_stats_mutex);
}

void yagl_stats_delete_ref(void)
{
    qemu_mutex_lock(&g_stats_mutex);
    if (g_num_refs-- == 0) {
        assert(0);
    }
    qemu_mutex_unlock(&g_stats_mutex);
}

void yagl_stats_dump(void)
{
    uint32_t num_refs;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_stats_dump, NULL);

    qemu_mutex_lock(&g_stats_mutex);
    num_refs = g_num_refs;
    qemu_mutex_unlock(&g_stats_mutex);

    YAGL_LOG_DEBUG("<<STATS");
    YAGL_LOG_DEBUG("num yagl_ref's: %u", num_refs);
    YAGL_LOG_DEBUG(">>STATS");

    YAGL_LOG_FUNC_EXIT(NULL);
}

#endif
