/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 ******************************************************************************/

#ifndef __ET_P2PDMA_H
#define __ET_P2PDMA_H

#include <linux/kernel.h>
#include <linux/pci-p2pdma.h>
#include <linux/rwsem.h>

#include "et_dev_iface_regs.h"
#include "et_pci_dev.h"

/**
 * struct et_p2pdma_region - P2P DMA region node struct
 * @list: struct list_head node / glue logic for lists
 * @region: Pointer of type struct et_mapped_region - the data of node
 */
struct et_p2pdma_region {
	struct list_head list;
	struct et_mapped_region *region;
};

/**
 * struct et_p2pdma_mapping - P2P DMA mapping
 * @pdev: Pointer of struct pci_dev
 * @region_list: struct list_head - list of pointer of P2P regions
 * @dev_compat_bitmap: P2P compatibility with other devices
 */
struct et_p2pdma_mapping {
	struct pci_dev *pdev;
	/**
	 * @rwsem: Implements multiple readers, single writer support for this
	 * struct. e.g. write lock is held during ETSOC reset vs read access by
	 * other devices
	 */
	struct rw_semaphore rwsem;
	struct list_head region_list;
	DECLARE_BITMAP(dev_compat_bitmap, ET_MAX_DEVS);
};

void et_p2pdma_init(u8 devnum);
u64 et_p2pdma_get_compat_bitmap(u8 devnum);
int et_p2pdma_add_resource(struct et_pci_dev *et_dev,
			   const struct et_bar_mapping *bm_info,
			   struct et_mapped_region *region);
void et_p2pdma_release_resource(struct et_pci_dev *et_dev,
				struct et_mapped_region *region);
ssize_t et_p2pdma_move_data(struct et_pci_dev *et_dev, u16 queue_index,
			    char __user *ucmd, size_t ucmd_size);

#endif
