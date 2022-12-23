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

#include "et_sysfs.h"
#include "et_pci_dev.h"

int et_sysfs_add_group(struct et_pci_dev *et_dev, int group_id)
{
	int rv;
	struct attribute_group *group = NULL;

	switch (group_id) {
	case ET_SYSFS_GID_MGMT_VQ_STATS:
		group = &et_sysfs_mgmt_vq_stats_group;
		break;
	case ET_SYSFS_GID_OPS_VQ_STATS:
		group = &et_sysfs_ops_vq_stats_group;
		break;
	case ET_SYSFS_GID_MEM_STATS:
		group = &et_sysfs_mem_stats_group;
		break;
	case ET_SYSFS_GID_ERR_STATS:
		group = &et_sysfs_err_stats_group;
		break;
	case ET_SYSFS_GID_SOC_RESET:
		group = &et_sysfs_soc_reset_group;
		break;
	default:
		return -EINVAL;
	}

	if (et_dev->sysfs_data.is_group_created[group_id])
		return -EEXIST;

	rv = device_add_group(&et_dev->pdev->dev, group);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Failed to add sysfs group %s to device!\n",
			group->name);
		return rv;
	}

	et_dev->sysfs_data.is_group_created[group_id] = true;

	return rv;
}

void et_sysfs_remove_group(struct et_pci_dev *et_dev, int group_id)
{
	struct attribute_group *group = NULL;

	switch (group_id) {
	case ET_SYSFS_GID_MGMT_VQ_STATS:
		group = &et_sysfs_mgmt_vq_stats_group;
		break;
	case ET_SYSFS_GID_OPS_VQ_STATS:
		group = &et_sysfs_ops_vq_stats_group;
		break;
	case ET_SYSFS_GID_MEM_STATS:
		group = &et_sysfs_mem_stats_group;
		break;
	case ET_SYSFS_GID_ERR_STATS:
		group = &et_sysfs_err_stats_group;
		break;
	case ET_SYSFS_GID_SOC_RESET:
		group = &et_sysfs_soc_reset_group;
		break;
	default:
		return;
	}

	if (!et_dev->sysfs_data.is_group_created[group_id])
		return;

	device_remove_group(&et_dev->pdev->dev, group);

	et_dev->sysfs_data.is_group_created[group_id] = false;
}

void et_sysfs_remove_groups(struct et_pci_dev *et_dev)
{
	int group_id;

	for (group_id = 0; group_id < ET_SYSFS_GROUPS; group_id++)
		et_sysfs_remove_group(et_dev, group_id);
}

int et_sysfs_add_file(struct et_pci_dev *et_dev, int file_id)
{
	int rv;
	struct device_attribute *dev_attr = NULL;

	switch (file_id) {
	/* To be added here */
	default:
		return -EINVAL;
	}

	if (et_dev->sysfs_data.is_file_created[file_id])
		return -EEXIST;

	rv = device_create_file(&et_dev->pdev->dev, dev_attr);
	if (rv)
		dev_err(&et_dev->pdev->dev,
			"Failed to create %s attribute!\n",
			dev_attr->attr.name);
	else
		et_dev->sysfs_data.is_file_created[file_id] = true;

	return rv;
}

void et_sysfs_remove_file(struct et_pci_dev *et_dev, int file_id)
{
	struct device_attribute *dev_attr = NULL;

	switch (file_id) {
	/* To be added here */
	default:
		return;
	}

	if (!et_dev->sysfs_data.is_file_created[file_id])
		return;

	device_remove_file(&et_dev->pdev->dev, dev_attr);
	et_dev->sysfs_data.is_file_created[file_id] = false;
}

void et_sysfs_remove_files(struct et_pci_dev *et_dev)
{
	int file_id;

	for (file_id = 0; file_id < ET_SYSFS_FILES; file_id++)
		et_sysfs_remove_file(et_dev, file_id);
}
