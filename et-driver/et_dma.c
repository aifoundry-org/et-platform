/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "et_dma.h"
#include "et_io.h"
#include "et_pci_dev.h"
#include "et_vqueue.h"

static inline void et_dma_free_coherent(struct et_dma_info *dma_info)
{
	if (dma_info)
		dma_free_coherent(&dma_info->pdev->dev,
				  dma_info->size,
				  dma_info->kern_vaddr,
				  dma_info->dma_addr);
}

static inline void *et_dma_alloc_coherent(struct et_dma_info *dma_info)
{
	if (dma_info)
		return dma_alloc_coherent(&dma_info->pdev->dev,
					  dma_info->size,
					  &dma_info->dma_addr,
					  GFP_KERNEL);
	return NULL;
}

// TODO: Enable with DMA scatter/gather and zero-copy implementation
#if 0
static ssize_t et_dma_pin_ubuf(struct et_dma_info *dma_info)
{
	size_t offs, i;
	ssize_t rv;

	if (!dma_info->usr_vaddr || dma_info->size == 0)
		return -EINVAL;

	/* determine space needed for page_list. */
	offs = offset_in_page(dma_info->usr_vaddr);
	dma_info->nr_pages = DIV_ROUND_UP(offs + dma_info->size, PAGE_SIZE);
	dma_info->page_list = kcalloc(dma_info->nr_pages,
				      sizeof(struct page *), GFP_KERNEL);
	if (!dma_info->page_list)
		return -ENOMEM;

	/* pin user pages in memory */
	rv = get_user_pages_fast((size_t)dma_info->usr_vaddr & PAGE_MASK,
				 dma_info->nr_pages,
				 dma_info->is_writable,
				 dma_info->page_list);
	if (rv < 0)
		goto fail_get_user_pages;

	if (rv < dma_info->nr_pages) {
		for (i = 0; i < rv; i++) {
			if (dma_info->page_list[i])
				put_page(dma_info->page_list[i]);
		}
		rv = -EFAULT;
		goto fail_get_user_pages;
	}

	return rv;

fail_get_user_pages:
	kfree(dma_info->page_list);
	return rv;
}

static void et_dma_unpin_ubuf(struct et_dma_info *dma_info)
{
	size_t i;

	if (dma_info && dma_info->page_list) {
		for (i = 0; i < dma_info->nr_pages; i++) {
			if (dma_info->is_writable)
				set_page_dirty_lock(dma_info->page_list[i]);
			if (dma_info->page_list[i])
				put_page(dma_info->page_list[i]);
		}
		kfree(dma_info->page_list);
	}
}
#endif

struct et_dma_info *et_dma_search_info(struct rb_root *root, u16 tag_id)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct et_dma_info *dma_info =
			rb_entry(node, struct et_dma_info, node);
		if (tag_id < dma_info->tag_id)
			node = node->rb_left;
		else if (tag_id > dma_info->tag_id)
			node = node->rb_right;
		else
			return dma_info;
	}
	return NULL;
}

bool et_dma_insert_info(struct rb_root *root, struct et_dma_info *dma_info)
{
	struct rb_node **new = &root->rb_node, *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct et_dma_info *this =
			rb_entry(*new, struct et_dma_info, node);
		parent = *new;
		if (dma_info->tag_id < this->tag_id)
			new = &((*new)->rb_left);
		else if (dma_info->tag_id > this->tag_id)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&dma_info->node, parent, new);
	rb_insert_color(&dma_info->node, root);

	return true;
}

void et_dma_delete_info(struct rb_root *root, struct et_dma_info *dma_info)
{
	if (dma_info) {
		// TODO JIRA SW-957: Uncomment when zero copy support is
		// available
		//et_dma_unpin_ubuf(dma_info);
		et_dma_free_coherent(dma_info);

		rb_erase(&dma_info->node, root);
		kfree(dma_info);
	}
}

void et_dma_delete_all_info(struct rb_root *root)
{
	struct et_dma_info *dma_info;
	struct rb_node *node;

	for (node = rb_first(root); node;) {
		dma_info = rb_entry(node, struct et_dma_info, node);
		node = rb_next(node);

		// TODO JIRA SW-957: Uncomment when zero copy support is
		// available
		//et_dma_unpin_ubuf(dma_info);
		et_dma_free_coherent(dma_info);

		rb_erase(&dma_info->node, root);
		kfree(dma_info);
	}
}

