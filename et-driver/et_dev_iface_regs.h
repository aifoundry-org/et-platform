/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (C) 2020 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ******************************************************************************

 ******************************************************************************
 *
 *   DESCRIPTION
 *
 *       Header/Interface to access, initialize and manage
 *       Management and Operations Device Interface Registers.
 *
 ******************************************************************************
 */

#ifndef __ET_DEV_IFACE_REGS_H__
#define __ET_DEV_IFACE_REGS_H__

#include <linux/dev_printk.h>
#include <linux/slab.h>
/*
 * The structure arrangment of DIRs is same for both Mgmt and Ops devices. Mgmt
 * and Ops device have their separate DIRs.
 *
 * DIRs structural arrangment is as follows:
 *
 ******************************************************************************
 *      DIR General Attributes		(struct et_mgmt/ops_dir_header)	      *
 ******************************************************************************
 *      DIR Virtual Queue Attributes	(struct et_mgmt/ops_dir_vqueue)	      *
 ******************************************************************************
 *      DIR Memory Region 0		(struct et_dir_mem_region)	      *
 ******************************************************************************
 *      DIR Memory Region 1		(struct et_dir_mem_region)	      *
 ******************************************************************************
 *      ...				(struct et_dir_mem_region)	      *
 ******************************************************************************
 *      DIR Memory Region N		(struct et_dir_mem_region)	      *
 ******************************************************************************
 *
 * The size of each region may increase in later device firmware versions as a
 * result of addition of new attributes. For backward compatibility, device
 * firmware will make sure to add new attributes after the already available
 * attributes. The driver will support only the known attributes and known
 * region types. If any new attributes/regions are detected, dmesg warnings
 * will be generated to update the driver.
 */

/* MEM_REGION_PRIVILEGE_MODE */
#define MEM_REGION_PRIVILEGE_MODE_KERNEL (0x0)
#define MEM_REGION_PRIVILEGE_MODE_USER	 (0x1)

/* MEM_REGION_NODE_ACCESSIBLE */
#define MEM_REGION_NODE_ACCESSIBLE_NONE (0x0)
#define MEM_REGION_NODE_ACCESSIBLE_MGMT (0x1)
#define MEM_REGION_NODE_ACCESSIBLE_OPS	(0x2)
#define MEM_REGION_NODE_ACCESSIBLE_ALL	(0x3)

/* MEM_REGION_DMA_ALIGNMENT */
#define MEM_REGION_DMA_ALIGNMENT_NONE  (0x0)
#define MEM_REGION_DMA_ALIGNMENT_8BIT  (0x1)
#define MEM_REGION_DMA_ALIGNMENT_32BIT (0x2)
#define MEM_REGION_DMA_ALIGNMENT_64BIT (0x3)

/*
 * Macro to define increment size for DMA element. Size is provided in the
 * increment of 32 MBytes.
 */
#define MEM_REGION_DMA_ELEMENT_STEP_SIZE (32 * 1024 * 1024)

/*
 * Holds the information memory region access attributes
 */
struct et_dir_reg_access {
	/* Description:
	 *
	 *	priv_mode: The mode can be kernel or user. If set to kernel,
	 *	the region will not be exposed to user.
	 *
	 *	Kernel:			0x0
	 *	User:			0x1
	 */
	u32 priv_mode : 1; /* bit 0 */

	/* Description:
	 *
	 *	node_access: Indicates which device nodes can access this
	 *	region.
	 *
	 *	None:			0x0
	 *	Mgmt only:		0x1
	 *	Ops Only:		0x2
	 *	Both Mgmt & Ops:	0x3
	 */
	u32 node_access : 2; /* bit 1-2 */

	/* Description:
	 *
	 *	dma_align: Indicates the DMA alignment to be followed for this
	 *	region.
	 *
	 *	None:			0x0
	 *	8-bit:			0x1
	 *	32-bit:			0x2
	 *	64-bit:			0x3
	 */
	u32 dma_align : 2; /* bit 3-4 */

	/*
	 * Description:
	 *
	 *	dma_elem_count: Indicates number of elements in DMA list
	 *
	 * 	Not applicable:		0x0
	 *	1 element:		0x1
	 *	2 elements:		0x2
	 *	...
	 *	15 elements		0xf
	 */
	u32 dma_elem_count : 4; /* bit 5-8 */

