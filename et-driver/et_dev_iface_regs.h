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
#include <linux/mutex.h>
#include <linux/pci.h>
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
 * region types. If any new attributes/regions are detected, dmesg warnings will
 * be generated to update the driver.
 */

/* Mgmt and Ops DIR Versions */
#define MGMT_DIR_VERSION (0x2)
#define OPS_DIR_VERSION	 (0x1)

/* MEM_REGION_IOACCESS */
#define MEM_REGION_IOACCESS_DISABLED (0x0)
#define MEM_REGION_IOACCESS_ENABLED  (0x1)

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

/**
 * Macro to define increment size for DMA element. Size is provided in the
 * increment of 32 MBytes.
 */
#define MEM_REGION_DMA_ELEMENT_STEP_SIZE (32 * 1024 * 1024)

/* MEM_REGION_P2PACCESS */
#define MEM_REGION_P2PACCESS_DISABLED (0x0)
#define MEM_REGION_P2PACCESS_ENABLED  (0x1)

/**
 * struct et_dir_reg_access - Device interface memory region access attributes
 * @io_access: Indicates whether the region needs to be mapped by host
 *	Bit 0
 *	Region not to be mapped for IO accesses	0x0
 *	Region to be mapped for IO accesses	0x1
 * @node_access: Indicates which device nodes can access this region
 *	Bit 1-2
 *	None:			0x0
 *	Mgmt only:		0x1
 *	Ops Only:		0x2
 *	Both Mgmt & Ops:	0x3
 * @dma_align: Indicates the DMA alignment to be followed for this region
 *	Bit 3-4
 *	None:			0x0
 *	8-bit:			0x1
 *	32-bit:			0x2
 *	64-bit:			0x3
 * @dma_elem_count: Indicates number of elements in DMA list
 *	Bit 5-8
 *	Not applicable:		0x0
 *	1 element:		0x1
 *	2 elements:		0x2
 *	...
 *	15 elements		0xf
 * @dma_elem_size: Indicates DMA element size in steps of 32MB
 *	Bit 9-16
 *	Not applicable:		0x00
 *	32MB			0x01
 *	64MB			0x02
 *	96MB			0x03
 *	...
 *	4096MB			0x80
 *	NOTE: 4GB is maximum size that can be transferred per element
 * @p2p_access: Indicates whether the region needs to be used as P2P resource
 *	Bit 17
 *	Region not to be mapped for P2P accesses: 0x0
 *	Region to be mapped for P2P accesses:	  0x1
 */
struct et_dir_reg_access {
	u32 io_access : 1;
	u32 node_access : 2;
	u32 dma_align : 2;
	u32 dma_elem_count : 4;
	u32 dma_elem_size : 8;
	u32 p2p_access : 1;
	u32 reserved : 14;
} __packed;

/**
 * struct et_dir_mem_region - Device interface memory region
 * @attributes_size: Attributes size
 * @type: Memory region unique type
 * @bar: BAR region index of the memory region
 * @access: Region accessibility attributes
 * @bar_offset: Offset in BAR region
 * @dev_address: SOC address of the start of the region (if valid otherwise 0)
 */
struct et_dir_mem_region {
	u16 attributes_size;
	u8 type;
	u8 bar;
	struct et_dir_reg_access access;
	u64 bar_offset;
	u64 bar_size;
	u64 dev_address;
} __packed __aligned(8);

/**
 * enum et_mgmt_boot_status - Mgmt device Boot status
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

/**
 * enum et_mgmt_arch_revision - Mgmt device architecture revision types
 */
enum et_mgmt_arch_revision {
	MGMT_ARCH_REVISION_ETSOC1 = 0,
	MGMT_ARCH_REVISION_PANTERO,
	MGMT_ARCH_REVISION_GEPARDO
};

/**
 * enum et_mgmt_form_factor - Mgmt device form factor types
 */
enum et_mgmt_form_factor { MGMT_FORM_FACTOR_PCIE = 1, MGMT_FORM_FACTOR_M_2 };

/**
 * enum et_mgmt_mem_region_type - Mgmt device unique memory region types
 */
