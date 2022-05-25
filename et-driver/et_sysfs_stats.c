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
#include "et_sysfs_stats.h"

/*
 * Notes on addition of new attributes and group of attirbutes
 * To add a new attribute to an existing attribute group
 * 1. Define device attribute (preferrably using kernel provided macro
 *    DEVICE_ATTR*)
 * 2. For read-only device attribute, define a show function
 * 3. For write-only device attribute, define a store function
 * 4. For readable and writeable device attribute, define a show function and
 *    a store function
 * 5. Make entry in the array of pointers to struct attribute for that
 *    existing group
 *
 *
 * To add a new attribute group
 * 1. Create new header and source files et_sysfs_<name>.c|h and update Makefile
 * 2. In source file define device attributes (preferrably using kernel provided
 *    macro DEVICE_ATTR*)
 * 3. For every read-only device attribute, define a show function
 * 4. For every write-only device attribute, define a store function
 * 5. For every readable and writeable device attribute, define a show function
 *    and a store function
 * 6. Create an array of pointers to stuct attribute
 * 7. Define and initialize new variable:
 *    struct attribute_group et_sysfs_<name>_attr_group
 * 8. In header file extern above defined variable
 * 9. Make entry in existing array of pointers to struct attribute_group for the
 *    new group in et_sysfs_stats.c
 */

static const struct attribute_group *et_sysfs_mgmt_stats_attr_groups[] = {
	&et_sysfs_mgmt_vq_stats_attr_group,
	&et_sysfs_err_stats_attr_group,
	NULL,
};

static const struct attribute_group *et_sysfs_ops_stats_attr_groups[] = {
	&et_sysfs_ops_vq_stats_attr_group,
	&et_sysfs_mem_stats_attr_group,
	NULL,
};

int et_sysfs_stats_init(struct et_pci_dev *et_dev, bool is_mgmt)
{
	int rv;

	rv = is_mgmt ? device_add_groups(&et_dev->pdev->dev,
					 et_sysfs_mgmt_stats_attr_groups) :
		       device_add_groups(&et_dev->pdev->dev,
					 et_sysfs_ops_stats_attr_groups);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Failed to add sysfs %s stats group to device, error %d\n",
			is_mgmt ? "mgmt" : "ops",
			rv);
		return rv;
	}

	return rv;
}

void et_sysfs_stats_remove(struct et_pci_dev *et_dev, bool is_mgmt)
{
	is_mgmt ? device_remove_groups(&et_dev->pdev->dev,
				       et_sysfs_mgmt_stats_attr_groups) :
		  device_remove_groups(&et_dev->pdev->dev,
				       et_sysfs_ops_stats_attr_groups);
}
