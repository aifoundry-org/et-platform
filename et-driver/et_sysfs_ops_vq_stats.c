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

#include <linux/sysfs.h>

#include "et_pci_dev.h"
#include "et_sysfs_vq_stats.h"

/*
 * Virtual queue statistics for Ops Devices
 *
 * ops_vq_stats
 * |- msg_count
 * |- byte_count
 * |- msg_rate
 * |- byte_rate
 * `- utilization_percent
 *
 * e.g.:
 * cat /sys/bus/pci/devices/<bus:function:device>/ops_vq_stats/byte_count
 * HpSQ0:                    0 B
 * SQ0:                      0 B
 * CQ0:                      0 B
 */
static ssize_t
msg_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->ops.vq_data;
	struct et_vq_stats *stats;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		stats = &vq_data->hp_sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"HpSQ%u: %20llu msg(s)\n",
			vq_data->hp_sqs[i].index,
			atomic64_read(
				&stats->counters[ET_VQ_COUNTER_STATS_MSG_COUNT]));
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		stats = &vq_data->sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u:   %20llu msg(s)\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&stats->counters[ET_VQ_COUNTER_STATS_MSG_COUNT]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		stats = &vq_data->cqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u:   %20llu msg(s)\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&stats->counters[ET_VQ_COUNTER_STATS_MSG_COUNT]));
	}

	return bytes;
}

static ssize_t
byte_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->ops.vq_data;
	struct et_vq_stats *stats;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		stats = &vq_data->hp_sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"HpSQ%u: %20llu B\n",
			vq_data->hp_sqs[i].index,
			atomic64_read(
				&stats->counters
					 [ET_VQ_COUNTER_STATS_BYTE_COUNT]));
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		stats = &vq_data->sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u:   %20llu B\n",
			vq_data->sqs[i].index,
			atomic64_read(
				&stats->counters
					 [ET_VQ_COUNTER_STATS_BYTE_COUNT]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		stats = &vq_data->cqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u:   %20llu B\n",
			vq_data->cqs[i].index,
			atomic64_read(
				&stats->counters
					 [ET_VQ_COUNTER_STATS_BYTE_COUNT]));
	}

	return bytes;
}

static ssize_t
msg_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->ops.vq_data;
	struct et_vq_stats *stats;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		stats = &vq_data->hp_sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"HpSQ%u: %20llu msg(s)/sec\n",
			vq_data->hp_sqs[i].index,
			et_rate_entry_calculate(
				&stats->rates[ET_VQ_RATE_STATS_MSG_RATE]));
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		stats = &vq_data->sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u:   %20llu msg(s)/sec\n",
			vq_data->sqs[i].index,
			et_rate_entry_calculate(
				&stats->rates[ET_VQ_RATE_STATS_MSG_RATE]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		stats = &vq_data->cqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u:   %20llu msg(s)/sec\n",
			vq_data->cqs[i].index,
			et_rate_entry_calculate(
				&stats->rates[ET_VQ_RATE_STATS_MSG_RATE]));
	}

	return bytes;
}

static ssize_t
byte_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->ops.vq_data;
	struct et_vq_stats *stats;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		stats = &vq_data->hp_sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"HpSQ%u: %20llu B/sec\n",
			vq_data->hp_sqs[i].index,
			et_rate_entry_calculate(
				&stats->rates[ET_VQ_RATE_STATS_BYTE_RATE]));
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		stats = &vq_data->sqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u:   %20llu B/sec\n",
			vq_data->sqs[i].index,
			et_rate_entry_calculate(
				&stats->rates[ET_VQ_RATE_STATS_BYTE_RATE]));
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		stats = &vq_data->cqs[i].stats;
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u:   %20llu B/sec\n",
			vq_data->cqs[i].index,
			et_rate_entry_calculate(
				&stats->rates[ET_VQ_RATE_STATS_BYTE_RATE]));
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
	struct et_vq_data *vq_data = &et_dev->ops.vq_data;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		et_squeue_sync_cb_for_host(&vq_data->hp_sqs[i]);
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"HpSQ%u: %3llu %%\n",
			vq_data->hp_sqs[i].index,
			100 * et_circbuffer_used(&vq_data->hp_sqs[i].cb) /
				vq_data->hp_sqs[i].cb.len);
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		et_squeue_sync_cb_for_host(&vq_data->sqs[i]);
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"SQ%u:   %3llu %%\n",
			vq_data->sqs[i].index,
			100 * et_circbuffer_used(&vq_data->sqs[i].cb) /
				vq_data->sqs[i].cb.len);
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		et_cqueue_sync_cb_for_host(&vq_data->cqs[i]);
		bytes += sysfs_emit_at(
			buf,
			bytes,
			"CQ%u:   %3llu %%\n",
			vq_data->cqs[i].index,
			100 * et_circbuffer_used(&vq_data->cqs[i].cb) /
				vq_data->cqs[i].cb.len);
	}

	return bytes;
}

static ssize_t clear_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	int i;
	ssize_t rv;
	unsigned long value;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_vq_data *vq_data = &et_dev->ops.vq_data;

	rv = kstrtoul(buf, 0, &value);
	if (rv)
		return rv;

	if (value != 1)
		return -EINVAL;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++)
		et_vq_stats_init(&vq_data->hp_sqs[i].stats);

	for (i = 0; i < vq_data->vq_common.sq_count; i++)
		et_vq_stats_init(&vq_data->sqs[i].stats);

	for (i = 0; i < vq_data->vq_common.cq_count; i++)
		et_vq_stats_init(&vq_data->cqs[i].stats);

	return count;
}

static DEVICE_ATTR_RO(msg_count);
static DEVICE_ATTR_RO(byte_count);
static DEVICE_ATTR_RO(msg_rate);
static DEVICE_ATTR_RO(byte_rate);
static DEVICE_ATTR_RO(utilization_percent);
static DEVICE_ATTR_WO(clear);

static struct attribute *ops_vq_stats_attrs[] = {
	&dev_attr_msg_count.attr,
	&dev_attr_byte_count.attr,
	&dev_attr_msg_rate.attr,
	&dev_attr_byte_rate.attr,
	&dev_attr_utilization_percent.attr,
	&dev_attr_clear.attr,
	NULL,
};

struct attribute_group et_sysfs_ops_vq_stats_group = {
	.name = "ops_vq_stats",
	.attrs = ops_vq_stats_attrs,
};