enum et_mgmt_mem_region_type {
	MGMT_MEM_REGION_TYPE_VQ_BUFFER = 0,
	MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG,
	MGMT_MEM_REGION_TYPE_SCRATCH,
	MGMT_MEM_REGION_TYPE_SPFW_TRACE,
	MGMT_MEM_REGION_TYPE_MMFW_TRACE,
	MGMT_MEM_REGION_TYPE_CMFW_TRACE,
	MGMT_MEM_REGION_TYPE_SP_STATS,
	MGMT_MEM_REGION_TYPE_MM_STATS,
	MGMT_MEM_REGION_TYPE_NUM
};

/**
 * struct et_mgmt_dir_vqueue - Mgmt device virtual queues information
 * @attributes_size: Attributes size
 * @intrpt_trg_size: Size of interrupt trigger region
 * @intrpt_id: Interrupt ID for mgmt device
 * @sq_offset: Offset of SQs in BAR region
 * @sq_count: SQs count
 * @sq_size: SQ size in bytes
 * @cq_offset: Offset of CQs in BAR region
 * @cq_count: CQs count
 * @cq_size: CQ size in bytes
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
} __packed __aligned(8);

/**
 * struct et_mgmt_dir_header - Mgmt device general information attributes
 * @attributes_size: Attributes size
 * @version: Montonically increasing version of DIRs for breaking changes
 * @total_size: Total size of DIRs in bytes
 * @num_regions: Total number of memory regions in DIRs
 * @reserved0: Reserved bytes
 * @crc32: CRC32 of mgmt DIRs
 * @ddr_bandwidth: DDR bandwidth
 * @cm_shire_mask: Compute minion shires mask
 * @minion_boot_freq: Minion boot frequency
 * @scp_size: Scratchpad size
 * @l2_size: L2 cache size
 * @l3_size: L3 cache size
 * @status: DIRs readiness status
 * @l2_shire_banks: L2 shire banks
 * @sync_min_shire_id: Sync minion shire ID
 * @arch_revision: Architecture revision
 * @form_factor: Form factor
 * @device_tdp: Device TDP
 * @cache_line_size: Cache line size
 * @reserved1: Reserved bytes
 * @bar0_size: BAR0 total size
 * @bar2_size: BAR2 total size
 */
struct et_mgmt_dir_header {
	u16 attributes_size;
	u16 version;
	u16 total_size;
	u16 num_regions;
	u8 reserved0[8];
	u32 crc32;
	u32 ddr_bandwidth;
	u32 cm_shires_mask;
	u32 minion_boot_freq;
	u32 scp_size;
	u32 l2_size;
	u32 l3_size;
	s16 status;
	u8 l2_shire_banks;
	u8 sync_min_shire_id;
	u8 arch_revision;
	u8 form_factor;
	u8 device_tdp;
	u8 cache_line_size;
	u8 reserved1[4];
	u64 bar0_size;
	u64 bar2_size;
} __packed __aligned(8);

/**
 * struct et_mgmt_dir - Mgmt device interface registers
 * @header: Mgmt DIR header
 * @vqueue: Mgmt DIR virtual queue information
 * @mem_region: Mgmt DIR flexible array of memory regions
 */
struct et_mgmt_dir {
	struct et_mgmt_dir_header header;
	struct et_mgmt_dir_vqueue vqueue;
	struct et_dir_mem_region mem_region[];
} __packed __aligned(8);

/**
 * struct et_mapped_region - Mapped information of memory region for driver use
 * @is_valid: The mapping has completed and is valid. The compulsory regions
 *	must be valid, non-compulsory regions may or may not be valid
 * @access: DIR region accessibility information
 * @io: DIR region mapped as IO-region with kernel IOMEM base and physical base
 *	addresses
 * @p2p: DIR region mapped as P2P-region with device-resource ID, kernel
 *	virtual base address and PCI bus address (DMAable)
 * @dev_phys_addr: Device physical/SOC base address
 * @size: Total size of the mapped region
 */
struct et_mapped_region {
	bool is_valid;
	struct et_dir_reg_access access;
	union {
		struct {
			void __iomem *mapped_baseaddr;
			phys_addr_t host_phys_addr;
		} io;
		struct {
			void *devres_id;
			void *virt_addr;
			pci_bus_addr_t pci_bus_addr;
		} p2p;
	};
	u64 dev_phys_addr;
	u64 size;
};