ssize_t et_dma_write_to_device(struct et_pci_dev *et_dev,
			       u16 queue_index,
			       struct device_ops_data_write_cmd_t *cmd,
			       size_t cmd_size)
{
	struct et_dma_info *dma_info;
	ssize_t rv;

	if (!cmd->src_host_virt_addr) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA write cmd (src_host_virt_addr: 0x%llx)!",
			cmd->src_host_virt_addr);
		return -EINVAL;
	}

	if (!cmd->size) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA write cmd (size: %u)!",
			cmd->size);
		return -EINVAL;
	}

	if (cmd->size > S32_MAX) {
		dev_err(&et_dev->pdev->dev,
			"Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	dma_info = kmalloc(sizeof(*dma_info), GFP_KERNEL);
	if (!dma_info)
		return -ENOMEM;

	dma_info->tag_id = cmd->command_info.cmd_hdr.tag_id;
	dma_info->usr_vaddr = (void __user __force *)cmd->src_host_virt_addr;
	dma_info->pdev = et_dev->pdev;
	dma_info->size = cmd->size;
	dma_info->is_writable = false; /* readonly */
	dma_info->kern_vaddr = et_dma_alloc_coherent(dma_info);
	if (!dma_info->kern_vaddr) {
		dev_err(&et_dev->pdev->dev, "dma alloc failed\n");
		rv = -ENOMEM;
		goto error_free_dma_info;
	}

	// TODO JIRA SW-957: Uncomment when zero copy support is available
	// Pin the user buffer to avoid swapping out of pages during DMA
	// operations
	//rv = et_dma_pin_ubuf(dma_info);
	//if (rv < 0) {
	//	pr_err("et_dma_pin_ubuf failed\n");
	//	goto error_dma_free_coherent;
	//}

	mutex_lock(&et_dev->ops.dma_rbtree_mutex);
	if (!et_dma_insert_info(&et_dev->ops.dma_rbtree, dma_info)) {
		dev_err(&et_dev->pdev->dev, "tag_id already exists\n");
		rv = -EINVAL;
		mutex_unlock(&et_dev->ops.dma_rbtree_mutex);
		// TODO JIRA SW-957: Uncomment when zero copy support is
		// available
		//goto error_dma_unpin_ubuf;
		goto error_dma_free_coherent;
	}
	mutex_unlock(&et_dev->ops.dma_rbtree_mutex);

	rv = copy_from_user(dma_info->kern_vaddr,
			    dma_info->usr_vaddr,
			    dma_info->size);
	if (rv != 0) {
		dev_err(&et_dev->pdev->dev, "Failed to copy from user\n");
		rv = -EFAULT;
		goto error_dma_delete_info;
	}

	cmd->src_host_phy_addr = dma_info->dma_addr;
	rv = et_squeue_push(et_dev->ops.sq_pptr[queue_index], cmd, cmd_size);
	if (rv < 0)
		goto error_dma_delete_info;

	if (rv != cmd_size) {
		dev_err(&et_dev->pdev->dev,
			"vqueue write didn't send all bytes\n");
		rv = -EIO;
		goto error_dma_delete_info;
	}

	return rv;

error_dma_delete_info:
	et_dma_delete_info(&et_dev->ops.dma_rbtree, dma_info);
	return rv;

	// TODO JIRA SW-957: Uncomment when zero copy support is available
	//error_dma_unpin_ubuf:
	//	et_dma_unpin_ubuf(dma_info);

error_dma_free_coherent:
	et_dma_free_coherent(dma_info);

error_free_dma_info:
	kfree(dma_info);

	return rv;
}

ssize_t et_dma_read_from_device(struct et_pci_dev *et_dev,
				u16 queue_index,
				struct device_ops_data_read_cmd_t *cmd,
				size_t cmd_size)
{
	struct et_dma_info *dma_info;
	ssize_t rv;

	if (!cmd->dst_host_virt_addr) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA read cmd (dst_host_virt_addr: 0x%llx)!",
			cmd->dst_host_virt_addr);
		return -EINVAL;
	}

	if (!cmd->size) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA read cmd (size: %u)!",
			cmd->size);
		return -EINVAL;
	}

	if (cmd->size > S32_MAX) {
		dev_err(&et_dev->pdev->dev,
			"Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	dma_info = kmalloc(sizeof(*dma_info), GFP_KERNEL);
	if (!dma_info)
		return -ENOMEM;

	dma_info->tag_id = cmd->command_info.cmd_hdr.tag_id;
	dma_info->usr_vaddr = (void __user __force *)cmd->dst_host_virt_addr;
	dma_info->pdev = et_dev->pdev;
	dma_info->size = cmd->size;
	dma_info->is_writable = true; /* writable */
	dma_info->kern_vaddr = et_dma_alloc_coherent(dma_info);
	if (!dma_info->kern_vaddr) {
		dev_err(&et_dev->pdev->dev, "dma alloc failed\n");
		rv = -ENOMEM;
		goto error_free_dma_info;
	}

	// TODO JIRA SW-957: Uncomment when zero copy support is available
	// Pin the user buffer to avoid swapping out of pages during DMA
	// operations
	//rv = et_dma_pin_ubuf(dma_info);
	//if (rv < 0) {
	//	pr_err("et_dma_pin_ubuf failed\n");
	//	goto error_dma_free_coherent;
	//}

	mutex_lock(&et_dev->ops.dma_rbtree_mutex);
	if (!et_dma_insert_info(&et_dev->ops.dma_rbtree, dma_info)) {
		dev_err(&et_dev->pdev->dev, "tag_id already exists\n");
		rv = -EINVAL;
		mutex_unlock(&et_dev->ops.dma_rbtree_mutex);
		// TODO JIRA SW-957: Uncomment when zero copy support is
		// available
		//goto error_dma_unpin_ubuf;
		goto error_dma_free_coherent;
	}
	mutex_unlock(&et_dev->ops.dma_rbtree_mutex);

	cmd->dst_host_phy_addr = dma_info->dma_addr;
	rv = et_squeue_push(et_dev->ops.sq_pptr[queue_index], cmd, cmd_size);
	if (rv < 0)
		goto error_dma_delete_info;

	if (rv != cmd_size) {
		dev_err(&et_dev->pdev->dev,
			"vqueue write didn't send all bytes\n");
		rv = -EIO;
		goto error_dma_delete_info;
	}

	return rv;

error_dma_delete_info:
	et_dma_delete_info(&et_dev->ops.dma_rbtree, dma_info);
	return rv;

	// TODO JIRA SW-957: Uncomment when zero copy support is available
	//error_dma_unpin_ubuf:
	//	et_dma_unpin_ubuf(dma_info);

error_dma_free_coherent:
	et_dma_free_coherent(dma_info);

error_free_dma_info:
	kfree(dma_info);

	return rv;
}

ssize_t et_dma_writelist_to_device(struct et_pci_dev *et_dev,
				   u16 queue_index,
				   struct device_ops_dma_writelist_cmd_t *cmd,
				   size_t cmd_size)
{
	struct et_dma_mapping *map;
	struct vm_area_struct *vma;
	ssize_t rv = -EINVAL;
	u32 node_num, nodes_count = 0;

	nodes_count =
		(cmd_size - sizeof(struct device_ops_dma_writelist_cmd_t)) /
		sizeof(struct dma_write_node);

	if (cmd_size <= sizeof(struct device_ops_dma_writelist_cmd_t) ||
	    !nodes_count) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA writelist cmd (size %zu)!",
			cmd_size);
	}

	for (node_num = 0; node_num < nodes_count; node_num++) {
		if (!cmd->list[node_num].src_host_virt_addr) {
			dev_err(&et_dev->pdev->dev,
				"Invalid writelist[%u].src_host_virt_addr: 0x%llx!",
				node_num,
				cmd->list[node_num].src_host_virt_addr);
			return rv;
		}

		if (!cmd->list[node_num].size) {
			dev_err(&et_dev->pdev->dev,
				"Invalid writelist[%u].size: %u!",
				node_num,
				cmd->list[node_num].size);
			return rv;
		}

		vma = find_vma(current->mm,
			       cmd->list[node_num].src_host_virt_addr);
		if (!vma) {
			dev_err(&et_dev->pdev->dev,
				"mapping for writelist[%u].src_host_virt_addr not found!",
				node_num);
			return rv;
		}

		if (cmd->list[node_num].src_host_virt_addr +
			    cmd->list[node_num].size >
		    vma->vm_end) {
			dev_err(&et_dev->pdev->dev,
				"writelist[%u]{.src_host_virt_addr + .size} out of bound!",
				node_num);
			return rv;
		}

		map = vma->vm_private_data;
		cmd->list[node_num].src_host_phy_addr =
			map->dma_addr + cmd->list[node_num].src_host_virt_addr -
			vma->vm_start;
	}

	rv = et_squeue_push(et_dev->ops.sq_pptr[queue_index], cmd, cmd_size);
	if (rv < 0)
		return rv;

	if (rv != cmd_size) {
		dev_err(&et_dev->pdev->dev,
			"vqueue write didn't send all bytes\n");
		return -EIO;
	}

	return rv;
}

