#ifndef __ET_DMA_H
#define __ET_DMA_H

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>

#include "et_device_api.h"

struct et_pci_dev;

struct et_dma_mapping {
	void *usr_vaddr;
	struct pci_dev *pdev;
	void *kern_vaddr;
	dma_addr_t dma_addr;
	size_t size;
	size_t ref_count;
};

/*
 * Parses DMA data move commands
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_move_data(struct et_pci_dev *et_dev,
			 u16 queue_index,
			 char __user *ucmd,
			 size_t ucmd_size);

/*
 * Searches for coherent memory against mmapped virtual address and fills in
 * the device address of DMA writelist command nodes and pushes vqueue message.
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_writelist_to_device(struct et_pci_dev *et_dev,
				   u16 queue_index,
				   struct device_ops_dma_writelist_cmd_t *cmd,
				   size_t cmd_size);

/*
 * Searches for coherent memory against mmapped virtual address and fills in
 * the device address of DMA readlist command nodes and pushes vqueue message.
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_readlist_from_device(struct et_pci_dev *et_dev,
				    u16 queue_index,
				    struct device_ops_dma_readlist_cmd_t *cmd,
				    size_t cmd_size);

#endif