/* Serializes access to Mgmt and Ops DIRs for printing */
static DEFINE_MUTEX(dir_print_mutex);

/**
 * et_print_mgmt_dir() - Print mgmt device interface registers
 * @dev : pointer to device structure
 * @dir_data : Base address of DIRs copy in kernel virtual address space
 * @dir_size : Total size of DIRs
 * @region : pointer to et_mapped_region structure array first element
 */
static inline void et_print_mgmt_dir(struct device *dev, u8 *dir_data,
				     size_t dir_size,
				     struct et_mapped_region *region)
{
	int i = 0;
	size_t remaining_size;
	struct et_mgmt_dir *mgmt_dir;

	if (!dir_data || !dir_size)
		return;

	mgmt_dir = (struct et_mgmt_dir *)dir_data;

	mutex_lock(&dir_print_mutex);
	dev_dbg(dev, "Mgmt DIRs Header\n");
	dev_dbg(dev, "Version                 : 0x%x\n",
		mgmt_dir->header.version);
	dev_dbg(dev, "Total Size              : 0x%x\n",
		mgmt_dir->header.total_size);
	dev_dbg(dev, "Attributes Size         : 0x%x\n",
		mgmt_dir->header.attributes_size);
	dev_dbg(dev, "Number of Regions       : 0x%x\n",
		mgmt_dir->header.num_regions);
	dev_dbg(dev, "BAR0 Size               : 0x%llx\n",
		mgmt_dir->header.bar0_size);
	dev_dbg(dev, "BAR2 Size               : 0x%llx\n",
		mgmt_dir->header.bar2_size);
	dev_dbg(dev, "CRC32                   : 0x%x\n",
		mgmt_dir->header.crc32);
	dev_dbg(dev, "DDR Bandwidth           : 0x%x\n",
		mgmt_dir->header.ddr_bandwidth);
	dev_dbg(dev, "CM Shire Mask           : 0x%x\n",
		mgmt_dir->header.cm_shires_mask);
	dev_dbg(dev, "Minion Boot Frequency   : 0x%x\n",
		mgmt_dir->header.minion_boot_freq);
	dev_dbg(dev, "SCP Size (KB)           : 0x%x\n",
		mgmt_dir->header.scp_size);
	dev_dbg(dev, "L2 Size (KB)            : 0x%x\n",
		mgmt_dir->header.l2_size);
	dev_dbg(dev, "L3 Size (KB)            : 0x%x\n",
		mgmt_dir->header.l3_size);
	dev_dbg(dev, "Status                  : 0x%x\n",
		mgmt_dir->header.status);
	dev_dbg(dev, "L2 Shire Banks          : 0x%x\n",
		mgmt_dir->header.l2_shire_banks);
	dev_dbg(dev, "Sync Minion Shire ID    : 0x%x\n",
		mgmt_dir->header.sync_min_shire_id);
	dev_dbg(dev, "Architecture Revision   : 0x%x\n",
		mgmt_dir->header.arch_revision);
	dev_dbg(dev, "PCIe Form Factor        : 0x%x\n",
		mgmt_dir->header.form_factor);
	dev_dbg(dev, "Device TDP              : 0x%x\n",
		mgmt_dir->header.device_tdp);
	dev_dbg(dev, "Cache Line Size         : 0x%x\n\n",
		mgmt_dir->header.cache_line_size);
	dev_dbg(dev, "Mgmt DIRs Vqueue\n");
	dev_dbg(dev, "SQ Offset               : 0x%x\n",
		mgmt_dir->vqueue.sq_offset);
	dev_dbg(dev, "SQ Count                : 0x%x\n",
		mgmt_dir->vqueue.sq_count);
	dev_dbg(dev, "SQ Size                 : 0x%x\n",
		mgmt_dir->vqueue.sq_size);
	dev_dbg(dev, "CQ Offset               : 0x%x\n",
		mgmt_dir->vqueue.cq_offset);
	dev_dbg(dev, "CQ Count                : 0x%x\n",
		mgmt_dir->vqueue.cq_count);
	dev_dbg(dev, "CQ Size                 : 0x%x\n",
		mgmt_dir->vqueue.cq_size);
	dev_dbg(dev, "Interrupt Trigger Offset: 0x%x\n",
		mgmt_dir->vqueue.intrpt_trg_offset);
	dev_dbg(dev, "Interrupt Trigger Size  : 0x%x\n",
		mgmt_dir->vqueue.intrpt_trg_size);
	dev_dbg(dev, "Interrupt ID            : 0x%x\n",
		mgmt_dir->vqueue.intrpt_id);
	dev_dbg(dev, "Attributes Size         : 0x%x\n\n",
		mgmt_dir->vqueue.attributes_size);

