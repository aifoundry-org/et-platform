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

#include "et_pci_dev.h"
#include "et_sysfs_vq_stats.h"

/*
 * Virtual queue statistics for Mgmt Devices
 *
 * mgmt_vq_stats
 * |- msg_count
 * |- byte_count
 * |- msg_rate
 * |- byte_rate
 * `- utilization_percent
 *
 * e.g.:
 * cat /sys/bus/pci/devices/<bus:function:device>/mgmt_vq_stats/byte_count
 * SQ0:                    0 B
 * CQ0:                    0 B
 */
static ssize_t
msg_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->mgmt.vq_data;

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u: %20llu msg(s)\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&vq_data->sqs[i].stats[ET_VQ_STATS_MSG_COUNT]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u: %20llu msg(s)\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&vq_data->cqs[i].stats[ET_VQ_STATS_MSG_COUNT]));
	}

	return bytes;
}

static ssize_t
byte_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->mgmt.vq_data;

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u: %20llu B\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&vq_data->sqs[i].stats[ET_VQ_STATS_BYTE_COUNT]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u: %20llu B\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&vq_data->cqs[i].stats[ET_VQ_STATS_BYTE_COUNT]));
	}

	return bytes;
}

static ssize_t
msg_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->mgmt.vq_data;

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u: %20llu msg(s)/sec\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&vq_data->sqs[i].stats[ET_VQ_STATS_MSG_RATE]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u: %20llu msg(s)/sec\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&vq_data->cqs[i].stats[ET_VQ_STATS_MSG_RATE]));
	}

	return bytes;
}

static ssize_t
byte_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->mgmt.vq_data;

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u: %20llu B/sec\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&vq_data->sqs[i].stats[ET_VQ_STATS_BYTE_RATE]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u: %20llu B/sec\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&vq_data->cqs[i].stats[ET_VQ_STATS_BYTE_RATE]));
	}

	return bytes;
}

static ssize_t utilization_percent_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->mgmt.vq_data;

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u: %3llu %%\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&vq_data->sqs[i].stats
					 [ET_VQ_STATS_UTILIZATION_PERCENT]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u: %3llu %%\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&vq_data->cqs[i].stats
					 [ET_VQ_STATS_UTILIZATION_PERCENT]));
	}

	return bytes;
}

static ssize_t clear_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	int i, j;
	ssize_t rv;
	unsigned long value;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->mgmt.vq_data;

	rv = kstrtoul(buf, 0, &value);
	if (rv)
		return rv;

	if (value != 1)
		return -EINVAL;

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		for (j = 0; j < ARRAY_SIZE(vq_data->sqs[i].stats); j++)
			atomic64_set(&vq_data->sqs[i].stats[j], 0);
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		for (j = 0; j < ARRAY_SIZE(vq_data->cqs[i].stats); j++)
			atomic64_set(&vq_data->cqs[i].stats[j], 0);
	}

	return count;
}

static DEVICE_ATTR_RO(msg_count);
static DEVICE_ATTR_RO(byte_count);
static DEVICE_ATTR_RO(msg_rate);
static DEVICE_ATTR_RO(byte_rate);
static DEVICE_ATTR_RO(utilization_percent);
static DEVICE_ATTR_WO(clear);

static struct attribute *mgmt_vq_stats_attrs[] = {
	&dev_attr_msg_count.attr,
	&dev_attr_byte_count.attr,
	&dev_attr_msg_rate.attr,
	&dev_attr_byte_rate.attr,
	&dev_attr_utilization_percent.attr,
	&dev_attr_clear.attr,
	NULL,
};

const struct attribute_group et_sysfs_mgmt_vq_stats_attr_group = {
	.name = "mgmt_vq_stats",
	.attrs = mgmt_vq_stats_attrs,
};
