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

enum et_err_stats {
	ET_ERR_STATS_DRAM_CE_COUNT = 0,
	ET_ERR_STATS_MINION_CE_COUNT,
	ET_ERR_STATS_PCIE_CE_COUNT,
	ET_ERR_STATS_PMIC_CE_COUNT,
	ET_ERR_STATS_SP_CE_COUNT,
	ET_ERR_STATS_SP_EXCEPT_CE_COUNT,
	ET_ERR_STATS_SRAM_CE_COUNT,
	ET_ERR_STATS_THERM_OVERSHOOT_CE_COUNT,
	ET_ERR_STATS_THERM_THROTTLE_CE_COUNT,
	ET_ERR_STATS_DRAM_UCE_COUNT,
	ET_ERR_STATS_MINION_HANG_UCE_COUNT,
	ET_ERR_STATS_PCIE_UCE_COUNT,
	ET_ERR_STATS_SP_HANG_UCE_COUNT,
	ET_ERR_STATS_SP_WDOG_UCE_COUNT,
	ET_ERR_STATS_SRAM_UCE_COUNT,
	ET_ERR_STATS_MAX_ATTRIBUTES,
};

extern const struct attribute_group et_sysfs_err_stats_attr_group;

#endif
