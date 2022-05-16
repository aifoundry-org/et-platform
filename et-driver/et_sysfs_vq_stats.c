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

#include "et_sysfs_vq_stats.h"
#include "et_pci_dev.h"

/*
 * Virtual queue statistics for Mgmt and Ops Devices
 *
 * vq_stats
 * |- msg_count
 * |- byte_count
 * |- msg_rate
 * |- byte_rate
 * `- utilization_percent
 *
 * e.g.:
 * cat /sys/bus/pci/devices/<bus:function:device>/vq_stats/byte_count
 * MgmtSQ0:                     0 B
 * MgmtCQ0:                     0 B
 * OpsHpSQ0:                    0 B
 * OpsHpSQ1:                    0 B
 * OpsSQ0:                      0 B
 * OpsSQ1:                      0 B
 * OpsCQ0:                      0 B
 */
static ssize_t
msg_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	if (atomic_read(&et_dev->mgmt.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->mgmt.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtSQ%u:  %20llu msg(s)\n",
				et_dev->mgmt.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.sq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_COUNT]));
		}

		for (i = 0; i < et_dev->mgmt.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtCQ%u:  %20llu msg(s)\n",
				et_dev->mgmt.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.cq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_COUNT]));
		}
	}

	if (atomic_read(&et_dev->ops.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->ops.vq_common.hpsq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsHpSQ%u: %20llu msg(s)\n",
				et_dev->ops.hpsq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.hpsq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_COUNT]));
		}

		for (i = 0; i < et_dev->ops.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsSQ%u:   %20llu msg(s)\n",
				et_dev->ops.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.sq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_COUNT]));
		}

		for (i = 0; i < et_dev->ops.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsCQ%u:   %20llu msg(s)\n",
				et_dev->ops.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.cq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_COUNT]));
		}
	}

	return bytes;
}

static ssize_t
byte_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	if (atomic_read(&et_dev->mgmt.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->mgmt.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtSQ%u:  %20llu B\n",
				et_dev->mgmt.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.sq_pptr[i]->stats
						 [ET_VQ_STATS_BYTE_COUNT]));
		}

		for (i = 0; i < et_dev->mgmt.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtCQ%u:  %20llu B\n",
				et_dev->mgmt.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.cq_pptr[i]->stats
						 [ET_VQ_STATS_BYTE_COUNT]));
		}
	}

	if (atomic_read(&et_dev->ops.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->ops.vq_common.hpsq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsHpSQ%u: %20llu B\n",
				et_dev->ops.hpsq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.hpsq_pptr[i]->stats
						 [ET_VQ_STATS_BYTE_COUNT]));
		}

		for (i = 0; i < et_dev->ops.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsSQ%u:   %20llu B\n",
				et_dev->ops.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.sq_pptr[i]->stats
						 [ET_VQ_STATS_BYTE_COUNT]));
		}

		for (i = 0; i < et_dev->ops.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsCQ%u:   %20llu B\n",
				et_dev->ops.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.cq_pptr[i]->stats
						 [ET_VQ_STATS_BYTE_COUNT]));
		}
	}

	return bytes;
}

static ssize_t
msg_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	if (atomic_read(&et_dev->mgmt.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->mgmt.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtSQ%u:  %20llu msg(s)/sec\n",
				et_dev->mgmt.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.sq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_RATE]));
		}

		for (i = 0; i < et_dev->mgmt.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtCQ%u:  %20llu msg(s)/sec\n",
				et_dev->mgmt.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.cq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_RATE]));
		}
	}

	if (atomic_read(&et_dev->ops.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->ops.vq_common.hpsq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsHpSQ%u: %20llu msg(s)/sec\n",
				et_dev->ops.hpsq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.hpsq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_RATE]));
		}

		for (i = 0; i < et_dev->ops.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsSQ%u:   %20llu msg(s)/sec\n",
				et_dev->ops.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.sq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_RATE]));
		}

		for (i = 0; i < et_dev->ops.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsCQ%u:   %20llu msg(s)/sec\n",
				et_dev->ops.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.cq_pptr[i]
						 ->stats[ET_VQ_STATS_MSG_RATE]));
		}
	}

	return bytes;
}

