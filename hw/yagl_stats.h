#ifndef _QEMU_STATS_H
#define _QEMU_STATS_H

#include "yagl_types.h"

#ifdef CONFIG_YAGL_STATS

void yagl_stats_init(void);

void yagl_stats_cleanup(void);

void yagl_stats_new_ref(void);

void yagl_stats_delete_ref(void);

void yagl_stats_dump(void);

#else
#define yagl_stats_init()
#define yagl_stats_cleanup()
#define yagl_stats_new_ref()
#define yagl_stats_delete_ref()
#define yagl_stats_dump()
#endif

#endif
