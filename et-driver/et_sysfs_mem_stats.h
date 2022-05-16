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

enum et_mem_stats {
	ET_MEM_STATS_CMA_ALLOCATED = 0,
	ET_MEM_STATS_CMA_ALLOCATION_RATE,
	ET_MEM_STATS_CMA_UTILIZATION_PERCENT,
	ET_MEM_STATS_MAX_ATTRIBUTES,
};

extern const struct attribute_group et_sysfs_mem_stats_attr_group;

#endif