ssize_t et_dma_readlist_from_device(struct et_pci_dev *et_dev,
				    u16 queue_index,
				    struct device_ops_dma_readlist_cmd_t *cmd,
				    size_t cmd_size)
{
	struct et_dma_mapping *map;
	struct vm_area_struct *vma;
	ssize_t rv = -EINVAL;
	u32 node_num, nodes_count = 0;

	nodes_count =
		(cmd_size - sizeof(struct device_ops_dma_readlist_cmd_t)) /
		sizeof(struct dma_read_node);

	if (cmd_size <= sizeof(struct device_ops_dma_readlist_cmd_t) ||
	    !nodes_count) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA readlist cmd (size %zu)!",
			cmd_size);
	}

	for (node_num = 0; node_num < nodes_count; node_num++) {
		if (!cmd->list[node_num].dst_host_virt_addr) {
			dev_err(&et_dev->pdev->dev,
				"Invalid readlist[%u].dst_host_virt_addr: 0x%llx!",
				node_num,
				cmd->list[node_num].dst_host_virt_addr);
			return rv;
		}

		if (!cmd->list[node_num].size) {
			dev_err(&et_dev->pdev->dev,
				"Invalid readlist[%u].size: %u!",
				node_num,
				cmd->list[node_num].size);
			return rv;
		}

		vma = find_vma(current->mm,
			       cmd->list[node_num].dst_host_virt_addr);
		if (!vma) {
			dev_err(&et_dev->pdev->dev,
				"mapping for readlist[%u].dst_host_virt_addr not found!",
				node_num);
			return rv;
		}

		if (cmd->list[node_num].dst_host_virt_addr +
			    cmd->list[node_num].size >
		    vma->vm_end) {
			dev_err(&et_dev->pdev->dev,
				"readlist[%u]{.dst_host_virt_addr + .size} out of bound!",
				node_num);
			return rv;
		}

		map = vma->vm_private_data;
		cmd->list[node_num].dst_host_phy_addr =
			map->dma_addr + cmd->list[node_num].dst_host_virt_addr -
			vma->vm_start;
	}

	rv = et_squeue_push(et_dev->ops.sq_pptr[queue_index], cmd, cmd_size);
	if (rv < 0)
		return rv;

	if (rv != cmd_size) {
		dev_err(&et_dev->pdev->dev,
			"vqueue write didn't send all bytes\n");
		return -EIO;
	}

	return rv;
}

