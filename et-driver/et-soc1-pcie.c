/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <uapi/linux/pci_regs.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/crc32.h>

#include "et_io.h"
#include "et_dma.h"
#include "et_ioctl.h"
#include "et_vqueue.h"
#include "et_mmio.h"
#include "et_pci_dev.h"

#ifdef ENABLE_DRIVER_TESTS
#include "et_test.h"
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME "Esperanto"

#define ET_PCIE_VENDOR_ID 0x1e0a
#define ET_PCIE_TEST_DEVICE_ID 0x9038
#define ET_PCIE_SOC1_ID 0xeb01

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

#define ET_MAX_DEVS	64
static DECLARE_BITMAP(dev_bitmap, ET_MAX_DEVS);

static u8 get_index(void)
{
	u8 index;

	if (bitmap_full(dev_bitmap, ET_MAX_DEVS))
		return -EBUSY;
	index = find_first_zero_bit(dev_bitmap, ET_MAX_DEVS);
	set_bit(index, dev_bitmap);
	return index;
}

#ifndef ENABLE_DRIVER_TESTS
static __poll_t esperanto_pcie_ops_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = 0;
	struct et_ops_dev *ops;
	int i;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);

	// TODO: Separate ISRs for SQ and CQ. waitqueue is wake up whenever an
	// interrupt is received. Currently same interrupt is being used for SQ
	// and CQ. Move `et_squeue_event_available()` to SQ ISR. waitqueue
	// should be wake up from different ISRs after the bitmaps are updated.
	poll_wait(fp, &ops->vq_common.waitqueue, wait);

	if (ops->vq_common.aborting)
		return -EINTR;

	// Update sq_bitmap for all SQs, set corresponding bit when space
	// available is more than threshold
	for (i = 0; i < ops->vq_common.dir_vq.sq_count; i++)
		et_squeue_event_available(ops->sq_pptr[i]);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(ops->vq_common.sq_bitmap,
			  ops->vq_common.dir_vq.sq_count))
		mask |= EPOLLOUT;

	// Generate EPOLLIN event if any CQ msg is saved for userspace
	if (!bitmap_empty(ops->vq_common.cq_bitmap,
			  ops->vq_common.dir_vq.cq_count))
		mask |= EPOLLIN;

	return mask;
}
#endif

