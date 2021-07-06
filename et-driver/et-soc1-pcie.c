/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <uapi/linux/pci_regs.h>

#include "et_dma.h"
#include "et_event_handler.h"
#include "et_fw_update.h"
#include "et_io.h"
#include "et_ioctl.h"
#include "et_pci_dev.h"
#include "et_vqueue.h"

#ifdef ENABLE_DRIVER_TESTS
#include "et_test.h"
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME	       "Esperanto"
#define ET_PCIE_VENDOR_ID      0x1e0a
#define ET_PCIE_TEST_DEVICE_ID 0x9038
#define ET_PCIE_SOC1_ID	       0xeb01
#define ET_MAX_DEVS	       64
#define DMA_MAX_ALLOC_SIZE     (BIT(31)) /* 2GB */

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

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
		return mask;

	mutex_lock(&ops->vq_common.sq_bitmap_mutex);
	mutex_lock(&ops->vq_common.cq_bitmap_mutex);

	// Update sq_bitmap for all SQs, set corresponding bit when space
	// available is more than threshold
	for (i = 0; i < ops->vq_common.dir_vq.sq_count; i++)
		et_squeue_event_available(ops->sq_pptr[i]);

	mutex_unlock(&ops->vq_common.sq_bitmap_mutex);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(ops->vq_common.sq_bitmap,
			  ops->vq_common.dir_vq.sq_count))
		mask |= EPOLLOUT;

	// Generate EPOLLIN event if any CQ msg is saved for userspace
	if (!bitmap_empty(ops->vq_common.cq_bitmap,
			  ops->vq_common.dir_vq.cq_count))
		mask |= EPOLLIN;

	mutex_unlock(&ops->vq_common.cq_bitmap_mutex);

	return mask;
}
#endif

