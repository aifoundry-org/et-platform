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
#include <linux/sizes.h>
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
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME	       "Esperanto"
#define ET_PCIE_VENDOR_ID      0x1e0a
#define ET_PCIE_TEST_DEVICE_ID 0x9038
#define ET_PCIE_SOC1_ID	       0xeb01
#define ET_MAX_DEVS	       64

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

static struct et_bar_addr_dbg *mgmt_bar_addr_dbg;
static struct et_bar_addr_dbg *ops_bar_addr_dbg;
/*
 * DIR discovery timeout in seconds for mgmt/ops nodes, if set to 0, checks
 * DIR status only once and returns immediately if not ready.
 *
 * TODO: Remove these params in production after bringup
 */
static uint mgmt_discovery_timeout = 300;
module_param(mgmt_discovery_timeout, uint, 0);

static uint ops_discovery_timeout = 300;
module_param(ops_discovery_timeout, uint, 0);

static DECLARE_BITMAP(dev_bitmap, ET_MAX_DEVS);

static int get_dev_index(void)
{
	int index = -ENODEV;

	if (bitmap_full(dev_bitmap, ET_MAX_DEVS))
		return index;

	index = find_first_zero_bit(dev_bitmap, ET_MAX_DEVS);
	set_bit(index, dev_bitmap);
	return index;
}

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
	for (i = 0; i < ops->dir_vq.sq_count; i++)
		et_squeue_event_available(ops->sq_pptr[i]);

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
	struct et_mapped_region *region;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	size_t size;
	u16 max_size;
	u32 dev_state;
	u8 trace_type;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);
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
				// break loop
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
		region = &ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED];
		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_OPS))
			return -EACCES;

		user_dram.base = region->soc_addr;
		user_dram.size = region->size;
		user_dram.dma_max_elem_size = region->access.dma_elem_size *
					      MEM_REGION_DMA_ELEMENT_STEP_SIZE;
		user_dram.dma_max_elem_count = region->access.dma_elem_count;

		switch (region->access.dma_align) {
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

	case ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE:
		if (copy_from_user(&trace_type, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		switch (trace_type) {
		case TRACE_BUFFER_MM:
			region = &et_dev->mgmt.regions
					  [MGMT_MEM_REGION_TYPE_MMFW_TRACE];
			break;
		case TRACE_BUFFER_CM:
			region = &et_dev->mgmt.regions
					  [MGMT_MEM_REGION_TYPE_CMFW_TRACE];
			break;
		default:
			return -EINVAL;
		}

		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_OPS))
			return -EACCES;

		return (u32)region->size;

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
			if (cmd_info.sq_index >= ops->dir_vq.hpsq_count)
				return -EINVAL;

			return et_squeue_copy_from_user(
				et_dev,
				false /* ops_dev */,
				true /* high priority SQ */,
				cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);
		} else {
			if (cmd_info.sq_index >= ops->dir_vq.sq_count)
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

		if (rsp_info.cq_index >= ops->dir_vq.cq_count ||
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

		if (sq_threshold_info.sq_index >= ops->dir_vq.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
			    (ops->dir_vq.sq_size -
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
	for (i = 0; i < mgmt->dir_vq.sq_count; i++)
		et_squeue_event_available(mgmt->sq_pptr[i]);

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

	if (!map)
		return;

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

struct vm_area_struct *et_find_vma(unsigned long vaddr)
{
	struct vm_area_struct *vma;

	mmap_read_lock(current->mm);

	vma = find_vma(current->mm, vaddr);
	if (!vma && vma->vm_ops != &esperanto_pcie_vm_ops)
		vma = NULL;

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

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);

	if (vma->vm_pgoff != 0) {
		dev_err(&et_dev->pdev->dev, "mmap() offset must be 0.\n");
		return -EINVAL;
	}

	vma->vm_flags |= VM_DONTCOPY | VM_NORESERVE;
	vma->vm_ops = &esperanto_pcie_vm_ops;

	// TODO: Remove GFP_NOWAIT, adding GFP_NOWAIT for experimental purpose
	kern_vaddr = dma_alloc_coherent(&et_dev->pdev->dev,
					size,
					&dma_addr,
					GFP_USER | GFP_NOWAIT);
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
	struct trace_desc trace_info;
	struct sq_threshold sq_threshold_info;
	struct fw_update_desc fw_update_info;
	struct et_mapped_region *region;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	size_t size;
	u16 max_size;
	u32 dev_state;
	u8 trace_type;
	void *trace_buf;

	mgmt = container_of(fp->private_data,
			    struct et_mgmt_dev,
			    misc_mgmt_dev);
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
				// break loop
				break;
			}
		}
		mutex_unlock(&mgmt->vq_common.sq_bitmap_mutex);

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

		if (cmd_info.sq_index >= mgmt->dir_vq.sq_count ||
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

		if (rsp_info.cq_index >= mgmt->dir_vq.cq_count ||
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

		if (sq_threshold_info.sq_index >= mgmt->dir_vq.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
			    (mgmt->dir_vq.sq_size -
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

	case ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE:
		if (copy_from_user(&trace_type, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		switch (trace_type) {
		case TRACE_BUFFER_SP:
			region =
				&mgmt->regions[MGMT_MEM_REGION_TYPE_SPFW_TRACE];
			break;
		case TRACE_BUFFER_MM:
			region =
				&mgmt->regions[MGMT_MEM_REGION_TYPE_MMFW_TRACE];
			break;
		case TRACE_BUFFER_CM:
			region =
				&mgmt->regions[MGMT_MEM_REGION_TYPE_CMFW_TRACE];
			break;
		default:
			return -EINVAL;
		}

		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_MGMT))
			return -EACCES;

		return (u32)region->size;

	case ETSOC1_IOCTL_EXTRACT_TRACE_BUFFER:
		if (copy_from_user(&trace_info, usr_arg, _IOC_SIZE(cmd)))
			return -EINVAL;

		if (!trace_info.buf)
			return -EINVAL;

		switch (trace_info.trace_type) {
		case TRACE_BUFFER_SP:
			region =
				&mgmt->regions[MGMT_MEM_REGION_TYPE_SPFW_TRACE];
			break;
		case TRACE_BUFFER_MM:
			region =
				&mgmt->regions[MGMT_MEM_REGION_TYPE_MMFW_TRACE];
			break;
		case TRACE_BUFFER_CM:
			region =
				&mgmt->regions[MGMT_MEM_REGION_TYPE_CMFW_TRACE];
			break;
		default:
			return -EINVAL;
		}
		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_MGMT))
			return -EACCES;

		trace_buf = kvmalloc(region->size, GFP_KERNEL);
		if (!trace_buf)
			return -ENOMEM;

		et_ioread(region->mapped_baseaddr, 0, trace_buf, region->size);
		if (copy_to_user((char __user __force *)trace_info.buf,
				 trace_buf,
				 region->size)) {
			pr_err("ioctl: ETSOC1_IOCTL_EXTRACT_TRACE_BUFFER: failed to copy to user\n");
			kvfree(trace_buf);
			return -ENOMEM;
		}
		kvfree(trace_buf);
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
	.poll			= esperanto_pcie_ops_poll,
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
	int index = get_dev_index();

	if (index < 0)
		return index;

	et_dev = devm_kzalloc(&pdev->dev, sizeof(*et_dev), GFP_KERNEL);
	if (!et_dev)
		return -ENOMEM;

	et_dev->pdev = pdev;
	et_dev->dev_index = (u8)index;
	*new_dev = et_dev;

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
					 int regs_count)
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
	struct et_bar_addr_dbg *bar_addr_dbg;
	struct et_bar_region *existing_node, *new_node;

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

	if (is_mgmt)
		bar_addr_dbg = mgmt_bar_addr_dbg;
	else
		bar_addr_dbg = ops_bar_addr_dbg;

	memset(regions, 0, num_reg_types * sizeof(*regions));
	for (i = 0; i < regs_count; i++, reg_pos += section_size) {
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
				dbg_msg.level = LEVEL_WARN;
				dbg_msg.desc =
					"DIRs invalid memory region found!";
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

		new_node = kzalloc(sizeof(*new_node), GFP_KERNEL);
		if (!new_node) {
			rv = -ENOMEM;
			goto error_unmap_discovered_regions;
		}

		new_node->is_mgmt = is_mgmt;
		new_node->bar = dir_mem_region->bar;
		new_node->region_type = dir_mem_region->type;
		new_node->region_start =
			pci_resource_start(et_dev->pdev, dir_mem_region->bar) +
			dir_mem_region->bar_offset;
		new_node->region_end =
			pci_resource_start(et_dev->pdev, dir_mem_region->bar) +
			dir_mem_region->bar_offset + dir_mem_region->bar_size -
			1;

		if (new_node->region_end >
		    pci_resource_end(et_dev->pdev, dir_mem_region->bar)) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "BAR overflow detected!";
			sprintf(dbg_msg.syndrome,
				"Requested region size: 0x%llx at offset: 0x%llx is more than available BAR[%d] length\n",
				dir_mem_region->bar_size,
				dir_mem_region->bar_offset,
				dir_mem_region->bar);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			kfree(new_node);
			goto error_unmap_discovered_regions;
		}

		list_for_each_entry (existing_node,
				     &et_dev->bar_region_list,
				     list) {
			if (new_node->region_start >
				    existing_node->region_end ||
			    new_node->region_end <
				    existing_node->region_start) {
				continue;
			} else {
				dbg_msg.level = LEVEL_FATAL;
				dbg_msg.desc =
					"Requested BAR region overlaps existing BAR region!";
				sprintf(dbg_msg.syndrome,
					"\n"
					"Existing region info:\n"
					"\tNode         : %s\n"
					"\tBAR          : %d\n"
					"\tRegion Type  : %d\n"
					"\tRegion start : 0x%llx\n"
					"\tRegion end   : 0x%llx\n"
					"Requested region info:\n"
					"\tNode         : %s\n"
					"\tBAR          : %d\n"
					"\tRegion Type  : %d\n"
					"\tRegion start : 0x%llx\n"
					"\tRegion end   : 0x%llx\n",
					existing_node->is_mgmt ? "Mgmt" : "Ops",
					existing_node->bar,
					existing_node->region_type,
					existing_node->region_start,
					existing_node->region_end,
					new_node->is_mgmt ? "Mgmt" : "Ops",
					new_node->bar,
					new_node->region_type,
					new_node->region_start,
					new_node->region_end);
				et_print_event(et_dev->pdev, &dbg_msg);
				rv = -EINVAL;
				kfree(new_node);
				goto error_unmap_discovered_regions;
			}
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
			kfree(new_node);
			goto error_unmap_discovered_regions;
		}

		bar_addr_dbg->phys_addr =
			pci_resource_start(et_dev->pdev, bm_info.bar) +
			bm_info.bar_offset;
		bar_addr_dbg->mapped_baseaddr =
			regions[dir_mem_region->type].mapped_baseaddr;
		bar_addr_dbg++;

		// Save other region information
		regions[dir_mem_region->type].size = dir_mem_region->bar_size;
		regions[dir_mem_region->type].soc_addr =
			dir_mem_region->dev_address;
		memcpy(&regions[dir_mem_region->type].access,
		       (u8 *)&dir_mem_region->access,
		       sizeof(dir_mem_region->access));

		regions[dir_mem_region->type].is_valid = true;
		list_add_tail(&new_node->list, &et_dev->bar_region_list);
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
	struct et_mgmt_dir_vqueue *dir_vq_mgmt;
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
	// Waiting for device to be ready, wait for mgmt_discovery_timeout
	for (i = 0; !dir_ready && i <= mgmt_discovery_timeout; i++) {
		rv = (int)ioread16(&dir_mgmt_mem->status);
		if (rv == MGMT_BOOT_STATUS_DEV_READY) {
			pr_debug("Mgmt: DIRs ready, status: %d", rv);
			dir_ready = true;
		} else {
			// Do not flood the console, only print once in 10s
			if (i % 10 == 0)
				pr_debug("Mgmt: DIRs not ready, status: %d",
					 rv);
			if (i > 0)
				msleep(1000);
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
	if (!dir_size && dir_size > DIR_MAPPINGS[IOMEM_R_DIR_MGMT].size) {
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

	// BAR0 size check
	et_dev->bar0_size = SZ_1G * (u64)(dir_mgmt->bar0_size + 1);
	if (et_dev->bar0_size != pci_resource_len(et_dev->pdev, 0 /* BAR0 */)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"BAR0 size doesn't match BAR0 size exposed by DIRs!";
		sprintf(dbg_msg.syndrome,
			"BAR0 size detected by host: 0x%llx\nBAR0 size exposed by DIRs: 0x%llx\n",
			pci_resource_len(et_dev->pdev, 0),
			et_dev->bar0_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	// BAR2 size check
	et_dev->bar2_size = SZ_32K * (dir_mgmt->bar2_size + 1);
	if (et_dev->bar2_size != pci_resource_len(et_dev->pdev, 2 /* BAR2 */)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"BAR2 size doesn't match BAR2 size exposed by DIRs!";
		sprintf(dbg_msg.syndrome,
			"BAR2 size detected by host: 0x%llx\nBAR2 size exposed by DIRs: 0x%llx\n",
			pci_resource_len(et_dev->pdev, 2),
			et_dev->bar2_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	}

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

	// TODO: Discover device configuration from DIRs
	et_dev->cfg.form_factor = DEV_CONFIG_FORM_FACTOR_PCIE;
	et_dev->cfg.tdp = dir_mgmt->device_tdp;
	et_dev->cfg.total_l3_size = dir_mgmt->l3_size;
	et_dev->cfg.total_l2_size = dir_mgmt->l2_size;
	et_dev->cfg.total_scp_size = dir_mgmt->scp_size;
	et_dev->cfg.cache_line_size = dir_mgmt->cache_line_size;
	et_dev->cfg.minion_boot_freq = dir_mgmt->minion_boot_freq;
	et_dev->cfg.cm_shire_mask = dir_mgmt->minion_shire_mask;

	dir_pos += section_size;

	/*
	 * Parse and save vqueue information from DIRs
	 */
	dir_vq_mgmt = (struct et_mgmt_dir_vqueue *)dir_pos;
	section_size = dir_vq_mgmt->attributes_size;

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
	if (section_size > sizeof(*dir_vq_mgmt)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIR region has extra attributes, skipping extra attributes";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: VQ Region\nSize: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_vq_mgmt),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_vq_mgmt)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nRegion: VQ Region\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_vq_mgmt),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	if (!valid_mgmt_vq_region(dir_vq_mgmt,
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

	memcpy(&et_dev->mgmt.dir_vq, (u8 *)dir_vq_mgmt, sizeof(*dir_vq_mgmt));

	dir_pos += section_size;

	mgmt_bar_addr_dbg = kmalloc_array(dir_mgmt->num_regions,
					  sizeof(struct et_bar_addr_dbg),
					  GFP_KERNEL);
	if (!mgmt_bar_addr_dbg) {
		rv = -ENOMEM;
		goto error_free_dir_data;
	}

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
			"Mgmt: DIR Memory Regions mapping failed!");
		goto error_free_mgmt_bar_addr_dbg;
	}

	et_print_mgmt_dir(&et_dev->pdev->dev,
			  dir_data,
			  dir_size,
			  mgmt_bar_addr_dbg);

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"Total DIR size does not match sum of respective sizes of attribute regions";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\nSize: (DIRs: %zu, All Attribute Regions: %d)\n",
			dir_size,
			(int)(dir_pos - dir_data));
		et_print_event(et_dev->pdev, &dbg_msg);
	}

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

	kfree(mgmt_bar_addr_dbg);
	kfree(dir_data);

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, true /* mgmt_dev */);

error_free_mgmt_bar_addr_dbg:
	kfree(mgmt_bar_addr_dbg);

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
	struct et_ops_dir_vqueue *dir_vq_ops;
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
	// Waiting for device to be ready, wait for ops_discovery_timeout
	for (i = 0; !dir_ready && i <= ops_discovery_timeout; i++) {
		rv = (int)ioread16(&dir_ops_mem->status);
		if (rv == OPS_BOOT_STATUS_MM_READY) {
			pr_debug("Ops: DIRs ready, status: %d", rv);
			dir_ready = true;
		} else {
			// Do not flood the console, only print once in 10s
			if (i % 10 == 0)
				pr_debug("Ops: DIRs not ready, status: %d", rv);
			if (i > 0)
				msleep(1000);
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
	 * Parse and save vqueue information from DIRs
	 */
	dir_vq_ops = (struct et_ops_dir_vqueue *)dir_pos;
	section_size = dir_vq_ops->attributes_size;

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
	if (section_size > sizeof(*dir_vq_ops)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIR region has extra attributes, skipping extra attributes";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: VQ Region\nSize: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_vq_ops),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_vq_ops)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: VQ Region\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_vq_ops),
			section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	if (!valid_ops_vq_region(dir_vq_ops,
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

	memcpy(&et_dev->ops.dir_vq, (u8 *)dir_vq_ops, sizeof(*dir_vq_ops));

	dir_pos += section_size;

	ops_bar_addr_dbg = kmalloc_array(dir_ops->num_regions,
					 sizeof(struct et_bar_addr_dbg),
					 GFP_KERNEL);
	if (!ops_bar_addr_dbg) {
		rv = -ENOMEM;
		goto error_free_dir_data;
	}

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
		goto error_free_ops_bar_addr_dbg;
	}

	et_print_ops_dir(&et_dev->pdev->dev,
			 dir_data,
			 dir_size,
			 ops_bar_addr_dbg);

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

	kfree(ops_bar_addr_dbg);
	kfree(dir_data);

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, false /* ops_dev */);

error_free_ops_bar_addr_dbg:
	kfree(ops_bar_addr_dbg);

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

static void et_destroy_region_list(struct et_pci_dev *et_dev)
{
	struct et_bar_region *pos, *tmp;

	list_for_each_entry_safe (pos, tmp, &et_dev->bar_region_list, list) {
		list_del(&pos->list);
		kfree(pos);
	}
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

	INIT_LIST_HEAD(&et_dev->bar_region_list);

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
	rv = pci_alloc_irq_vectors(pdev,
				   ET_MAX_MSI_VECS,
				   ET_MAX_MSI_VECS,
				   PCI_IRQ_MSI);
	if (rv != ET_MAX_MSI_VECS) {
		dev_err(&pdev->dev,
			"msi vectors=%d alloc failed\n",
			ET_MAX_MSI_VECS);
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

	et_destroy_region_list(et_dev);

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
	et_destroy_region_list(et_dev);
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
