/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 **********************************************************************/

#ifndef __ET_DMA_H
#define __ET_DMA_H

#include <linux/kernel.h>
#include <linux/pci.h>

#include "et_pci_dev.h"

/**
 * struct et_dma_mapping - CMA-backed mmap mapping information for DMA
 * @usr_vaddr: virtual address pointer in user-space
 * @pdev: pointer to pci_dev structure
 * @kern_vaddr: virtual address pointer in kernel-space
 * @dma_addr: DMA address to be used by DMA engine
 * @size: size of mapping in bytes
 * @ref_count: reference count of mapping
 */
struct et_dma_mapping {
	void *usr_vaddr;
	struct pci_dev *pdev;
	void *kern_vaddr;
	dma_addr_t dma_addr;
	size_t size;
	size_t ref_count;
};

ssize_t et_dma_move_data(struct et_pci_dev *et_dev, u16 queue_index,
			 char __user *ucmd, size_t ucmd_size);

#endif
