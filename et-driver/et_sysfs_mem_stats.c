// SPDX-License-Identifier: GPL-2.0

/*-----------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 *-----------------------------------------------------------------------------
 */

// clang-format off

#include <linux/sizes.h>
#include <linux/sysfs.h>

#include "et_pci_dev.h"
#include "et_sysfs_mem_stats.h"

// clang-format on

/**
 * cma_allocated_show() - Show function for SysFS attribute cma_allocated
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: buffer memory for the attribute
 *
 * Total CMA allocated (MB) by this device instance
 * mem_stats/cma_allocated
 *
 * Return: number of bytes written in buf, negative value on failure
 */
static ssize_t cma_allocated_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf, "%llu MB\n",
		atomic64_read(
			&et_dev->ops.mem_stats
				 .counters[ET_MEM_COUNTER_STATS_CMA_ALLOCATED]) /
			SZ_1M);
}

/**
 * cma_allocation_rate_show() - Show function for attribute cma_allocation_rate
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: buffer memory for the attribute
 *
 * CMA allocation rate (MB/sec)
 * mem_stats/cma_allocation_rate
 *
 * Return: number of bytes written in buf, negative value on failure
 */
static ssize_t cma_allocation_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf, "%llu MB/sec\n",
		et_rate_entry_calculate(
			&et_dev->ops.mem_stats
				 .rates[ET_MEM_RATE_STATS_CMA_ALLOCATION_RATE]) /
			SZ_1M);
}

/* SysFS attributes in group mem_stats */
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
