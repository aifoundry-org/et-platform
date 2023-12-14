// SPDX-License-Identifier: GPL-2.0

/***********************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 **********************************************************************/

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
#include "et_p2pdma.h"
#include "et_pci_dev.h"
#include "et_sysfs.h"
#include "et_vma.h"
#include "et_vqueue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe loopback device driver for esperanto soc-1");
#ifdef ET_MODULE_VERSION
MODULE_VERSION(ET_MODULE_VERSION);
#else
#error ET_MODULE_VERSION is not available
#endif

#define DRIVER_NAME		  "Esperanto"
#define PCI_VENDOR_ID_REDHAT	  0x1b36
#define PCI_DEVICE_ID_REDHAT_TEST 0x0005
/* Define Vendor ID and Device ID to utilize QEMU PCI test device */
#define ET_PCIE_VENDOR_ID      PCI_VENDOR_ID_REDHAT
#define ET_PCIE_TEST_DEVICE_ID PCI_DEVICE_ID_REDHAT_TEST
#define ET_PCIE_SOC1_ID	       0xeb01

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
static uint mgmt_discovery_timeout = 10;
module_param(mgmt_discovery_timeout, uint, 0);

static uint ops_discovery_timeout = 10;
module_param(ops_discovery_timeout, uint, 0);

DECLARE_BITMAP(dev_bitmap, ET_MAX_DEVS);

static int get_next_devnum(void)
{
	int devnum = -ENODEV;

	if (bitmap_full(dev_bitmap, ET_MAX_DEVS))
		return devnum;

	devnum = find_first_zero_bit(dev_bitmap, ET_MAX_DEVS);
	set_bit(devnum, dev_bitmap);
	return devnum;
}

static __poll_t esperanto_pcie_ops_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = 0;
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);

	poll_wait(fp, &ops->vq_data.vq_common.waitqueue, wait);

	mutex_lock(&ops->vq_data.vq_common.sq_bitmap_mutex);
	mutex_lock(&ops->vq_data.vq_common.cq_bitmap_mutex);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(ops->vq_data.vq_common.sq_bitmap,
			  ops->vq_data.vq_common.sq_count))
		mask |= EPOLLOUT;

	mutex_unlock(&ops->vq_data.vq_common.sq_bitmap_mutex);

	// Generate EPOLLIN event if any CQ msg is saved for userspace
	if (!bitmap_empty(ops->vq_data.vq_common.cq_bitmap,
			  ops->vq_data.vq_common.cq_count))
		mask |= EPOLLIN;

	mutex_unlock(&ops->vq_data.vq_common.cq_bitmap_mutex);

	return mask;
}