static ssize_t
byte_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int i;
	ssize_t bytes = 0;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	if (atomic_read(&et_dev->mgmt.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->mgmt.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtSQ%u:  %20llu B/sec\n",
				et_dev->mgmt.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.sq_pptr[i]
						 ->stats[ET_VQ_STATS_BYTE_RATE]));
		}

		for (i = 0; i < et_dev->mgmt.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtCQ%u:  %20llu B/sec\n",
				et_dev->mgmt.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.cq_pptr[i]
						 ->stats[ET_VQ_STATS_BYTE_RATE]));
		}
	}

	if (atomic_read(&et_dev->ops.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->ops.vq_common.hpsq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsHpSQ%u: %20llu B/sec\n",
				et_dev->ops.hpsq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.hpsq_pptr[i]
						 ->stats[ET_VQ_STATS_BYTE_RATE]));
		}

		for (i = 0; i < et_dev->ops.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsSQ%u:   %20llu B/sec\n",
				et_dev->ops.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.sq_pptr[i]
						 ->stats[ET_VQ_STATS_BYTE_RATE]));
		}

		for (i = 0; i < et_dev->ops.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsCQ%u:   %20llu B/sec\n",
				et_dev->ops.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.cq_pptr[i]
						 ->stats[ET_VQ_STATS_BYTE_RATE]));
		}
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

	if (atomic_read(&et_dev->mgmt.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->mgmt.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtSQ%u:  %3llu %%\n",
				et_dev->mgmt.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.sq_pptr[i]->stats
						 [ET_VQ_STATS_UTILIZATION_PERCENT]));
		}

		for (i = 0; i < et_dev->mgmt.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"MgmtCQ%u:  %3llu %%\n",
				et_dev->mgmt.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->mgmt.cq_pptr[i]->stats
						 [ET_VQ_STATS_UTILIZATION_PERCENT]));
		}
	}

	if (atomic_read(&et_dev->ops.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->ops.vq_common.hpsq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsHpSQ%u: %3llu %%\n",
				et_dev->ops.hpsq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.hpsq_pptr[i]->stats
						 [ET_VQ_STATS_UTILIZATION_PERCENT]));
		}

		for (i = 0; i < et_dev->ops.vq_common.sq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsSQ%u:   %3llu %%\n",
				et_dev->ops.sq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.sq_pptr[i]->stats
						 [ET_VQ_STATS_UTILIZATION_PERCENT]));
		}

		for (i = 0; i < et_dev->ops.vq_common.cq_count; i++) {
			bytes += sysfs_emit_at(
				buf,
				bytes,
				"OpsCQ%u:   %3llu %%\n",
				et_dev->ops.cq_pptr[i]->index,
				atomic64_read(
					&et_dev->ops.cq_pptr[i]->stats
						 [ET_VQ_STATS_UTILIZATION_PERCENT]));
		}
	}

	return bytes;
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
	int j;

	rv = kstrtoul(buf, 0, &value);
	if (rv)
		return rv;

	if (value != 1)
		return -EINVAL;

	if (atomic_read(&et_dev->mgmt.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->mgmt.vq_common.sq_count; i++) {
			for (j = 0;
			     j < ARRAY_SIZE(et_dev->mgmt.sq_pptr[i]->stats);
			     j++)
				atomic64_set(&et_dev->mgmt.sq_pptr[i]->stats[j],
					     0);
		}

		for (i = 0; i < et_dev->mgmt.vq_common.cq_count; i++) {
			for (j = 0;
			     j < ARRAY_SIZE(et_dev->mgmt.cq_pptr[i]->stats);
			     j++)
				atomic64_set(&et_dev->mgmt.cq_pptr[i]->stats[j],
					     0);
		}
	}

	if (atomic_read(&et_dev->ops.state) == DEV_STATE_READY) {
		for (i = 0; i < et_dev->ops.vq_common.hpsq_count; i++) {
			for (j = 0;
			     j < ARRAY_SIZE(et_dev->ops.hpsq_pptr[i]->stats);
			     j++)
				atomic64_set(
					&et_dev->ops.hpsq_pptr[i]->stats[j],
					0);
		}

		for (i = 0; i < et_dev->ops.vq_common.sq_count; i++) {
			for (j = 0;
			     j < ARRAY_SIZE(et_dev->ops.sq_pptr[i]->stats);
			     j++)
				atomic64_set(&et_dev->ops.sq_pptr[i]->stats[j],
					     0);
		}

		for (i = 0; i < et_dev->ops.vq_common.cq_count; i++) {
			for (j = 0;
			     j < ARRAY_SIZE(et_dev->ops.cq_pptr[i]->stats);
			     j++)
				atomic64_set(&et_dev->ops.cq_pptr[i]->stats[j],
					     0);
		}
	}

	return count;
}

static DEVICE_ATTR_RO(msg_count);
static DEVICE_ATTR_RO(byte_count);
static DEVICE_ATTR_RO(msg_rate);
static DEVICE_ATTR_RO(byte_rate);
static DEVICE_ATTR_RO(utilization_percent);
static DEVICE_ATTR_WO(clear);

static struct attribute *vq_stats_attrs[] = {
	&dev_attr_msg_count.attr,
	&dev_attr_byte_count.attr,
	&dev_attr_msg_rate.attr,
	&dev_attr_byte_rate.attr,
	&dev_attr_utilization_percent.attr,
	&dev_attr_clear.attr,
	NULL,
};

const struct attribute_group et_sysfs_vq_stats_attr_group = {
	.name = "vq_stats",
	.attrs = vq_stats_attrs,
};
