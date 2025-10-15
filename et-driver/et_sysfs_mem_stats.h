/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 *-----------------------------------------------------------------------------
 */

#ifndef __ET_SYSFS_MEM_STATS_H
#define __ET_SYSFS_MEM_STATS_H

#include <linux/atomic.h>

#include "et_rate_entry.h"

/**
 * enum et_mem_counter_stats - IDs for memory counter statistics
 */
enum et_mem_counter_stats {
	ET_MEM_COUNTER_STATS_CMA_ALLOCATED = 0,
	ET_MEM_COUNTER_STATS_MAX_COUNTERS,
};

/**
 * enum et_mem_rate_stats - IDs for memory rate attribute statistics
 */
enum et_mem_rate_stats {
	ET_MEM_RATE_STATS_CMA_ALLOCATION_RATE = 0,
	ET_MEM_RATE_STATS_MAX_RATES,
};

/**
 * struct et_mem_stats - Memory statistics
 * @counters: (RO) attributes to present record of CMA memory allocation
 * @rates: (RO) attributes to present record of CMA rate of memory allocation
 */
struct et_mem_stats {
	atomic64_t counters[ET_MEM_COUNTER_STATS_MAX_COUNTERS];
	struct et_rate_entry rates[ET_MEM_RATE_STATS_MAX_RATES];
};

/**
 * et_mem_stats_init() - Initialize memory statistics attributes
 * @stats: Pointer to struct et_mem_stats
 */
static inline void et_mem_stats_init(struct et_mem_stats *stats)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stats->counters); i++)
		atomic64_set(&stats->counters[i], 0);

	for (i = 0; i < ARRAY_SIZE(stats->rates); i++)
		et_rate_entry_init(&stats->rates[i]);
}

/* SysFS attribute group for memory statistics */
extern struct attribute_group et_sysfs_mem_stats_group;

#endif
