// SPDX-License-Identifier: GPL-2.0

/*-----------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 *-----------------------------------------------------------------------------
 */

// clang-format off

#include <linux/sysfs.h>

#include "et_pci_dev.h"
#include "et_sysfs_err_stats.h"

// clang-format on

/**
 * ce_count_show() - Show function for SysFS attribute ce_count
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: buffer memory for the attribute
 *
 * ET Specific Correctable Error statistics
 * err_stats/ce_count
 * |- DramCeEvent               DRAM correctable errors count
 * |- MinionCeEvent             Minion errors count
 * |- PcieCeEvent               PCIe correctable errors count
 * |- PmicCeEvent               PMIC errors count
 * |- SpCeEvent                 SP errors count
 * |- SpExceptCeEvent           SP exceptions count
 * |- SramCeEvent               SRAM correctable errors count
 * |- ThermOvershootCeEvent     Thermal overshoots count
 * |- ThermThrottleCeEvent      Thermal throttles count
 * |- SpTraceBufferFullCeEvent  SP trace buffer full events count
 *
 * Return: number of bytes written in buf, negative value on failure
 */
static ssize_t ce_count_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);
	struct et_err_stats *stats;

	stats = &et_dev->mgmt.err_stats;
	return sysfs_emit(
		buf,
		"DramCeEvent:              %llu\n"
		"MinionCeEvent:            %llu\n"
		"PcieCeEvent:              %llu\n"
		"PmicCeEvent:              %llu\n"
		"SpCeEvent:                %llu\n"
		"SpExceptCeEvent:          %llu\n"
		"SramCeEvent:              %llu\n"
		"ThermOvershootCeEvent:    %llu\n"
		"ThermThrottleCeEvent:     %llu\n"
		"SpTraceBufferFullCeEvent: %llu\n",
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
				 [ET_ERR_COUNTER_STATS_THERM_THROTTLE_CE_COUNT]),
		atomic64_read(
			&stats->counters
				 [ET_ERR_COUNTER_STATS_SP_TRACE_FULL_CE_COUNT]));
}

/**
 * uce_count_show() - Show function for SysFS attribute uce_count
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: buffer memory for the attribute
 *
 * ET Specific Uncorrectable Error statistics
 * err_stats/uce_count
 * |- DramUceEvent              DDR un-correctable errors count
 * |- MinionHangUceEvent        Minion hangs count
 * |- PcieUceEvent              PCIe un-correctable errors count
 * |- SpHangUceEvent            SP hangs count
 * |- SpWdogResetUceEvent       SP WDog reset count
 * `- SramUceEvent              SRAM un-correctable errors count
 *
 * Return: number of bytes written in buf, negative value on failure
 */
static ssize_t uce_count_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
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

/**
 * clear_store() - Store function for SysFS attribute clear
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: Buffer memory for the attribute
 * @count: Number of bytes received in buf
 *
 * When 1 is written in attribute file err_stats/clear, this clears the error
 * statistics
 *
 * Return: number of bytes read/processed from buf, negative value on failure
 */
static ssize_t clear_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
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

/* SysFS attributes in group err_stats */
static DEVICE_ATTR_RO(ce_count);
static DEVICE_ATTR_RO(uce_count);
static DEVICE_ATTR_WO(clear);

static struct attribute *err_stats_attrs[] = {
	&dev_attr_ce_count.attr,
	&dev_attr_uce_count.attr,
	&dev_attr_clear.attr,
	NULL,
};

struct attribute_group et_sysfs_err_stats_group = {
	.name = "err_stats",
	.attrs = err_stats_attrs,
};