static long esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd,
				     unsigned long arg)
{
	long rv = 0;
	struct et_pci_dev *et_dev;
	struct et_ops_dev *ops;
	struct dram_info user_dram;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	u16 max_size;
	u32 ops_state;
	u64 dev_compat_bitmap;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);

	if ((cmd & ~IOCSIZE_MASK) == ETSOC1_IOCTL_GET_PCIBUS_DEVICE_NAME(0)) {
		if (strlen(dev_name(&et_dev->pdev->dev)) + 1 > _IOC_SIZE(cmd)) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: arg size is not enough!\n",
				_IOC_NR(cmd));
			return -ENOMEM;
		}

		if (copy_to_user(usr_arg, dev_name(&et_dev->pdev->dev),
				 strlen(dev_name(&et_dev->pdev->dev)) + 1)) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		return strlen(dev_name(&et_dev->pdev->dev)) + 1;

	} else if (cmd == ETSOC1_IOCTL_GET_DEVICE_STATE) {
		if (ops->is_resetting)
			ops_state = DEV_STATE_RESET_IN_PROGRESS;
		else if (ops->is_initialized)
			ops_state = DEV_STATE_READY;
		else
			ops_state = DEV_STATE_NOT_RESPONDING;

		// TODO: SW-10535: DEV_STATE_PENDING_COMMANDS is to be removed.
		// A corner case is possible here that single command sent that
		// causes hang will not be detected because the command will be
		// popped out of SQ by device and SQ will appear empty here.
		for (sq_idx = 0; sq_idx < ops->vq_data.vq_common.sq_count;
		     sq_idx++) {
			if (ops_state != DEV_STATE_READY)
				break;
			// Check if SQ has pending command(s)
			if (!et_squeue_empty(&ops->vq_data.sqs[sq_idx]))
				ops_state = DEV_STATE_PENDING_COMMANDS;
		}

		if (copy_to_user(usr_arg, &ops_state, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			rv = -EFAULT;
		}

		return rv;
	}

	if (ops->is_resetting) {
		dev_err(&et_dev->pdev->dev,
			"ops_ioctl[%u]: device is resetting, request cannot be completed! Re-open the node\n",
			_IOC_NR(cmd));
		return -EUCLEAN;
	} else if (!ops->is_initialized) {
		dev_err(&et_dev->pdev->dev,
			"ops_ioctl[%u]: device is not initialized, request cannot be completed!\n",
			_IOC_NR(cmd));
		return -ENODEV;
	}

	switch (cmd) {
	case ETSOC1_IOCTL_GET_USER_DRAM_INFO:
		if (!ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED].is_valid)
			return -EINVAL;

		user_dram.base = ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]
					 .dev_phys_addr;
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

		if (copy_to_user(usr_arg, &user_dram, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (copy_to_user(usr_arg, &ops->vq_data.vq_common.sq_count,
				 _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = ops->vq_data.vq_common.sq_size -
			   sizeof(struct et_circbuffer);
		if (copy_to_user(usr_arg, &max_size, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION:
		if (copy_to_user(usr_arg, &et_dev->cfg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (!cmd_info.cmd || !cmd_info.size ||
		    cmd_info.flags & CMD_DESC_FLAG_MM_RESET ||
		    cmd_info.flags & CMD_DESC_FLAG_ETSOC_RESET ||
		    ((cmd_info.flags & CMD_DESC_FLAG_DMA ||
		      cmd_info.flags & CMD_DESC_FLAG_P2PDMA) &&
		     cmd_info.flags & CMD_DESC_FLAG_HIGH_PRIORITY))
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_HIGH_PRIORITY) {
			if (cmd_info.sq_index >=
			    ops->vq_data.vq_common.hp_sq_count)
				return -EINVAL;

			rv = et_squeue_copy_from_user(
				et_dev, false /* ops_dev */,
				true /* high priority SQ */, cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);
		} else {
			if (cmd_info.sq_index >=
			    ops->vq_data.vq_common.sq_count)
				return -EINVAL;

			if (cmd_info.flags & CMD_DESC_FLAG_P2PDMA)
				rv = et_p2pdma_move_data(
					et_dev, cmd_info.sq_index,
					(char __user __force *)cmd_info.cmd,
					cmd_info.size);
			else if (cmd_info.flags & CMD_DESC_FLAG_DMA)
				rv = et_dma_move_data(
					et_dev, cmd_info.sq_index,
					(char __user __force *)cmd_info.cmd,
					cmd_info.size);
			else
				rv = et_squeue_copy_from_user(
					et_dev, false /* ops_dev */,
					false /* normal SQ */,
					cmd_info.sq_index,
					(char __user __force *)cmd_info.cmd,
					cmd_info.size);
		}

		break;

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (rsp_info.cq_index >= ops->vq_data.vq_common.cq_count ||
		    !rsp_info.rsp || !rsp_info.size)
			return -EINVAL;

		rv = et_cqueue_copy_to_user(et_dev, false /* ops_dev */,
					    rsp_info.cq_index,
					    (char __user __force *)rsp_info.rsp,
					    rsp_info.size);
		break;

	case ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP:
		if (copy_to_user(usr_arg, ops->vq_data.vq_common.sq_bitmap,
				 _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (copy_to_user(usr_arg, ops->vq_data.vq_common.cq_bitmap,
				 _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_SET_SQ_THRESHOLD:
		if (copy_from_user(&sq_threshold_info, usr_arg,
				   _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (sq_threshold_info.sq_index >=
			    ops->vq_data.vq_common.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
			    (ops->vq_data.vq_common.sq_size -
			     sizeof(struct et_circbuffer)))
			return -EINVAL;

		atomic_set(&ops->vq_data.sqs[sq_threshold_info.sq_index]
				    .sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		et_squeue_sync_bitmap(
			&ops->vq_data.sqs[sq_threshold_info.sq_index]);

		break;

	case ETSOC1_IOCTL_GET_P2PDMA_DEVICE_COMPAT_BITMAP:
		dev_compat_bitmap = et_p2pdma_get_compat_bitmap(et_dev->devnum);
		if (copy_to_user(usr_arg, &dev_compat_bitmap, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	default:
		dev_err(&et_dev->pdev->dev, "ops_ioctl: unknown cmd: 0x%x\n",
			cmd);
		return -EINVAL;
	}

	return rv;
}

static __poll_t esperanto_pcie_mgmt_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = 0;
	struct et_mgmt_dev *mgmt;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);

	poll_wait(fp, &mgmt->vq_data.vq_common.waitqueue, wait);

	mutex_lock(&mgmt->vq_data.vq_common.sq_bitmap_mutex);
	mutex_lock(&mgmt->vq_data.vq_common.cq_bitmap_mutex);

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(mgmt->vq_data.vq_common.sq_bitmap,
			  mgmt->vq_data.vq_common.sq_count))
		mask |= EPOLLOUT;

	mutex_unlock(&mgmt->vq_data.vq_common.sq_bitmap_mutex);

	// Generate EPOLLIN event if any CQ msg is saved for userspace
	if (!bitmap_empty(mgmt->vq_data.vq_common.cq_bitmap,
			  mgmt->vq_data.vq_common.cq_count))
		mask |= EPOLLIN;

	mutex_unlock(&mgmt->vq_data.vq_common.cq_bitmap_mutex);

	return mask;
}

static void esperanto_pcie_vm_open(struct vm_area_struct *vma)
{
	struct et_dma_mapping *map = vma->vm_private_data;
	struct et_pci_dev *et_dev;

	if (!map)
		return;

	dev_dbg(&map->pdev->dev, "vm_open: %p, [size=%lu,vma=%08lx-%08lx]\n",
		map, map->size, vma->vm_start, vma->vm_end);

	map->ref_count++;
	if (map->ref_count == 1) {
		et_dev = pci_get_drvdata(map->pdev);
		atomic64_add(
			map->size,
			&et_dev->ops.mem_stats
				 .counters[ET_MEM_COUNTER_STATS_CMA_ALLOCATED]);
		et_rate_entry_update(
			map->size,
			&et_dev->ops.mem_stats
				 .rates[ET_MEM_RATE_STATS_CMA_ALLOCATION_RATE]);
	}
}

static void esperanto_pcie_vm_close(struct vm_area_struct *vma)
{
	struct et_dma_mapping *map = vma->vm_private_data;
	struct et_pci_dev *et_dev;

	if (!map)
		return;

	dev_dbg(&map->pdev->dev, "vm_close: %p, [size=%lu,vma=%08lx-%08lx]\n",
		map, map->size, vma->vm_start, vma->vm_end);

	map->ref_count--;
	if (map->ref_count == 0) {
		dma_free_coherent(&map->pdev->dev, map->size, map->kern_vaddr,
				  map->dma_addr);
		et_dev = pci_get_drvdata(map->pdev);
		atomic64_sub(
			map->size,
			&et_dev->ops.mem_stats
				 .counters[ET_MEM_COUNTER_STATS_CMA_ALLOCATED]);

		kfree(map);
	}
}

// clang-format off
static const struct vm_operations_struct esperanto_pcie_vm_ops = {
	.open	= esperanto_pcie_vm_open,
	.close	= esperanto_pcie_vm_close,
};

// clang-format on

struct vm_area_struct *et_find_vma(struct et_pci_dev *et_dev,
				   unsigned long vaddr)
{
	struct vm_area_struct *vma;

	mmap_read_lock(current->mm);

	vma = find_vma(current->mm, vaddr);
	if (!vma || vma->vm_ops != &esperanto_pcie_vm_ops ||
	    ((struct et_dma_mapping *)vma->vm_private_data)->pdev !=
		    et_dev->pdev)
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

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);

	if (vma->vm_pgoff != 0) {
		dev_err(&et_dev->pdev->dev, "mmap() offset must be 0.\n");
		return -EINVAL;
	}

	vma->vm_flags |= VM_DONTCOPY | VM_NORESERVE;
	vma->vm_ops = &esperanto_pcie_vm_ops;

	kern_vaddr = dma_alloc_coherent(&et_dev->pdev->dev, size, &dma_addr,
					GFP_USER | GFP_NOWAIT | __GFP_NOWARN);
	if (!kern_vaddr) {
		dev_err(&et_dev->pdev->dev, "dma_alloc_coherent() failed!\n");
		return -ENOMEM;
	}

	rv = dma_mmap_coherent(&et_dev->pdev->dev, vma, kern_vaddr, dma_addr,
			       size);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "dma_mmap_coherent() failed!");
		goto error_dma_free_coherent;
	}

	map = kzalloc(sizeof(*map), GFP_KERNEL);
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

static long esperanto_pcie_mgmt_ioctl(struct file *fp, unsigned int cmd,
				      unsigned long arg)
{
	long rv = 0;
	struct et_pci_dev *et_dev;
	struct et_mgmt_dev *mgmt;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	struct fw_update_desc fw_update_info;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	u16 max_size;
	u32 mgmt_state;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	if ((cmd & ~IOCSIZE_MASK) == ETSOC1_IOCTL_GET_PCIBUS_DEVICE_NAME(0)) {
		if (strlen(dev_name(&et_dev->pdev->dev)) + 1 > _IOC_SIZE(cmd)) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: arg size is not enough!\n",
				_IOC_NR(cmd));
			return -ENOMEM;
		}

		if (copy_to_user(usr_arg, dev_name(&et_dev->pdev->dev),
				 strlen(dev_name(&et_dev->pdev->dev)) + 1)) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		return strlen(dev_name(&et_dev->pdev->dev)) + 1;

	} else if (cmd == ETSOC1_IOCTL_GET_DEVICE_STATE) {
		if (mgmt->is_resetting)
			mgmt_state = DEV_STATE_RESET_IN_PROGRESS;
		else if (mgmt->is_initialized)
			mgmt_state = DEV_STATE_READY;
		else
			mgmt_state = DEV_STATE_NOT_RESPONDING;

		// TODO: SW-10535: DEV_STATE_PENDING_COMMANDS is to be removed.
		// A corner case is possible here that single command sent that
		// causes hang will not be detected because the command will be
		// popped out of SQ by device and SQ will appear empty here.
		for (sq_idx = 0; sq_idx < mgmt->vq_data.vq_common.sq_count;
		     sq_idx++) {
			if (mgmt_state != DEV_STATE_READY)
				break;
			// Check if SQ has pending command(s)
			if (!et_squeue_empty(&mgmt->vq_data.sqs[sq_idx]))
				mgmt_state = DEV_STATE_PENDING_COMMANDS;
		}

		if (copy_to_user(usr_arg, &mgmt_state, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			rv = -EFAULT;
		}

		return rv;
	}

	if (mgmt->is_resetting) {
		dev_err(&et_dev->pdev->dev,
			"mgmt_ioctl[%u]: device is resetting, request cannot be completed! Re-open the node\n",
			_IOC_NR(cmd));
		return -EUCLEAN;
	} else if (!mgmt->is_initialized) {
		dev_err(&et_dev->pdev->dev,
			"mgmt_ioctl[%u]: device is not initialized, request cannot be completed!\n",
			_IOC_NR(cmd));
		return -ENODEV;
	}

	switch (cmd) {
	case ETSOC1_IOCTL_FW_UPDATE:
		if (copy_from_user(&fw_update_info, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (!fw_update_info.ubuf || !fw_update_info.size)
			return -EINVAL;

		rv = et_mmio_write_fw_image(
			et_dev, (char __user __force *)fw_update_info.ubuf,
			fw_update_info.size);
		break;

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (copy_to_user(usr_arg, &mgmt->vq_data.vq_common.sq_count,
				 _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = mgmt->vq_data.vq_common.sq_size -
			   sizeof(struct et_circbuffer);
		if (copy_to_user(usr_arg, &max_size, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION:
		if (copy_to_user(usr_arg, &et_dev->cfg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_PUSH_SQ:
		if (copy_from_user(&cmd_info, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (cmd_info.sq_index >= mgmt->vq_data.vq_common.sq_count ||
		    !cmd_info.cmd || !cmd_info.size ||
		    cmd_info.flags &
			    (CMD_DESC_FLAG_DMA | CMD_DESC_FLAG_HIGH_PRIORITY))
			return -EINVAL;

		if (cmd_info.flags & CMD_DESC_FLAG_ETSOC_RESET) {
			mutex_lock(&et_dev->mgmt.reset_mutex);
			et_dev->mgmt.is_resetting = true;
		}

		if (cmd_info.flags &
		    (CMD_DESC_FLAG_ETSOC_RESET | CMD_DESC_FLAG_MM_RESET)) {
			mutex_lock(&et_dev->ops.reset_mutex);
			spin_lock(&et_dev->ops.open_lock);
			if (!et_dev->ops.is_open) {
				et_dev->ops.is_resetting = true;
			} else {
				dev_err(&et_dev->pdev->dev,
					"mgmt_ioctl[%u]: Ops Device is in use, reset cannot be done!\n",
					_IOC_NR(cmd));
				rv = -EPERM;
			}
			spin_unlock(&et_dev->ops.open_lock);
		}

		if (rv == 0)
			rv = et_squeue_copy_from_user(
				et_dev, true /* mgmt_dev */,
				false /* normal SQ */, cmd_info.sq_index,
				(char __user __force *)cmd_info.cmd,
				cmd_info.size);

		if (cmd_info.flags &
		    (CMD_DESC_FLAG_ETSOC_RESET | CMD_DESC_FLAG_MM_RESET)) {
			if (rv != cmd_info.size)
				et_dev->ops.is_resetting = false;
			mutex_unlock(&et_dev->ops.reset_mutex);
		}

		if (cmd_info.flags & CMD_DESC_FLAG_ETSOC_RESET) {
			if (rv != cmd_info.size)
				et_dev->mgmt.is_resetting = false;
			mutex_unlock(&et_dev->mgmt.reset_mutex);
		}

		break;

	case ETSOC1_IOCTL_POP_CQ:
		if (copy_from_user(&rsp_info, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (rsp_info.cq_index >= mgmt->vq_data.vq_common.cq_count ||
		    !rsp_info.rsp || !rsp_info.size)
			return -EINVAL;

		rv = et_cqueue_copy_to_user(et_dev, true /* mgmt_dev */,
					    rsp_info.cq_index,
					    (char __user __force *)rsp_info.rsp,
					    rsp_info.size);
		break;

	case ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP:
		if (copy_to_user(usr_arg, mgmt->vq_data.vq_common.sq_bitmap,
				 _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (copy_to_user(usr_arg, mgmt->vq_data.vq_common.cq_bitmap,
				 _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_SET_SQ_THRESHOLD:
		if (copy_from_user(&sq_threshold_info, usr_arg,
				   _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		if (sq_threshold_info.sq_index >=
			    mgmt->vq_data.vq_common.sq_count ||
		    !sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
			    (mgmt->vq_data.vq_common.sq_size -
			     sizeof(struct et_circbuffer)))
			return -EINVAL;

		atomic_set(&mgmt->vq_data.sqs[sq_threshold_info.sq_index]
				    .sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		et_squeue_sync_bitmap(
			&mgmt->vq_data.sqs[sq_threshold_info.sq_index]);

		break;

	default:
		dev_err(&et_dev->pdev->dev, "ops_ioctl: unknown cmd: 0x%x\n",
			cmd);
		return -EINVAL;
	}

	return rv;
}

static int esperanto_pcie_ops_open(struct inode *inode, struct file *fp)
{
	struct et_ops_dev *ops;
	struct et_pci_dev *et_dev;
	int rv = 0;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	et_dev = container_of(ops, struct et_pci_dev, ops);

	spin_lock(&ops->open_lock);
	if (ops->is_open) {
		dev_err(&et_dev->pdev->dev,
			"Ops: Tried to open same device multiple times\n");
		rv = -EBUSY; /* already open */
		goto spin_unlock;
	} else if (ops->is_resetting) {
		dev_err(&et_dev->pdev->dev,
			"Ops: Device is resetting, action cannot be completed!\n");
		rv = -EPERM;
		goto spin_unlock;
	}
	ops->is_open = true;

spin_unlock:
	spin_unlock(&ops->open_lock);

	return rv;
}

static int esperanto_pcie_mgmt_open(struct inode *inode, struct file *fp)
{
	struct et_mgmt_dev *mgmt;
	struct et_pci_dev *et_dev;
	int lock_acquired;
	int rv = 0;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	spin_lock(&mgmt->open_lock);
	if (mgmt->is_open) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: Tried to open same device multiple times\n");
		rv = -EBUSY; /* already open */
		goto spin_unlock;
	}
	lock_acquired = mutex_trylock(&mgmt->reset_mutex);
	if (!lock_acquired || mgmt->is_resetting) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: Device is resetting, action cannot be completed!\n");
		rv = -EPERM;
		goto unlock_reset_mutex;
	}

	mgmt->is_open = true;

unlock_reset_mutex:
	if (lock_acquired)
		mutex_unlock(&mgmt->reset_mutex);

spin_unlock:
	spin_unlock(&mgmt->open_lock);

	return rv;
}

static int esperanto_pcie_ops_release(struct inode *inode, struct file *fp)
{
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	spin_lock(&ops->open_lock);
	ops->is_open = false;
	spin_unlock(&ops->open_lock);

	return 0;
}

static int esperanto_pcie_mgmt_release(struct inode *inode, struct file *fp)
{
	struct et_mgmt_dev *mgmt;
	struct et_pci_dev *et_dev;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);
	spin_lock(&mgmt->open_lock);
	mgmt->is_open = false;
	spin_unlock(&mgmt->open_lock);

	mutex_lock(&mgmt->reset_mutex);
	if (mgmt->is_resetting) {
		et_dev = container_of(mgmt, struct et_pci_dev, mgmt);
		queue_work(et_dev->reset_workqueue, &et_dev->isr_work);
	}
	mutex_unlock(&mgmt->reset_mutex);

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
	int devnum = get_next_devnum();

	if (devnum < 0)
		return devnum;

	et_dev = devm_kzalloc(&pdev->dev, sizeof(*et_dev), GFP_KERNEL);
	if (!et_dev)
		return -ENOMEM;

	et_dev->pdev = pdev;
	et_dev->devnum = (u8)devnum;
	*new_dev = et_dev;

	INIT_LIST_HEAD(&et_dev->bar_region_list);
	et_p2pdma_init(devnum);

	et_dev->is_initialized = false;

	et_dev->mgmt.is_initialized = false;
	mutex_init(&et_dev->mgmt.init_mutex);
	et_dev->mgmt.is_resetting = false;
	mutex_init(&et_dev->mgmt.reset_mutex);
	et_dev->mgmt.miscdev_created = false;

	et_dev->ops.is_initialized = false;
	mutex_init(&et_dev->ops.init_mutex);
	et_dev->ops.is_resetting = false;
	mutex_init(&et_dev->ops.reset_mutex);
	et_dev->ops.miscdev_created = false;

	return 0;
}

int et_mgmt_dev_init(struct et_pci_dev *et_dev, u32 timeout_secs,
		     bool miscdev_create)
{
	struct et_mapped_region *region;
	int rv;

	(void)timeout_secs;
	mutex_lock(&et_dev->mgmt.init_mutex);

	if (et_dev->mgmt.is_initialized) {
		dev_info(&et_dev->pdev->dev,
			 "Mgmt: Device is already initialized\n");
		rv = 0;
		goto error_unlock_init_mutex;
	}

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
	et_dev->cfg.devnum = et_dev->devnum;
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
	region->io.mapped_baseaddr =
		(void __iomem __force *)kzalloc(region->size, GFP_KERNEL);
	if (!region->io.mapped_baseaddr) {
		region->is_valid = false;
		rv = -ENOMEM;
		goto error_unlock_init_mutex;
	}

	// SysFs error statistics initialization
	rv = et_sysfs_add_group(et_dev, ET_SYSFS_GID_ERR_STATS);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: et_sysfs_add_group() failed, group_id: %d\n",
			ET_SYSFS_GID_ERR_STATS);
		goto error_free_vq_buffer;
	}
	et_err_stats_init(&et_dev->mgmt.err_stats);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: et_vqueue_init_all() failed!\n");
		goto error_sysfs_remove_group;
	}

	if (miscdev_create && !et_dev->mgmt.miscdev_created) {
		// Create Mgmt device node
		et_dev->mgmt.misc_dev.minor = MISC_DYNAMIC_MINOR;
		et_dev->mgmt.misc_dev.fops = &et_pcie_mgmt_fops;
		et_dev->mgmt.misc_dev.mode = 0666;
		et_dev->mgmt.misc_dev.name =
			devm_kasprintf(&et_dev->pdev->dev, GFP_KERNEL,
				       "et%d_mgmt", et_dev->devnum);
		rv = misc_register(&et_dev->mgmt.misc_dev);
		if (rv) {
			dev_err(&et_dev->pdev->dev,
				"Mgmt: misc register failed\n");
			goto error_vqueue_destroy_all;
		}
		et_dev->mgmt.miscdev_created = true;
	}

	et_dev->mgmt.is_initialized = true;
	mutex_unlock(&et_dev->mgmt.init_mutex);

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

error_sysfs_remove_group:
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_ERR_STATS);

error_free_vq_buffer:
	kfree((void __force *)et_dev->mgmt
		      .regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
		      .io.mapped_baseaddr);
	et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

error_unlock_init_mutex:
	mutex_unlock(&et_dev->mgmt.init_mutex);

	return rv;
}

void et_mgmt_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy)
{
	mutex_lock(&et_dev->mgmt.init_mutex);

	if (miscdev_destroy && et_dev->mgmt.miscdev_created) {
		misc_deregister(&et_dev->mgmt.misc_dev);
		et_dev->mgmt.miscdev_created = false;
	}

	if (!et_dev->mgmt.is_initialized)
		goto unlock_init_mutex;

	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_ERR_STATS);

	kfree((void __force *)et_dev->mgmt
		      .regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
		      .io.mapped_baseaddr);
	et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

	et_dev->mgmt.is_initialized = false;

unlock_init_mutex:
	mutex_unlock(&et_dev->mgmt.init_mutex);
}

int et_ops_dev_init(struct et_pci_dev *et_dev, u32 timeout_secs,
		    bool miscdev_create)
{
	struct et_mapped_region *region;
	int rv;

	(void)timeout_secs;
	mutex_lock(&et_dev->ops.init_mutex);

	if (et_dev->ops.is_initialized) {
		dev_info(&et_dev->pdev->dev,
			 "Ops: Device is already initialized\n");
		rv = 0;
		goto error_unlock_init_mutex;
	}

	et_dev->ops.is_open = false;
	spin_lock_init(&et_dev->ops.open_lock);

	et_dev->ops.dir_vq.sq_count = 2;
	et_dev->ops.dir_vq.sq_size = 0x400UL;
	et_dev->ops.dir_vq.sq_offset = 0;
	et_dev->ops.dir_vq.hp_sq_count = 2;
	et_dev->ops.dir_vq.hp_sq_size = 0x100UL;
	et_dev->ops.dir_vq.hp_sq_offset =
		et_dev->ops.dir_vq.sq_offset +
		et_dev->ops.dir_vq.sq_count * et_dev->ops.dir_vq.sq_size;
	et_dev->ops.dir_vq.cq_count = 1;
	et_dev->ops.dir_vq.cq_size = 0x600UL;
	et_dev->ops.dir_vq.cq_offset =
		et_dev->ops.dir_vq.hp_sq_offset +
		et_dev->ops.dir_vq.hp_sq_count * et_dev->ops.dir_vq.hp_sq_size;

	// Initialize VQ_BUFFER region
	region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
	region->is_valid = true;
	region->access.io_access = MEM_REGION_IOACCESS_ENABLED;
	region->access.node_access = MEM_REGION_NODE_ACCESSIBLE_NONE;
	region->access.dma_align = MEM_REGION_DMA_ALIGNMENT_NONE;
	region->size =
		et_dev->ops.dir_vq.sq_count * et_dev->ops.dir_vq.sq_size +
		et_dev->ops.dir_vq.hp_sq_count * et_dev->ops.dir_vq.hp_sq_size +
		et_dev->ops.dir_vq.cq_count * et_dev->ops.dir_vq.cq_size;
	region->io.mapped_baseaddr =
		(void __iomem __force *)kzalloc(region->size, GFP_KERNEL);
	if (!region->io.mapped_baseaddr) {
		region->is_valid = false;
		rv = -ENOMEM;
		goto error_unlock_init_mutex;
	}

	// Initialize HOST_MANAGED region
	region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED];
	region->is_valid = true;
	region->access.io_access = MEM_REGION_IOACCESS_DISABLED;
	region->access.p2p_access = MEM_REGION_P2PACCESS_ENABLED;
	region->access.node_access = MEM_REGION_NODE_ACCESSIBLE_OPS;
	region->access.dma_elem_size =
		SZ_128M / MEM_REGION_DMA_ELEMENT_STEP_SIZE;
	region->access.dma_elem_count = 0x8;
	region->access.dma_align = MEM_REGION_DMA_ALIGNMENT_64BIT;
	region->dev_phys_addr = 0x8005801000ULL;
	region->size = 0x7fa600000ULL;
	rv = et_p2pdma_add_resource(et_dev, NULL, region);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops: et_p2pdma_add_resource() failed!\n");
		goto error_free_vq_buffer;
	}

	// SysFs memory statistics initialization
	rv = et_sysfs_add_group(et_dev, ET_SYSFS_GID_MEM_STATS);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops: et_sysfs_add_group() failed, group_id: %d\n",
			ET_SYSFS_GID_MEM_STATS);
		goto error_p2pdma_release_resource;
	}
	et_mem_stats_init(&et_dev->ops.mem_stats);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, false /* ops_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops: et_vqueue_init_all() failed!\n");
		goto error_sysfs_remove_group;
	}

	if (miscdev_create && !et_dev->ops.miscdev_created) {
		// Create Ops device node
		et_dev->ops.misc_dev.minor = MISC_DYNAMIC_MINOR;
		et_dev->ops.misc_dev.fops = &et_pcie_ops_fops;
		et_dev->ops.misc_dev.mode = 0666;
		et_dev->ops.misc_dev.name =
			devm_kasprintf(&et_dev->pdev->dev, GFP_KERNEL,
				       "et%d_ops", et_dev->devnum);
		rv = misc_register(&et_dev->ops.misc_dev);
		if (rv) {
			dev_err(&et_dev->pdev->dev,
				"misc ops register failed\n");
			goto error_vqueue_destroy_all;
		}
		et_dev->ops.miscdev_created = true;
	}

	et_dev->ops.is_initialized = true;
	mutex_unlock(&et_dev->ops.init_mutex);

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

error_sysfs_remove_group:
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_MEM_STATS);

error_p2pdma_release_resource:
	et_p2pdma_release_resource(
		et_dev, &et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]);

error_free_vq_buffer:
	kfree((void __force *)et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
		      .io.mapped_baseaddr);
	et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

error_unlock_init_mutex:
	mutex_unlock(&et_dev->ops.init_mutex);

	return rv;
}

void et_ops_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy)
{
	mutex_lock(&et_dev->ops.init_mutex);

	if (miscdev_destroy && et_dev->ops.miscdev_created) {
		misc_deregister(&et_dev->ops.misc_dev);
		et_dev->ops.miscdev_created = false;
	}

	if (!et_dev->ops.is_initialized)
		goto unlock_init_mutex;

	et_vqueue_destroy_all(et_dev, false /* ops_dev */);
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_MEM_STATS);
	et_p2pdma_release_resource(
		et_dev, &et_dev->ops.regions[OPS_MEM_REGION_TYPE_HOST_MANAGED]);

	kfree((void __force *)et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
		      .io.mapped_baseaddr);
	et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

	et_dev->ops.is_initialized = false;

unlock_init_mutex:
	mutex_unlock(&et_dev->ops.init_mutex);
}

static void destroy_et_pci_dev(struct et_pci_dev *et_dev)
{
	u8 devnum;

	if (!et_dev)
		return;

	devnum = et_dev->devnum;
	clear_bit(devnum, dev_bitmap);
	mutex_destroy(&et_dev->mgmt.init_mutex);
	mutex_destroy(&et_dev->mgmt.reset_mutex);
	mutex_destroy(&et_dev->ops.init_mutex);
	mutex_destroy(&et_dev->ops.reset_mutex);
}

static int init_et_pci_dev(struct et_pci_dev *et_dev, bool miscdev_create)
{
	int rv;
	struct pci_dev *pdev = et_dev->pdev;

	if (et_dev->is_initialized) {
		dev_info(&pdev->dev, "et_pci_dev is already initialized\n");
		return 0;
	}

	rv = pci_enable_device_mem(pdev);
	if (rv < 0) {
		dev_err(&pdev->dev, "enable device failed\n");
		return rv;
	}

	rv = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (rv) {
		dev_err(&pdev->dev, "set dma mask failed\n");
		goto error_disable_dev;
	}

	pci_set_master(pdev);

	rv = et_mgmt_dev_init(et_dev, mgmt_discovery_timeout, miscdev_create);
	if (rv) {
		dev_err(&pdev->dev, "Mgmt device initialization failed\n");
		goto error_clear_master;
	}

	rv = et_ops_dev_init(et_dev, ops_discovery_timeout, miscdev_create);
	if (rv) {
		dev_warn(&pdev->dev,
			 "Ops device initialization failed, errno: %d\n", -rv);
		rv = 0;
	}

	et_dev->is_initialized = true;

	dev_info(&et_dev->pdev->dev,
		 "\n------------ET PCIe loop back driver!-------------\n\n");

	return rv;

error_clear_master:
	pci_clear_master(pdev);

error_disable_dev:
	pci_disable_device(pdev);

	return rv;
}

static void uninit_et_pci_dev(struct et_pci_dev *et_dev, bool miscdev_destroy)
{
	struct pci_dev *pdev = et_dev->pdev;

	et_ops_dev_destroy(et_dev, miscdev_destroy);
	et_mgmt_dev_destroy(et_dev, miscdev_destroy);

	if (!et_dev->is_initialized)
		return;

	pci_clear_master(pdev);
	pci_disable_device(pdev);
	et_dev->is_initialized = false;
}

static void et_reset_isr_work(struct work_struct *work)
{
	int rv = 0;
	unsigned int wait_ms = 100, time_ms, uptime_ms;
	struct et_pci_dev *et_dev =
		container_of(work, struct et_pci_dev, isr_work);

	mutex_lock(&et_dev->mgmt.reset_mutex);
	mutex_lock(&et_dev->ops.reset_mutex);

	if (!et_dev->mgmt.is_resetting || !et_dev->ops.is_resetting) {
		dev_warn(&et_dev->pdev->dev,
			 "Reset ISR is invoked but no reset was triggered!");
		goto unlock_reset_mutex;
	}

	uninit_et_pci_dev(et_dev, false);

	dev_dbg(&et_dev->pdev->dev, "Waiting for PCIe link to settle...\n");
	// After reset is triggered, the device goes down after some time and
	// then gets up again. To detect that the link is stable, the device
	// should be present for `pcilink_max_estim_downtime_ms`
	for (time_ms = 0, uptime_ms = 0;
	     time_ms < et_dev->reset_cfg.pcilink_discovery_timeout_ms &&
	     uptime_ms < et_dev->reset_cfg.pcilink_max_estim_downtime_ms;
	     time_ms += wait_ms) {
		if (pci_device_is_present(et_dev->pdev))
			uptime_ms += wait_ms;
		else
			uptime_ms = 0;
		msleep(wait_ms);
	}

	if (uptime_ms < et_dev->reset_cfg.pcilink_max_estim_downtime_ms) {
		dev_err(&et_dev->pdev->dev,
			"Unable to detect the device on bus!");
		goto exit_reset;
	}

	rv = pci_load_saved_state(et_dev->pdev, et_dev->pstate);
	if (rv) {
		dev_warn(&et_dev->pdev->dev,
			 "Failed to load PCI state, errno: %d\n", -rv);
	} else {
		pci_restore_state(et_dev->pdev);
	}

	rv = init_et_pci_dev(et_dev, false);
	if (rv < 0)
		dev_err(&et_dev->pdev->dev,
			"PCIe re-initialization failed, errno: %d\n", -rv);

exit_reset:
	et_dev->mgmt.is_resetting = false;
	et_dev->ops.is_resetting = false;

unlock_reset_mutex:
	mutex_unlock(&et_dev->ops.reset_mutex);
	mutex_unlock(&et_dev->mgmt.reset_mutex);
}

static int esperanto_pcie_probe(struct pci_dev *pdev,
				const struct pci_device_id *pci_id)
{
	int rv;
	struct et_pci_dev *et_dev = NULL;

	// Create instance data for this device, save it to drvdata
	rv = create_et_pci_dev(&et_dev, pdev);
	pci_set_drvdata(pdev, et_dev); // Set even if NULL
	if (rv < 0) {
		dev_err(&pdev->dev, "create_et_pci_dev failed\n");
		return rv;
	}

	rv = pci_save_state(pdev);
	if (rv)
		dev_warn(&pdev->dev, "couldn't save PCI state\n");
	else
		et_dev->pstate = pci_store_saved_state(pdev);

	rv = init_et_pci_dev(et_dev, true);
	if (rv < 0) {
		dev_err(&pdev->dev, "PCIe initialization failed\n");
		goto error_free_saved_state;
	}

	rv = et_sysfs_add_group(et_dev, ET_SYSFS_GID_SOC_RESET);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"et_sysfs_add_group() failed, group_id: %d\n",
			ET_SYSFS_GID_SOC_RESET);
		goto error_uninit_et_pci_dev;
	}
	et_soc_reset_cfg_init(&et_dev->reset_cfg);

	rv = et_sysfs_add_file(et_dev, ET_SYSFS_FID_DEVNUM);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"et_sysfs_add_file() failed, file_id: %d\n",
			ET_SYSFS_FID_DEVNUM);
		goto error_sysfs_remove_group;
	}

	et_dev->reset_workqueue =
		alloc_workqueue("%s:et%d_rstwq", WQ_MEM_RECLAIM | WQ_UNBOUND, 1,
				dev_name(&et_dev->pdev->dev), et_dev->devnum);
	if (!et_dev->reset_workqueue) {
		dev_err(&pdev->dev, "Mgmt device initialization failed\n");
		rv = -ENOMEM;
		goto error_sysfs_remove_file;
	}
	INIT_WORK(&et_dev->isr_work, et_reset_isr_work);

	return rv;

error_sysfs_remove_file:
	et_sysfs_remove_file(et_dev, ET_SYSFS_FID_DEVNUM);

error_sysfs_remove_group:
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_SOC_RESET);

error_uninit_et_pci_dev:
	uninit_et_pci_dev(et_dev, true);

error_free_saved_state:
	kfree(et_dev->pstate);
	et_dev->pstate = NULL;

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

	cancel_work_sync(&et_dev->isr_work);
	destroy_workqueue(et_dev->reset_workqueue);

	uninit_et_pci_dev(et_dev, true);
	et_sysfs_remove_files(et_dev);
	et_sysfs_remove_groups(et_dev);
	kfree(et_dev->pstate);
	et_dev->pstate = NULL;
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
