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

// clang-format off

#include <linux/sizes.h>
#include <linux/sysfs.h>

#include "et_pci_dev.h"
#include "et_sysfs_mem_stats.h"

// clang-format on

/*
 * CMA memory utilization statistics
 *
 * mem_stats
 * |- cma_allocated		Total CMA allocated (MB) by this device instance
 * |- cma_allocation_rate	CMA allocation rate (MB/sec)
 */
static ssize_t
cma_allocated_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf,
		"%llu MB\n",
		atomic64_read(
			&et_dev->ops.mem_stats
				 .counters[ET_MEM_COUNTER_STATS_CMA_ALLOCATED]) /
			SZ_1M);
}

static ssize_t cma_allocation_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf,
		"%llu MB/sec\n",
		et_rate_entry_calculate(
			&et_dev->ops.mem_stats
				 .rates[ET_MEM_RATE_STATS_CMA_ALLOCATION_RATE]) /
			SZ_1M);
}

static DEVICE_ATTR_RO(cma_allocated);
static DEVICE_ATTR_RO(cma_allocation_rate);

static struct attribute *mem_stats_attrs[] = {
	&dev_attr_cma_allocated.attr,
	&dev_attr_cma_allocation_rate.attr,
	NULL,
};

struct attribute_group et_sysfs_mem_stats_group = {
	.name = "mem_stats",
	.attrs = mem_stats_attrs,
};
