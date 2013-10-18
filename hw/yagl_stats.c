#include "yagl_stats.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

#ifdef CONFIG_YAGL_STATS

#define YAGL_STATS_MAX_BATCHES 100

static uint32_t g_num_refs = 0;

static uint32_t g_num_batches = 0;

static uint32_t g_num_calls_min = UINT32_MAX;
static uint32_t g_num_calls_max = 0;

static uint32_t g_bytes_used_min = UINT32_MAX;
static uint32_t g_bytes_used_max = 0;

void yagl_stats_new_ref(void)
{
    ++g_num_refs;
}

void yagl_stats_delete_ref(void)
{
    if (g_num_refs-- == 0) {
        assert(0);
    }
}

void yagl_stats_batch(uint32_t num_calls, uint32_t bytes_used)
{
    if (num_calls > g_num_calls_max) {
        g_num_calls_max = num_calls;
    }

    if (bytes_used > g_bytes_used_max) {
        g_bytes_used_max = bytes_used;
    }

    if (num_calls < g_num_calls_min) {
        g_num_calls_min = num_calls;
    }

    if (bytes_used < g_bytes_used_min) {
        g_bytes_used_min = bytes_used;
    }

    if (++g_num_batches >= YAGL_STATS_MAX_BATCHES) {
        yagl_stats_dump();

        g_num_batches = 0;
        g_num_calls_min = UINT32_MAX;
        g_num_calls_max = 0;
        g_bytes_used_min = UINT32_MAX;
        g_bytes_used_max = 0;
    }
}

void yagl_stats_dump(void)
{
    YAGL_LOG_FUNC_ENTER(yagl_stats_dump, NULL);

    YAGL_LOG_DEBUG("<<STATS");
    YAGL_LOG_DEBUG("# yagl_ref's: %u", g_num_refs);
    YAGL_LOG_DEBUG("# of calls per batch: %u - %u",
                   ((g_num_calls_min == UINT32_MAX) ? 0 : g_num_calls_min),
                   g_num_calls_max);
    YAGL_LOG_DEBUG("# of bytes used per batch: %u - %u",
                   ((g_bytes_used_min == UINT32_MAX) ? 0 : g_bytes_used_min),
                   g_bytes_used_max);
    YAGL_LOG_DEBUG(">>STATS");

    YAGL_LOG_FUNC_EXIT(NULL);
}

#endif