	remaining_size = dir_size - sizeof(struct et_mgmt_dir_header) -
			 sizeof(struct et_mgmt_dir_vqueue);
	while (remaining_size >= sizeof(struct et_dir_mem_region)) {
		dev_dbg(dev, "Mgmt DIRs Memory Region[%d]\n", i);
		dev_dbg(dev, "Attributes Size         : 0x%x\n",
			mgmt_dir->mem_region[i].attributes_size);
		dev_dbg(dev, "I/O Access              : 0x%x\n",
			mgmt_dir->mem_region[i].access.io_access);
		dev_dbg(dev, "Access (Node access)    : 0x%x\n",
			mgmt_dir->mem_region[i].access.node_access);
		dev_dbg(dev, "Access (DMA Alignment)  : 0x%x\n",
			mgmt_dir->mem_region[i].access.dma_align);
		dev_dbg(dev, "Access (Reserved)       : 0x%x\n",
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
			dev_dbg(dev, "Type                    : %d - Scratch\n",
				MGMT_MEM_REGION_TYPE_SCRATCH);
			break;
		case MGMT_MEM_REGION_TYPE_SPFW_TRACE:
			dev_dbg(dev,
				"Type                    : %d - SP FW Trace\n",
				MGMT_MEM_REGION_TYPE_SPFW_TRACE);
			break;
		case MGMT_MEM_REGION_TYPE_MMFW_TRACE:
			dev_dbg(dev,
				"Type                    : %d - MM FW Trace\n",
				MGMT_MEM_REGION_TYPE_MMFW_TRACE);
			break;
		case MGMT_MEM_REGION_TYPE_CMFW_TRACE:
			dev_dbg(dev,
				"Type                    : %d - CM FW Trace\n",
				MGMT_MEM_REGION_TYPE_CMFW_TRACE);
			break;
		case MGMT_MEM_REGION_TYPE_SP_STATS:
			dev_dbg(dev,
				"Type                    : %d - SP Dev Stats\n",
				MGMT_MEM_REGION_TYPE_SP_STATS);
			break;
		case MGMT_MEM_REGION_TYPE_MM_STATS:
			dev_dbg(dev,
				"Type                    : %d - MM Dev Stats\n",
				MGMT_MEM_REGION_TYPE_MM_STATS);
			break;
		default:
			dev_dbg(dev,
				"Type                    : Unknown region\n");
		}
		dev_dbg(dev, "BAR                     : 0x%x\n",
			mgmt_dir->mem_region[i].bar);
		dev_dbg(dev, "BAR Offset              : 0x%llx\n",
			mgmt_dir->mem_region[i].bar_offset);
		dev_dbg(dev, "BAR Size                : 0x%llx (%lld)",
			mgmt_dir->mem_region[i].bar_size,
			mgmt_dir->mem_region[i].bar_size);
		dev_dbg(dev, "BAR Region PhysAddr     : 0x%llx\n",
			region[mgmt_dir->mem_region[i].type].access.p2p_access ?
				region[mgmt_dir->mem_region[i].type]
					.p2p.pci_bus_addr :
				region[mgmt_dir->mem_region[i].type]
					.io.host_phys_addr);
		if (!region[mgmt_dir->mem_region[i].type].io.mapped_baseaddr)
			dev_dbg(dev, "BAR Region Kern VirtAddr: N/A\n");
		else
			// NOTE: It is not allowed to expose kernel virtual
			// address through prints. Such code should be removed
			// before upstreaming the driver
			// Suppressed the checkpatch.pl warning by replacing
			// %px usage with %llx
			dev_dbg(dev, "BAR Region Kern VirtAddr: 0x%llx\n",
				(u64 __force)region[mgmt_dir->mem_region[i].type]
					.io.mapped_baseaddr);
		if (mgmt_dir->mem_region[i].dev_address == 0)
			dev_dbg(dev, "Dev Address             : N/A\n\n");
		else
			dev_dbg(dev, "Dev Address             : 0x%llx\n\n",
				mgmt_dir->mem_region[i].dev_address);
		remaining_size -= sizeof(struct et_dir_mem_region);
		i++;
	}
	mutex_unlock(&dir_print_mutex);
}

