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

#ifndef __ET_PCI_DEV_H
#define __ET_PCI_DEV_H

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "et_ioctl.h"
#include "et_sysfs.h"
#include "et_vqueue.h"

/**
 * enum et_iomem_r - IO memory DIR regions
 */
enum et_iomem_r { IOMEM_R_DIR_OPS = 0, IOMEM_R_DIR_MGMT, IOMEM_REGIONS };

/**
 * enum et_msi_vec_idx - MSI vector IDs
 */
enum et_msi_vec_idx {
	ET_MGMT_SQ_VEC_IDX = 0,
	ET_MGMT_CQ_VEC_IDX,
	ET_OPS_SQ_VEC_IDX,
	ET_OPS_CQ_VEC_IDX,
	ET_MAX_MSI_VECS
};

/**
 * struct et_bar_mapping - Compact information to map a BAR region
 * @bar: The BAR number
 * @bar_offset: Offset in BAR region
 * @size: Size of the region to map
 */
struct et_bar_mapping {
	u32 bar;
	u64 bar_offset;
	u64 size;
};

/**
 * struct et_bar_region - BAR region node struct
 * @list: struct list_head node / glue logic for lists
 * @is_mgmt: mgmt device if true otherwise ops device
 * @bar: The BAR number
 * @region_type: The BAR region type
 * @region_start: The BAR region starting address
 * @region_end: The BAR region last valid address
 */
struct et_bar_region {
	struct list_head list;
	bool is_mgmt;
	u8 bar;
	u8 region_type;
	u64 region_start;
	u64 region_end;
};

/**
 * struct et_ops_dev - Ops device
 * @is_initialized: Ops device is initialized and ready to use
 * @is_resetting: Ops device is going through reset operation
 * @misc_dev: Miscellaneous device information
 * @miscdev_created: Miscellaneous device is created
 * @is_open: Ops device file-descriptor is open
 * @dir: IOMEM address of ops DIR base address
 * @regions: List of memory regions corresponding to ops device
 * @dir_vq: VQ information discovered from ops DIRs
 * @vq_data: VQ data other than the ops DIRs VQ information
 * @mem_stats: Memory statistics for ops device
 */
struct et_ops_dev {
	bool is_initialized;
	/**
	 * @init_mutex: serializes access to is_initialized
	 */
	struct mutex init_mutex;
	bool is_resetting;
	/**
	 * @reset_mutex: serializes access to is_resetting
	 */
	struct mutex reset_mutex;
	struct miscdevice misc_dev;
	bool miscdev_created;
	bool is_open;
	/**
	 * @open_lock: serializes access to is_open
	 */
	spinlock_t open_lock;
	void __iomem *dir;
	struct et_mapped_region regions[OPS_MEM_REGION_TYPE_NUM];
	struct et_ops_dir_vqueue dir_vq;
	struct et_vq_data vq_data;
	struct et_mem_stats mem_stats;
};

/**
 * struct et_mgmt_dev - Mgmt device
 * @is_initialized: Mgmt device is initialized and ready to use
 * @is_resetting: Mgmt device is going through reset operation
 * @misc_dev: Miscellaneous device information
 * @miscdev_created: Miscellaneous device is created
 * @is_open: Mgmt device file-descriptor is open
 * @dir: IOMEM address of mgmt DIR base address
 * @regions: List of memory regions corresponding to mgmt device
 * @dir_vq: VQ information discovered from mgmt DIRs
 * @vq_data: VQ data other than the mgmt DIRs VQ information
 * @err_stats: Error statistics for mgmt device
 */
struct et_mgmt_dev {
	bool is_initialized;
	/**
	 * @init_mutex: serializes access to is_initialized
	 */
	struct mutex init_mutex;
	bool is_resetting;
	/**
	 * @reset_mutex: serializes access to is_resetting
	 */
	struct mutex reset_mutex;
	struct miscdevice misc_dev;
	bool miscdev_created;
	bool is_open;
	/**
	 * @open_lock: serializes access to is_open
	 */
	spinlock_t open_lock;
	void __iomem *dir;
	struct et_mapped_region regions[MGMT_MEM_REGION_TYPE_NUM];
	struct et_mgmt_dir_vqueue dir_vq;
	struct et_vq_data vq_data;
	struct et_err_stats err_stats;
};

/**
 * struct et_pci_dev - ETSoC1 PCIe Device
 * @devnum: Physical device node index / device number
 * @is_initialized: ETSoC1 device as whole is initialized
 * @is_err_reporting: AER error reporting is enabled
 * @pdev: Pointer to struct pci_dev
 * @pstate: Pointer to struct pci_saved_state - holds PCIe config space state
 * @cfg: Device configuration received from DIRs
 * @ops: Ops device information
 * @mgmt: Mgmt device information
 * @bar_region_list: struct list_head - list of pointer of BAR regions
 * @reset_cfg: SOC reset configuration options
 * @reset_workqueue: workqueue to process reset ISR work items
 * @isr_work: SOC reset ISR
 * @sysfs_data: SysFS information
 */
struct et_pci_dev {
	u8 devnum;
	bool is_initialized;
	bool is_err_reporting;
	struct pci_dev *pdev;
	struct pci_saved_state *pstate;
	struct dev_config cfg;
	struct et_ops_dev ops;
	struct et_mgmt_dev mgmt;
	struct list_head bar_region_list;
	struct et_soc_reset_cfg reset_cfg;
	struct workqueue_struct *reset_workqueue;
	struct work_struct isr_work;
	struct et_sysfs_data sysfs_data;
};

int et_map_bar(struct et_pci_dev *et_dev, const struct et_bar_mapping *bm_info,
	       void __iomem **mapped_addr_ptr);
void et_unmap_bar(void __iomem *mapped_addr);
int et_mgmt_dev_init(struct et_pci_dev *et_dev, u32 timeout_secs,
		     bool miscdev_create);
void et_mgmt_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy);
int et_ops_dev_init(struct et_pci_dev *et_dev, u32 timeout_secs,
		    bool miscdev_create);
void et_ops_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy);

/* Maximum number of ETSOC devices */
#define ET_MAX_DEVS 64

/* Bitmap representing initialized devices */
extern DECLARE_BITMAP(dev_bitmap, ET_MAX_DEVS);

/* Mapping information for static regions mgmt and ops DIRs */
extern const struct et_bar_mapping DIR_MAPPINGS[];

#endif
