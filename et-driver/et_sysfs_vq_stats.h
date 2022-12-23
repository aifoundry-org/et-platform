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

#ifndef __ET_SYSFS_VQ_STATS_H
#define __ET_SYSFS_VQ_STATS_H

#include <linux/atomic.h>

#include "et_rate_entry.h"

enum et_vq_counter_stats {
	ET_VQ_COUNTER_STATS_MSG_COUNT = 0,
	ET_VQ_COUNTER_STATS_BYTE_COUNT,
	ET_VQ_COUNTER_STATS_MAX_COUNTERS,
};

enum et_vq_rate_stats {
	ET_VQ_RATE_STATS_MSG_RATE = 0,
	ET_VQ_RATE_STATS_BYTE_RATE,
	ET_VQ_RATE_STATS_MAX_RATES,
};

struct et_vq_stats {
	/*
	 * Counters (RO) and `struct et_rate_entry` (RO) attributes to present
	 * record of VQs usage and rate of VQs usage respectively
	 */
	atomic64_t counters[ET_VQ_COUNTER_STATS_MAX_COUNTERS];
	struct et_rate_entry rates[ET_VQ_RATE_STATS_MAX_RATES];
};

static inline void et_vq_stats_init(struct et_vq_stats *stats)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stats->counters); i++)
		atomic64_set(&stats->counters[i], 0);

	for (i = 0; i < ARRAY_SIZE(stats->rates); i++)
		et_rate_entry_init(&stats->rates[i]);
}

extern struct attribute_group et_sysfs_mgmt_vq_stats_group;
extern struct attribute_group et_sysfs_ops_vq_stats_group;

#endif