/**
 * enum et_ops_boot_status - Ops device Boot status
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

/**
 * enum et_ops_mem_region_type - Ops device unique memory region types
 */
enum et_ops_mem_region_type {
	OPS_MEM_REGION_TYPE_VQ_BUFFER = 0,
	OPS_MEM_REGION_TYPE_HOST_MANAGED,
	OPS_MEM_REGION_TYPE_NUM
};

/**
 * struct et_ops_dir_vqueue - Ops device virtual queues information
 * @attributes_size: Attributes size
 * @intrpt_trg_size: Size of interrupt trigger region
 * @intrpt_id: Interrupt ID for mgmt device
 * @intrpt_trg_offset: Offset in interrupt trigger region
 * @sq_offset: Offset of SQs in BAR region
 * @sq_count: SQs count
 * @sq_size: SQ size in bytes
 * @cq_offset: Offset of CQs in BAR region
 * @cq_count: CQs count
 * @cq_size: CQ size in bytes
 * @hp_sq_offset: Offset of HPSQs in BAR region
 * @hp_sq_count: HPSQs count
 * @hp_sq_size: HPSQ size in bytes
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
	u32 hp_sq_offset;
	u16 hp_sq_count;
	u16 hp_sq_size;
} __packed __aligned(8);

/**
 * struct et_ops_dir_header - Ops device general information attributes
 * @attributes_size: Attributes size
 * @version: Montonically increasing version of DIRs for breaking changes
 * @total_size: Total size of DIRs in bytes
 * @num_regions: Total number of memory regions in DIRs
 * @status: DIRs readiness status
 * @crc32: CRC32 of ops DIRs
 * @reserved: Reserved bytes
 */
struct et_ops_dir_header {
	u16 attributes_size;
	u16 version;
	u16 total_size;
	u16 num_regions;
	s16 status;
	u32 crc32;
	u8 reserved[2];
} __packed __aligned(8);

/**
 * struct et_ops_dir - Ops device interface registers
 * @header: Ops DIR header
 * @vqueue: Ops DIR virtual queue information
 * @mem_region: Ops DIR flexible array of memory regions
 */
struct et_ops_dir {
	struct et_ops_dir_header header;
	struct et_ops_dir_vqueue vqueue;
	struct et_dir_mem_region mem_region[];
} __packed __aligned(8);

/**
 * et_print_ops_dir() - Print ops device interface registers
 * @dev : pointer to device structure
 * @dir_data : Base address of DIRs copy in kernel virtual address space
 * @dir_size : Total size of DIRs
 * @region : pointer to et_mapped_region structure array first element
 */
static inline void et_print_ops_dir(struct device *dev, u8 *dir_data,
				    size_t dir_size,
				    struct et_mapped_region *region)
{
	int i = 0;
	size_t remaining_size;
	struct et_ops_dir *ops_dir;

	if (!dir_data || !dir_size)
		return;

	ops_dir = (struct et_ops_dir *)dir_data;

