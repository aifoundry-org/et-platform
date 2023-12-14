// SPDX-License-Identifier: GPL-2.0

/******************************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ******************************************************************************
 */

#include "et_device_api.h"
#include "et_p2pdma.h"
#include "et_vqueue.h"

static struct et_p2pdma_mapping p2pdma_mappings[ET_MAX_DEVS] = {};

void et_p2pdma_init(u8 devnum)
{
	init_rwsem(&p2pdma_mappings[devnum].rwsem);
	INIT_LIST_HEAD(&p2pdma_mappings[devnum].region_list);
}

u64 et_p2pdma_get_compat_bitmap(u8 devnum)
{
	u64 dev_compat_bitmap;

	memcpy(&dev_compat_bitmap, p2pdma_mappings[devnum].dev_compat_bitmap,
	       sizeof(dev_compat_bitmap));
	return dev_compat_bitmap;
}

int et_p2pdma_add_resource(struct et_pci_dev *et_dev,
			   const struct et_bar_mapping *bm_info,
			   struct et_mapped_region *region)
{
	int rv = 0;
	u8 devnum;
	bool first_map;
	struct et_p2pdma_region *new_region;

	(void)bm_info;
	region->p2p.devres_id = NULL;
	region->p2p.virt_addr = NULL;
	region->p2p.pci_bus_addr = region->dev_phys_addr;

	new_region = kmalloc(sizeof(*new_region), GFP_KERNEL);
	if (!new_region)
		return -ENOMEM;

	new_region->region = region;

	down_write(&p2pdma_mappings[et_dev->devnum].rwsem);

	first_map = list_empty(&p2pdma_mappings[et_dev->devnum].region_list);
	list_add_tail(&new_region->list,
		      &p2pdma_mappings[et_dev->devnum].region_list);

	// Initialize fields for current device when first P2P region is mapped
	if (!first_map) {
		up_write(&p2pdma_mappings[et_dev->devnum].rwsem);
		return rv;
	}

	p2pdma_mappings[et_dev->devnum].pdev = et_dev->pdev;
	bitmap_zero(p2pdma_mappings[et_dev->devnum].dev_compat_bitmap,
		    ET_MAX_DEVS);

	up_write(&p2pdma_mappings[et_dev->devnum].rwsem);

	// Find P2P DMA compatibility of current device with other devices when
	// first P2P region is mapped for current device
	for (devnum = find_first_bit(dev_bitmap, ET_MAX_DEVS);
	     devnum < ET_MAX_DEVS;
	     devnum = find_next_bit(dev_bitmap, ET_MAX_DEVS, ++devnum)) {
		if (devnum == et_dev->devnum)
			continue;

		// Introducing lock ordering to avoid nested lock deadlock.
		// Grab the lock for device with smaller devnum first.
		if (et_dev->devnum < devnum) {
			down_write(&p2pdma_mappings[et_dev->devnum].rwsem);
			down_write(&p2pdma_mappings[devnum].rwsem);
		} else {
			down_write(&p2pdma_mappings[devnum].rwsem);
			down_write(&p2pdma_mappings[et_dev->devnum].rwsem);
		}

		// Setting all devices to be P2P compatible as long they are
		// initialized
		if (p2pdma_mappings[et_dev->devnum].pdev &&
		    p2pdma_mappings[devnum].pdev) {
			// Set bit in dev_compat_bitmap of this device
			set_bit(devnum, p2pdma_mappings[et_dev->devnum]
						.dev_compat_bitmap);
			// Set bit in dev_compat_bitmap of other device
			set_bit(et_dev->devnum,
				p2pdma_mappings[devnum].dev_compat_bitmap);
		}

		up_write(&p2pdma_mappings[devnum].rwsem);
		up_write(&p2pdma_mappings[et_dev->devnum].rwsem);
	}

	return rv;
}

void et_p2pdma_release_resource(struct et_pci_dev *et_dev,
				struct et_mapped_region *region)
{
	u8 devnum;
	struct et_p2pdma_region *pos, *tmp;

	down_write(&p2pdma_mappings[et_dev->devnum].rwsem);

	if (p2pdma_mappings[et_dev->devnum].pdev) {
		list_for_each_entry_safe (
			pos, tmp, &p2pdma_mappings[et_dev->devnum].region_list,
			list) {
			if (pos->region == region) {
				list_del(&pos->list);
				kfree(pos);
				break;
			}
		}
	}

	if (!list_empty(&p2pdma_mappings[et_dev->devnum].region_list)) {
		up_write(&p2pdma_mappings[et_dev->devnum].rwsem);
		return;
	}

	// Clear fields of current device when last P2P region is removed
	p2pdma_mappings[et_dev->devnum].pdev = NULL;
	bitmap_zero(p2pdma_mappings[et_dev->devnum].dev_compat_bitmap,
		    ET_MAX_DEVS);

	up_write(&p2pdma_mappings[et_dev->devnum].rwsem);

