#include "et_pci_dev.h"

#include "et_layout.h"
#include "hal_device.h"

const enum et_cdev_type MINOR_TYPES[] = {
	et_cdev_type_mb_sp,
	et_cdev_type_mb_mm,
	et_cdev_type_bulk
};

const char *MINOR_NAMES[] = {
	"et%dmb_sp",
	"et%dmb_mm",
	"et%dbulk"
};

//BAR0
//Name              Host Addr       Size   Notes
//R_L3_DRAM         BAR0 + 0x0000   ~32GB  SoC DRAM

//BAR2
//Name              Host Addr       Size   Notes
//R_PU_MBOX_PC_MM   BAR2 + 0x0000   4k     Mailbox shared memory
//R_PU_MBOX_PC_SP   BAR2 + 0x1000   4k     Mailbox shared memory
//R_PU_TRG_PCIE     BAR2 + 0x2000   8k     Mailbox interrupts
//R_PCIE_USRESR     BAR2 + 0x4000   4k     DMA control registers

const struct et_bar_mapping BAR_MAPPINGS[] = {
	{
		.soc_addr =              DRAM_MEMMAP_BEGIN,
		.size =                  DRAM_MEMMAP_SIZE,
		.bar_offset =            0,
		.bar =                   0,
		.strictly_order_access = false
	},
	{
		.soc_addr=               R_PU_MBOX_PC_MM_BASEADDR,
		.size =                  R_PU_MBOX_PC_MM_SIZE,
		.bar_offset =            0,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PU_MBOX_PC_SP_BASEADDR,
		.size =                  R_PU_MBOX_PC_SP_SIZE,
		.bar_offset =            0x1000,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PU_TRG_PCIE_BASEADDR,
		.size =                  R_PU_TRG_PCIE_SIZE,
		.bar_offset =            0x2000,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PCIE_USRESR_BASEADDR,
		.size =                  R_PCIE_USRESR_SIZE,
		.bar_offset =            0x4000,
		.bar =                   2,
		.strictly_order_access = true
	}
};