	mutex_lock(&dir_print_mutex);
	dev_dbg(dev, "Ops DIRs Header\n");
	dev_dbg(dev, "Version                 : 0x%x\n",
		ops_dir->header.version);
	dev_dbg(dev, "Total Size              : 0x%x\n",
		ops_dir->header.total_size);
	dev_dbg(dev, "Attributes Size         : 0x%x\n",
		ops_dir->header.attributes_size);
	dev_dbg(dev, "Number of Regions       : 0x%x\n",
		ops_dir->header.num_regions);
	dev_dbg(dev, "Reserved[0]             : 0x%x\n",
		ops_dir->header.reserved[0]);
	dev_dbg(dev, "Reserved[1]             : 0x%x\n",
		ops_dir->header.reserved[1]);
	dev_dbg(dev, "Status                  : 0x%x\n",
		ops_dir->header.status);
	dev_dbg(dev, "CRC32                   : 0x%x\n\n",
		ops_dir->header.crc32);
	dev_dbg(dev, "Ops DIRs Vqueue\n");
	dev_dbg(dev, "SQ Offset               : 0x%x\n",
		ops_dir->vqueue.sq_offset);
	dev_dbg(dev, "SQ Count                : 0x%x\n",
		ops_dir->vqueue.sq_count);
	dev_dbg(dev, "SQ Size                 : 0x%x\n",
		ops_dir->vqueue.sq_size);
	dev_dbg(dev, "CQ Offset               : 0x%x\n",
		ops_dir->vqueue.cq_offset);
	dev_dbg(dev, "CQ Count                : 0x%x\n",
		ops_dir->vqueue.cq_count);
	dev_dbg(dev, "CQ Size                 : 0x%x\n",
		ops_dir->vqueue.cq_size);
	dev_dbg(dev, "Interrupt Trigger Offset: 0x%x\n",
		ops_dir->vqueue.intrpt_trg_offset);
	dev_dbg(dev, "Interrupt Trigger Size  : 0x%x\n",
		ops_dir->vqueue.intrpt_trg_size);
	dev_dbg(dev, "Interrupt ID            : 0x%x\n",
		ops_dir->vqueue.intrpt_id);
	dev_dbg(dev, "Attributes Size         : 0x%x\n",
		ops_dir->vqueue.attributes_size);
	dev_dbg(dev, "HP SQ Offset            : 0x%x\n",
		ops_dir->vqueue.hp_sq_offset);
	dev_dbg(dev, "HP SQ Count             : 0x%x\n",
		ops_dir->vqueue.hp_sq_count);
	dev_dbg(dev, "HP SQ Size              : 0x%x\n\n",
		ops_dir->vqueue.hp_sq_size);

	remaining_size = dir_size - sizeof(struct et_ops_dir_header) -
			 sizeof(struct et_ops_dir_vqueue);
	while (remaining_size > 0) {
		dev_dbg(dev, "Ops DIRs Memory Region[%d]\n", i);
		dev_dbg(dev, "Attributes Size         : 0x%x\n",
			ops_dir->mem_region[i].attributes_size);
		dev_dbg(dev, "I/O Access              : 0x%x\n",
			ops_dir->mem_region[i].access.io_access);
		dev_dbg(dev, "Access (Node access)    : 0x%x\n",
			ops_dir->mem_region[i].access.node_access);
		dev_dbg(dev, "Access (DMA Alignment)  : 0x%x\n",
			ops_dir->mem_region[i].access.dma_align);
		dev_dbg(dev, "Access (Reserved)       : 0x%x\n",
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
		dev_dbg(dev, "BAR                     : 0x%x\n",
			ops_dir->mem_region[i].bar);
		dev_dbg(dev, "BAR Offset              : 0x%llx\n",
			ops_dir->mem_region[i].bar_offset);
		dev_dbg(dev, "BAR Size                : 0x%llx (%lld)",
			ops_dir->mem_region[i].bar_size,
			ops_dir->mem_region[i].bar_size);
		dev_dbg(dev, "BAR Region PhysAddr     : 0x%llx\n",
			region[ops_dir->mem_region[i].type].access.p2p_access ?
				region[ops_dir->mem_region[i].type]
					.p2p.pci_bus_addr :
				region[ops_dir->mem_region[i].type]
					.io.host_phys_addr);
		if (!region[ops_dir->mem_region[i].type].io.mapped_baseaddr)
			dev_dbg(dev, "BAR Region Kern VirtAddr: N/A\n");
		else
			// NOTE: It is not allowed to expose kernel virtual
			// address through prints. Such code should be removed
			// before upstreaming the driver
			// Suppressed the checkpatch.pl warning by replacing
			// %px usage with %llx
			dev_dbg(dev, "BAR Region Kern VirtAddr: 0x%llx\n",
				(u64 __force)region[ops_dir->mem_region[i].type]
					.io.mapped_baseaddr);
		if (ops_dir->mem_region[i].dev_address == 0)
			dev_dbg(dev, "Dev Address             : N/A\n\n");
		else
			dev_dbg(dev, "Dev Address             : 0x%llx\n\n",
				ops_dir->mem_region[i].dev_address);
		remaining_size -= sizeof(struct et_dir_mem_region);
		i++;
	}
	mutex_unlock(&dir_print_mutex);
}

/**
 * valid_mgmt_vq_region() - Checks validity of mgmt virtual queue region
 * @vq_region : pointer to et_mgmt_dir_vqueue structure
 * @err_str : Placeholder for error string to be returned
 * @len : Length of the error string
 *
 * Return: True on success, otherwise false
 */
static inline bool valid_mgmt_vq_region(struct et_mgmt_dir_vqueue *vq_region,
					char *err_str, size_t len)
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

/**
 * valid_ops_vq_region() - Checks validity of ops virtual queue region
 * @vq_region : pointer to et_ops_dir_vqueue structure
 * @err_str : Placeholder for error string to be returned
 * @len : Length of the error string
 *
 * Return: True on success, otherwise false
 */
static inline bool valid_ops_vq_region(struct et_ops_dir_vqueue *vq_region,
				       char *err_str, size_t len)
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

