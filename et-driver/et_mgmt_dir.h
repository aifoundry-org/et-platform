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
 *       Management Device Interface Registers.
 *
 ***********************************************************************/

#ifndef __ET_MGMT_DIR_H__
#define __ET_MGMT_DIR_H__

/*
 * Values representing Service Processor Boot status.
 */
enum et_mgmt_boot_status {
	MGMT_BOOT_STATUS_BOOT_ERROR = -1,
	MGMT_BOOT_STATUS_DEV_NOT_READY = 0,
	MGMT_BOOT_STATUS_VQ_READY,
	MGMT_BOOT_STATUS_NOC_INITIALIZED,
	MGMT_BOOT_STATUS_DDR_INITIALIZED,
	MGMT_BOOT_STATUS_MINION_INITIALIZED,
	MGMT_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED,
	MGMT_BOOT_STATUS_COMMAND_DISPTATCHER_INITIALIZED,
	MGMT_BOOT_STATUS_MM_FW_LAUNCHED,
	MGMT_BOOT_STATUS_ATU_PROGRAMMED,
	MGMT_BOOT_STATUS_MGMT_WATCHDOG_TASK_READY,
	MGMT_BOOT_STATUS_DEV_READY
};

/*
 * Values representing BAR attributes.
 */
enum et_mgmt_ddr_region_attribute {
	MGMT_DDR_REGION_ATTR_READ_ONLY = 0,
	MGMT_DDR_REGION_ATTR_WRITE_ONLY,
	MGMT_DDR_REGION_ATTR_READ_WRITE
};

/*
 * Values representing DDR region mappings.
 */
enum et_mgmt_ddr_region_map {
	MGMT_DDR_REGION_MAP_TRACE_BUFFER = 0,
	MGMT_DDR_REGION_MAP_DEV_MANAGEMENT_SCRATCH,
	MGMT_DDR_REGION_MAP_NUM
};

/*
 * Holds the information of Management Device DDR region.
 */
struct et_mgmt_ddr_region {
	u32 reserved;
	u16 attr;
	u16 bar;
	u64 offset;
	u64 devaddr;
	u64 size;
} __attribute__((__packed__));

/*
 * Holds the information of all the available Management Device DDR regions.
 */
struct et_mgmt_ddr_regions {
	u32 reserved;
	u32 num_regions;
	struct et_mgmt_ddr_region regions[MGMT_DDR_REGION_MAP_NUM];
} __attribute__((__packed__));

/*
 * Holds the information of Device's interrupt trigger region.
 */
struct et_mgmt_intrpt_region {
	u8 reserved[6];
	u16 bar;
	u64 offset;
	u64 size;
} __attribute__((__packed__));

/*
 * Holds the information of Service Processor Virtual Queues.
 */
struct et_mgmt_vqueue {
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
 * Management device DIR which will be used to public device capability to
 * Host.
 */
struct et_mgmt_dir {
	u32 version;
	u32 size;
	u64 minion_shires;
	struct et_mgmt_vqueue vq_mgmt;
	struct et_mgmt_ddr_regions ddr_regions;
	struct et_mgmt_intrpt_region intrpt_region;
	u32 intrpt_trg_offset;
	s32 status;
} __attribute__((__packed__));

#endif /* ET_MGMT_DIR_H */