static long esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd,
				     unsigned long arg)
{
	struct et_pci_dev *et_dev;
	struct et_ops_dev *ops;
	struct mmio_desc mmio_info;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	enum et_dma_buf_type type = 0;
	u16 sq_idx;
	size_t size;
	u16 max_size;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);
	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_USER_DRAM_BASE:
		if (!ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].is_valid)
			return -EINVAL;

		if (copy_to_user((u64 *)arg, &ops->regions
		    [OPS_MEM_REGION_TYPE_HOST_MANAGED].soc_addr, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_USER_DRAM_SIZE:
		if (!ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].is_valid)
			return -EINVAL;

		if (copy_to_user((u64 *)arg, &ops->regions
		    [OPS_MEM_REGION_TYPE_HOST_MANAGED].size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_MMIO_WRITE:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (!mmio_info.ubuf || !mmio_info.size || !mmio_info.devaddr)
			return -EINVAL;

		return et_mmio_write_to_device(et_dev, false /* ops_dev */,
					       (void __user *)mmio_info.ubuf,
					       mmio_info.size,
					       mmio_info.devaddr);

	case ETSOC1_IOCTL_MMIO_READ:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (!mmio_info.ubuf || !mmio_info.size || !mmio_info.devaddr)
			return -EINVAL;

		return et_mmio_read_from_device(et_dev, false /* ops_dev */,
						(void __user *)mmio_info.ubuf,
						mmio_info.size,
						mmio_info.devaddr);

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (copy_to_user((u64 *)arg, &ops->vq_common.dir_vq.sq_count,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = ops->vq_common.dir_vq.per_sq_size -
			   sizeof(struct et_circbuffer);
		if (copy_to_user((u64 *)arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (cmd_info.sq_index >=
		    et_dev->ops.vq_common.dir_vq.sq_count ||
		    !cmd_info.cmd || !cmd_info.size)
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_DMA) {
			if (cmd_info.flags & CMD_DESC_FLAG_DMA_MMFW_TRACEBUF)
				type = ET_DMA_TRACEBUF_MMFW;
			else if (cmd_info.flags &
				 CMD_DESC_FLAG_DMA_CMFW_TRACEBUF)
				type = ET_DMA_TRACEBUF_CMFW;
			else
				type = ET_DMA_UBUF;

			return et_dma_move_data(et_dev, cmd_info.sq_index,
						(void __user *)cmd_info.cmd,
						cmd_info.size, type);
		} else {
			return et_squeue_copy_from_user
				(et_dev, false /* ops_dev */,
				 cmd_info.sq_index,
				 (char __user *)cmd_info.cmd,
				 cmd_info.size);
		}

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (rsp_info.cq_index >=
		    et_dev->ops.vq_common.dir_vq.cq_count ||
		    !rsp_info.rsp || !rsp_info.size)
			return -EINVAL;

		return et_cqueue_copy_to_user
				(et_dev, false /* ops_dev */,
				 rsp_info.cq_index,
				 (char __user *)rsp_info.rsp,
				 rsp_info.size);

	case ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP:
		if (copy_to_user((u64 *)arg, ops->vq_common.sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (copy_to_user((u64 *)arg, ops->vq_common.cq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_SQ_THRESHOLD:
		if (copy_from_user(&sq_threshold_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (sq_threshold_info.sq_index >=
		    et_dev->ops.vq_common.dir_vq.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
		    (ops->vq_common.dir_vq.per_sq_size -
		     sizeof(struct et_circbuffer)))
			return -EINVAL;

		sq_idx = sq_threshold_info.sq_index;

		atomic_set(&ops->sq_pptr[sq_idx]->sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		clear_bit(sq_idx, ops->vq_common.sq_bitmap);
		if (et_squeue_event_available(ops->sq_pptr[sq_idx]))
			wake_up_interruptible
				(&ops->sq_pptr[sq_idx]->vq_common->waitqueue);

		return 0;

#ifdef ENABLE_DRIVER_TESTS
	case ETSOC1_IOCTL_TEST_VQ:
		return test_virtqueue(et_dev, (u16)arg);
#endif

	default:
		pr_err("%s: unknown cmd: 0x%x\n", __func__, cmd);
		return -EINVAL;
	}
	return -EINVAL;
}

static __poll_t esperanto_pcie_mgmt_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = 0;
	struct et_mgmt_dev *mgmt;
	int i;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);

	// TODO: Separate ISRs for SQ and CQ. waitqueue is wake up whenever an
	// interrupt is received. Currently same interrupt is being used for SQ
	// and CQ. Move `et_squeue_event_available()` to SQ ISR. waitqueue
	// should be wake up from different ISRs after the bitmaps are updated.
	poll_wait(fp, &mgmt->vq_common.waitqueue, wait);

	if (mgmt->vq_common.aborting)
		return -EINTR;

	// Update sq_bitmap for all SQs, set corresponding bit when space
	// available is more than threshold
	for (i = 0; i < mgmt->vq_common.dir_vq.sq_count; i++)
		et_squeue_event_available(mgmt->sq_pptr[i]);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(mgmt->vq_common.sq_bitmap,
			  mgmt->vq_common.dir_vq.sq_count))
		mask |= EPOLLOUT;

	// Generate EPOLLIN event if any CQ msg list has message for userspace
	if (!bitmap_empty(mgmt->vq_common.cq_bitmap,
			  mgmt->vq_common.dir_vq.cq_count))
		mask |= EPOLLIN;

	return mask;
}

static long esperanto_pcie_mgmt_ioctl(struct file *fp, unsigned int cmd,
				      unsigned long arg)
{
	struct et_mgmt_dev *mgmt;
	struct et_pci_dev *et_dev;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	u16 sq_idx;
	size_t size;
	u16 max_size;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (copy_to_user((u64 *)arg, &mgmt->vq_common.dir_vq.sq_count,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = mgmt->vq_common.dir_vq.per_sq_size -
			   sizeof(struct et_circbuffer);
		if (copy_to_user((u64 *)arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_ACTIVE_SHIRE:
		if (copy_to_user((u64 *)arg, &mgmt->minion_shires, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_ACTIVE_SHIRE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (cmd_info.sq_index >=
		    et_dev->mgmt.vq_common.dir_vq.sq_count ||
		    !cmd_info.cmd || !cmd_info.size)
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_DMA) {
			return -EINVAL;
		} else {
			return et_squeue_copy_from_user
				(et_dev, true /* mgmt_dev */,
				 cmd_info.sq_index,
				 (char __user *)cmd_info.cmd,
				 cmd_info.size);
		}

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (rsp_info.cq_index >=
		    et_dev->mgmt.vq_common.dir_vq.cq_count ||
		    !rsp_info.rsp || !rsp_info.size)
			return -EINVAL;

		return et_cqueue_copy_to_user(et_dev, true /* mgmt_dev */,
				rsp_info.cq_index, (char __user *)rsp_info.rsp,
				rsp_info.size);

	case ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP:
		if (copy_to_user((u64 *)arg, mgmt->vq_common.sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (copy_to_user((u64 *)arg, mgmt->vq_common.cq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_SQ_THRESHOLD:
		if (copy_from_user(&sq_threshold_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (sq_threshold_info.sq_index >=
		    et_dev->mgmt.vq_common.dir_vq.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
		    (mgmt->vq_common.dir_vq.per_sq_size -
		     sizeof(struct et_circbuffer)))
			return -EINVAL;

		sq_idx = sq_threshold_info.sq_index;

		atomic_set(&mgmt->sq_pptr[sq_idx]->sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		clear_bit(sq_idx, mgmt->vq_common.sq_bitmap);
		if (et_squeue_event_available(mgmt->sq_pptr[sq_idx]))
			wake_up_interruptible
				(&mgmt->sq_pptr[sq_idx]->vq_common->waitqueue);

		return 0;

	default:
		pr_err("%s: unknown cmd: 0x%x\n", __func__, cmd);
		return -EINVAL;
	}
	return -EINVAL;
}

static int esperanto_pcie_ops_open(struct inode *inode, struct file *fp)
{
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);

	spin_lock(&ops->ops_open_lock);
	if (ops->is_ops_open) {
		spin_unlock(&ops->ops_open_lock);
		pr_err("Tried to open same device multiple times\n");
		return -EBUSY; /* already open */
	}
	ops->is_ops_open = true;
	spin_unlock(&ops->ops_open_lock);

	return 0;
}

static int esperanto_pcie_mgmt_open(struct inode *inode, struct file *fp)
{
	struct et_mgmt_dev *mgmt;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);

	spin_lock(&mgmt->mgmt_open_lock);
	if (mgmt->is_mgmt_open) {
		spin_unlock(&mgmt->mgmt_open_lock);
		pr_err("Tried to open same device multiple times\n");
		return -EBUSY; /* already open */
	}
	mgmt->is_mgmt_open = true;
	spin_unlock(&mgmt->mgmt_open_lock);

	return 0;
}

static int esperanto_pcie_ops_release(struct inode *inode, struct file *fp)
{
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);
	ops->is_ops_open = false;

	return 0;
}

static int esperanto_pcie_mgmt_release(struct inode *inode, struct file *fp)
{
	struct et_mgmt_dev *mgmt;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);
	mgmt->is_mgmt_open = false;

	return 0;
}

static const struct file_operations et_pcie_ops_fops = {
	.owner = THIS_MODULE,
#ifndef ENABLE_DRIVER_TESTS
	.poll = esperanto_pcie_ops_poll,
#endif
	.unlocked_ioctl = esperanto_pcie_ops_ioctl,
	.open = esperanto_pcie_ops_open,
	.release = esperanto_pcie_ops_release,
};

static const struct file_operations et_pcie_mgmt_fops = {
	.owner = THIS_MODULE,
	.poll = esperanto_pcie_mgmt_poll,
	.unlocked_ioctl = esperanto_pcie_mgmt_ioctl,
	.open = esperanto_pcie_mgmt_open,
	.release = esperanto_pcie_mgmt_release,
};

static int create_et_pci_dev(struct et_pci_dev **new_dev, struct pci_dev *pdev)
{
	struct et_pci_dev *et_dev;

	et_dev = devm_kzalloc(&pdev->dev, sizeof(*et_dev), GFP_KERNEL);
	*new_dev = et_dev;

	if (!et_dev)
		return -ENOMEM;

	et_dev->pdev = pdev;

	et_dev->dev_index = get_index();
	if (et_dev->dev_index < 0) {
		dev_err(&pdev->dev, "get index failed\n");
		return -ENODEV;
	}

	et_dev->num_irq_vecs = 0;
	et_dev->used_irq_vecs = 0;

	return 0;
}

static void et_unmap_discovered_regions(struct et_pci_dev *et_dev,
					bool is_mgmt)
{
	struct et_mapped_region *regions;
	int num_reg_types, i;

	if (is_mgmt) {
		regions = et_dev->mgmt.regions;
		num_reg_types = sizeof(et_dev->mgmt.regions) /
				sizeof(et_dev->mgmt.regions[0]);
	} else {
		regions = et_dev->ops.regions;
		num_reg_types = sizeof(et_dev->ops.regions) /
				sizeof(et_dev->ops.regions[0]);
	}

	for (i = 0; i < num_reg_types; i++) {
		if (regions[i].is_valid) {
			et_unmap_bar(regions[i].mapped_baseaddr);
			regions[i].is_valid = false;
		}
	}
}

static ssize_t et_map_discovered_regions(struct et_pci_dev *et_dev,
					 bool is_mgmt, u8 *regs_data,
					 size_t regs_size,
					 int num_discovered_regions)
{
	u8 *reg_pos = regs_data;
	size_t section_size;
	struct et_dir_mem_region *dir_mem_region;
	struct et_mapped_region *regions;
	struct et_bar_mapping bm_info;
	int num_reg_types, i;
	ssize_t rv;

	if (is_mgmt) {
		regions = et_dev->mgmt.regions;
		num_reg_types = sizeof(et_dev->mgmt.regions) /
				sizeof(et_dev->mgmt.regions[0]);
	} else {
		regions = et_dev->ops.regions;
		num_reg_types = sizeof(et_dev->ops.regions) /
				sizeof(et_dev->ops.regions[0]);
	}

	memset(regions, 0, num_reg_types * sizeof(*regions));
	for (i = 0; i < num_discovered_regions; i++, reg_pos += section_size) {
		dir_mem_region = (struct et_dir_mem_region *)reg_pos;
		section_size = dir_mem_region->attributes_size;

		// End of region check
		if (reg_pos + section_size > regs_data + regs_size) {
			dev_err(&et_dev->pdev->dev,
				"DIR memory region[%d] size out of range!",
				i);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// Region type check
		if (dir_mem_region->type >= num_reg_types) {
			dev_warn(&et_dev->pdev->dev,
				 "Region type %d is unknown to driver, skipping this region",
				 dir_mem_region->type);
			continue;
		}

		// Attributes size check
		if (section_size > sizeof(*dir_mem_region)) {
			dev_warn(&et_dev->pdev->dev,
				 "Region type: %d has extra attributes, skipping extra attributes",
				 dir_mem_region->type);
		} else if (section_size < sizeof(*dir_mem_region)) {
			dev_err(&et_dev->pdev->dev,
				"Region type: %d does not have enough attributes!",
				dir_mem_region->type);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// Region attributes validity check
		if (!valid_mem_region(dir_mem_region, is_mgmt)) {
			dev_warn(&et_dev->pdev->dev,
				 "Region type: %d has invalid attributes, skipping this region",
				 dir_mem_region->type);
			continue;
		}

		// Region type uniqueness check
		if (regions[dir_mem_region->type].is_valid) {
			// if valid means already mapped
			dev_err(&et_dev->pdev->dev,
				"Region type: %d found again; types must be unique\n",
				dir_mem_region->type);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// BAR mapping for the discovered region
		bm_info.bar             = dir_mem_region->bar;
		bm_info.bar_offset      = dir_mem_region->bar_offset;
		bm_info.size            = dir_mem_region->bar_size;
		rv = et_map_bar(et_dev, &bm_info, &regions
				[dir_mem_region->type].mapped_baseaddr);
		if (rv) {
			dev_err(&et_dev->pdev->dev,
				"Region type: %d mapping failed\n",
				dir_mem_region->type);
			goto error_unmap_discovered_regions;
		}

		// Save other region information
		regions[dir_mem_region->type].size =
			dir_mem_region->bar_size;
		regions[dir_mem_region->type].soc_addr =
			dir_mem_region->dev_address;
		memcpy(&regions[dir_mem_region->type].access,
		       (u8 *)&dir_mem_region->access,
		       sizeof(dir_mem_region->access));

		regions[dir_mem_region->type].is_valid = true;
	}

	// Check if all compulsory region types have been discovered and mapped
	for (i = 0; i < num_reg_types; i++) {
		if (!compulsory_region_type(i, is_mgmt))
			continue;

		if (!regions[i].is_valid) {
			dev_err(&et_dev->pdev->dev,
				"Compulsory region type: %d, not found!\n",
				i);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}
	}

	return (ssize_t)((u64)reg_pos - (u64)regs_data);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, is_mgmt);

	return rv;
}

static int et_mgmt_dev_init(struct et_pci_dev *et_dev)
{
	bool dir_ready = false;
	u8 *dir_data, *dir_pos;
	size_t section_size, dir_size, regs_size;
	struct et_mgmt_dir_header *dir_mgmt;
	struct et_dir_vqueue *dir_vq;
	int rv, i;

	et_dev->mgmt.is_mgmt_open = false;
	spin_lock_init(&et_dev->mgmt.mgmt_open_lock);

	// Map DIR region
	rv = et_map_bar(et_dev, &DIR_MAPPINGS[IOMEM_R_PU_DIR_PC_SP],
			&et_dev->mgmt.dir);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Mgmt: DIR mapping failed\n");
		return rv;
	}

	dir_mgmt = (struct et_mgmt_dir_header *)et_dev->mgmt.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 300 secs
	for (i = 0; !dir_ready && i < 30; i++) {
		rv = (int)ioread16(&dir_mgmt->status);
		if (rv >= MGMT_BOOT_STATUS_DEV_READY) {
			pr_debug("Mgmt: DIRs ready, status: %d", rv);
			dir_ready = true;
		} else {
			pr_debug("Mgmt: DIRs not ready, status: %d, waiting...",
				 rv);
			msleep(10000);
		}
	}

	if (!dir_ready) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIRs not ready; discovery timed out\n");
		rv = -EBUSY;
		goto error_unmap_dir_region;
	}

	// DIR size sanity check
	dir_size = ioread16(&dir_mgmt->total_size);
	if (dir_size > DIR_MAPPINGS[IOMEM_R_PU_DIR_PC_SP].size) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIRs size out of range!");
		rv = -EINVAL;
		goto error_unmap_dir_region;
	}

	// Allocate memory for reading DIRs
	dir_data = kmalloc(dir_size, GFP_KERNEL);
	if (!dir_data) {
		rv = -ENOMEM;
		goto error_unmap_dir_region;
	}

	// Read complete DIRs from device memory
	et_ioread(et_dev->mgmt.dir, 0, dir_data, dir_size);
	dir_pos = dir_data;

	/*
	 * Parse and save DIR general attributes from DIR header
	 */
	dir_mgmt = (struct et_mgmt_dir_header *)dir_pos;
	section_size = dir_mgmt->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR header size out of range!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_mgmt)) {
		dev_warn(&et_dev->pdev->dev,
			 "Mgmt: DIR header has extra attributes, skipping extra attributes");
	} else if (section_size < sizeof(*dir_mgmt)) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR header does not have enough attributes!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Calculate crc32 checksum starting after DIRs header to the end
	if (~crc32(~0, dir_pos + section_size, dir_size - section_size) !=
	    dir_mgmt->crc32) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIRs checksum mismatch!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	et_dev->mgmt.minion_shires = dir_mgmt->minion_shire_mask;

	dir_pos += section_size;

	/*
	 * Save vqueue information from DIRs
	 */
	dir_vq = (struct et_dir_vqueue *)dir_pos;
	section_size = dir_vq->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR vqueue size out of range!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_vq)) {
		dev_warn(&et_dev->pdev->dev,
			 "Mgmt: DIR vqueue has extra attributes, skipping extra attributes");
	} else if (section_size < sizeof(*dir_vq)) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR vqueue does not have enough attributes!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	memcpy(&et_dev->mgmt.vq_common.dir_vq, (u8 *)dir_vq, sizeof(*dir_vq));

	dir_pos += section_size;

	/*
	 * Map all memory regions and save attributes
	 */
	regs_size = dir_size - ((u64)dir_pos - (u64)dir_data);
	rv = et_map_discovered_regions(et_dev, true /* mgmt_dev */, dir_pos,
				       regs_size, dir_mgmt->num_regions);
	if (rv < 0) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR Memory regions mapping failed!");
		goto error_free_dir_data;
	}

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dev_warn(&et_dev->pdev->dev,
			 "Mgmt: DIR total_size != sum of all region sizes!");
	}

	kfree(dir_data);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: VQs initialization failed\n");
		goto error_unmap_discovered_regions;
	}

	// Create Mgmt device node
	et_dev->mgmt.misc_mgmt_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->mgmt.misc_mgmt_dev.fops  = &et_pcie_mgmt_fops;
	et_dev->mgmt.misc_mgmt_dev.name  = devm_kasprintf(&et_dev->pdev->dev,
							GFP_KERNEL,
							"et%d_mgmt",
							et_dev->dev_index);
	rv = misc_register(&et_dev->mgmt.misc_mgmt_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Mgmt: misc register failed\n");
		goto error_vqueue_destroy_all;
	}

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, true /* mgmt_dev */);

error_free_dir_data:
	kfree(dir_data);

error_unmap_dir_region:
	et_unmap_bar(et_dev->mgmt.dir);

	return rv;
}

static void et_mgmt_dev_destroy(struct et_pci_dev *et_dev)
{
	misc_deregister(&et_dev->mgmt.misc_mgmt_dev);
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);
	et_unmap_discovered_regions(et_dev, true /* mgmt_dev */);
	et_unmap_bar(et_dev->mgmt.dir);
}

static int et_ops_dev_init(struct et_pci_dev *et_dev)
{
	bool dir_ready = false;
	u8 *dir_data, *dir_pos;
	size_t section_size, dir_size, regs_size;
	struct et_ops_dir_header *dir_ops;
	struct et_dir_vqueue *dir_vq;
	int rv, i;

	et_dev->ops.is_ops_open = false;
	spin_lock_init(&et_dev->ops.ops_open_lock);

	// Init DMA rbtree
	mutex_init(&et_dev->ops.dma_rbtree_mutex);
	et_dev->ops.dma_rbtree = RB_ROOT;

	// Map DIR region
	rv = et_map_bar(et_dev, &DIR_MAPPINGS[IOMEM_R_PU_DIR_PC_MM],
			&et_dev->ops.dir);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Ops: DIR mapping failed\n");
		return rv;
	}

	dir_ops = (struct et_ops_dir_header *)et_dev->ops.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 300 secs
	for (i = 0; !dir_ready && i < 30; i++) {
		rv = (int)ioread16(&dir_ops->status);
		if (rv >= OPS_BOOT_STATUS_MM_READY) {
			pr_debug("Ops: DIRs ready, status: %d", rv);
			dir_ready = true;
		} else {
			pr_debug("Ops: DIRs not ready, status: %d, waiting...",
				 rv);
			msleep(10000);
		}
	}

	if (!dir_ready) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIRs not ready; discovery timed out\n");
		rv = -EBUSY;
		goto error_unmap_dir_region;
	}

	// DIR size sanity check
	dir_size = ioread16(&dir_ops->total_size);
	if (dir_size > DIR_MAPPINGS[IOMEM_R_PU_DIR_PC_MM].size) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIRs size out of range!");
		rv = -EINVAL;
		goto error_unmap_dir_region;
	}

	// Allocate memory for reading DIRs
	dir_data = kmalloc(dir_size, GFP_KERNEL);
	if (!dir_data) {
		rv = -ENOMEM;
		goto error_unmap_dir_region;
	}

	// Read complete DIRs from device memory
	et_ioread(et_dev->ops.dir, 0, dir_data, dir_size);
	dir_pos = dir_data;

	/*
	 * Parse and save DIR general attributes from DIR header
	 */
	dir_ops = (struct et_ops_dir_header *)dir_pos;
	section_size = dir_ops->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR header size out of range!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_ops)) {
		dev_warn(&et_dev->pdev->dev,
			 "Ops: DIR header has extra attributes, skipping extra attributes");
	} else if (section_size < sizeof(*dir_ops)) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR header does not have enough attributes!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Calculate crc32 checksum starting after DIRs header to the end
	if (~crc32(~0, dir_pos + section_size, dir_size - section_size) !=
	    dir_ops->crc32) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIRs checksum mismatch!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	dir_pos += section_size;

	/*
	 * Save vqueue information from DIRs
	 */
	dir_vq = (struct et_dir_vqueue *)dir_pos;
	section_size = dir_vq->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR vqueue size out of range!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_vq)) {
		dev_warn(&et_dev->pdev->dev,
			 "Ops: DIR vqueue has extra attributes, skipping extra attributes");
	} else if (section_size < sizeof(*dir_vq)) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR vqueue does not have enough attributes!");
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	memcpy(&et_dev->ops.vq_common.dir_vq, (u8 *)dir_vq, sizeof(*dir_vq));

	dir_pos += section_size;

	/*
	 * Map all memory regions and save attributes
	 */
	regs_size = dir_size - ((u64)dir_pos - (u64)dir_data);
	rv = et_map_discovered_regions(et_dev, false /* ops_dev */, dir_pos,
				       regs_size, dir_ops->num_regions);
	if (rv < 0) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR Memory regions mapping failed!");
		goto error_free_dir_data;
	}

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dev_warn(&et_dev->pdev->dev,
			 "Ops: DIR total_size != sum of all region sizes!");
	}

	kfree(dir_data);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, false /* ops_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops device VQs initialization failed\n");
		goto error_unmap_discovered_regions;
	}

	// Create Ops device node
	et_dev->ops.misc_ops_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->ops.misc_ops_dev.fops  = &et_pcie_ops_fops;
	et_dev->ops.misc_ops_dev.name  = devm_kasprintf(&et_dev->pdev->dev,
							GFP_KERNEL, "et%d_ops",
							et_dev->dev_index);
	rv = misc_register(&et_dev->ops.misc_ops_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "misc ops register failed\n");
		goto error_vqueue_destroy_all;
	}

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, false /* ops_dev */);