	/*
	 * Description:
	 *
	 * 	dma_elem_size: Indicates DMA element size in steps of 32MB
	 *
	 * 	Not applicable:		0x00
	 *	32MB			0x01
	 *	64MB			0x02
	 *	96MB			0x03
	 *	...
	 *	4096MB			0x80
	 *
	 *	NOTE: 4GB is maximum size that can be transferred per element
	 */
	u32 dma_elem_size : 8; /* bit 9-16 */
	u32 reserved : 15;
} __packed;

/*
 * Holds the information of Device interface memory region.
 */
struct et_dir_mem_region {
	u16 attributes_size;
	u8 type;
	u8 bar;
	struct et_dir_reg_access access;
	u64 bar_offset;
	u64 bar_size;
	u64 dev_address;
} __packed;

/*
 * Values representing Mgmt Device Boot status.
 */
enum et_mgmt_boot_status {
	MGMT_BOOT_STATUS_BOOT_ERROR = -1,
	MGMT_BOOT_STATUS_DEV_NOT_READY = 0,
	MGMT_BOOT_STATUS_VQ_READY,
	MGMT_BOOT_STATUS_NOC_INITIALIZED,
	MGMT_BOOT_STATUS_DDR_INITIALIZED,
	MGMT_BOOT_STATUS_MINION_INITIALIZED,
	MGMT_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED,
	MGMT_BOOT_STATUS_COMMAND_DISPATCHER_INITIALIZED,
	MGMT_BOOT_STATUS_MM_FW_LAUNCHED,
	MGMT_BOOT_STATUS_ATU_PROGRAMMED,
	MGMT_BOOT_STATUS_PM_READY,
	MGMT_BOOT_STATUS_SP_WATCHDOG_TASK_READY,
	MGMT_BOOT_STATUS_EVENT_HANDLER_READY,
	MGMT_BOOT_STATUS_DEV_READY
};

/*
 * Values representing the available types of memory regions supported by the
 * Mgmt Device. All region types are unique.
 */
enum et_mgmt_mem_region_type {
	MGMT_MEM_REGION_TYPE_VQ_BUFFER = 0,
	MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG,
	MGMT_MEM_REGION_TYPE_SCRATCH,
	MGMT_MEM_REGION_TYPE_SPFW_TRACE,
	MGMT_MEM_REGION_TYPE_MMFW_TRACE,
	MGMT_MEM_REGION_TYPE_CMFW_TRACE,
	MGMT_MEM_REGION_TYPE_NUM
};

/*
 * Holds the information of Mgmt Device Virtual Queues.
 */
struct et_mgmt_dir_vqueue {
	u16 attributes_size;
	u8 intrpt_trg_size;
	u8 intrpt_id;
	u32 intrpt_trg_offset;
	u32 sq_offset;
	u16 sq_count;
	u16 sq_size;
	u32 cq_offset;
	u16 cq_count;
	u16 cq_size;
} __packed;

/*
 * Holds the general information of Mgmt Device Interface Registers.
 */
struct et_mgmt_dir_header {
	u16 attributes_size;
	u16 version;
	u16 total_size;
	u16 num_regions;
	u64 minion_shire_mask;
	u32 minion_boot_freq;
	u32 crc32;
	s16 status;
	u16 form_factor;
	u16 device_tdp;
	u16 l3_size;
	u16 l2_size;
	u16 scp_size;
	u16 cache_line_size;
	u8 reserved[2];
} __packed;

struct et_mgmt_dir {
	struct et_mgmt_dir_header header;
	struct et_mgmt_dir_vqueue vqueue;
	struct et_dir_mem_region mem_region[];
} __packed;

struct et_bar_addr_dbg {
	u64 phys_addr;
	void __iomem *mapped_baseaddr;
};

static inline void et_print_mgmt_dir(struct device *dev,
				     u8 *dir_data,
				     size_t dir_size,
				     struct et_bar_addr_dbg *bar_addr_dbg)
{
	int i = 0;
	size_t remaining_size;
	struct et_mgmt_dir *mgmt_dir;
	struct et_bar_addr_dbg *mgmt_bar_addr_dbg = bar_addr_dbg;

