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

#ifndef __ET_SYSFS_SOC_RESET_H
#define __ET_SYSFS_SOC_RESET_H

#include <linux/atomic.h>

struct et_soc_reset_cfg {
	/*
	 * pcilink_discovery_timeout_ms (RW): Timeout parameter for ETSOC reset.
	 * The driver will wait for this time for PCI link to get stable after
	 * ETSOC reset is triggered. This can be updated at run-time by writing
	 * the new value in milliseconds
	 */
	unsigned long pcilink_discovery_timeout_ms;
	/*
	 * pcilink_max_estim_downtime_ms (RO): The maximum estimated time
	 * for which device remains down for ETSOC reset
	 */
	unsigned long pcilink_max_estim_downtime_ms;
};

static inline void et_soc_reset_cfg_init(struct et_soc_reset_cfg *cfg)
{
	// Default configuration
	cfg->pcilink_discovery_timeout_ms = 10000;
	cfg->pcilink_max_estim_downtime_ms = 300;
}

extern struct attribute_group et_sysfs_soc_reset_group;

#endif