error_free_dir_data:
	kfree(dir_data);

error_unmap_dir_region:
	et_unmap_bar(et_dev->ops.dir);

	return rv;
}

static void et_ops_dev_destroy(struct et_pci_dev *et_dev)
{
	misc_deregister(&et_dev->ops.misc_ops_dev);
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);
	et_unmap_discovered_regions(et_dev, false /* ops_dev */);
	et_unmap_bar(et_dev->ops.dir);

	mutex_lock(&et_dev->ops.dma_rbtree_mutex);
	et_dma_delete_all_info(&et_dev->ops.dma_rbtree);
	mutex_unlock(&et_dev->ops.dma_rbtree_mutex);

	mutex_destroy(&et_dev->ops.dma_rbtree_mutex);
}

static void destroy_et_pci_dev(struct et_pci_dev *et_dev)
{
	u8 dev_index;

	if (!et_dev)
		return;

	dev_index = et_dev->dev_index;
	clear_bit(dev_index, dev_bitmap);
}

static int esperanto_pcie_probe(struct pci_dev *pdev,
				const struct pci_device_id *pci_id)
{
	int rv;
	struct et_pci_dev *et_dev;

	// Create instance data for this device, save it to drvdata
	rv = create_et_pci_dev(&et_dev, pdev);
	pci_set_drvdata(pdev, et_dev); // Set even if NULL
	if (rv < 0) {
		dev_err(&pdev->dev, "create_et_pci_dev failed\n");
		return rv;
	}

	rv = pci_enable_device_mem(pdev);
	if (rv < 0) {
		dev_err(&pdev->dev, "enable device failed\n");
		goto error_free_dev;
	}

	rv = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (rv) {
		dev_err(&pdev->dev, "set dma mask failed\n");
		goto error_disable_dev;
	}

	pci_set_master(pdev);

	// TODO: Split MSI for SQ and CQ, currently following MSIs are
	// supported
        // Device Management:
        //  - Vector[0] - Mgmt VQ (both SQ & CQ)
        // Device Operations:
        //  - Vector[1] - Ops VQ (both SQ & CQ)
	et_dev->num_irq_vecs = 2;
	rv = pci_alloc_irq_vectors(pdev, et_dev->num_irq_vecs,
				   et_dev->num_irq_vecs, PCI_IRQ_MSI);
	if (rv != et_dev->num_irq_vecs) {
		dev_err(&pdev->dev, "msi vectors %d alloc failed\n",
			et_dev->num_irq_vecs);
		goto error_clear_master;
	}

	rv = pci_request_regions(pdev, DRIVER_NAME);
	if (rv) {
		dev_err(&pdev->dev, "request regions failed\n");
		goto error_free_irq_vectors;
	}

	rv = et_mgmt_dev_init(et_dev);
	if (rv) {
		dev_err(&pdev->dev,
			"Mgmt device initialization failed\n");
		goto error_pci_release_regions;
	}

	rv = et_ops_dev_init(et_dev);
	if (rv) {
		dev_err(&pdev->dev, "Ops device initialization failed\n");
		goto error_mgmt_dev_destroy;
	}

	return 0;

error_mgmt_dev_destroy:
	et_mgmt_dev_destroy(et_dev);

error_pci_release_regions:
	pci_release_regions(pdev);

error_free_irq_vectors:
	pci_free_irq_vectors(pdev);

error_clear_master:
	pci_clear_master(pdev);

error_disable_dev:
	pci_disable_device(pdev);

error_free_dev:
	destroy_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);

	return rv;
}

static void esperanto_pcie_remove(struct pci_dev *pdev)
{
	struct et_pci_dev *et_dev;

	et_dev = pci_get_drvdata(pdev);
	if (!et_dev)
		return;

	et_ops_dev_destroy(et_dev);
	et_mgmt_dev_destroy(et_dev);

	pci_release_regions(pdev);
	pci_free_irq_vectors(pdev);
	pci_clear_master(pdev);
	pci_disable_device(pdev);

	destroy_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);
}

static struct pci_driver et_pcie_driver = {
	.name = DRIVER_NAME,
	.id_table = esperanto_pcie_tbl,
	.probe = esperanto_pcie_probe,
	.remove = esperanto_pcie_remove,
};

module_pci_driver(et_pcie_driver);