	if (!dir_data || !dir_size)
		return;

	mgmt_dir = (struct et_mgmt_dir *)dir_data;

	dev_dbg(dev, "Mgmt DIRs Header\n");
	dev_dbg(dev,
		"Version                 : 0x%x\n",
		mgmt_dir->header.version);
	dev_dbg(dev,
		"Total Size              : 0x%x\n",
		mgmt_dir->header.total_size);
	dev_dbg(dev,
		"Attributes Size         : 0x%x\n",
		mgmt_dir->header.attributes_size);
	dev_dbg(dev,
		"Number of Regions       : 0x%x\n",
		mgmt_dir->header.num_regions);
	dev_dbg(dev,
		"Minion Shire Mask       : 0x%llx\n",
		mgmt_dir->header.minion_shire_mask);
	dev_dbg(dev,
		"Status                  : 0x%x\n",
		mgmt_dir->header.status);
	dev_dbg(dev,
		"CRC32                   : 0x%x\n\n",
		mgmt_dir->header.crc32);
	dev_dbg(dev,
		"form_factor             : 0x%x\n",
		mgmt_dir->header.form_factor);
	dev_dbg(dev,
		"device_tdp              : 0x%x\n",
		mgmt_dir->header.device_tdp);
	dev_dbg(dev,
		"minion_boot_freq        : 0x%x\n",
		mgmt_dir->header.minion_boot_freq);
	dev_dbg(dev,
		"l3_size                 : 0x%x\n",
		mgmt_dir->header.l3_size);
	dev_dbg(dev,
		"l2_size                 : 0x%x\n",
		mgmt_dir->header.l2_size);
	dev_dbg(dev,
		"scp_size                : 0x%x\n",
		mgmt_dir->header.scp_size);
	dev_dbg(dev,
		"cache_line_size         : 0x%x\n",
		mgmt_dir->header.cache_line_size);
	dev_dbg(dev,
		"Reserved[0]             : 0x%x\n",
		mgmt_dir->header.reserved[0]);
	dev_dbg(dev,
		"Reserved[1]             : 0x%x\n",
		mgmt_dir->header.reserved[1]);
	dev_dbg(dev, "Mgmt DIRs Vqueue\n");
	dev_dbg(dev,
		"SQ Offset               : 0x%x\n",
		mgmt_dir->vqueue.sq_offset);
	dev_dbg(dev,
		"SQ Count                : 0x%x\n",
		mgmt_dir->vqueue.sq_count);
	dev_dbg(dev,
		"SQ Size                 : 0x%x\n",
		mgmt_dir->vqueue.sq_size);
	dev_dbg(dev,
		"CQ Offset               : 0x%x\n",
		mgmt_dir->vqueue.cq_offset);
	dev_dbg(dev,
		"CQ Count                : 0x%x\n",
		mgmt_dir->vqueue.cq_count);
	dev_dbg(dev,
		"CQ Size                 : 0x%x\n",
		mgmt_dir->vqueue.cq_size);
	dev_dbg(dev,
		"Interrupt Trigger Offset: 0x%x\n",
		mgmt_dir->vqueue.intrpt_trg_offset);
	dev_dbg(dev,
		"Interrupt Trigger Size  : 0x%x\n",
		mgmt_dir->vqueue.intrpt_trg_size);
	dev_dbg(dev,
		"Interrupt ID            : 0x%x\n",
		mgmt_dir->vqueue.intrpt_id);
	dev_dbg(dev,
		"Attributes Size         : 0x%x\n\n",
		mgmt_dir->vqueue.attributes_size);

