#ifndef __ET_DMA_H
#define __ET_DMA_H

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "et_device_api.h"

struct et_pci_dev;

/* TODO: tag_id is user dependent field, use some other key here, which
 * should be unique and available from response message as well
 */
struct et_dma_info {
	struct rb_node node;
	/* node key */
	u16 tag_id;
	void __user *usr_vaddr;
	/* info to free dma buffer */
	struct pci_dev *pdev;
	void *kern_vaddr;
	dma_addr_t dma_addr;
	size_t size;
	/* info to unpin ubuf */
	struct page **page_list;
	size_t nr_pages;
	bool is_writable;
};

// TODO: Enable/Remove while implementing DMA Scatter/Gather
// We might need old DMA transfer list based approach for DMA Scatter/Gather
#if 0
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

/*
 * rbtree functions for DMA rbtree which holds information for each allocated
 * DMA coherent memory and user pinned buffers
 */
bool et_dma_insert_info(struct rb_root *root, struct et_dma_info *dma_info);
struct et_dma_info *et_dma_search_info(struct rb_root *root, u16 tag_id);
void et_dma_delete_info(struct rb_root *root, struct et_dma_info *dma_info);
void et_dma_delete_all_info(struct rb_root *root);

/*
 * Parses DMA data move commands
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_move_data(struct et_pci_dev *et_dev, u16 queue_index,
			 char __user *ucmd, size_t ucmd_size);

/*
 * Allocates DMA coherent memory for data write command and pushes vqueue
 * message.
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_write_to_device(struct et_pci_dev *et_dev, u16 queue_index,
			       struct device_ops_data_write_cmd_t *cmd,
			       size_t cmd_size);

/*
 * Allocates DMA coherent memory for data read command and pushes vqueue
 * message.
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_read_from_device(struct et_pci_dev *et_dev, u16 queue_index,
				struct device_ops_data_read_cmd_t *cmd,
				size_t cmd_size);

#endif
