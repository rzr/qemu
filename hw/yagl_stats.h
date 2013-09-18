#ifndef _QEMU_STATS_H
#define _QEMU_STATS_H

#include "yagl_types.h"

#ifdef CONFIG_YAGL_STATS

void yagl_stats_new_ref(void);
void yagl_stats_delete_ref(void);

void yagl_stats_batch(uint32_t num_calls, uint32_t bytes_used);

void yagl_stats_dump(void);

#else
#define yagl_stats_new_ref()
#define yagl_stats_delete_ref()
#define yagl_stats_batch(num_calls, bytes_used)
#define yagl_stats_dump()
#endif

#endif
