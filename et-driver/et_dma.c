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
#include "et_vma.h"
#include "et_vqueue.h"

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

		vma = et_find_vma(cmd->list[node_num].src_host_virt_addr);
		if (!vma) {
			dev_err(&et_dev->pdev->dev,
				"mapping for writelist[%u].src_host_virt_addr not found!",
				node_num);
			return rv;
		}

		map = vma->vm_private_data;
		if (!map) {
			dev_err(&et_dev->pdev->dev,
				"mapping for writelist[%u].src_host_virt_addr is corrupted!",
				node_num);
			return rv;
		}

		if (cmd->list[node_num].size >
		    et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				    .access.dma_elem_size *
			    MEM_REGION_DMA_ELEMENT_STEP_SIZE) {
			dev_err(&et_dev->pdev->dev,
				"writelist[%u].size out of bound (0x%x/0x%x)!",
				node_num,
				cmd->list[node_num].size,
				et_dev->ops.regions
						[OPS_MEM_REGION_TYPE_HOST_MANAGED]
							.access.dma_elem_size *
					MEM_REGION_DMA_ELEMENT_STEP_SIZE);
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

	rv = et_squeue_push(&et_dev->ops.vq_data.sqs[queue_index],
			    cmd,
			    cmd_size);
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

		vma = et_find_vma(cmd->list[node_num].dst_host_virt_addr);
		if (!vma) {
			dev_err(&et_dev->pdev->dev,
				"mapping for readlist[%u].dst_host_virt_addr not found!",
				node_num);
			return rv;
		}

		map = vma->vm_private_data;
		if (!map) {
			dev_err(&et_dev->pdev->dev,
				"mapping for readlist[%u].dst_host_virt_addr is corrupted!",
				node_num);
			return rv;
		}

		if (cmd->list[node_num].size >
		    et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				    .access.dma_elem_size *
			    MEM_REGION_DMA_ELEMENT_STEP_SIZE) {
			dev_err(&et_dev->pdev->dev,
				"readlist[%u].size out of bound (0x%x/0x%x)!",
				node_num,
				cmd->list[node_num].size,
				et_dev->ops.regions
						[OPS_MEM_REGION_TYPE_HOST_MANAGED]
							.access.dma_elem_size *
					MEM_REGION_DMA_ELEMENT_STEP_SIZE);
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

		cmd->list[node_num].dst_host_phy_addr =
			map->dma_addr + cmd->list[node_num].dst_host_virt_addr -
			vma->vm_start;
	}

	rv = et_squeue_push(&et_dev->ops.vq_data.sqs[queue_index],
			    cmd,
			    cmd_size);
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

	if (header->msg_id == DEV_OPS_API_MID_DMA_READLIST_CMD) {
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