	if (!vq_region->hp_sq_count) {
		strlcat(err_str, "HP SQ count is 0\n", len);
		rv = false;
	}

	if (!vq_region->hp_sq_size) {
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

/**
 * valid_mem_region() - Checks validity of memory region
 * @region : Pointer to et_dir_mem_region structure
 * @is_mgmt : Indicates if memory region belongs to mgmt/ops device
 * @err_str : Placeholder for error string to be returned
 * @len : Length of the error string
 *
 * Return: True on success, otherwise false
 */
static inline bool valid_mem_region(struct et_dir_mem_region *region,
				    bool is_mgmt, char *err_str, size_t len)
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
			// Attributes compatibility check
			if (region->access.io_access !=
				    MEM_REGION_IOACCESS_ENABLED ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_NONE ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ", len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		case MGMT_MEM_REGION_TYPE_SPFW_TRACE:
		case MGMT_MEM_REGION_TYPE_MMFW_TRACE:
		case MGMT_MEM_REGION_TYPE_CMFW_TRACE:
		case MGMT_MEM_REGION_TYPE_SP_STATS:
		case MGMT_MEM_REGION_TYPE_MM_STATS:
			// Attributes compatibility check
			if (region->access.io_access !=
				    MEM_REGION_IOACCESS_ENABLED ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ", len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		case MGMT_MEM_REGION_TYPE_SCRATCH:
			// Attributes compatibility check
			if (region->access.io_access !=
				    MEM_REGION_IOACCESS_ENABLED ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_MGMT ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ", len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		default:
			strlcat(err_str, "Incorrect Mgmt region type: ", len);
			strlcat(err_str, reg_type_str, len);
			rv = false;
			break;
		}
	} else {
		switch (region->type) {
		case OPS_MEM_REGION_TYPE_VQ_BUFFER:
			// Attributes compatibility check
			if (region->access.io_access !=
				    MEM_REGION_IOACCESS_ENABLED ||
			    region->access.node_access !=
				    MEM_REGION_NODE_ACCESSIBLE_NONE ||
			    region->access.dma_align !=
				    MEM_REGION_DMA_ALIGNMENT_NONE) {
				strlcat(err_str,
					"Incorrect access for region: ", len);
				strlcat(err_str, reg_type_str, len);
				rv = false;
			}
			break;

		case OPS_MEM_REGION_TYPE_HOST_MANAGED:
			// Attributes compatibility check
			if (region->access.io_access !=
				    MEM_REGION_IOACCESS_DISABLED ||
			    region->access.node_access &
				    MEM_REGION_NODE_ACCESSIBLE_MGMT) {
				strlcat(err_str,
					"Incorrect access for region: ", len);
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
			strlcat(err_str, "Incorrect Ops region type: ", len);
			strlcat(err_str, reg_type_str, len);
			rv = false;
			break;
		}
	}

	if (rv)
		err_str[0] = '\0';

	return rv;
}

/**
 * compulsory_region_type() - Checks if given region type is compulsory region
 * @type : integer value from et_mgmt/ops_mem_region_type enums
 * @is_mgmt : Indicates if memory region belongs to mgmt/ops device
 *
 * Return: True on success, otherwise false
 */
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