	// Clear current device bits from other devices' compatibility bitmap
	// as well when last P2P region is removed
	for (devnum = find_first_bit(dev_bitmap, ET_MAX_DEVS);
	     devnum < ET_MAX_DEVS;
	     devnum = find_next_bit(dev_bitmap, ET_MAX_DEVS, ++devnum)) {
		if (devnum == et_dev->devnum)
			continue;

		down_write(&p2pdma_mappings[devnum].rwsem);

		if (p2pdma_mappings[devnum].pdev)
			clear_bit(et_dev->devnum,
				  p2pdma_mappings[devnum].dev_compat_bitmap);

		up_write(&p2pdma_mappings[devnum].rwsem);
	}
}

ssize_t et_p2pdma_move_data(struct et_pci_dev *et_dev, u16 queue_index,
			    char __user *ucmd, size_t cmd_size)
{
	ssize_t rv;
	u32 node_num, nodes_count = 0;
	struct et_p2pdma_mapping *map;
	struct et_p2pdma_region *pos;
	struct et_mapped_region *region;
	struct device_ops_p2pdma_list_cmd_t *cmd;

	nodes_count = (cmd_size - sizeof(*cmd)) / sizeof(cmd->list[0]);

	if (cmd_size <= sizeof(*cmd) || !nodes_count) {
		dev_err(&et_dev->pdev->dev,
			"P2PDMA list cmd_size: %zu is invalid!", cmd_size);
		return -EINVAL;
	}

	cmd = kzalloc(cmd_size, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	rv = copy_from_user(cmd, ucmd, cmd_size);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"P2PDMA list: copy_from_user failed!");
		rv = -EFAULT;
		goto free_cmd_mem;
	}

	for (node_num = 0; node_num < nodes_count; node_num++) {
		if (!test_bit(cmd->list[node_num].peer_devnum, dev_bitmap)) {
			dev_err(&et_dev->pdev->dev,
				"P2PDMA list[%u].peer_devnum: %u does not exist!",
				node_num, cmd->list[node_num].peer_devnum);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		if (!cmd->list[node_num].size) {
			dev_err(&et_dev->pdev->dev,
				"P2PDMA list[%u].size: %u is invalid!",
				node_num, cmd->list[node_num].size);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		if (cmd->list[node_num].size >
		    et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				    .access.dma_elem_size *
			    MEM_REGION_DMA_ELEMENT_STEP_SIZE) {
			dev_err(&et_dev->pdev->dev,
				"P2PDMA list[%u].size out of bound (0x%x/0x%x)!",
				node_num, cmd->list[node_num].size,
				et_dev->ops.regions
						[OPS_MEM_REGION_TYPE_HOST_MANAGED]
							.access.dma_elem_size *
					MEM_REGION_DMA_ELEMENT_STEP_SIZE);
			rv = -EINVAL;
			goto free_cmd_mem;
		}

		map = &p2pdma_mappings[cmd->list[node_num].peer_devnum];
		down_read(&map->rwsem);

		if (!map->pdev ||
		    !test_bit(et_dev->devnum, map->dev_compat_bitmap)) {
			dev_err(&et_dev->pdev->dev,
				"P2PDMA list[%u].peer_devnum: %u is incompatible peer!",
				node_num, cmd->list[node_num].peer_devnum);
			rv = -EOPNOTSUPP;
			up_read(&map->rwsem);
			goto free_cmd_mem;
		}

		region = NULL;
		list_for_each_entry (pos, &map->region_list, list) {
			if (cmd->list[node_num].peer_device_phys_addr >=
				    pos->region->dev_phys_addr &&
			    cmd->list[node_num].peer_device_phys_addr +
					    cmd->list[node_num].size <=
				    pos->region->dev_phys_addr +
					    pos->region->size) {
				region = pos->region;
				break;
			}
		}

		if (!region) {
			dev_err(&et_dev->pdev->dev,
				"P2PDMA list[%u]{.peer_device_phys_addr: 0x%llx + .size: 0x%x} range not found!",
				node_num,
				cmd->list[node_num].peer_device_phys_addr,
				cmd->list[node_num].size);
			rv = -EINVAL;
			up_read(&map->rwsem);
			goto free_cmd_mem;
		}

		cmd->list[node_num].peer_device_bus_addr =
			region->p2p.pci_bus_addr +
			(cmd->list[node_num].peer_device_phys_addr -
			 region->dev_phys_addr);

		up_read(&map->rwsem);
	}

	rv = et_squeue_push(&et_dev->ops.vq_data.sqs[queue_index], cmd,
			    cmd_size);
	if (rv < 0)
		goto free_cmd_mem;

	if (rv != cmd_size) {
		dev_err(&et_dev->pdev->dev,
			"P2PDMA list: vqueue write could not send all bytes!");
		rv = -EIO;
	}

free_cmd_mem:
	kfree(cmd);

	return rv;
}
