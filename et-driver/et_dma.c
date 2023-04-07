/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include "et_dma.h"
#include "et_device_api.h"
#include "et_vma.h"
#include "et_vqueue.h"

ssize_t et_dma_move_data(struct et_pci_dev *et_dev,
			 u16 queue_index,
			 char __user *ucmd,
			 size_t cmd_size)
{
	ssize_t rv;
	u32 node_num, nodes_count = 0;
	struct et_dma_mapping *map;
	struct vm_area_struct *vma;
	struct device_ops_dma_list_cmd_t *cmd;

	nodes_count = (cmd_size - sizeof(*cmd)) / sizeof(cmd->list[0]);

	if (cmd_size <= sizeof(*cmd) || !nodes_count) {
		dev_err(&et_dev->pdev->dev,
			"Invalid DMA list cmd (size %zu)!",
			cmd_size);
		return -EINVAL;
	}

	cmd = kzalloc(cmd_size, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	rv = copy_from_user(cmd, ucmd, cmd_size);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "DMA list: copy_from_user failed!");
		rv = -EFAULT;
		goto free_cmd_mem;
	}

	for (node_num = 0; node_num < nodes_count; node_num++) {
		if (!cmd->list[node_num].host_virt_addr) {
			dev_err(&et_dev->pdev->dev,
				"Invalid DMA list[%u].host_virt_addr: 0x%llx!",
				node_num,
				cmd->list[node_num].host_virt_addr);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		if (!cmd->list[node_num].size) {
			dev_err(&et_dev->pdev->dev,
				"Invalid DMA list[%u].size: %u!",
				node_num,
				cmd->list[node_num].size);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		vma = et_find_vma(et_dev, cmd->list[node_num].host_virt_addr);
		if (!vma) {
			dev_err(&et_dev->pdev->dev,
				"mapping for DMA list[%u].host_virt_addr not found!",
				node_num);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		map = vma->vm_private_data;
		if (!map) {
			dev_err(&et_dev->pdev->dev,
				"mapping for DMA list[%u].host_virt_addr is corrupted!",
				node_num);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		if (cmd->list[node_num].size >
		    et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				    .access.dma_elem_size *
			    MEM_REGION_DMA_ELEMENT_STEP_SIZE) {
			dev_err(&et_dev->pdev->dev,
				"DMA list[%u].size out of bound (0x%x/0x%x)!",
				node_num,
				cmd->list[node_num].size,
				et_dev->ops.regions
						[OPS_MEM_REGION_TYPE_HOST_MANAGED]
							.access.dma_elem_size *
					MEM_REGION_DMA_ELEMENT_STEP_SIZE);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		if (cmd->list[node_num].host_virt_addr +
			    cmd->list[node_num].size >
		    vma->vm_end) {
			dev_err(&et_dev->pdev->dev,
				"DMA list[%u]{.host_virt_addr + .size} out of bound!",
				node_num);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		cmd->list[node_num].host_phys_addr =
			map->dma_addr + cmd->list[node_num].host_virt_addr -
			vma->vm_start;
	}

	rv = et_squeue_push(&et_dev->ops.vq_data.sqs[queue_index],
			    cmd,
			    cmd_size);
	if (rv < 0)
		goto free_cmd_mem;

	if (rv != cmd_size) {
		dev_err(&et_dev->pdev->dev,
			"DMA list: vqueue write didn't send all bytes\n");
		rv = -EIO;
	}

free_cmd_mem:
	kfree(cmd);

	return rv;
}
