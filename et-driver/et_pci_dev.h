#ifndef __ET_PCI_DEV_H
#define __ET_PCI_DEV_H

#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include "et_dma.h"
#include "et_mbox.h"

struct et_iomem {
	void __iomem *mem;
	uint64_t soc_addr;
	uint64_t size;
};

enum et_cdev_type {
	et_cdev_type_mb_sp = 0,
	et_cdev_type_mb_mm,
	et_cdev_type_bulk
};

extern const enum et_cdev_type MINOR_TYPES[];

extern const char *MINOR_NAMES[];

#define MINORS_PER_SOC 3

struct et_pci_minor_dev {
	struct device *pdevice;
	struct et_pci_dev *et_dev;
	struct cdev cdev;
	struct mutex open_close_mutex;
	enum et_cdev_type type;
	int ref_count;
};

#define IOMEM_REGIONS 5

enum et_iomem_r {
	IOMEM_R_DRCT_DRAM = 0,
	IOMEM_R_PU_MBOX_PC_MM,
	IOMEM_R_PU_MBOX_PC_SP,
	IOMEM_R_PU_TRG_PCIE,
	IOMEM_R_PCIE_USRESR
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
	struct pci_dev *pdev;
	struct et_mbox mbox_mm;
	struct et_mbox mbox_sp;
	struct et_dma_chan dma_chans[ET_DMA_NUM_CHANS];
	struct mutex dev_mutex;
	struct et_pci_minor_dev et_minor_devs[MINORS_PER_SOC];
	void __iomem *iomem[IOMEM_REGIONS];
	uint32_t bulk_cfg;
	int num_irq_vecs;
	struct workqueue_struct *workqueue;
	struct work_struct isr_work;
	struct timer_list missed_irq_timer;
	spinlock_t abort_lock;
	bool aborting;
};

#endif
