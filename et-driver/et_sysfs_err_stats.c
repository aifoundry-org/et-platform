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

#include "et_pci_dev.h"
#include "et_sysfs_err_stats.h"

// clang-format on

/*
 * ET Specific Correctable Error statistics
 *
 * ce_count
 * |- DramCeEvent		DRAM correctable errors count
 * |- MinionCeEvent		Minion errors count
 * |- PcieCeEvent		PCIe correctable errors count
 * |- PmicCeEvent		PMIC errors count
 * |- SpCeEvent			SP errors count
 * |- SpExceptCeEvent		SP exceptions count
 * |- SramCeEvent		SRAM correctable errors count
 * |- ThermOvershootCeEvent	Thermal overshoots count
 * `- ThermThrottleCeEvent	Thermal throttles count
 */
static ssize_t
ce_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_err_stats *stats;

	stats = &et_dev->mgmt.err_stats;
	return sysfs_emit(
		buf,
		"DramCeEvent:           %llu\n"
		"MinionCeEvent:         %llu\n"
		"PcieCeEvent:           %llu\n"
		"PmicCeEvent:           %llu\n"
		"SpCeEvent:             %llu\n"
		"SpExceptCeEvent:       %llu\n"
		"SramCeEvent:           %llu\n"
		"ThermOvershootCeEvent: %llu\n"
		"ThermThrottleCeEvent:  %llu\n",
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_DRAM_CE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_MINION_CE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_PCIE_CE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_PMIC_CE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_SP_CE_COUNT]),
		atomic64_read(
			&stats->counters
				 [ET_ERR_COUNTER_STATS_SP_EXCEPT_CE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_SRAM_CE_COUNT]),
		atomic64_read(
			&stats->counters
				 [ET_ERR_COUNTER_STATS_THERM_OVERSHOOT_CE_COUNT]),
		atomic64_read(
			&stats->counters
				 [ET_ERR_COUNTER_STATS_THERM_THROTTLE_CE_COUNT]));
}

/*
 * ET Specific Uncorrectable Error statistics
 * uce_count
 * |- DramUceEvent              DDR un-correctable errors count
 * |- MinionHangUceEvent        Minion hangs count
 * |- PcieUceEvent              PCIe un-correctable errors count
 * |- SpHangUceEvent            SP hangs count
 * |- SpWdogResetUceEvent       SP WDog reset count
 * `- SramUceEvent              SRAM un-correctable errors count
 */
static ssize_t
uce_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_err_stats *stats;

	stats = &et_dev->mgmt.err_stats;
	return sysfs_emit(
		buf,
		"DramUceEvent:       %llu\n"
		"MinionHangUceEvent: %llu\n"
		"PcieUceEvent:       %llu\n"
		"SpHangUceEvent:     %llu\n"
		"SpWdogUceEvent:     %llu\n"
		"SramUceEvent:       %llu\n",
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_DRAM_UCE_COUNT]),
		atomic64_read(
			&stats->counters
				 [ET_ERR_COUNTER_STATS_MINION_HANG_UCE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_PCIE_UCE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_SP_HANG_UCE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_SP_WDOG_UCE_COUNT]),
		atomic64_read(
			&stats->counters[ET_ERR_COUNTER_STATS_SRAM_UCE_COUNT]));
}

static ssize_t clear_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	ssize_t rv;
	unsigned long value;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	rv = kstrtoul(buf, 0, &value);
	if (rv)
		return rv;

	if (value != 1)
		return -EINVAL;

	et_err_stats_init(&et_dev->mgmt.err_stats);

	return count;
}

static DEVICE_ATTR_RO(ce_count);
static DEVICE_ATTR_RO(uce_count);
static DEVICE_ATTR_WO(clear);

static struct attribute *err_stats_attrs[] = {
	&dev_attr_ce_count.attr,
	&dev_attr_uce_count.attr,
	&dev_attr_clear.attr,
	NULL,
};

const struct attribute_group et_sysfs_err_stats_attr_group = {
	.name = "err_stats",
	.attrs = err_stats_attrs,
};
