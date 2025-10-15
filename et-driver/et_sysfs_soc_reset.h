/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 *-----------------------------------------------------------------------------
 */

#ifndef __ET_SYSFS_SOC_RESET_H
#define __ET_SYSFS_SOC_RESET_H

#include <linux/atomic.h>

/**
 * struct et_soc_reset_cfg - ETSOC reset configuration
 * @pcilink_discovery_timeout_ms: (RW) Timeout parameter for ETSOC reset. The
 * driver will wait for this time for PCI link to get stable after ETSOC reset
 * is triggered. This can be updated at run-time by writing the new value in
 * milliseconds
 * @pcilink_max_estim_downtime_ms: (RO) The maximum estimated time for which
 * device remains down for ETSOC reset
 */
struct et_soc_reset_cfg {
	unsigned long pcilink_discovery_timeout_ms;
	unsigned long pcilink_max_estim_downtime_ms;
};

/**
 * et_soc_reset_cfg_init() - Initialize ETSOC reset default configuration
 * @cfg: Pointer to struct et_soc_reset_cfg
 */
static inline void et_soc_reset_cfg_init(struct et_soc_reset_cfg *cfg)
{
	cfg->pcilink_discovery_timeout_ms = 10000;
	cfg->pcilink_max_estim_downtime_ms = 300;
}

/* SysFS attribute group for ETSOC reset statistics */
extern struct attribute_group et_sysfs_soc_reset_group;

#endif