	remaining_size = dir_size - sizeof(struct et_mgmt_dir_header) -
			 sizeof(struct et_mgmt_dir_vqueue);
	while (remaining_size > 0) {
		dev_dbg(dev, "Mgmt DIRs Memory Region[%d]\n", i);
		dev_dbg(dev,
			"Attributes Size         : 0x%x\n",
			mgmt_dir->mem_region[i].attributes_size);
		dev_dbg(dev,
			"Access (Privilege mode) : 0x%x\n",
			mgmt_dir->mem_region[i].access.priv_mode);
		dev_dbg(dev,
			"Access (Node access)    : 0x%x\n",
			mgmt_dir->mem_region[i].access.node_access);
		dev_dbg(dev,
			"Access (DMA Alignment)  : 0x%x\n",
			mgmt_dir->mem_region[i].access.dma_align);
		dev_dbg(dev,
			"Access (Reserved)       : 0x%x\n",
			mgmt_dir->mem_region[i].access.reserved);
		switch (mgmt_dir->mem_region[i].type) {
		case MGMT_MEM_REGION_TYPE_VQ_BUFFER:
			dev_dbg(dev,
				"Type                    : %d - VQ Buffer\n",
				MGMT_MEM_REGION_TYPE_VQ_BUFFER);
			break;
		case MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG:
			dev_dbg(dev,
				"Type                    : %d - VQ INT TRG\n",
				MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG);
			break;
		case MGMT_MEM_REGION_TYPE_SCRATCH:
			dev_dbg(dev,
				"Type                    : %d - Scratch\n",
				MGMT_MEM_REGION_TYPE_SCRATCH);
			break;
		case MGMT_MEM_REGION_TYPE_SPFW_TRACE:
			dev_dbg(dev,
				"Type                    : %d - SP FW Trace\n",
				MGMT_MEM_REGION_TYPE_SPFW_TRACE);
			break;
		default:
			dev_dbg(dev,
				"Type                    : Unknown region\n");
		}
		dev_dbg(dev,
			"BAR                     : 0x%x\n",
			mgmt_dir->mem_region[i].bar);
		dev_dbg(dev,
			"BAR Offset              : 0x%llx\n",
			mgmt_dir->mem_region[i].bar_offset);
		dev_dbg(dev,
			"BAR Size                : 0x%llx (%lld)",
			mgmt_dir->mem_region[i].bar_size,
			mgmt_dir->mem_region[i].bar_size);
		dev_dbg(dev,
			"BAR Region PhysAddr     : 0x%llx\n",
			mgmt_bar_addr_dbg->phys_addr);
		dev_dbg(dev,
			"BAR Region Kern VirtAddr: 0x%px\n",
			mgmt_bar_addr_dbg->mapped_baseaddr);
		if (mgmt_dir->mem_region[i].dev_address == 0) {
			dev_dbg(dev, "Dev Address             : N/A\n\n");
		} else {
			dev_dbg(dev,
				"Dev Address             : 0x%llx\n\n",
				mgmt_dir->mem_region[i].dev_address);
		}
		remaining_size -= sizeof(struct et_dir_mem_region);
		i++;
		mgmt_bar_addr_dbg++;
	}
}

/*
 * Values representing Ops Device Boot status.
 */
enum et_ops_boot_status {
	OPS_BOOT_STATUS_MM_FW_ERROR = -1,
	OPS_BOOT_STATUS_DEV_INTF_NOT_READY = 0,
	OPS_BOOT_STATUS_DEV_INTF_READY,
	OPS_BOOT_STATUS_INTERRUPT_INITIALIZED,
	OPS_BOOT_STATUS_MM_CM_INTERFACE_READY,
	OPS_BOOT_STATUS_CM_WORKERS_INITIALIZED,
	OPS_BOOT_STATUS_MM_WORKERS_INITIALIZED,
	OPS_BOOT_STATUS_MM_HOST_VQ_READY,
	OPS_BOOT_STATUS_MM_SP_INTERFACE_READY,
	OPS_BOOT_STATUS_MM_READY
};

/*
 * Values representing the available types of memory regions supported by the
 * Ops Device. All region types are unique.
 */
enum et_ops_mem_region_type {
	OPS_MEM_REGION_TYPE_VQ_BUFFER = 0,
	OPS_MEM_REGION_TYPE_HOST_MANAGED,
	OPS_MEM_REGION_TYPE_NUM
};

/*
 * Holds the information of Ops Device Virtual Queues.
 */
struct et_ops_dir_vqueue {
	u16 attributes_size;
	u8 intrpt_trg_size;
	u8 intrpt_id;
	u32 intrpt_trg_offset;
	u32 sq_offset;
	u16 sq_count;
	u16 sq_size;
	u32 cq_offset;
	u16 cq_count;
	u16 cq_size;
	u32 hpsq_offset;
	u16 hpsq_count;
	u16 hpsq_size;
} __packed;

