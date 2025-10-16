// SPDX-License-Identifier: GPL-2.0

/*-----------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 *-----------------------------------------------------------------------------
 */

// clang-format off

#include <linux/sysfs.h>

#include "et_pci_dev.h"
#include "et_sysfs_soc_reset.h"

// clang-format on

/**
 * reinitiate_store() - Store function for SysFS attribute reinitiate
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: Buffer memory for the attribute
 * @count: Number of bytes received in buf
 *
 * When 1 is written in attribute file soc_reset/reinitiate, this performs the
 * reinitialization steps on driver instance same as performed in result of
 * ETSOC reset.
 *
 * Return: number of bytes read/processed from buf, negative value on failure
 */
static ssize_t reinitiate_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
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

	mutex_lock(&et_dev->mgmt.reset_mutex);

	spin_lock(&et_dev->mgmt.open_lock);
	if (et_dev->mgmt.is_open) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt Device is in use, re-initiation cannot be done!\n");
		rv = -EPERM;
	}
	spin_unlock(&et_dev->mgmt.open_lock);

	if (rv)
		goto unlock_mgmt_reset_mutex;

	mutex_lock(&et_dev->ops.reset_mutex);

	spin_lock(&et_dev->ops.open_lock);
	if (et_dev->ops.is_open) {
		dev_err(&et_dev->pdev->dev,
			"Ops Device is in use, re-initiation cannot be done!\n");
		rv = -EPERM;
	}
	spin_unlock(&et_dev->ops.open_lock);

	if (rv)
		goto unlock_ops_reset_mutex;

	et_dev->mgmt.is_resetting = true;
	et_dev->ops.is_resetting = true;
	queue_work(et_dev->reset_workqueue, &et_dev->isr_work);

unlock_ops_reset_mutex:
	mutex_unlock(&et_dev->ops.reset_mutex);

unlock_mgmt_reset_mutex:
	mutex_unlock(&et_dev->mgmt.reset_mutex);

	return count;
}

/**
 * pcilink_max_estim_downtime_ms_show() - Show function for SysFS attribute
 *					  pcilink_max_estim_downtime_ms
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: buffer memory for the attribute
 *
 * Displays the currently configured value for pcilink_max_estim_downtime_ms
 * soc_reset/pcilink_max_estim_downtime_ms
 *
 * Return: number of bytes written in buf, negative value on failure
 */
static ssize_t pcilink_max_estim_downtime_ms_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%lu\n",
			  et_dev->reset_cfg.pcilink_max_estim_downtime_ms);
}

/**
 * pcilink_discovery_timeout_ms_show() - Show function for SysFS attribute
 *					 pcilink_discovery_timeout_ms
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: buffer memory for the attribute
 *
 * Displays the currently configured value for pcilink_discovery_timeout_ms
 * soc_reset/pcilink_discovery_timeout_ms
 *
 * Return: number of bytes written in buf, negative value on failure
 */
static ssize_t pcilink_discovery_timeout_ms_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%lu\n",
			  et_dev->reset_cfg.pcilink_discovery_timeout_ms);
}

/**
 * pcilink_discovery_timeout_ms_store() - Store function for SysFS attribute
 *					  pcilink_discovery_timeout_ms
 * @dev: Pointer to struct device
 * @attr: Pointer to struct device_attribute
 * @buf: Buffer memory for the attribute
 * @count: Number of bytes received in buf
 *
 * Stores the written value to attribute file in configuration variable
 * pcilink_discovery_timeout_ms.
 * soc_reset/pcilink_discovery_timeout_ms
 *
 * Return: number of bytes read/processed from buf, negative value on failure
 */
static ssize_t pcilink_discovery_timeout_ms_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf, size_t count)
{
	ssize_t rv;
	unsigned long value;
	struct et_pci_dev *et_dev = dev_get_drvdata(dev);

	rv = kstrtoul(buf, 0, &value);
	if (rv)
		return rv;

	et_dev->reset_cfg.pcilink_discovery_timeout_ms = value;

	return count;
}

/* SysFS attributes in group soc_reset */
static DEVICE_ATTR_WO(reinitiate);
static DEVICE_ATTR_RO(pcilink_max_estim_downtime_ms);
static DEVICE_ATTR_RW(pcilink_discovery_timeout_ms);

static struct attribute *soc_reset_attrs[] = {
	&dev_attr_reinitiate.attr,
	&dev_attr_pcilink_max_estim_downtime_ms.attr,
	&dev_attr_pcilink_discovery_timeout_ms.attr,
	NULL,
};

struct attribute_group et_sysfs_soc_reset_group = {
	.name = "soc_reset",
	.attrs = soc_reset_attrs,
};
