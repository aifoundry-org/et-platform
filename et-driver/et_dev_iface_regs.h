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
 *      DIR Virtual Queue Attributes	(struct et_dir_vqueue)		      *
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

#define MEM_REGION_PRIVILEGE_MODE_KERNEL	(0x0)
#define	MEM_REGION_PRIVILEGE_MODE_USER		(0x1)

#define MEM_REGION_NODE_ACCESSIBLE_NONE		(0x0)
#define MEM_REGION_NODE_ACCESSIBLE_MGMT		(0x1)
#define MEM_REGION_NODE_ACCESSIBLE_OPS		(0x2)
#define MEM_REGION_NODE_ACCESSIBLE_ALL		(0x3)

#define MEM_REGION_DMA_ALIGNMENT_NONE		(0x0)
#define MEM_REGION_DMA_ALIGNMENT_8BIT		(0x1)
#define MEM_REGION_DMA_ALIGNMENT_32BIT		(0x2)
#define MEM_REGION_DMA_ALIGNMENT_64BIT		(0x3)

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

	u32 reserved : 27;
} __attribute__((__packed__));

/*
 * Holds the information of Device interface memory region.
 */
struct et_dir_mem_region {
	u8 type;
	u8 bar;
	u16 attributes_size;
	struct et_dir_reg_access access;
	u64 bar_offset;
	u64 bar_size;
	u64 dev_address;
} __attribute__((__packed__));

/*
 * Holds the information of Device Virtual Queues.
 */
struct et_dir_vqueue {
	u32 sq_offset;
	u16 sq_count;
	u16 per_sq_size;
	u32 cq_offset;
	u16 cq_count;
	u16 per_cq_size;
	u32 intrpt_trg_offset;
	u8 intrpt_trg_size;
	u8 intrpt_id;
	u16 attributes_size;
} __attribute__((__packed__));

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
	MGMT_MEM_REGION_TYPE_NUM
};

/*
 * Holds the general information of Mgmt Device Interface Registers.
 */
struct et_mgmt_dir_header {
	u16 version;
	u16 total_size;
	u16 attributes_size;
	u16 num_regions;
	u64 minion_shire_mask;
	u8 reserved[2];
	s16 status;
	u32 crc32;
} __attribute__((__packed__));

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
	OPS_MEM_REGION_TYPE_MMFW_TRACE,
	OPS_MEM_REGION_TYPE_CMFW_TRACE,
	OPS_MEM_REGION_TYPE_HOST_MANAGED,
	OPS_MEM_REGION_TYPE_NUM
};

/*
 * Holds the general information of Ops Device Interface Registers.
 */
struct et_ops_dir_header {
	u16 version;
	u16 total_size;
	u16 attributes_size;
	u16 num_regions;
	u8 reserved[2];
	s16 status;
	u32 crc32;
} __attribute__((__packed__));

#define MAX_LEN		64

static inline bool valid_vq_region(struct et_dir_vqueue *vq_region,
				   bool is_mgmt, char *syndrome, size_t len)
{
	char *syndrome_str;
	bool rv;

	if (!vq_region) {
		strcat(syndrome, "");
		return false;
	}

	if (!syndrome || !len) {
		strcat(syndrome, "");
		return false;
	}

	syndrome_str = kmalloc(len, GFP_KERNEL);
	if (!syndrome_str) {
		strcat(syndrome, "");
		return false;
	}

	syndrome_str[0] = '\0';
	rv = true;

	if (!vq_region->sq_count) {
		strcat(syndrome_str, "VQ SQ count is 0\n");
		rv = false;
	}

	if (!vq_region->per_sq_size) {
		strcat(syndrome_str, "VQ SQ size is 0\n");
		rv = false;
	}

	if (!vq_region->cq_count) {
		strcat(syndrome_str, "VQ CQ count is 0\n");
		rv = false;
	}

	if (!vq_region->per_cq_size) {
		strcat(syndrome_str, "VQ CQ size is 0\n");
		rv = false;
	}

	if (!vq_region->intrpt_trg_size) {
		strcat(syndrome_str, "VQ Interrupt trigger size is 0\n");
		rv = false;
	}

	if (!vq_region->attributes_size) {
		strcat(syndrome_str, "VQ Attributes size is 0\n");
		rv = false;
	}

	if (!rv)
		strncpy(syndrome, syndrome_str, len);

	kfree(syndrome_str);
	return rv;
}

static inline bool valid_mem_region(struct et_dir_mem_region *region,
				    bool is_mgmt, char *syndrome, size_t len)
{
	char *syndrome_str;
	char err_str[MAX_LEN];
	bool rv;

	if (!region) {
		strcat(syndrome, "");
		return false;
	}

	if (!syndrome || !len) {
		strcat(syndrome, "");
		return false;
	}

	syndrome_str = kmalloc(len, GFP_KERNEL);
	if (!syndrome_str) {
		strcat(syndrome, "");
		return false;
	}

	syndrome_str[0] = '\0';
	rv = true;

	if (!region->bar_size) {
		strcat(syndrome_str, "BAR size is 0\n");
		rv = false;
	}

	if (!region->attributes_size) {
		strcat(syndrome_str, "Attributes size is 0\n");
		rv = false;
	}

	snprintf(err_str, MAX_LEN, "%d\n", region->type);
	if (is_mgmt) {
		switch (region->type) {
		case MGMT_MEM_REGION_TYPE_VQ_BUFFER:
		case MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG:
		case MGMT_MEM_REGION_TYPE_SPFW_TRACE:
			// Attributes compatibility check
			if (region->access.priv_mode !=
			    MEM_REGION_PRIVILEGE_MODE_KERNEL ||
			    region->access.node_access !=
			    MEM_REGION_NODE_ACCESSIBLE_NONE ||
			    region->access.dma_align !=
			    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strcat(syndrome_str,
				       "Incorrect access for region: ");
				strcat(syndrome_str, err_str);
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
				strcat(syndrome_str,
				       "Incorrect access for region: ");
				strcat(syndrome_str, err_str);
				rv = false;
			}
			break;

		default:
			strcat(syndrome_str, "Incorrect region type: ");
			strcat(syndrome_str, err_str);
			rv = false;
			break;
		}
	} else {
		switch (region->type) {
		case OPS_MEM_REGION_TYPE_VQ_BUFFER:
		case OPS_MEM_REGION_TYPE_MMFW_TRACE:
		case OPS_MEM_REGION_TYPE_CMFW_TRACE:
			// Attributes compatibility check
			if (region->access.priv_mode !=
			    MEM_REGION_PRIVILEGE_MODE_KERNEL ||
			    region->access.node_access !=
			    MEM_REGION_NODE_ACCESSIBLE_NONE ||
			    region->access.dma_align !=
			    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strcat(syndrome_str,
				       "Incorrect access for region: ");
				strcat(syndrome_str, err_str);
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
				strcat(syndrome_str,
				       "Incorrect access for region: ");
				strcat(syndrome_str, err_str);
				rv = false;
			}
			// Check dev_address compulsory field
			if (!region->dev_address) {
				strcat(syndrome_str,
				       "Device Base Address is 0 for region: ");
				strcat(syndrome_str, err_str);
				rv = false;
			}
			break;
		default:
			strcat(syndrome_str, "Incorrect region type: ");
			strcat(syndrome_str, err_str);
			rv = false;
			break;
		}
	}

	if (!rv)
		strncpy(syndrome, syndrome_str, len);

	kfree(syndrome_str);
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