/*
 * Holds the general information of Ops Device Interface Registers.
 */
struct et_ops_dir_header {
	u16 attributes_size;
	u16 version;
	u16 total_size;
	u16 num_regions;
	s16 status;
	u32 crc32;
	u8 reserved[2];
} __packed;

struct et_ops_dir {
	struct et_ops_dir_header header;
	struct et_ops_dir_vqueue vqueue;
	struct et_dir_mem_region mem_region[];
} __packed;

static inline void et_print_ops_dir(struct device *dev,
				    u8 *dir_data,
				    size_t dir_size,
				    struct et_bar_addr_dbg *bar_addr_dbg)
{
	int i = 0;
	size_t remaining_size;
	struct et_ops_dir *ops_dir;
	struct et_bar_addr_dbg *ops_bar_addr_dbg = bar_addr_dbg;

	if (!dir_data || !dir_size)
		return;

	ops_dir = (struct et_ops_dir *)dir_data;

	dev_dbg(dev, "Ops DIRs Header\n");
	dev_dbg(dev,
		"Version                 : 0x%x\n",
		ops_dir->header.version);
	dev_dbg(dev,
		"Total Size              : 0x%x\n",
		ops_dir->header.total_size);
	dev_dbg(dev,
		"Attributes Size         : 0x%x\n",
		ops_dir->header.attributes_size);
	dev_dbg(dev,
		"Number of Regions       : 0x%x\n",
		ops_dir->header.num_regions);
	dev_dbg(dev,
		"Reserved[0]             : 0x%x\n",
		ops_dir->header.reserved[0]);
	dev_dbg(dev,
		"Reserved[1]             : 0x%x\n",
		ops_dir->header.reserved[1]);
	dev_dbg(dev,
		"Status                  : 0x%x\n",
		ops_dir->header.status);
	dev_dbg(dev,
		"CRC32                   : 0x%x\n\n",
		ops_dir->header.crc32);
	dev_dbg(dev, "Ops DIRs Vqueue\n");
	dev_dbg(dev,
		"SQ Offset               : 0x%x\n",
		ops_dir->vqueue.sq_offset);
	dev_dbg(dev,
		"SQ Count                : 0x%x\n",
		ops_dir->vqueue.sq_count);
	dev_dbg(dev,
		"SQ Size                 : 0x%x\n",
		ops_dir->vqueue.sq_size);
	dev_dbg(dev,
		"CQ Offset               : 0x%x\n",
		ops_dir->vqueue.cq_offset);
	dev_dbg(dev,
		"CQ Count                : 0x%x\n",
		ops_dir->vqueue.cq_count);
	dev_dbg(dev,
		"CQ Size                 : 0x%x\n",
		ops_dir->vqueue.cq_size);
	dev_dbg(dev,
		"Interrupt Trigger Offset: 0x%x\n",
		ops_dir->vqueue.intrpt_trg_offset);
	dev_dbg(dev,
		"Interrupt Trigger Size  : 0x%x\n",
		ops_dir->vqueue.intrpt_trg_size);
	dev_dbg(dev,
		"Interrupt ID            : 0x%x\n",
		ops_dir->vqueue.intrpt_id);
	dev_dbg(dev,
		"Attributes Size         : 0x%x\n",
		ops_dir->vqueue.attributes_size);
	dev_dbg(dev,
		"HP SQ Offset            : 0x%x\n",
		ops_dir->vqueue.hpsq_offset);
	dev_dbg(dev,
		"HP SQ Count             : 0x%x\n",
		ops_dir->vqueue.hpsq_count);
	dev_dbg(dev,
		"HP SQ Size              : 0x%x\n\n",
		ops_dir->vqueue.hpsq_size);

