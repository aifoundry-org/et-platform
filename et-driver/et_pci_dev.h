#ifndef __ET_PCI_DEV_H
#define __ET_PCI_DEV_H

#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include "et_dma.h"
#include "et_mbox.h"
#include "et_vqueue.h"

struct et_iomem {
	void __iomem *mem;
	uint64_t soc_addr;
	uint64_t size;
};

#define IOMEM_REGIONS 5
#define SP_VQUEUE 1
#define MM_VQUEUE 0

enum et_iomem_r {
	IOMEM_R_DRCT_DRAM = 0,
	IOMEM_R_PU_MBOX_PC_MM,
	IOMEM_R_PU_MBOX_PC_SP,
	IOMEM_R_PU_TRG_PCIE,
	IOMEM_R_PCIE_USRESR,
};

struct et_bar_mapping {
	uint64_t soc_addr;
	uint64_t size;
	uint64_t bar_offset;
	uint32_t bar;
	bool strictly_order_access;
};

extern const struct et_bar_mapping BAR_MAPPINGS[];

struct et_pci_dev {
	/* SP */
	struct et_vqueue vqueue_sp;
	/* MM */
	struct et_vqueue **vqueue_mm_pptr;
	struct pci_dev *pdev;
	void __iomem *r_pu_trg_pcie;
	struct et_mbox mbox_mm;
	struct et_mbox mbox_sp;
	struct et_dma_chan dma_chans[ET_DMA_NUM_CHANS];
	struct mutex dev_mutex;
	struct miscdevice misc_ops_dev;
	spinlock_t ops_open_lock; /* for serializing access to is_ops_open */
	bool is_ops_open;
	struct miscdevice misc_mgmt_dev;
	spinlock_t mgmt_open_lock; /* for serializing access to is_mgmt_open */
	bool is_mgmt_open;
	void __iomem *iomem[IOMEM_REGIONS];
	uint32_t bulk_cfg;
	int num_irq_vecs;
	struct workqueue_struct *workqueue;
	struct work_struct isr_work;
	struct timer_list missed_irq_timer;
	spinlock_t abort_lock;
	bool aborting;
	u8 index;
	u64 hm_dram_base;
	u64 hm_dram_size;
};

#endif
