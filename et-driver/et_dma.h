#ifndef __ET_DMA_H
#define __ET_DMA_H

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "device_api_rpc_types_privileged.h"
#include "et_mbox.h"

struct et_pci_dev;

#define ET_DMA_NUM_CHANS 8

struct et_dma_chan {
	enum ET_DMA_STATE state;
	struct mutex state_mutex;
	wait_queue_head_t wait_queue;
};

void et_dma_init(struct et_pci_dev *et_dev);

void et_dma_destroy(struct et_pci_dev *et_dev);

enum ET_DMA_STATE et_dma_get_state(enum ET_DMA_CHAN_ID id,
				   struct et_pci_dev *et_dev);

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
			      enum ET_DMA_CHAN_ID id, struct et_pci_dev *et_dev);

/*
 * Uses the SoC's DMA engines to push data from the SoC to a user-mode buffer.
 * The user will call read() to cause this; the SoC will issue PCIe memory
 * writes to the x86 host to execute it.
 *
 * Returns the number of bytes DMAed on success, or a negative value on failure.
 */
ssize_t et_dma_push_to_user(char __user *buf, size_t count, loff_t *pos,
			    enum ET_DMA_CHAN_ID id, struct et_pci_dev *et_dev);

#endif