	remaining_size = dir_size - sizeof(struct et_ops_dir_header) -
			 sizeof(struct et_ops_dir_vqueue);
	while (remaining_size > 0) {
		dev_dbg(dev, "Ops DIRs Memory Region[%d]\n", i);
		dev_dbg(dev,
			"Attributes Size         : 0x%x\n",
			ops_dir->mem_region[i].attributes_size);
		dev_dbg(dev,
			"Access (Privilege mode) : 0x%x\n",
			ops_dir->mem_region[i].access.priv_mode);
		dev_dbg(dev,
			"Access (Node access)    : 0x%x\n",
			ops_dir->mem_region[i].access.node_access);
		dev_dbg(dev,
			"Access (DMA Alignment)  : 0x%x\n",
			ops_dir->mem_region[i].access.dma_align);
		dev_dbg(dev,
			"Access (Reserved)       : 0x%x\n",
			ops_dir->mem_region[i].access.reserved);
		switch (ops_dir->mem_region[i].type) {
		case OPS_MEM_REGION_TYPE_VQ_BUFFER:
			dev_dbg(dev,
				"Type                    : %d - VQ Buffer\n",
				OPS_MEM_REGION_TYPE_VQ_BUFFER);
			break;
		case OPS_MEM_REGION_TYPE_HOST_MANAGED:
			dev_dbg(dev,
				"Type                    : %d - Host Managed DRAM\n",
				OPS_MEM_REGION_TYPE_HOST_MANAGED);
			break;
		default:
			dev_dbg(dev,
				"Type                    : Unknown region\n");
		}
		dev_dbg(dev,
			"BAR                     : 0x%x\n",
			ops_dir->mem_region[i].bar);
		dev_dbg(dev,
			"BAR Offset              : 0x%llx\n",
			ops_dir->mem_region[i].bar_offset);
		dev_dbg(dev,
			"BAR Size                : 0x%llx (%lld)",
			ops_dir->mem_region[i].bar_size,
			ops_dir->mem_region[i].bar_size);
		dev_dbg(dev,
			"BAR Region PhysAddr     : 0x%llx\n",
			ops_bar_addr_dbg->phys_addr);
		dev_dbg(dev,
			"BAR Region Kern VirtAddr: 0x%px\n",
			ops_bar_addr_dbg->mapped_baseaddr);
		if (ops_dir->mem_region[i].dev_address == 0) {
			dev_dbg(dev, "Dev Address             : N/A\n\n");
		} else {
			dev_dbg(dev,
				"Dev Address             : 0x%llx\n\n",
				ops_dir->mem_region[i].dev_address);
		}
		remaining_size -= sizeof(struct et_dir_mem_region);
		i++;
		ops_bar_addr_dbg++;
	}
}

static inline bool valid_mgmt_vq_region(struct et_mgmt_dir_vqueue *vq_region,
					char *err_str,
					size_t len)
{
	bool rv = true;

	if (!err_str || !len)
		return false;

	err_str[0] = '\0';

	if (!vq_region)
		return false;

	strlcat(err_str, "\nDevice: Mgmt\nRegion: VQ Region\n", len);

	if (!vq_region->sq_count) {
		strlcat(err_str, "SQ count is 0\n", len);
		rv = false;
	}

	if (!vq_region->sq_size) {
		strlcat(err_str, "SQ size is 0\n", len);
		rv = false;
	}

	if (!vq_region->cq_count) {
		strlcat(err_str, "CQ count is 0\n", len);
		rv = false;
	}

	if (!vq_region->cq_size) {
		strlcat(err_str, "CQ size is 0\n", len);
		rv = false;
	}

	if (!vq_region->intrpt_trg_size) {
		strlcat(err_str, "VQ Interrupt trigger size is 0\n", len);
		rv = false;
	}

	if (!vq_region->attributes_size) {
		strlcat(err_str, "VQ Attributes size is 0\n", len);
		rv = false;
	}

	if (rv)
		err_str[0] = '\0';

	return rv;
}

