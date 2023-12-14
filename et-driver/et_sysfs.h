/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ******************************************************************************/

#ifndef __ET_SYSFS_H
#define __ET_SYSFS_H

#include "et_sysfs_err_stats.h"
#include "et_sysfs_mem_stats.h"
#include "et_sysfs_soc_reset.h"
#include "et_sysfs_vq_stats.h"

/**
 * Notes on addition of new attributes and group of attirbutes
 * To add a new attribute group
 * 1.  Create new header and source files et_sysfs_<name>.c|h and update
 *     Makefile
 * 2.  Add entry to enum et_sysfs_gid_e with name ET_SYSFS_GID_<name>
 * 3.  Add case statements for new attribute in et_sysfs.c
 * 4.  In source file define device attributes (preferrably using kernel
 *     provided macro DEVICE_ATTR*)
 * 5.  For every read-only device attribute, define a show function
 * 6.  For every write-only device attribute, define a store function
 * 7.  For every readable and writeable device attribute, define a show
 *     function and a store function
 * 8.  Create an array of pointers to struct attribute
 * 9.  Define and initialize new variable:
 *     struct attribute_group et_sysfs_<name>_attr_group
 * 10. In header file extern above defined variable
 * 11. Make entry in existing array of pointers to struct attribute_group for
 *     the new group in et_sysfs_stats.c
 *
 * To add a new attribute to an existing attribute group
 * 1.  Define device attribute (preferrably using kernel provided macro
 *     DEVICE_ATTR*)
 * 2.  For read-only device attribute, define a show function
 * 3.  For write-only device attribute, define a store function
 * 4.  For readable and writeable device attribute, define a show function and
 *     a store function
 * 5.  Make entry in the array of pointers to struct attribute for that
 *     existing group
 *
 * To add a new singular attribute which cannot be part of any attribute group
 * 1.  Add entry to enum et_sysfs_fid_e with name ET_SYSFS_FID_<name>
 * 2.  Add case statements for new attribute in et_sysfs.c
 * 3.  Define device attribute (preferrably using kernel provided macro
 *     DEVICE_ATTR*)
 * 4.  For read-only device attribute, define a show function
 * 5.  For write-only device attribute, define a store function
 * 6.  For readable and writeable device attribute, define a show function and
 *     a store function
 */

/**
 * enum et_sysfs_gid_e - SysFS attributes group ID
 */
enum et_sysfs_gid_e {
	ET_SYSFS_GID_MGMT_VQ_STATS = 0,
	ET_SYSFS_GID_OPS_VQ_STATS,
	ET_SYSFS_GID_MEM_STATS,
	ET_SYSFS_GID_ERR_STATS,
	ET_SYSFS_GID_SOC_RESET,
	ET_SYSFS_GROUPS,
};

/**
 * enum et_sysfs_fid_e - SysFS attribute file ID
 */
enum et_sysfs_fid_e {
	ET_SYSFS_FID_DEVNUM = 0,
	ET_SYSFS_FILES,
};

/**
 * struct et_sysfs_data - SysFS groups and files initialization information
 * @is_group_created: Array indicating if SysFS attribute group(s) are created
 * @is_file_created: Array indicating if SysFS attribute file(s) are created
 */
struct et_sysfs_data {
	bool is_group_created[ET_SYSFS_GROUPS];
	bool is_file_created[ET_SYSFS_FILES];
};

struct et_pci_dev;

int et_sysfs_add_group(struct et_pci_dev *et_dev, int group_id);
void et_sysfs_remove_group(struct et_pci_dev *et_dev, int group_id);
void et_sysfs_remove_groups(struct et_pci_dev *et_dev);

int et_sysfs_add_file(struct et_pci_dev *et_dev, int file_id);
void et_sysfs_remove_file(struct et_pci_dev *et_dev, int file_id);
void et_sysfs_remove_files(struct et_pci_dev *et_dev);

#endif
