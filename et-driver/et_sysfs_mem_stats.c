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

#include "et_sysfs_mem_stats.h"
#include "et_pci_dev.h"

/*
 * CMA memory utilization statistics
 *
 * mem_stats
 * |- cma_allocated		Total CMA allocated by this device instance
 * |- cma_allocation_rate	CMA allocation rate
 * `- cma_utilization_percent	Percentage of CMA as compared to Free system
 *				CMA memory
 */
static ssize_t
cma_allocated_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf,
		"%llu\n",
		atomic64_read(
			&et_dev->ops.mem_stats[ET_MEM_STATS_CMA_ALLOCATED]));
}

static ssize_t cma_allocation_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf,
		"%llu\n",
		atomic64_read(
			&et_dev->ops
				 .mem_stats[ET_MEM_STATS_CMA_ALLOCATION_RATE]));
}

static ssize_t cma_utilization_percent_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(
		buf,
		"%llu\n",
		atomic64_read(&et_dev->ops.mem_stats
				       [ET_MEM_STATS_CMA_UTILIZATION_PERCENT]));
}

static ssize_t clear_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	unsigned long value;
	ssize_t rv;
	int i;

	rv = kstrtoul(buf, 0, &value);
	if (rv)
		return rv;

	if (value != 1)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(et_dev->ops.mem_stats); i++)
		atomic64_set(&et_dev->ops.mem_stats[i], 0);

	return count;
}

static DEVICE_ATTR_RO(cma_allocated);
static DEVICE_ATTR_RO(cma_allocation_rate);
static DEVICE_ATTR_RO(cma_utilization_percent);
static DEVICE_ATTR_WO(clear);

static struct attribute *mem_stats_attrs[] = {
	&dev_attr_cma_allocated.attr,
	&dev_attr_cma_allocation_rate.attr,
	&dev_attr_cma_utilization_percent.attr,
	&dev_attr_clear.attr,
	NULL,
};

const struct attribute_group et_sysfs_mem_stats_attr_group = {
	.name = "mem_stats",
	.attrs = mem_stats_attrs,
};