static inline bool valid_ops_vq_region(struct et_ops_dir_vqueue *vq_region,
				       char *err_str,
				       size_t len)
{
	bool rv = true;

	if (!err_str || !len)
		return false;

	err_str[0] = '\0';

	if (!vq_region)
		return false;

	strlcat(err_str, "\nDevice: Ops\nRegion: VQ Region\n", len);

	if (!vq_region->sq_count) {
		strlcat(err_str, "SQ count is 0\n", len);
		rv = false;
	}

	if (!vq_region->sq_size) {
		strlcat(err_str, "SQ size is 0\n", len);
		rv = false;
	}

	if (!vq_region->hpsq_count) {
		strlcat(err_str, "HP SQ count is 0\n", len);
		rv = false;
	}

	if (!vq_region->hpsq_size) {
		strlcat(err_str, "HP SQ size is 0\n", len);
		rv = false;
	}

	if (!vq_region->cq_count) {
		strlcat(err_str, "CQ count is 0\n", len);
		rv = false;
	}

	if (!vq_region->cq_size) {
		strlcat(err_str, "CQ size is 0\n", len);
		rv = false;
	}

	if (!vq_region->intrpt_trg_size) {
		strlcat(err_str, "VQ Interrupt trigger size is 0\n", len);
		rv = false;
	}

	if (!vq_region->attributes_size) {
		strlcat(err_str, "VQ Attributes size is 0\n", len);
		rv = false;
	}

	if (rv)
		err_str[0] = '\0';

	return rv;
}

static inline bool valid_mem_region(struct et_dir_mem_region *region,
				    bool is_mgmt,
				    char *err_str,
				    size_t len)
{
	char reg_type_str[8];
	bool rv = true;

	if (!err_str || !len)
		return false;

	err_str[0] = '\0';

	if (!region)
		return false;

	if (is_mgmt)
		strlcat(err_str, "\nDevice: Mgmt\n", len);
	else
		strlcat(err_str, "\nDevice: Ops\n", len);

	if (!region->bar_size) {
		strlcat(err_str, "BAR size is 0\n", len);
		rv = false;
	}

	if (!region->attributes_size) {
		strlcat(err_str, "Attributes size is 0\n", len);
		rv = false;
	}

	snprintf(reg_type_str, sizeof(reg_type_str), "%d\n", region->type);
	if (is_mgmt) {
		switch (region->type) {
		case MGMT_MEM_REGION_TYPE_VQ_BUFFER:
		case MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG:
		case MGMT_MEM_REGION_TYPE_SPFW_TRACE:
		case MGMT_MEM_REGION_TYPE_MMFW_TRACE:
		case MGMT_MEM_REGION_TYPE_CMFW_TRACE:
			// Attributes compatibility check
			if (region->access.priv_mode !=
				    MEM_REGION_PRIVILEGE_MODE_KERNEL ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_NONE ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ",
					len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		case MGMT_MEM_REGION_TYPE_SCRATCH:
			// Attributes compatibility check
			if (region->access.priv_mode !=
				    MEM_REGION_PRIVILEGE_MODE_KERNEL ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_MGMT ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ",
					len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		default:
			strlcat(err_str, "Incorrect MGMT region type: ", len);
			strlcat(err_str, reg_type_str, len);
			rv = false;
			break;
		}
	} else {
		switch (region->type) {
		case OPS_MEM_REGION_TYPE_VQ_BUFFER:
			// Attributes compatibility check
			if (region->access.priv_mode !=
				    MEM_REGION_PRIVILEGE_MODE_KERNEL ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_NONE ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ",
					len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		case OPS_MEM_REGION_TYPE_HOST_MANAGED:
			// Attributes compatibility check
			if (region->access.priv_mode !=
				    MEM_REGION_PRIVILEGE_MODE_USER ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_OPS ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_64BIT) {
				strlcat(err_str,
					"Incorrect access for region: ",
					len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			// Check dev_address compulsory field
			if (!region->dev_address) {
				strlcat(err_str,
					"Device Base Address is 0 for region: ",
					len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;
		default:
			strlcat(err_str, "Incorrect OPS region type: ", len);
			strlcat(err_str, reg_type_str, len);
			rv = false;
			break;
		}
	}

	if (rv)
		err_str[0] = '\0';

	return rv;
}

static inline bool compulsory_region_type(int type, bool is_mgmt)
{
	if (is_mgmt) {
		switch (type) {
		case MGMT_MEM_REGION_TYPE_VQ_BUFFER:
		case MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG:
		case MGMT_MEM_REGION_TYPE_SCRATCH:
			break;

		default:
			return false;
		}
	} else {
		switch (type) {
		case OPS_MEM_REGION_TYPE_VQ_BUFFER:
		case OPS_MEM_REGION_TYPE_HOST_MANAGED:
			break;

		default:
			return false;
		}
	}

	return true;
}

#endif /* ET_DEV_IFACE_REGS_H */
