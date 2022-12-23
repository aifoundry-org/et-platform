/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-----------------------------------------------------------------------------
 */

#ifndef __ET_SYSFS_MEM_STATS_H
#define __ET_SYSFS_MEM_STATS_H

#include <linux/atomic.h>

#include "et_rate_entry.h"

enum et_mem_counter_stats {
	ET_MEM_COUNTER_STATS_CMA_ALLOCATED = 0,
	ET_MEM_COUNTER_STATS_MAX_COUNTERS,
};

enum et_mem_rate_stats {
	ET_MEM_RATE_STATS_CMA_ALLOCATION_RATE = 0,
	ET_MEM_RATE_STATS_MAX_RATES,
};

struct et_mem_stats {
	/*
	 * Counter (RO) and `struct et_rate_entry` (RO) attributes to present
	 * record of CMA memory allocation and rate of memory allocation
	 * respectively
	 */
	atomic64_t counters[ET_MEM_COUNTER_STATS_MAX_COUNTERS];
	struct et_rate_entry rates[ET_MEM_RATE_STATS_MAX_RATES];
};

static inline void et_mem_stats_init(struct et_mem_stats *stats)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stats->counters); i++)
		atomic64_set(&stats->counters[i], 0);

	for (i = 0; i < ARRAY_SIZE(stats->rates); i++)
		et_rate_entry_init(&stats->rates[i]);
}

extern struct attribute_group et_sysfs_mem_stats_group;

#endif
