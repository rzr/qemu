#include "yagl_stats.h"
#include "yagl_log.h"
#include "qemu-thread.h"

#ifdef CONFIG_YAGL_STATS

#define YAGL_STATS_MAX_BATCHES 100

static uint32_t g_num_refs = 0;

static uint32_t g_num_batches = 0;
static uint32_t g_num_calls = 0;
static uint32_t g_bytes_unused = UINT32_MAX;

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

void yagl_stats_batch(uint32_t num_calls, uint32_t bytes_unused)
{
    qemu_mutex_lock(&g_stats_mutex);

    if (num_calls > g_num_calls) {
        g_num_calls = num_calls;
    }

    if (bytes_unused < g_bytes_unused) {
        g_bytes_unused = bytes_unused;
    }

    if (++g_num_batches >= YAGL_STATS_MAX_BATCHES) {
        qemu_mutex_unlock(&g_stats_mutex);
        yagl_stats_dump();
        qemu_mutex_lock(&g_stats_mutex);

        g_num_batches = 0;
        g_num_calls = 0;
        g_bytes_unused = UINT32_MAX;
    }

    qemu_mutex_unlock(&g_stats_mutex);
}

void yagl_stats_dump(void)
{
    YAGL_LOG_FUNC_ENTER_NPT(yagl_stats_dump, NULL);

    qemu_mutex_lock(&g_stats_mutex);

    YAGL_LOG_DEBUG("<<STATS");
    YAGL_LOG_DEBUG("num yagl_ref's: %u", g_num_refs);
    YAGL_LOG_DEBUG("# of calls per batch: %u",
                   g_num_calls);
    YAGL_LOG_DEBUG("# of bytes unused per batch: %u",
                   ((g_bytes_unused == UINT32_MAX) ? 0 : g_bytes_unused));
    YAGL_LOG_DEBUG(">>STATS");

    qemu_mutex_unlock(&g_stats_mutex);

    YAGL_LOG_FUNC_EXIT(NULL);
}

#endif
