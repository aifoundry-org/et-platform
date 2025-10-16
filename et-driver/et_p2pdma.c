// SPDX-License-Identifier: GPL-2.0

/******************************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 ******************************************************************************/

#include "et_p2pdma.h"
#include "et_device_api.h"
#include "et_vqueue.h"

/* P2P DMA mapping information for all devices */
static struct et_p2pdma_mapping p2pdma_mappings[ET_MAX_DEVS] = {};

/**
 * et_p2pdma_init() - Initialize P2P DMA data structs
 * @devnum: Device physical number
 */
void et_p2pdma_init(u8 devnum)
{
	init_rwsem(&p2pdma_mappings[devnum].rwsem);
	INIT_LIST_HEAD(&p2pdma_mappings[devnum].region_list);
}

/**
 * et_p2pdma_get_compat_bitmap() - Get P2P DMA compatibility bitmap
 * @devnum: Device physical number
 *
 * Provides the information about P2P DMA compatibility of device with `devnum`
 * with other devices e.g. Bit0 being set means this device is P2P DMA
 * compatible with device0.
 *
 * Return: u64 bitmap
 */
u64 et_p2pdma_get_compat_bitmap(u8 devnum)
{
	u64 dev_compat_bitmap;

	memcpy(&dev_compat_bitmap, p2pdma_mappings[devnum].dev_compat_bitmap,
	       sizeof(dev_compat_bitmap));
	return dev_compat_bitmap;
}

/**
 * et_p2pdma_add_resource() - Add device BAR region as P2P DMA resource
 * @et_dev: Pointer to et_pci_dev structure
 * @bm_info: BAR mapping information
 * @region: Mapping information goes into this struct
 *
 * Maps a BAR region as a P2P DMA resource and also adds the mapped region
 * information into the linked list of P2P DMA regions for the particular
 * device. If this is a first region being added for this particular device
 * then it also performs initializations w.r.t finding compatibility with other
 * devices.
 * NOTE: P2P DMA resource added between devres_{open|close}_group() calls can
 * be freed-up using devres_release_group()
 *	  devres_open_group()
 *	  ...
 *	  pci_p2pdma_add_resource()
 *	  ...
 *	  devres_close_group()
 *
 * Return: 0 on success, negative value for error
 */
int et_p2pdma_add_resource(struct et_pci_dev *et_dev,
			   const struct et_bar_mapping *bm_info,
			   struct et_mapped_region *region)
{
	int rv;
	u8 devnum;
	bool first_map;
	struct et_p2pdma_region *new_region;

	region->p2p.devres_id =
		devres_open_group(&et_dev->pdev->dev, NULL, GFP_KERNEL);
	if (!region->p2p.devres_id)
		return -ENOMEM;

	rv = pci_p2pdma_add_resource(et_dev->pdev, bm_info->bar, bm_info->size,
				     bm_info->bar_offset);
	devres_close_group(&et_dev->pdev->dev, region->p2p.devres_id);
	if (rv)
		goto error_devres_release_group;

	region->p2p.virt_addr = pci_alloc_p2pmem(et_dev->pdev, bm_info->size);
	if (!region->p2p.virt_addr) {
		rv = -ENOMEM;
		goto error_devres_release_group;
	}

	region->p2p.pci_bus_addr =
		pci_p2pmem_virt_to_bus(et_dev->pdev, region->p2p.virt_addr);
	if (!region->p2p.pci_bus_addr) {
		rv = -ENOMEM;
		goto error_free_p2pmem;
	}

	new_region = kmalloc(sizeof(*new_region), GFP_KERNEL);
	if (!new_region) {
		rv = -ENOMEM;
		goto error_free_p2pmem;
	}

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

		// Find compatibility of this device with other device
		if (p2pdma_mappings[et_dev->devnum].pdev &&
		    p2pdma_mappings[devnum].pdev) {
			if (pci_p2pdma_distance(
				    p2pdma_mappings[et_dev->devnum].pdev,
				    &p2pdma_mappings[devnum].pdev->dev,
				    false) >= 0) {
				// Set bit in dev_compat_bitmap of this device
				set_bit(devnum, p2pdma_mappings[et_dev->devnum]
							.dev_compat_bitmap);
				// Set bit in dev_compat_bitmap of other device
				set_bit(et_dev->devnum,
					p2pdma_mappings[devnum]
						.dev_compat_bitmap);
			} else {
				dev_warn(
					&et_dev->pdev->dev,
					"peer-to-peer DMA not supported with %s",
					dev_name(&p2pdma_mappings[devnum]
							  .pdev->dev));
			}
		}

		up_write(&p2pdma_mappings[devnum].rwsem);
		up_write(&p2pdma_mappings[et_dev->devnum].rwsem);
	}

	return rv;

error_free_p2pmem:
	pci_free_p2pmem(et_dev->pdev, region->p2p.virt_addr, bm_info->size);
	region->p2p.virt_addr = NULL;
	region->p2p.pci_bus_addr = 0;

error_devres_release_group:
	devres_release_group(&et_dev->pdev->dev, region->p2p.devres_id);
	region->p2p.devres_id = NULL;

	return rv;
}

/**
 * et_p2pdma_release_resource() - Release the P2P DMA resource
 * @et_dev: Pointer to et_pci_dev structure
 * @region: Mapped region information
 *
 * Releases a P2P DMA resource and also removes the mapped region information
 * from the linked list of P2P DMA regions for this particular device. If this
 * is the last region being removed for this particular device then it also
 * performs clean-ups w.r.t clearing the compatibility with other devices.
 */
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
				pci_free_p2pmem(
					p2pdma_mappings[et_dev->devnum].pdev,
					pos->region->p2p.virt_addr,
					pos->region->size);
				devres_release_group(
					&p2pdma_mappings[et_dev->devnum]
						 .pdev->dev,
					pos->region->p2p.devres_id);
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

/**
 * et_p2pdma_move_data() - Forward P2P DMA read/write request on SQ
 * @et_dev: pointer to et_pci_dev structure
 * @queue_index: SQ index
 * @ucmd: user pointer to command coming from user-space
 * @ucmd_size: size of user command
 *
 * Searches for P2P memory for perr_devnum against peer device physical address
 * and fills in the corresponding PCI bus address of each node in P2P DMA list
 * command and pushes the command on SQ.
 *
 * Return: number of bytes written on SQ on success, negative value for error
 */
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
