#ifndef __ET_DMA_H
#define __ET_DMA_H

#include <linux/pci.h>
#include <linux/types.h>

#include "et_mbox.h"

enum et_dma_chan {
	et_dma_chan_read_0 = 0,
	et_dma_chan_read_1,
	et_dma_chan_read_2,
	et_dma_chan_read_3,
	et_dma_chan_write_0,
	et_dma_chan_write_1,
	et_dma_chan_write_2,
	et_dma_chan_write_3
};

/*
 * Returns 0 if the memory region (in SoC address space) is valid to DMA with.
 */
int et_dma_bounds_check(uint64_t soc_addr, uint64_t count);

/* 
 * Uses the SoC's DMA engines to pull a user-mode buffer to the SoC.
 * The user will call write() to cause this; the SoC will issue PCIe memory
 * reads to the x86 host to execute it.
 * 
 * Returns the number of bytes DMAed.
 */
ssize_t et_dma_pull_from_user(const char __user *buf, size_t count, loff_t *pos,
			      enum et_dma_chan chan, struct et_mbox *mbox_mm,
			      void __iomem *r_drct_dram, struct pci_dev *pdev);

/*
 * Uses the SoC's DMA engines to push data from the SoC to a user-mode buffer.
 * The user will call read() to cause this; the SoC will issue PCIe memory
 * writes to the x86 host to execute it.
 *
 * Returns the number of bytes DMAed on success, or a negative value on failure.
 */
ssize_t et_dma_push_to_user(char __user *buf, size_t count, loff_t *pos,
			    enum et_dma_chan chan, struct et_mbox *mbox_mm,
			    void __iomem *r_drct_dram, struct pci_dev *pdev);

#endif