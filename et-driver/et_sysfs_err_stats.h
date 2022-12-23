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

#ifndef __ET_SYSFS_ERR_STATS_H
#define __ET_SYSFS_ERR_STATS_H

#include <linux/atomic.h>
#include <linux/kernel.h>

enum et_err_counter_stats {
	ET_ERR_COUNTER_STATS_DRAM_CE_COUNT = 0,
	ET_ERR_COUNTER_STATS_MINION_CE_COUNT,
	ET_ERR_COUNTER_STATS_PCIE_CE_COUNT,
	ET_ERR_COUNTER_STATS_PMIC_CE_COUNT,
	ET_ERR_COUNTER_STATS_SP_CE_COUNT,
	ET_ERR_COUNTER_STATS_SP_EXCEPT_CE_COUNT,
	ET_ERR_COUNTER_STATS_SRAM_CE_COUNT,
	ET_ERR_COUNTER_STATS_THERM_OVERSHOOT_CE_COUNT,
	ET_ERR_COUNTER_STATS_THERM_THROTTLE_CE_COUNT,
	ET_ERR_COUNTER_STATS_DRAM_UCE_COUNT,
	ET_ERR_COUNTER_STATS_MINION_HANG_UCE_COUNT,
	ET_ERR_COUNTER_STATS_PCIE_UCE_COUNT,
	ET_ERR_COUNTER_STATS_SP_HANG_UCE_COUNT,
	ET_ERR_COUNTER_STATS_SP_WDOG_UCE_COUNT,
	ET_ERR_COUNTER_STATS_SRAM_UCE_COUNT,
	ET_ERR_COUNTER_STATS_SP_TRACE_FULL_CE_COUNT,
	ET_ERR_COUNTER_STATS_MAX_COUNTERS,
};

struct et_err_stats {
	/*
	 * Counters (RO) attributes to present record of correctable and
	 * uncorrectable errors reported by Mgmt VQ interface
	 */
	atomic64_t counters[ET_ERR_COUNTER_STATS_MAX_COUNTERS];
};

static inline void et_err_stats_init(struct et_err_stats *stats)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stats->counters); i++)
		atomic64_set(&stats->counters[i], 0);
}

extern struct attribute_group et_sysfs_err_stats_group;

#endif
