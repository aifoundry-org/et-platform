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

struct et_dma_mapping {
	void *usr_vaddr;
	struct pci_dev *pdev;
	void *kern_vaddr;
	dma_addr_t dma_addr;
	size_t size;
	size_t ref_count;
};

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
ssize_t et_dma_move_data(struct et_pci_dev *et_dev,
			 u16 queue_index,
			 char __user *ucmd,
			 size_t ucmd_size);

/*
 * Allocates DMA coherent memory for data write command and pushes vqueue
 * message.
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_write_to_device(struct et_pci_dev *et_dev,
			       u16 queue_index,
			       struct device_ops_data_write_cmd_t *cmd,
			       size_t cmd_size);

/*
 * Allocates DMA coherent memory for data read command and pushes vqueue
 * message.
 *
 * Returns the number of bytes pushed on vqueue on success, or a negative value
 * on failure.
 */
ssize_t et_dma_read_from_device(struct et_pci_dev *et_dev,
				u16 queue_index,
				struct device_ops_data_read_cmd_t *cmd,
				size_t cmd_size);

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