static long
esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct et_pci_dev *et_dev;
	struct et_ops_dev *ops;
	struct dram_info user_dram;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	size_t size;
	u16 max_size;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);
	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_USER_DRAM_INFO:
		if (!ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].is_valid)
			return -EINVAL;

		user_dram.base =
			ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].soc_addr;
		user_dram.size =
			ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].size;
		user_dram.dma_max_alloc_size = DMA_MAX_ALLOC_SIZE;
		// TODO: Discover from DIRs
		user_dram.dma_max_elem_size = BIT(27); /* 128MB */

		switch (ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				.access.dma_align) {
		case MEM_REGION_DMA_ALIGNMENT_NONE:
			user_dram.align_in_bits = 0;
			break;
		case MEM_REGION_DMA_ALIGNMENT_8BIT:
			user_dram.align_in_bits = 8;
			break;
		case MEM_REGION_DMA_ALIGNMENT_32BIT:
			user_dram.align_in_bits = 32;
			break;
		case MEM_REGION_DMA_ALIGNMENT_64BIT:
			user_dram.align_in_bits = 64;
			break;
		default:
			user_dram.align_in_bits = 0;
		}

		if (size >= sizeof(user_dram) &&
		    copy_to_user(usr_arg, &user_dram, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_INFO: failed to copy to user\n");
			return -ENOMEM;
		}

		return 0;

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (size >= sizeof(u16) &&
		    copy_to_user(usr_arg,
				 &ops->vq_common.dir_vq.sq_count,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = ops->vq_common.dir_vq.per_sq_size -
			   sizeof(struct et_circbuffer);
		if (size >= sizeof(u16) &&
		    copy_to_user(usr_arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (cmd_info.sq_index >=
			    et_dev->ops.vq_common.dir_vq.sq_count ||
		    !cmd_info.cmd || !cmd_info.size)
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_DMA) {
			return et_dma_move_data(
				et_dev,
				cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);
		} else {
			return et_squeue_copy_from_user(
				et_dev,
				false /* ops_dev */,
				cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);
		}

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (rsp_info.cq_index >=
			    et_dev->ops.vq_common.dir_vq.cq_count ||
		    !rsp_info.rsp || !rsp_info.size)
			return -EINVAL;

		return et_cqueue_copy_to_user(
			et_dev,
			false /* ops_dev */,
			rsp_info.cq_index,
			(char __user __force *)rsp_info.rsp,
			rsp_info.size);

	case ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP:
		if (size >= sizeof(u64) &&
		    copy_to_user(usr_arg, ops->vq_common.sq_bitmap, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (size >= sizeof(u64) &&
		    copy_to_user(usr_arg, ops->vq_common.cq_bitmap, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_SQ_THRESHOLD:
		if (copy_from_user(&sq_threshold_info, usr_arg, _IOC_SIZE(cmd)))
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
		mutex_lock(&ops->vq_common.sq_bitmap_mutex);
		clear_bit(sq_idx, ops->vq_common.sq_bitmap);
		mutex_unlock(&ops->vq_common.sq_bitmap_mutex);
		wake_up_interruptible(
			&ops->sq_pptr[sq_idx]->vq_common->waitqueue);

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

	mgmt = container_of(fp->private_data,
			    struct et_mgmt_dev,
			    misc_mgmt_dev);

	// TODO: Separate ISRs for SQ and CQ. waitqueue is wake up whenever an
	// interrupt is received. Currently same interrupt is being used for SQ
	// and CQ. Move `et_squeue_event_available()` to SQ ISR. waitqueue
	// should be wake up from different ISRs after the bitmaps are updated.
	poll_wait(fp, &mgmt->vq_common.waitqueue, wait);

	if (mgmt->vq_common.aborting)
		return mask;

	mutex_lock(&mgmt->vq_common.sq_bitmap_mutex);
	mutex_lock(&mgmt->vq_common.cq_bitmap_mutex);

	// Update sq_bitmap for all SQs, set corresponding bit when space
	// available is more than threshold
	for (i = 0; i < mgmt->vq_common.dir_vq.sq_count; i++)
		et_squeue_event_available(mgmt->sq_pptr[i]);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(mgmt->vq_common.sq_bitmap,
			  mgmt->vq_common.dir_vq.sq_count))
		mask |= EPOLLOUT;

	mutex_unlock(&mgmt->vq_common.sq_bitmap_mutex);

	// Generate EPOLLIN event if any CQ msg list has message for userspace
	if (!bitmap_empty(mgmt->vq_common.cq_bitmap,
			  mgmt->vq_common.dir_vq.cq_count))
		mask |= EPOLLIN;

	mutex_unlock(&mgmt->vq_common.cq_bitmap_mutex);

	return mask;
}

static void esperanto_pcie_vm_open(struct vm_area_struct *vma)
{
	struct et_dma_mapping *map = vma->vm_private_data;

	dev_dbg(&map->pdev->dev,
		"vm_open: %p, [size=%lu,vma=%08lx-%08lx]\n",
		map,
		map->size,
		vma->vm_start,
		vma->vm_end);

	map->ref_count++;
}

static void esperanto_pcie_vm_close(struct vm_area_struct *vma)
{
	struct et_dma_mapping *map = vma->vm_private_data;

	if (!map)
		return;

	dev_dbg(&map->pdev->dev,
		"vm_close: %p, [size=%lu,vma=%08lx-%08lx]\n",
		map,
		map->size,
		vma->vm_start,
		vma->vm_end);

	map->ref_count--;
	if (map->ref_count == 0) {
		dma_free_coherent(&map->pdev->dev,
				  map->size,
				  map->kern_vaddr,
				  map->dma_addr);

		kfree(map);
	}
}

static const struct vm_operations_struct esperanto_pcie_vm_ops = {
	.open = esperanto_pcie_vm_open,
	.close = esperanto_pcie_vm_close,
};

static int esperanto_pcie_ops_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct et_ops_dev *ops;
	struct et_pci_dev *et_dev;
	struct et_dma_mapping *map;
	dma_addr_t dma_addr;
	void *kern_vaddr;
	size_t size = vma->vm_end - vma->vm_start;
	int rv;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);

	if (vma->vm_pgoff != 0) {
		dev_err(&et_dev->pdev->dev, "mmap() offset must be 0.\n");
		return -EINVAL;
	}

	if (size > DMA_MAX_ALLOC_SIZE) {
		dev_err(&et_dev->pdev->dev,
			"mmap() can't map more than 2GB at a time!");
		return -EINVAL;
	}

	vma->vm_flags |= VM_DONTCOPY | VM_NORESERVE;
	vma->vm_ops = &esperanto_pcie_vm_ops;

	kern_vaddr = dma_alloc_coherent(&et_dev->pdev->dev,
					size,
					&dma_addr,
					GFP_USER);
	if (!kern_vaddr) {
		dev_err(&et_dev->pdev->dev, "dma_alloc_coherent() failed!\n");
		return -ENOMEM;
	}

	rv = remap_pfn_range(vma,
			     vma->vm_start,
			     __pa((unsigned long)kern_vaddr) >> PAGE_SHIFT,
			     vma_pages(vma) << PAGE_SHIFT,
			     vma->vm_page_prot);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "remap_pfn_range() failed!");
		goto error_dma_free_coherent;
	}

	map = kzalloc(sizeof(struct et_dma_mapping), GFP_KERNEL);
	if (!map) {
		rv = -ENOMEM;
		goto error_dma_free_coherent;
	}

	map->usr_vaddr = (void *)vma->vm_start;
	map->pdev = et_dev->pdev;
	map->kern_vaddr = kern_vaddr;
	map->dma_addr = dma_addr;
	map->size = size;
	map->ref_count = 0;
	vma->vm_private_data = map;

	esperanto_pcie_vm_open(vma);

	return 0;

error_dma_free_coherent:
	dma_free_coherent(&et_dev->pdev->dev, size, kern_vaddr, dma_addr);

	return rv;
}

static long
esperanto_pcie_mgmt_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct et_mgmt_dev *mgmt;
	struct et_pci_dev *et_dev;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	struct fw_update_desc fw_update_info;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	size_t size;
	u16 max_size;

	mgmt = container_of(fp->private_data,
			    struct et_mgmt_dev,
			    misc_mgmt_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_FW_UPDATE:
		if (copy_from_user(&fw_update_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (!fw_update_info.ubuf || !fw_update_info.size)
			return -EINVAL;

		return et_mmio_write_fw_image(
			et_dev,
			(char __user __force *)fw_update_info.ubuf,
			fw_update_info.size);

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (size >= sizeof(u16) &&
		    copy_to_user(usr_arg,
				 &mgmt->vq_common.dir_vq.sq_count,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = mgmt->vq_common.dir_vq.per_sq_size -
			   sizeof(struct et_circbuffer);
		if (size >= sizeof(u16) &&
		    copy_to_user(usr_arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_ACTIVE_SHIRE:
		if (size >= sizeof(u64) &&
		    copy_to_user(usr_arg, &mgmt->minion_shires, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_ACTIVE_SHIRE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (cmd_info.sq_index >=
			    et_dev->mgmt.vq_common.dir_vq.sq_count ||
		    !cmd_info.cmd || !cmd_info.size)
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_DMA) {
			return -EINVAL;
		} else {
			return et_squeue_copy_from_user(
				et_dev,
				true /* mgmt_dev */,
				cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);
		}

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (rsp_info.cq_index >=
			    et_dev->mgmt.vq_common.dir_vq.cq_count ||
		    !rsp_info.rsp || !rsp_info.size)
			return -EINVAL;

		return et_cqueue_copy_to_user(
			et_dev,
			true /* mgmt_dev */,
			rsp_info.cq_index,
			(char __user __force *)rsp_info.rsp,
			rsp_info.size);

	case ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP:
		if (size >= sizeof(u64) &&
		    copy_to_user(usr_arg, mgmt->vq_common.sq_bitmap, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (size >= sizeof(u64) &&
		    copy_to_user(usr_arg, mgmt->vq_common.cq_bitmap, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_SQ_THRESHOLD:
		if (copy_from_user(&sq_threshold_info, usr_arg, _IOC_SIZE(cmd)))
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
		mutex_lock(&mgmt->vq_common.sq_bitmap_mutex);
		clear_bit(sq_idx, mgmt->vq_common.sq_bitmap);
		mutex_unlock(&mgmt->vq_common.sq_bitmap_mutex);
		wake_up_interruptible(
			&mgmt->sq_pptr[sq_idx]->vq_common->waitqueue);

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

	mgmt = container_of(fp->private_data,
			    struct et_mgmt_dev,
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

	mgmt = container_of(fp->private_data,
			    struct et_mgmt_dev,
			    misc_mgmt_dev);
	mgmt->is_mgmt_open = false;

	return 0;
}

// clang-format off
static const struct file_operations et_pcie_ops_fops = {
	.owner			= THIS_MODULE,
#ifndef ENABLE_DRIVER_TESTS
	.poll			= esperanto_pcie_ops_poll,
#endif
	.unlocked_ioctl		= esperanto_pcie_ops_ioctl,
	.mmap			= esperanto_pcie_ops_mmap,
	.open			= esperanto_pcie_ops_open,
	.release		= esperanto_pcie_ops_release,
};

static const struct file_operations et_pcie_mgmt_fops = {
	.owner			= THIS_MODULE,
	.poll			= esperanto_pcie_mgmt_poll,
	.unlocked_ioctl		= esperanto_pcie_mgmt_ioctl,
	.open			= esperanto_pcie_mgmt_open,
	.release		= esperanto_pcie_mgmt_release,
};

// clang-format on

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

static void et_unmap_discovered_regions(struct et_pci_dev *et_dev, bool is_mgmt)
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
					 bool is_mgmt,
					 u8 *regs_data,
					 size_t regs_size,
					 int num_discovered_regions)
{
	u8 *reg_pos = regs_data;
	size_t section_size;
	struct et_dir_mem_region *dir_mem_region;
	struct et_mapped_region *regions;
	struct et_bar_mapping bm_info;
	int num_reg_types, compul_reg_count, non_compul_reg_count, i;
	ssize_t rv;
	struct event_dbg_msg dbg_msg;
	char syndrome_str[ET_EVENT_SYNDROME_LEN];

	if (is_mgmt) {
		regions = et_dev->mgmt.regions;
		num_reg_types = sizeof(et_dev->mgmt.regions) /
				sizeof(et_dev->mgmt.regions[0]);
	} else {
		regions = et_dev->ops.regions;
		num_reg_types = sizeof(et_dev->ops.regions) /
				sizeof(et_dev->ops.regions[0]);
	}

	syndrome_str[0] = '\0';
	dbg_msg.syndrome = syndrome_str;
	dbg_msg.count = 1;

	memset(regions, 0, num_reg_types * sizeof(*regions));
	for (i = 0; i < num_discovered_regions; i++, reg_pos += section_size) {
		dir_mem_region = (struct et_dir_mem_region *)reg_pos;
		section_size = dir_mem_region->attributes_size;

		// End of region check
		if (reg_pos + section_size > regs_data + regs_size) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIR region exceeded DIR total size!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\nRegion end - DIR end: %zd)\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type,
				reg_pos + section_size -
					(regs_data + regs_size));
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// Region type check
		if (dir_mem_region->type >= num_reg_types) {
			dbg_msg.level = LEVEL_WARN;
			dbg_msg.desc = "Skipping unknown DIR region type!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type);
			et_print_event(et_dev->pdev, &dbg_msg);
			continue;
		}

		// Attributes size check
		if (section_size > sizeof(*dir_mem_region)) {
			dbg_msg.level = LEVEL_WARN;
			dbg_msg.desc =
				"DIR region has extra attributes, skipping extra attributes";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\nSize: (expected: %zu < discovered: %zu)\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type,
				sizeof(*dir_mem_region),
				section_size);
			et_print_event(et_dev->pdev, &dbg_msg);
		} else if (section_size < sizeof(*dir_mem_region)) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc =
				"DIR region does not have enough attributes!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\nSize: (expected: %zu > discovered: %zu)\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type,
				sizeof(*dir_mem_region),
				section_size);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// Region attributes validity check
		if (!valid_mem_region(dir_mem_region,
				      is_mgmt,
				      dbg_msg.syndrome,
				      ET_EVENT_SYNDROME_LEN)) {
			if (strcmp(dbg_msg.syndrome, "") == 0) {
				dev_err(&et_dev->pdev->dev,
					"%s: Failed to check DIR region validity!",
					(is_mgmt) ? "Mgmt" : "Ops");
				rv = -EINVAL;
				goto error_unmap_discovered_regions;
			} else {
				dbg_msg.level = LEVEL_FATAL;
				dbg_msg.desc =
					"DIRs compulsory field(s) not set!";
				et_print_event(et_dev->pdev, &dbg_msg);
			}
			continue;
		}

		// Region type uniqueness check
		if (regions[dir_mem_region->type].is_valid) {
			// is_valid means already mapped
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIRs duplicate region type found!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// BAR mapping for the discovered region
		bm_info.bar = dir_mem_region->bar;
		bm_info.bar_offset = dir_mem_region->bar_offset;
		bm_info.size = dir_mem_region->bar_size;
		rv = et_map_bar(et_dev,
				&bm_info,
				&regions[dir_mem_region->type].mapped_baseaddr);
		if (rv) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIR discovered region mapping failed!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type);
			et_print_event(et_dev->pdev, &dbg_msg);
			goto error_unmap_discovered_regions;
		}

		// Save other region information
		regions[dir_mem_region->type].size = dir_mem_region->bar_size;
		regions[dir_mem_region->type].soc_addr =
			dir_mem_region->dev_address;
		memcpy(&regions[dir_mem_region->type].access,
		       (u8 *)&dir_mem_region->access,
		       sizeof(dir_mem_region->access));

		regions[dir_mem_region->type].is_valid = true;
	}

	// Check if all compulsory region types have been discovered and mapped
	compul_reg_count = 0;
	non_compul_reg_count = 0;
	for (i = 0; i < num_reg_types; i++) {
		if (!compulsory_region_type(i, is_mgmt)) {
			non_compul_reg_count += 1;
			continue;
		}

		compul_reg_count += 1;

		if (!regions[i].is_valid) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIRs missing compulsory region type!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				i);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}
	}

	if (compul_reg_count + non_compul_reg_count != num_reg_types) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIRs expected number of memory regions doesn't match discovered number of memory regions",
		sprintf(dbg_msg.syndrome,
			"\nDevice: %s\nExpected regions: %d\nDiscovered compulsory regions: %d\nDiscovered non-compulsory regions: %d\n",
			(is_mgmt) ? "Mgmt" : "Ops",
			num_reg_types,
			compul_reg_count,
			non_compul_reg_count);
		et_print_event(et_dev->pdev, &dbg_msg);
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
	struct et_mgmt_dir_header __iomem *dir_mgmt_mem;
	struct et_mgmt_dir_header *dir_mgmt;
	struct et_dir_vqueue *dir_vq;
	int rv, i;
	u32 crc32_result;
	struct event_dbg_msg dbg_msg;
	char syndrome_str[ET_EVENT_SYNDROME_LEN];

	et_dev->mgmt.is_mgmt_open = false;
	spin_lock_init(&et_dev->mgmt.mgmt_open_lock);

	// Map DIR region
	rv = et_map_bar(et_dev,
			&DIR_MAPPINGS[IOMEM_R_DIR_MGMT],
			&et_dev->mgmt.dir);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Mgmt: DIR mapping failed\n");
		return rv;
	}

	dir_mgmt_mem = (struct et_mgmt_dir_header __iomem *)et_dev->mgmt.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 300 secs
	for (i = 0; !dir_ready && i < 30; i++) {
		rv = (int)ioread16(&dir_mgmt_mem->status);
		if (rv == MGMT_BOOT_STATUS_DEV_READY) {
			pr_debug("Mgmt: DIRs ready, status: %d", rv);
			dir_ready = true;
		} else {
			pr_debug("Mgmt: DIRs not ready, status: %d, waiting...",
				 rv);
			msleep(10000);
		}
	}

	syndrome_str[0] = '\0';
	dbg_msg.syndrome = syndrome_str;
	dbg_msg.count = 1;

	if (!dir_ready) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIRs discovery timed out!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nBoot status: %d\n",
			rv);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EBUSY;
		goto error_unmap_dir_region;
	}

	// DIR size sanity check
	dir_size = ioread16(&dir_mgmt_mem->total_size);
	if (dir_size > DIR_MAPPINGS[IOMEM_R_DIR_MGMT].size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "Invalid DIRs total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nSize: (Max allowed: %llu, discovered: %zu)\n",
			DIR_MAPPINGS[IOMEM_R_DIR_MGMT].size,
			dir_size);
		et_print_event(et_dev->pdev, &dbg_msg);
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
	et_ioread(dir_mgmt_mem, 0, dir_data, dir_size);
	dir_pos = dir_data;

	/*
	 * Parse and save DIR general attributes from DIR header
	 */
	dir_mgmt = (struct et_mgmt_dir_header *)dir_pos;
	section_size = dir_mgmt->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region exceeded DIR total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: DIR header\nRegion end - DIR end: %zd)\n",
			dir_pos + section_size - (dir_data + dir_size));
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_mgmt)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIR region has extra attributes, skipping extra attributes";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: DIR header\nSize: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_mgmt),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_mgmt)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: DIR header\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_mgmt),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Calculate crc32 checksum starting after DIRs header to the end
	crc32_result =
		~crc32(~0, dir_pos + section_size, dir_size - section_size);
	if (crc32_result != dir_mgmt->crc32) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIRs CRC32 check mismatch!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nCRC: (expected: %x, calculated: %x)\n",
			dir_mgmt->crc32,
			crc32_result);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	et_dev->mgmt.minion_shires = dir_mgmt->minion_shire_mask;

	dir_pos += section_size;

	/*
	 * Save vqueue information from DIRs
	 */
	dir_vq = (struct et_dir_vqueue *)dir_pos;
	if (!valid_vq_region(dir_vq,
			     true,
			     dbg_msg.syndrome,
			     ET_EVENT_SYNDROME_LEN)) {
		if (strcmp(dbg_msg.syndrome, "") == 0) {
			dev_err(&et_dev->pdev->dev,
				"Mgmt: Failed to check DIR VQ region validity!");
		} else {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIRs compulsory field(s) not set!";
			et_print_event(et_dev->pdev, &dbg_msg);
		}
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	section_size = dir_vq->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region exceeded DIR total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: VQ Region\nRegion end - DIR end: %zd)\n",
			dir_pos + section_size - (dir_data + dir_size));
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_vq)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIR region has extra attributes, skipping extra attributes";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: VQ Region\nSize: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_vq),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_vq)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: VQ Region\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_vq),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	memcpy(&et_dev->mgmt.vq_common.dir_vq, (u8 *)dir_vq, sizeof(*dir_vq));

	dir_pos += section_size;

	/*
	 * Map all memory regions and save attributes
	 */
	regs_size = dir_size - ((u64)dir_pos - (u64)dir_data);
	rv = et_map_discovered_regions(et_dev,
				       true /* mgmt_dev */,
				       dir_pos,
				       regs_size,
				       dir_mgmt->num_regions);
	if (rv < 0) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR Memory regions mapping failed!");
		goto error_free_dir_data;
	}

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"Total DIR size does not match sum of respective Sizes of attribute regions";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nSize: (DIRs: %zu, All Attribute Regions: %d)\n",
			dir_size,
			(int)(dir_pos - dir_data));
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	kfree(dir_data);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: et_vqueue_init_all() failed!\n");
		goto error_unmap_discovered_regions;
	}

	// Create Mgmt device node
	et_dev->mgmt.misc_mgmt_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->mgmt.misc_mgmt_dev.fops = &et_pcie_mgmt_fops;
	et_dev->mgmt.misc_mgmt_dev.name = devm_kasprintf(&et_dev->pdev->dev,
							 GFP_KERNEL,
							 "et%d_mgmt",
							 et_dev->dev_index);
	rv = misc_register(&et_dev->mgmt.misc_mgmt_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Mgmt: misc_register() failed!\n");
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
	struct et_ops_dir_header __iomem *dir_ops_mem;
	struct et_ops_dir_header *dir_ops;
	struct et_dir_vqueue *dir_vq;
	int rv, i;
	u32 crc32_result;
	struct event_dbg_msg dbg_msg;
	char syndrome_str[ET_EVENT_SYNDROME_LEN];

	et_dev->ops.is_ops_open = false;
	spin_lock_init(&et_dev->ops.ops_open_lock);

	// Init DMA rbtree
	mutex_init(&et_dev->ops.dma_rbtree_mutex);
	et_dev->ops.dma_rbtree = RB_ROOT;

	// Map DIR region
	rv = et_map_bar(et_dev,
			&DIR_MAPPINGS[IOMEM_R_DIR_OPS],
			&et_dev->ops.dir);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Ops: DIR mapping failed\n");
		return rv;
	}

	dir_ops_mem = (struct et_ops_dir_header __iomem *)et_dev->ops.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 300 secs
	for (i = 0; !dir_ready && i < 30; i++) {
		rv = (int)ioread16(&dir_ops_mem->status);
		if (rv == OPS_BOOT_STATUS_MM_READY) {
			pr_debug("Ops: DIRs ready, status: %d", rv);
			dir_ready = true;
		} else {
			pr_debug("Ops: DIRs not ready, status: %d, waiting...",
				 rv);
			msleep(10000);
		}
	}

	syndrome_str[0] = '\0';
	dbg_msg.syndrome = syndrome_str;
	dbg_msg.count = 1;

	if (!dir_ready) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIRs discovery timed out!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nBoot status: %d\n",
			rv);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EBUSY;
		goto error_unmap_dir_region;
	}

	// DIR size sanity check
	dir_size = ioread16(&dir_ops_mem->total_size);
	if (!dir_size && dir_size > DIR_MAPPINGS[IOMEM_R_DIR_OPS].size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "Invalid DIRs total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nSize: (Max allowed: %llu, discovered: %zu)\n",
			DIR_MAPPINGS[IOMEM_R_DIR_OPS].size,
			dir_size);
		et_print_event(et_dev->pdev, &dbg_msg);
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
	et_ioread(dir_ops_mem, 0, dir_data, dir_size);
	dir_pos = dir_data;

	/*
	 * Parse and save DIR general attributes from DIR header
	 */
	dir_ops = (struct et_ops_dir_header *)dir_pos;
	section_size = dir_ops->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region exceeded DIR total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: DIR header\nRegion end - DIR end: %zd)\n",
			dir_pos + section_size - (dir_data + dir_size));
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_ops)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIR region has extra attributes, skipping extra attributes";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: DIR header\nSize: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_ops),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_ops)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: DIR header\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_ops),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Calculate crc32 checksum starting after DIRs header to the end
	crc32_result =
		~crc32(~0, dir_pos + section_size, dir_size - section_size);
	if (crc32_result != dir_ops->crc32) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIRs CRC32 check mismatch!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nCRC: (expected: %x, calculated: %x)\n",
			dir_ops->crc32,
			crc32_result);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	dir_pos += section_size;

	/*
	 * Save vqueue information from DIRs
	 */
	dir_vq = (struct et_dir_vqueue *)dir_pos;
	if (!valid_vq_region(dir_vq,
			     false,
			     dbg_msg.syndrome,
			     ET_EVENT_SYNDROME_LEN)) {
		if (strcmp(dbg_msg.syndrome, "") == 0) {
			dev_err(&et_dev->pdev->dev,
				"Ops: Failed to check DIR VQ region validity!");
		} else {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIRs compulsory field(s) not set!";
			et_print_event(et_dev->pdev, &dbg_msg);
		}
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	section_size = dir_vq->attributes_size;

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region exceeded DIR total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: VQ Region\nRegion end - DIR end: %zd)\n",
			dir_pos + section_size - (dir_data + dir_size));
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Attributes size check
	if (section_size > sizeof(*dir_vq)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIR region has extra attributes, skipping extra attributes";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: VQ Region\nSize: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_vq),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_vq)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: VQ Region\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_vq),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	memcpy(&et_dev->ops.vq_common.dir_vq, (u8 *)dir_vq, sizeof(*dir_vq));

	dir_pos += section_size;

	/*
	 * Map all memory regions and save attributes
	 */
	regs_size = dir_size - ((u64)dir_pos - (u64)dir_data);
	rv = et_map_discovered_regions(et_dev,
				       false /* ops_dev */,
				       dir_pos,
				       regs_size,
				       dir_ops->num_regions);
	if (rv < 0) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR Memory Regions mapping failed!");
		goto error_free_dir_data;
	}

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"Total DIR size does not match sum of respective sizes of attribute regions";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nSize: (DIRs: %zu, All Attribute Regions: %d)\n",
			dir_size,
			(int)(dir_pos - dir_data));
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	kfree(dir_data);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, false /* ops_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops: et_vqueue_init_all() failed!\n");
		goto error_unmap_discovered_regions;
	}

	// Create Ops device node
	et_dev->ops.misc_ops_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->ops.misc_ops_dev.fops = &et_pcie_ops_fops;
	et_dev->ops.misc_ops_dev.name = devm_kasprintf(&et_dev->pdev->dev,
						       GFP_KERNEL,
						       "et%d_ops",
						       et_dev->dev_index);
	rv = misc_register(&et_dev->ops.misc_ops_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Ops: misc_register() failed!\n");
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
	rv = pci_alloc_irq_vectors(pdev,
				   et_dev->num_irq_vecs,
				   et_dev->num_irq_vecs,
				   PCI_IRQ_MSI);
	if (rv != et_dev->num_irq_vecs) {
		dev_err(&pdev->dev,
			"msi vectors %d alloc failed\n",
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
		dev_err(&pdev->dev, "Mgmt device initialization failed\n");
		goto error_pci_release_regions;
	}

	rv = et_ops_dev_init(et_dev);
	if (rv) {
		dev_warn(&pdev->dev,
			 "Ops device initialization failed, errno: %d\n",
			 -rv);

		// Falling to recovery mode
		et_dev->is_recovery_mode = true;
		rv = 0;
	} else {
		et_dev->is_recovery_mode = false;
	}

	return rv;

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

	if (!et_dev->is_recovery_mode)
		et_ops_dev_destroy(et_dev);

	et_mgmt_dev_destroy(et_dev);

	pci_release_regions(pdev);
	pci_free_irq_vectors(pdev);
	pci_clear_master(pdev);
	pci_disable_device(pdev);

	destroy_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);
}

// clang-format off
static struct pci_driver et_pcie_driver = {
	.name		= DRIVER_NAME,
	.id_table	= esperanto_pcie_tbl,
	.probe		= esperanto_pcie_probe,
	.remove		= esperanto_pcie_remove,
};

// clang-format on

module_pci_driver(et_pcie_driver);
