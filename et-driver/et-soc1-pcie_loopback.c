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
#include "et_vma.h"
#include "et_vqueue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe loopback device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME		  "Esperanto"
#define PCI_VENDOR_ID_REDHAT	  0x1b36
#define PCI_DEVICE_ID_REDHAT_TEST 0x0005
/* Define Vendor ID and Device ID to utilize QEMU PCI test device */
#define ET_PCIE_VENDOR_ID      PCI_VENDOR_ID_REDHAT
#define ET_PCIE_TEST_DEVICE_ID PCI_DEVICE_ID_REDHAT_TEST
#define ET_PCIE_SOC1_ID	       0xeb01
#define ET_MAX_DEVS	       64

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

/*
 * DIR discovery timeout in seconds for mgmt/ops nodes, if set to 0, checks
 * DIR status only once and returns immediately if not ready.
 *
 * TODO: Remove these params in production after bringup
 */
static uint mgmt_discovery_timeout = 0;
module_param(mgmt_discovery_timeout, uint, 0);

static uint ops_discovery_timeout = 0;
module_param(ops_discovery_timeout, uint, 0);

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

static __poll_t esperanto_pcie_ops_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = 0;
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);

	poll_wait(fp, &ops->vq_common.waitqueue, wait);

	if (ops->vq_common.aborting)
		return mask;

	mutex_lock(&ops->vq_common.sq_bitmap_mutex);
	mutex_lock(&ops->vq_common.cq_bitmap_mutex);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(ops->vq_common.sq_bitmap, ops->dir_vq.sq_count))
		mask |= EPOLLOUT;

	mutex_unlock(&ops->vq_common.sq_bitmap_mutex);

	// Generate EPOLLIN event if any CQ msg is saved for userspace
	if (!bitmap_empty(ops->vq_common.cq_bitmap, ops->dir_vq.cq_count))
		mask |= EPOLLIN;

	mutex_unlock(&ops->vq_common.cq_bitmap_mutex);

	return mask;
}

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
	u32 dev_state;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);
	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_DEVICE_STATE:
		// TODO: SW-8811: Implement device state with
		// heart-beat mechanism. A corner case is possible
		// here that single command sent that causes hang will
		// not be detected because the command will be popped
		// out of SQ by device and SQ will appear empty here.
		dev_state = DEV_STATE_READY;

		// Check if any SQ has pending command(s)
		mutex_lock(&ops->vq_common.sq_bitmap_mutex);
		for (sq_idx = 0; sq_idx < ops->dir_vq.sq_count; sq_idx++) {
			if (!et_squeue_empty(ops->sq_pptr[sq_idx])) {
				dev_state = DEV_STATE_PENDING_COMMANDS;
				break;
			}
		}
		mutex_unlock(&ops->vq_common.sq_bitmap_mutex);

		if (size >= sizeof(u32) &&
		    copy_to_user(usr_arg, &dev_state, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DEVICE_STATE: failed to copy to user\n");
			return -EFAULT;
		}
		return 0;

	case ETSOC1_IOCTL_GET_USER_DRAM_INFO:
		if (!ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].is_valid)
			return -EINVAL;

		user_dram.base =
			ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].soc_addr;
		user_dram.size =
			ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].size;
		user_dram.dma_max_elem_size =
			ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				.access.dma_elem_size *
			MEM_REGION_DMA_ELEMENT_STEP_SIZE;
		user_dram.dma_max_elem_count =
			ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
				.access.dma_elem_count;

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
		    copy_to_user(usr_arg, &ops->dir_vq.sq_count, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = ops->dir_vq.sq_size - sizeof(struct et_circbuffer);
		if (size >= sizeof(u16) &&
		    copy_to_user(usr_arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION:
		if (size >= sizeof(et_dev->cfg) &&
		    copy_to_user(usr_arg, &et_dev->cfg, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION: failed to copy to user\n");
			return -EFAULT;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (!cmd_info.cmd || !cmd_info.size)
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_DMA &&
		    cmd_info.flags & CMD_DESC_FLAG_HIGH_PRIORITY) {
			return -EINVAL;
		}

		if (cmd_info.flags & CMD_DESC_FLAG_HIGH_PRIORITY) {
			if (cmd_info.sq_index >= et_dev->ops.dir_vq.hpsq_count)
				return -EINVAL;

			return et_squeue_copy_from_user(
				et_dev,
				false /* ops_dev */,
				true /* high priority SQ */,
				cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);
		} else {
			if (cmd_info.sq_index >= et_dev->ops.dir_vq.sq_count)
				return -EINVAL;

			if (cmd_info.flags & CMD_DESC_FLAG_DMA)
				return et_dma_move_data(
					et_dev,
					cmd_info.sq_index,
					(char __user __force *)cmd_info.cmd,
					cmd_info.size);
			else
				return et_squeue_copy_from_user(
					et_dev,
					false /* ops_dev */,
					false /* normal SQ */,
					cmd_info.sq_index,
					(char __user __force *)cmd_info.cmd,
					cmd_info.size);
		}

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (rsp_info.cq_index >= et_dev->ops.dir_vq.cq_count ||
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

		if (sq_threshold_info.sq_index >= et_dev->ops.dir_vq.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
			    (ops->dir_vq.sq_size -
			     sizeof(struct et_circbuffer)))
			return -EINVAL;

		sq_idx = sq_threshold_info.sq_index;

		atomic_set(&ops->sq_pptr[sq_idx]->sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		et_squeue_sync_bitmap(ops->sq_pptr[sq_idx]);

		return 0;

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

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);

	poll_wait(fp, &mgmt->vq_common.waitqueue, wait);

	if (mgmt->vq_common.aborting)
		return mask;

	mutex_lock(&mgmt->vq_common.sq_bitmap_mutex);
	mutex_lock(&mgmt->vq_common.cq_bitmap_mutex);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(mgmt->vq_common.sq_bitmap, mgmt->dir_vq.sq_count))
		mask |= EPOLLOUT;

	mutex_unlock(&mgmt->vq_common.sq_bitmap_mutex);

	// Generate EPOLLIN event if any CQ msg list has message for userspace
	if (!bitmap_empty(mgmt->vq_common.cq_bitmap, mgmt->dir_vq.cq_count))
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

// clang-format off
static const struct vm_operations_struct esperanto_pcie_vm_ops = {
	.open	= esperanto_pcie_vm_open,
	.close	= esperanto_pcie_vm_close,
};

// clang-format on

struct vm_area_struct *et_find_vma(unsigned long vaddr)
{
	struct vm_area_struct *vma;

	mmap_read_lock(current->mm);
	vma = find_vma(current->mm, vaddr);
	if (!vma || vma->vm_ops != &esperanto_pcie_vm_ops) {
		mmap_read_unlock(current->mm);
		return NULL;
	}
	mmap_read_unlock(current->mm);
	return vma;
}

static int esperanto_pcie_ops_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct et_ops_dev *ops;
	struct et_pci_dev *et_dev;
	struct et_dma_mapping *map;
	dma_addr_t dma_addr;
	void *kern_vaddr;
	size_t size = vma->vm_end - vma->vm_start;
	int rv;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);

	if (vma->vm_pgoff != 0) {
		dev_err(&et_dev->pdev->dev, "mmap() offset must be 0.\n");
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

	rv = dma_mmap_coherent(&et_dev->pdev->dev,
			       vma,
			       kern_vaddr,
			       dma_addr,
			       size);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "dma_mmap_coherent() failed!");
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
	struct et_pci_dev *et_dev;
	struct et_mgmt_dev *mgmt;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	struct fw_update_desc fw_update_info;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	size_t size;
	u16 max_size;
	u32 dev_state;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_DEVICE_STATE:
		// TODO: SW-8811: Implement device state with
		// heart-beat mechanism. A corner case is possible
		// here that single command sent that causes hang will
		// not be detected because the command will be popped
		// out of SQ by device and SQ will appear empty here.
		dev_state = DEV_STATE_READY;

		// Check if any SQ has pending command(s)
		mutex_lock(&mgmt->vq_common.sq_bitmap_mutex);
		for (sq_idx = 0; sq_idx < mgmt->dir_vq.sq_count; sq_idx++) {
			if (!et_squeue_empty(mgmt->sq_pptr[sq_idx])) {
				dev_state = DEV_STATE_PENDING_COMMANDS;
				break;
			}
		}

		if (size >= sizeof(u32) &&
		    copy_to_user(usr_arg, &dev_state, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DEVICE_STATE: failed to copy to user\n");
			return -EFAULT;
		}
		return 0;

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
		    copy_to_user(usr_arg, &mgmt->dir_vq.sq_count, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = mgmt->dir_vq.sq_size - sizeof(struct et_circbuffer);
		if (size >= sizeof(u16) &&
		    copy_to_user(usr_arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION:
		if (size >= sizeof(et_dev->cfg) &&
		    copy_to_user(usr_arg, &et_dev->cfg, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION: failed to copy to user\n");
			return -EFAULT;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (cmd_info.flags &
		    (CMD_DESC_FLAG_DMA | CMD_DESC_FLAG_HIGH_PRIORITY)) {
			return -EINVAL;
		}

		if (cmd_info.sq_index >= et_dev->mgmt.dir_vq.sq_count ||
		    !cmd_info.cmd || !cmd_info.size)
			return -EINVAL;

		return et_squeue_copy_from_user(
			et_dev,
			true /* mgmt_dev */,
			false /* normal SQ */,
			cmd_info.sq_index,
			(char __user __force *)cmd_info.cmd,
			cmd_info.size);

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (rsp_info.cq_index >= et_dev->mgmt.dir_vq.cq_count ||
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
			    et_dev->mgmt.dir_vq.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
			    (mgmt->dir_vq.sq_size -
			     sizeof(struct et_circbuffer)))
			return -EINVAL;

		sq_idx = sq_threshold_info.sq_index;

		atomic_set(&mgmt->sq_pptr[sq_idx]->sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		et_squeue_sync_bitmap(mgmt->sq_pptr[sq_idx]);

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

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);

	spin_lock(&ops->open_lock);
	if (ops->is_open) {
		spin_unlock(&ops->open_lock);
		pr_err("Tried to open same device multiple times\n");
		return -EBUSY; /* already open */
	}
	ops->is_open = true;
	spin_unlock(&ops->open_lock);

	return 0;
}

static int esperanto_pcie_mgmt_open(struct inode *inode, struct file *fp)
{
	struct et_mgmt_dev *mgmt;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);

	spin_lock(&mgmt->open_lock);
	if (mgmt->is_open) {
		spin_unlock(&mgmt->open_lock);
		pr_err("Tried to open same device multiple times\n");
		return -EBUSY; /* already open */
	}
	mgmt->is_open = true;
	spin_unlock(&mgmt->open_lock);

	return 0;
}

static int esperanto_pcie_ops_release(struct inode *inode, struct file *fp)
{
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	ops->is_open = false;

	return 0;
}

static int esperanto_pcie_mgmt_release(struct inode *inode, struct file *fp)
{
	struct et_mgmt_dev *mgmt;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);
	mgmt->is_open = false;

	return 0;
}

// clang-format off
static const struct file_operations et_pcie_ops_fops = {
	.owner		= THIS_MODULE,
	.poll		= esperanto_pcie_ops_poll,
	.unlocked_ioctl	= esperanto_pcie_ops_ioctl,
	.mmap		= esperanto_pcie_ops_mmap,
	.open		= esperanto_pcie_ops_open,
	.release	= esperanto_pcie_ops_release,
};

static const struct file_operations et_pcie_mgmt_fops = {
	.owner		= THIS_MODULE,
	.poll		= esperanto_pcie_mgmt_poll,
	.unlocked_ioctl	= esperanto_pcie_mgmt_ioctl,
	.open		= esperanto_pcie_mgmt_open,
	.release	= esperanto_pcie_mgmt_release,
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

	return 0;
}

static int et_mgmt_dev_init(struct et_pci_dev *et_dev)
{
	struct et_mapped_region *region;
	int rv;

	et_dev->mgmt.is_open = false;
	spin_lock_init(&et_dev->mgmt.open_lock);

	et_dev->cfg.form_factor = DEV_CONFIG_FORM_FACTOR_PCIE;
	et_dev->cfg.tdp = 25;
	et_dev->cfg.total_l3_size = 32;
	et_dev->cfg.total_l2_size = 64;
	et_dev->cfg.total_scp_size = 80;
	et_dev->cfg.cache_line_size = 64;
	et_dev->cfg.minion_boot_freq = 650;
	et_dev->cfg.cm_shire_mask = 0xffffffff;
	et_dev->mgmt.dir_vq.sq_count = 1;
	et_dev->mgmt.dir_vq.sq_size = 0x700UL;
	et_dev->mgmt.dir_vq.sq_offset = 0;
	et_dev->mgmt.dir_vq.cq_count = 1;
	et_dev->mgmt.dir_vq.cq_size = 0x700UL;
	et_dev->mgmt.dir_vq.cq_offset =
		et_dev->mgmt.dir_vq.sq_offset +
		et_dev->mgmt.dir_vq.sq_count * et_dev->mgmt.dir_vq.sq_size;

	region = &et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
	region->is_valid = true;
	region->access.io_access = MEM_REGION_IOACCESS_ENABLED;
	region->access.node_access = MEM_REGION_NODE_ACCESSIBLE_NONE;
	region->access.dma_align = MEM_REGION_DMA_ALIGNMENT_NONE;
	region->size =
		et_dev->mgmt.dir_vq.sq_count * et_dev->mgmt.dir_vq.sq_size +
		et_dev->mgmt.dir_vq.cq_count * et_dev->mgmt.dir_vq.cq_size;
	region->mapped_baseaddr =
		(void __iomem __force *)kzalloc(region->size, GFP_KERNEL);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: VQs initialization failed\n");
		goto error_free_vq_buffer;
	}

	// Create Mgmt device node
	et_dev->mgmt.misc_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->mgmt.misc_dev.fops = &et_pcie_mgmt_fops;
	et_dev->mgmt.misc_dev.name = devm_kasprintf(&et_dev->pdev->dev,
						    GFP_KERNEL,
						    "et%d_mgmt",
						    et_dev->dev_index);
	rv = misc_register(&et_dev->mgmt.misc_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Mgmt: misc register failed\n");
		goto error_vqueue_destroy_all;
	}

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

error_free_vq_buffer:
	kfree((void __force *)et_dev->mgmt
		      .regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
		      .mapped_baseaddr);
	et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

	return rv;
}

static void et_mgmt_dev_destroy(struct et_pci_dev *et_dev)
{
	misc_deregister(&et_dev->mgmt.misc_dev);
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);
	kfree((void __force *)et_dev->mgmt
		      .regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
		      .mapped_baseaddr);
	et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;
}

static int et_ops_dev_init(struct et_pci_dev *et_dev)
{
	struct et_mapped_region *region;
	int rv;

	et_dev->ops.is_open = false;
	spin_lock_init(&et_dev->ops.open_lock);

	// Init DMA rbtree
	mutex_init(&et_dev->ops.dma_rbtree_mutex);
	et_dev->ops.dma_rbtree = RB_ROOT;

	et_dev->ops.dir_vq.sq_count = 2;
	et_dev->ops.dir_vq.sq_size = 0x400UL;
	et_dev->ops.dir_vq.sq_offset = 0;
	et_dev->ops.dir_vq.hpsq_count = 2;
	et_dev->ops.dir_vq.hpsq_size = 0x100UL;
	et_dev->ops.dir_vq.hpsq_offset =
		et_dev->ops.dir_vq.sq_offset +
		et_dev->ops.dir_vq.sq_count * et_dev->ops.dir_vq.sq_size;
	et_dev->ops.dir_vq.cq_count = 1;
	et_dev->ops.dir_vq.cq_size = 0x600UL;
	et_dev->ops.dir_vq.cq_offset =
		et_dev->ops.dir_vq.hpsq_offset +
		et_dev->ops.dir_vq.hpsq_count * et_dev->ops.dir_vq.hpsq_size;

	// Initialize VQ_BUFFER region
	region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
	region->is_valid = true;
	region->access.io_access = MEM_REGION_IOACCESS_ENABLED;
	region->access.node_access = MEM_REGION_NODE_ACCESSIBLE_NONE;
	region->access.dma_align = MEM_REGION_DMA_ALIGNMENT_NONE;
	region->size =
		et_dev->ops.dir_vq.sq_count * et_dev->ops.dir_vq.sq_size +
		et_dev->ops.dir_vq.hpsq_count * et_dev->ops.dir_vq.hpsq_size +
		et_dev->ops.dir_vq.cq_count * et_dev->ops.dir_vq.cq_size;
	region->mapped_baseaddr =
		(void __iomem __force *)kzalloc(region->size, GFP_KERNEL);

	// Initialize HOST_MANAGED region
	region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED];
	region->is_valid = true;
	region->access.io_access = MEM_REGION_IOACCESS_DISABLED;
	region->access.node_access = MEM_REGION_NODE_ACCESSIBLE_OPS;
	region->access.dma_elem_size = 0x4; /* 4 * 32MB = 128MB */
	region->access.dma_elem_count = 0x4;
	region->access.dma_align = MEM_REGION_DMA_ALIGNMENT_64BIT;
	region->soc_addr = 0x8101000000ULL;
	region->size = 0x2FF000000ULL;
	region->mapped_baseaddr = NULL;

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, false /* ops_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops device VQs initialization failed\n");
		goto error_free_vq_buffer;
	}

	// Create Ops device node
	et_dev->ops.misc_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->ops.misc_dev.fops = &et_pcie_ops_fops;
	et_dev->ops.misc_dev.name = devm_kasprintf(&et_dev->pdev->dev,
						   GFP_KERNEL,
						   "et%d_ops",
						   et_dev->dev_index);
	rv = misc_register(&et_dev->ops.misc_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "misc ops register failed\n");
		goto error_vqueue_destroy_all;
	}

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

error_free_vq_buffer:
	kfree((void __force *)et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
		      .mapped_baseaddr);
	et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

	return rv;
}

static void et_ops_dev_destroy(struct et_pci_dev *et_dev)
{
	misc_deregister(&et_dev->ops.misc_dev);
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

	kfree((void __force *)et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
		      .mapped_baseaddr);
	et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

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

	rv = et_mgmt_dev_init(et_dev);
	if (rv) {
		dev_err(&pdev->dev, "Mgmt device initialization failed\n");
		goto error_clear_master;
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

	printk("\n------------ET PCIe loop back driver!-------------\n\n");

	return rv;

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
