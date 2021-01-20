/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (C) 2020 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ************************************************************************

 ************************************************************************
 *
 *   DESCRIPTION
 *
 *       Header/Interface to access, initialize and manage
 *       Operations Device Interface Registers.
 *
 ***********************************************************************/

#ifndef __ET_OPS_DIR_H__
#define __ET_OPS_DIR_H__

/*
 * Values representing Master Minion Boot status.
 */
enum et_ops_boot_status {
	OPS_BOOT_STATUS_DEV_INTF_NOT_READY = 0,
	OPS_BOOT_STATUS_DEV_INTF_READY,
	OPS_BOOT_STATUS_VQ_READY,
	OPS_BOOT_STATUS_MM_READY,
	OPS_BOOT_STATUS_DDR_INITIALIZED,
	OPS_BOOT_STATUS_MM_FW_LAUNCHED,
	OPS_BOOT_STATUS_MM_MBOX_INITIALIZED
};

/*
 * Values representing BAR attributes.
 */
enum et_ops_ddr_region_attribute {
	OPS_DDR_REGION_ATTR_READ_ONLY = 0,
	OPS_DDR_REGION_ATTR_WRITE_ONLY,
	OPS_DDR_REGION_ATTR_READ_WRITE
};

/*
 * Values representing DDR region mappings.
 */
enum et_ops_ddr_region_map {
	OPS_DDR_REGION_MAP_USER_KERNEL_SPACE = 0,
	OPS_DDR_REGION_MAP_NUM
};

/*
 * Holds the information of Master Minion DDR region.
 */
struct et_ops_ddr_region {
	u16 attr;
	u16 bar;
	u64 offset;
	u64 devaddr;
	u64 size;
	u32 reserved;
} __attribute__((__packed__));

/*
 * Holds the information of all available Master Minion DDR regions.
 */
struct et_ops_ddr_regions {
	u32 reserved;
	u32 num_regions;
	struct et_ops_ddr_region regions[OPS_DDR_REGION_MAP_NUM];
} __attribute__((__packed__));

/*
 * Holds the information of Master Minion Virtual Queues.
 */
struct et_ops_vqueue {
	u8 reserved[3];
	u8 bar;
	u32 bar_size;
	u32 sq_offset;
	u16 sq_count;
	u16 per_sq_size;
	u32 cq_offset;
	u16 cq_count;
	u16 per_cq_size;
} __attribute__((__packed__));

/*
 * Master Minion DIR which will be used to public device capability to Host.
 */
struct et_ops_dir {
	u32 version;
	u32 size;
	struct et_ops_vqueue vq_ops;
	struct et_ops_ddr_regions ddr_regions;
	u32 reserved;
	s32 status;
} __attribute__((__packed__));

#endif /* ET_OPS_DIR_H */
