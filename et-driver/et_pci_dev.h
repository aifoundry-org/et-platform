#ifndef __ET_PCI_DEV_H
#define __ET_PCI_DEV_H

#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include "et_vqueue.h"
#include "et_dma.h"

// MAP 3/5 Regions for now
#define IOMEM_REGIONS 3

enum et_iomem_r {
	IOMEM_R_PU_DIR_PC_MM = 0,
	IOMEM_R_PU_DIR_PC_SP,
	IOMEM_R_PU_TRG_PCIE,
	IOMEM_R_PCIE_USRESR,
	IOMEM_R_PU_SRAM_HI
};

struct et_bar_mapping {
	u64 soc_addr;
	u64 size;
	u64 bar_offset;
	u32 bar;
	bool strictly_order_access;
};

extern const struct et_bar_mapping BAR_MAPPINGS[];

struct et_ddr_region {
	void __iomem *mapped_baseaddr;
	u64 soc_addr;
	u64 size;
	u16 attr;
};

struct et_ops_dev {
	struct miscdevice misc_ops_dev;
	bool is_ops_open;
	spinlock_t ops_open_lock;	/* serializes access to is_ops_open */

	struct et_squeue **sq_pptr;
	struct et_cqueue **cq_pptr;
	struct et_vq_common vq_common;

	struct et_ddr_region **ddr_regions;
	u32 num_regions;

	struct rb_root dma_rbtree;
	struct mutex dma_rbtree_mutex;	/* serializes access to dma_rbtree */
};

struct et_mgmt_dev {
	struct miscdevice misc_mgmt_dev;
	bool is_mgmt_open;
	spinlock_t mgmt_open_lock;	/* serializes access to is_mgmt_open */

	struct et_squeue **sq_pptr;
	struct et_cqueue **cq_pptr;
	struct et_vq_common vq_common;

	struct et_ddr_region **ddr_regions;
	u32 num_regions;

	u64 minion_shires;
};

struct et_pci_dev {
	u8 dev_index;
	struct pci_dev *pdev;

	void __iomem *iomem[IOMEM_REGIONS];

	struct et_ops_dev ops;
	struct et_mgmt_dev mgmt;

	u32 num_irq_vecs;
	u32 used_irq_vecs;

	// TODO SW-4210: Remove when MSIx is enabled
	struct workqueue_struct *workqueue;
	struct work_struct isr_work;
	struct timer_list missed_irq_timer;
	bool aborting;
	spinlock_t abort_lock;		/* serializes access to aborting */
};

bool is_bar_prefetchable(struct et_pci_dev *et_dev, u8 bar_no);
int et_map_bar(struct et_pci_dev *et_dev, const struct et_bar_mapping *bm_info,
	       void __iomem **bar_map_ptr);
void et_unmap_bar(void __iomem *bar_map_ptr);

#endif