ssize_t et_dma_move_data(struct et_pci_dev *et_dev,
			 u16 queue_index,
			 char __user *ucmd,
			 size_t ucmd_size)
{
	void *kern_buf;
	struct cmn_header_t *header;
	ssize_t rv;

	if (ucmd_size < sizeof(struct cmn_header_t) || ucmd_size > U16_MAX) {
		pr_err("invalid cmd size: %ld", ucmd_size);
		return -EINVAL;
	}

	kern_buf = kzalloc(ucmd_size, GFP_KERNEL);

	if (!kern_buf)
		return -ENOMEM;

	rv = copy_from_user(kern_buf, ucmd, ucmd_size);
	if (rv) {
		pr_err("copy_from_user failed\n");
		rv = -ENOMEM;
		goto free_kern_buf;
	}

	header = (struct cmn_header_t *)kern_buf;

	if (header->msg_id == DEV_OPS_API_MID_DATA_READ_CMD) {
		if (ucmd_size < sizeof(struct device_ops_data_read_cmd_t)) {
			pr_err("Invalid DMA read cmd (size %ld)", ucmd_size);
			rv = -EINVAL;
			goto free_kern_buf;
		}
		rv = et_dma_read_from_device(et_dev,
					     queue_index,
					     kern_buf,
					     ucmd_size);
	} else if (header->msg_id == DEV_OPS_API_MID_DATA_WRITE_CMD) {
		if (ucmd_size < sizeof(struct device_ops_data_write_cmd_t)) {
			pr_err("Invalid DMA write cmd (size %ld)", ucmd_size);
			rv = -EINVAL;
			goto free_kern_buf;
		}
		rv = et_dma_write_to_device(et_dev,
					    queue_index,
					    kern_buf,
					    ucmd_size);
	} else if (header->msg_id == DEV_OPS_API_MID_DMA_READLIST_CMD) {
		// Command size should be large enough to have atleast one read
		// node entry
		if (ucmd_size < sizeof(struct device_ops_dma_readlist_cmd_t) +
					sizeof(struct dma_read_node)) {
			pr_err("Invalid DMA readlist cmd (size %ld)",
			       ucmd_size);
			rv = -EINVAL;
			goto free_kern_buf;
		}
		rv = et_dma_readlist_from_device(et_dev,
						 queue_index,
						 kern_buf,
						 ucmd_size);
	} else if (header->msg_id == DEV_OPS_API_MID_DMA_WRITELIST_CMD) {
		// Command size should be large enough to have atleast one
		// write node entry
		if (ucmd_size < sizeof(struct device_ops_dma_writelist_cmd_t) +
					sizeof(struct dma_write_node)) {
			pr_err("Invalid DMA writelist cmd (size %ld)",
			       ucmd_size);
			rv = -EINVAL;
			goto free_kern_buf;
		}
		rv = et_dma_writelist_to_device(et_dev,
						queue_index,
						kern_buf,
						ucmd_size);
	}

free_kern_buf:
	kfree(kern_buf);

	return rv;
}
