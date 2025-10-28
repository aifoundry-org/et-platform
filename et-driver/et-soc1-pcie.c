// SPDX-License-Identifier: GPL-2.0

/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 **********************************************************************/

#include <linux/aer.h>
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
#include "et_p2pdma.h"
#include "et_pci_dev.h"
#include "et_sysfs.h"
#include "et_vma.h"
#include "et_vqueue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ainekko <troubles@nekko.ai>");
MODULE_DESCRIPTION("PCIe device driver for ET soc-1");
#ifdef ET_MODULE_VERSION
MODULE_VERSION(ET_MODULE_VERSION);
#else
#error ET_MODULE_VERSION is not available
#endif

#define DRIVER_NAME	       "ET"
#define ET_PCIE_VENDOR_ID      0x1e0a
#define ET_PCIE_TEST_DEVICE_ID 0x9038
#define ET_PCIE_SOC1_ID	       0xeb01

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

/**
 * DIR discovery timeout in seconds for mgmt/ops nodes, if set to 0, checks
 * DIR status only once and returns immediately if not ready.
 *
 * TODO: Remove these params in production after bringup
 */
static uint mgmt_discovery_timeout = 100;
module_param(mgmt_discovery_timeout, uint, 0);

static uint ops_discovery_timeout = 100;
module_param(ops_discovery_timeout, uint, 0);

/* Device bitmap representing initialized device nodes */
DECLARE_BITMAP(dev_bitmap, ET_MAX_DEVS) = { 0 };

/**
 * get_next_devnum() - Returns next available device number
 *
 * Return: Positive devnum value on success, negative error on failure
 */
static int get_next_devnum(void)
{
	int devnum = -ENODEV;

	if (bitmap_full(dev_bitmap, ET_MAX_DEVS))
		return devnum;

	devnum = find_first_zero_bit(dev_bitmap, ET_MAX_DEVS);
	set_bit(devnum, dev_bitmap);
	return devnum;
}

/**
 * esperanto_pcie_ops_poll() - Ops device poll operation
 *
 * EPOLLHUP: If ops device is not initialized
 * EPOLLOUT: If any SQ of ops device is available
 * EPOLLIN: If any CQ of ops device is available
 *
 * Return: Positive devnum value on success, negative error on failure
 */
static __poll_t esperanto_pcie_ops_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = EPOLLHUP;
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	if (!ops->is_initialized)
		return mask;

	poll_wait(fp, &ops->vq_data.vq_common.waitqueue, wait);

	mutex_lock(&ops->vq_data.vq_common.sq_bitmap_mutex);
	mutex_lock(&ops->vq_data.vq_common.cq_bitmap_mutex);

	mask = 0;
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

/**
 * esperanto_pcie_ops_ioctl() - Ops device IOCTLs
 * @cmd: IOCTL commands
 * @arg: Argument passed from user-space
 *
 * This implements following IOCTLs for ops device:
 * - ETSOC1_IOCTL_GET_PCIBUS_DEVICE_NAME: Provides the PCI bus name of device.
 *   This IOCTL is functional regardless of the state of device
 * - ETSOC1_IOCTL_GET_DEVICE_STATE: Returns the current device state. This
 *   IOCTL is functional regardless of the state of device
 * - ETSOC1_IOCTL_GET_USER_DRAM_INFO: Provides DRAM information received from
 *   the device in DIRs
 * - ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE: Provies size trace buffer regions
 *   {MM, CM, MM_STATS} if regions defined by device
 * - ETSOC1_IOCTL_GET_SQ_COUNT: Provides ops device SQ count
 * - ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: Provides ops device SQ size
 * - ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION: Provides general device
 *   configuration received from the device in DIRs
 * - ETSOC1_IOCTL_PUSH_SQ: Forwards user command on SQ
 * - ETSOC1_IOCTL_POP_CQ: Pops out the response message from CQ to user
 * - ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: Provides SQ availability bitmap
 * - ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP: Provides CQ availability bitmap
 * - ETSOC1_IOCTL_SET_SQ_THRESHOLD: Sets SQ threshold for SQ availability
 * - ETSOC1_IOCTL_GET_P2PDMA_DEVICE_COMPAT_BITMAP: P2PDMA bitmap for its
 *   compatibility with other devices
 *
 * Return: Non-negative value on success, negative error on failure
 */
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
	struct et_mapped_region *region;
	void __user *usr_arg = (void __user *)arg;
	u16 sq_idx;
	u16 max_size;
	u32 ops_state;
	u8 trace_type;
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
		region = &ops->regions[OPS_MEM_REGION_TYPE_HOST_MANAGED];
		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_OPS))
			return -EACCES;

		user_dram.base = region->dev_phys_addr;
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

		if (copy_to_user(usr_arg, &user_dram, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		break;

	case ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE:
		if (copy_from_user(&trace_type, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		switch (trace_type) {
		case TRACE_BUFFER_MM:
			region = &et_dev->mgmt.regions
					  [MGMT_MEM_REGION_TYPE_MMFW_TRACE];
			break;
		case TRACE_BUFFER_CM:
			region = &et_dev->mgmt.regions
					  [MGMT_MEM_REGION_TYPE_CMFW_TRACE];
			break;
		case TRACE_BUFFER_MM_STATS:
			region =
				&et_dev->mgmt
					 .regions[MGMT_MEM_REGION_TYPE_MM_STATS];
			break;
		default:
			dev_err(&et_dev->pdev->dev,
				"ops_ioctl[%u]: invalid trace region type!\n",
				_IOC_NR(cmd));
			return -EINVAL;
		}

		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_OPS))
			return -EACCES;

		rv = region->size;
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

/**
 * esperanto_pcie_mgmt_poll() - Mgmt device poll operation
 *
 * EPOLLHUP: If mgmt device is not initialized
 * EPOLLOUT: If any SQ of mgmt device is available
 * EPOLLIN: If any CQ of mgmt device is available
 *
 * Return: Positive devnum value on success, negative error on failure
 */
static __poll_t esperanto_pcie_mgmt_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = EPOLLHUP;
	struct et_mgmt_dev *mgmt;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev, misc_dev);
	if (!mgmt->is_initialized)
		return mask;

	poll_wait(fp, &mgmt->vq_data.vq_common.waitqueue, wait);

	mutex_lock(&mgmt->vq_data.vq_common.sq_bitmap_mutex);
	mutex_lock(&mgmt->vq_data.vq_common.cq_bitmap_mutex);

	mask = 0;
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

/**
 * esperanto_pcie_vm_open() - VM open function for setting-up VMA mapping
 */
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

/**
 * esperanto_pcie_vm_close() - VM close function for tearing-down VMA mapping
 */
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

/**
 * et_find_vma() - Find VMA that provides mapping for the virtual address
 * @et_dev: Pointer to struct et_pci_dev
 * @vaddr: User virtual address mapped by VMA
 *
 * Return: Pointer to struct vm_area_struct containing mapping for virtual
 * address
 */
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

/**
 * esperanto_pcie_ops_mmap() - Esperanto PCIe mmap operation
 *
 * Allocates CMA buffer for the et_dev and mmap it into user virtual address
 * space
 *
 * Return: 0 on success, negative error on failure
 */
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

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)) && (!defined(RHEL_MAJOR) || (RHEL_MAJOR < 9))
	vma->vm_flags |= VM_DONTCOPY | VM_NORESERVE;
#else
	vm_flags_set(vma, VM_DONTCOPY | VM_NORESERVE);
#endif
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

/**
 * esperanto_pcie_mgmt_ioctl() - Mgmt device IOCTLs
 * @cmd: IOCTL commands
 * @arg: Argument passed from user-space
 *
 * This implements following IOCTLs for mgmt device:
 * - ETSOC1_IOCTL_GET_PCIBUS_DEVICE_NAME: Provides the PCI bus name of device.
 *   This IOCTL is functional regardless of the state of device
 * - ETSOC1_IOCTL_GET_DEVICE_STATE: Returns the current device state. This
 *   IOCTL is functional regardless of the state of device
 * - ETSOC1_IOCTL_FW_UPDATE: Writes the FW image on FW update scratch region
 *   with MMIOs
 * - ETSOC1_IOCTL_GET_SQ_COUNT: Provides mgmt device SQ count
 * - ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: Provides mgmt device SQ size
 * - ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION: Provides general device
 *   configuration received from the device in DIRs
 * - ETSOC1_IOCTL_PUSH_SQ: Forwards user command on SQ
 * - ETSOC1_IOCTL_POP_CQ: Pops out the response message from CQ to user
 * - ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: Provides SQ availability bitmap
 * - ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP: Provides CQ availability bitmap
 * - ETSOC1_IOCTL_SET_SQ_THRESHOLD: Sets SQ threshold for SQ availability
 * - ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE: Provies size trace buffer regions
 *   {SP, MM, CM, SP_STATS, MM_STATS} if region is defined by device
 * - ETSOC1_IOCTL_EXTRACT_TRACE_BUFFER: Extracts trace buffer regions using
 *   MMIOs for {SP, MM, CM, SP_STATS, MM_STATS} if region is defined by device
 *
 * Return: Non-negative value on success, negative error on failure
 */
static long esperanto_pcie_mgmt_ioctl(struct file *fp, unsigned int cmd,
				      unsigned long arg)
{
	long rv = 0;
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
	u16 max_size;
	u32 mgmt_state;
	u8 trace_type;
	void *trace_buf;

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

	case ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE:
		if (copy_from_user(&trace_type, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

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
		case TRACE_BUFFER_SP_STATS:
			region = &mgmt->regions[MGMT_MEM_REGION_TYPE_SP_STATS];
			break;
		case TRACE_BUFFER_MM_STATS:
			region = &mgmt->regions[MGMT_MEM_REGION_TYPE_MM_STATS];
			break;
		default:
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: invalid trace region type!\n",
				_IOC_NR(cmd));
			return -EINVAL;
		}

		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_MGMT))
			return -EACCES;

		rv = region->size;
		break;

	case ETSOC1_IOCTL_EXTRACT_TRACE_BUFFER:
		if (copy_from_user(&trace_info, usr_arg, _IOC_SIZE(cmd))) {
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy from user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

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
		case TRACE_BUFFER_SP_STATS:
			region = &mgmt->regions[MGMT_MEM_REGION_TYPE_SP_STATS];
			break;
		case TRACE_BUFFER_MM_STATS:
			region = &mgmt->regions[MGMT_MEM_REGION_TYPE_MM_STATS];
			break;
		default:
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: invalid trace region type!\n",
				_IOC_NR(cmd));
			return -EINVAL;
		}
		if (!region->is_valid || !(region->access.node_access &
					   MEM_REGION_NODE_ACCESSIBLE_MGMT))
			return -EACCES;

		trace_buf = kvmalloc(region->size, GFP_KERNEL);
		if (!trace_buf)
			return -ENOMEM;

		et_ioread(region->io.mapped_baseaddr, 0, trace_buf,
			  region->size);
		if (copy_to_user((char __user __force *)trace_info.buf,
				 trace_buf, region->size)) {
			kvfree(trace_buf);
			dev_err(&et_dev->pdev->dev,
				"mgmt_ioctl[%u]: failed to copy to user!\n",
				_IOC_NR(cmd));
			return -EFAULT;
		}

		kvfree(trace_buf);
		break;

	default:
		dev_err(&et_dev->pdev->dev, "ops_ioctl: unknown cmd: 0x%x\n",
			cmd);
		return -EINVAL;
	}

	return rv;
}

/**
 * esperanto_pcie_ops_open() - Ops device open operation
 *
 * The ops device node can only be accessed/opened by one process at a time. If
 * device is under reset, the user will not be allowed to access it and EPERM
 * error will be returned. For reset to be triggered, the device node must be
 * released first
 *
 * Return: 0 on success, negative error on failure
 */
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

/**
 * esperanto_pcie_mgmt_open() - Mgmt device open operation
 *
 * The mgmt device node can only be accessed/opened by one process at a time. If
 * device is under reset, the user will not be allowed to access it and EPERM
 * error will be returned. For reset to be triggered, the device node must be
 * released first
 *
 * Return: 0 on success, negative error on failure
 */
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

/**
 * esperanto_pcie_ops_release() - Ops device release operation
 *
 * Return: 0 on success, negative error on failure
 */
static int esperanto_pcie_ops_release(struct inode *inode, struct file *fp)
{
	struct et_ops_dev *ops;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_dev);
	spin_lock(&ops->open_lock);
	ops->is_open = false;
	spin_unlock(&ops->open_lock);

	return 0;
}

/**
 * esperanto_pcie_mgmt_release() - Mgmt device open operation
 *
 * If reset was requested before closing the mgmt device, then it will queue
 * work for reset ISR here
 *
 * Return: 0 on success, negative error on failure
 */
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
/* File operation for ops device */
static const struct file_operations et_pcie_ops_fops = {
	.owner			= THIS_MODULE,
	.poll			= esperanto_pcie_ops_poll,
	.unlocked_ioctl		= esperanto_pcie_ops_ioctl,
	.mmap			= esperanto_pcie_ops_mmap,
	.open			= esperanto_pcie_ops_open,
	.release		= esperanto_pcie_ops_release,
};

/* File operation for mgmt device */
static const struct file_operations et_pcie_mgmt_fops = {
	.owner			= THIS_MODULE,
	.poll			= esperanto_pcie_mgmt_poll,
	.unlocked_ioctl		= esperanto_pcie_mgmt_ioctl,
	.open			= esperanto_pcie_mgmt_open,
	.release		= esperanto_pcie_mgmt_release,
};

// clang-format on

/**
 * esperanto_pcie_error_detected() - Esperanto PCIe AER error detected callback
 * @pdev: Pointer to struct pci_dev
 * @state: value of enum pci_channel_state_t type received here
 *
 * Detects uncorrectable AER errors and uninitializes the device in case of
 * failure
 *
 * Return: pci_ers_result_t status code
 */
static pci_ers_result_t esperanto_pcie_error_detected(struct pci_dev *pdev,
						      pci_channel_state_t state)
{
	pci_ers_result_t rv;
	struct et_pci_dev *et_dev;

	dev_info(&pdev->dev, "PCI error: detected callback, state(%d)!\n",
		 state);

	switch (state) {
	case pci_channel_io_normal:
		rv = PCI_ERS_RESULT_CAN_RECOVER;
		break;
	/* TODO: SW-16311: Fatal error, prepare for slot reset
	 * A fix needed in kernel which is available in version >= 5.10.138,
	 * otherwise slot reset callback is not invoked
	 */
	case pci_channel_io_frozen:
	/* Permanent error, prepare for device removal */
	case pci_channel_io_perm_failure:
	default:
		rv = PCI_ERS_RESULT_DISCONNECT;
		et_dev = pci_get_drvdata(pdev);
		if (!et_dev)
			break;
		et_ops_dev_destroy(et_dev, false);
		et_mgmt_dev_destroy(et_dev, false);
	}

	return rv;
}

/**
 * esperanto_pcie_mmio_enabled() - Esperanto PCIe AER MMIO enabled callback
 * @pdev: Pointer to struct pci_dev
 *
 * This is called only if esperanto_pcie_error_detected returns
 * PCI_ERS_RESULT_CAN_RECOVER. Read/write to the device still works, no need to
 * reset slot.
 *
 * Return: pci_ers_result_t status code
 */
static pci_ers_result_t esperanto_pcie_mmio_enabled(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "PCI error: mmio enabled callback!\n");

	/* TODO: Dump whatever for debugging purposes, only MMIOs are enabled
	 * here but not DMA.
	 */

	return PCI_ERS_RESULT_RECOVERED;
}

/**
 * esperanto_pcie_slot_reset() - Esperanto PCIe AER slot reset callback
 * @pdev: Pointer to struct pci_dev
 *
 * This is called only if esperanto_pcie_error_detected returns
 * PCI_ERS_RESULT_NEED_RESET and slot is successfully reset
 *
 * Return: pci_ers_result_t status code
 */
static pci_ers_result_t esperanto_pcie_slot_reset(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "PCI error: slot reset callback!\n");

	return PCI_ERS_RESULT_DISCONNECT;
}

/**
 * esperanto_pcie_resume() - Esperanto PCIe AER resume callback
 * @pdev: Pointer to struct pci_dev
 *
 * A callback to resume the functionality. If all previous steps completed
 * successfully then this callback can be used to resume the normal operations
 *
 * Return: pci_ers_result_t status code
 */
static void esperanto_pcie_resume(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "PCI error: resume callback!\n");
}

// clang-format off
/* PCIe advanced error recovery callbacks */
static const struct pci_error_handlers et_pcie_err_handler = {
	.error_detected	= esperanto_pcie_error_detected,
	.mmio_enabled	= esperanto_pcie_mmio_enabled,
	.slot_reset	= esperanto_pcie_slot_reset,
	.resume		= esperanto_pcie_resume,
};

// clang-format on

/**
 * create_et_pci_dev() - Create struct et_pci_dev and initialize the fields
 * @new_dev: Created device pointer to be returned here
 * @pdev: Pointer to struct pci_dev
 *
 * Return: 0 on success, negative error on failure
 */
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

/**
 * et_destroy_region_list() - Destroy/Free the nodes of region list
 * @et_dev: Pointer to struct et_pci_dev
 * @is_mgmt: indicates if mgmt or ops device
 */
static void et_destroy_region_list(struct et_pci_dev *et_dev, bool is_mgmt)
{
	struct et_bar_region *pos, *tmp;

	list_for_each_entry_safe (pos, tmp, &et_dev->bar_region_list, list) {
		if (pos->is_mgmt == is_mgmt) {
			list_del(&pos->list);
			kfree(pos);
		}
	}
}

/**
 * et_unmap_discovered_regions() - Unmap all the discovered BAR regions
 * @et_dev: Pointer to struct et_pci_dev
 * @is_mgmt: indicates if mgmt or ops device
 */
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
			if (regions[i].access.p2p_access)
				et_p2pdma_release_resource(et_dev, &regions[i]);
			else
				et_unmap_bar(regions[i].io.mapped_baseaddr);
			regions[i].is_valid = false;
		}
	}

	et_destroy_region_list(et_dev, is_mgmt);
}

/**
 * et_map_discovered_regions() - Discover and map the discovered BAR regions
 * @et_dev: Pointer to struct et_pci_dev
 * @is_mgmt: indicates if mgmt or ops device
 * @regs_data: DIRs data copy
 * @regs_size: DIRs data size in bytes
 * @regs_count: DIRs memory regions count
 *
 * Return: Number of bytes parsed on success, negative error on failure
 */
static ssize_t et_map_discovered_regions(struct et_pci_dev *et_dev,
					 bool is_mgmt, u8 *regs_data,
					 size_t regs_size, int regs_count)
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

	memset(regions, 0, num_reg_types * sizeof(*regions));
	for (i = 0; i < regs_count; i++, reg_pos += section_size) {
		dir_mem_region = (struct et_dir_mem_region *)reg_pos;
		section_size = dir_mem_region->attributes_size;

		// End of region check
		if (reg_pos + section_size > regs_data + regs_size) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc = "DIR region exceeded DIR total size!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\n"
				"Region type: %d\n"
				"Region end - DIR end: %zd)\n",
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
				"\nDevice: %s\n"
				"Region type: %d\n",
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
				"\nDevice: %s\n"
				"Region type: %d\n"
				"Size: (expected: %zu < discovered: %zu)\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type, sizeof(*dir_mem_region),
				section_size);
			et_print_event(et_dev->pdev, &dbg_msg);
		} else if (section_size < sizeof(*dir_mem_region)) {
			dbg_msg.level = LEVEL_FATAL;
			dbg_msg.desc =
				"DIR region does not have enough attributes!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\n"
				"Region type: %d\n"
				"Size: (expected: %zu > discovered: %zu)\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type, sizeof(*dir_mem_region),
				section_size);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// Region attributes validity check
		if (!valid_mem_region(dir_mem_region, is_mgmt, dbg_msg.syndrome,
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
				"\nDevice: %s\n"
				"Region type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}

		// Skip BAR mapping of region if IO/P2P access is disabled by
		// device
		if (!dir_mem_region->access.io_access &&
		    !dir_mem_region->access.p2p_access) {
			regions[dir_mem_region->type].is_valid = true;
			regions[dir_mem_region->type].io.host_phys_addr =
				pci_resource_start(et_dev->pdev, bm_info.bar) +
				bm_info.bar_offset;
			regions[dir_mem_region->type].io.mapped_baseaddr = NULL;
			regions[dir_mem_region->type].dev_phys_addr =
				dir_mem_region->dev_address;
			regions[dir_mem_region->type].size =
				dir_mem_region->bar_size;
			memcpy(&regions[dir_mem_region->type].access,
			       (u8 *)&dir_mem_region->access,
			       sizeof(dir_mem_region->access));
			continue;
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
			goto error_free_new_node;
		}

		list_for_each_entry (existing_node, &et_dev->bar_region_list,
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
					"\nExisting region info:\n"
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
					new_node->bar, new_node->region_type,
					new_node->region_start,
					new_node->region_end);
				et_print_event(et_dev->pdev, &dbg_msg);
				rv = -EINVAL;
				goto error_free_new_node;
			}
		}

		if (dir_mem_region->access.io_access &&
		    dir_mem_region->access.p2p_access) {
			dbg_msg.level = LEVEL_WARN;
			dbg_msg.desc =
				"DIR discovered region enables both IO and P2P, choosing IO!";
			sprintf(dbg_msg.syndrome,
				"\nDevice: %s\nRegion type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops",
				dir_mem_region->type);
			et_print_event(et_dev->pdev, &dbg_msg);
		}

		// BAR mapping for the discovered region
		bm_info.bar = dir_mem_region->bar;
		bm_info.bar_offset = dir_mem_region->bar_offset;
		bm_info.size = dir_mem_region->bar_size;

		// Prioritize IO access If both IO and P2P accesses are enabled
		if (dir_mem_region->access.io_access) {
			rv = et_map_bar(et_dev, &bm_info,
					&regions[dir_mem_region->type]
						 .io.mapped_baseaddr);
			if (rv) {
				dbg_msg.level = LEVEL_FATAL;
				dbg_msg.desc =
					"DIR discovered region mapping failed!";
				sprintf(dbg_msg.syndrome,
					"\nDevice: %s\n"
					"Region type: %d\n",
					(is_mgmt) ? "Mgmt" : "Ops",
					dir_mem_region->type);
				et_print_event(et_dev->pdev, &dbg_msg);
				goto error_free_new_node;
			}
			regions[dir_mem_region->type].io.host_phys_addr =
				pci_resource_start(et_dev->pdev, bm_info.bar) +
				bm_info.bar_offset;
		} else if (dir_mem_region->access.p2p_access) {
			if (bm_info.size != round_down(bm_info.size, SZ_2M)) {
				dbg_msg.level = LEVEL_FATAL;
				dbg_msg.desc =
					"DIR discovered P2P region is not 2MB aligned!";
				sprintf(dbg_msg.syndrome,
					"\nDevice: %s\n"
					"Region type: %d\n",
					(is_mgmt) ? "Mgmt" : "Ops",
					dir_mem_region->type);
				et_print_event(et_dev->pdev, &dbg_msg);
				rv = -EINVAL;
				goto error_free_new_node;
			}
			rv = et_p2pdma_add_resource(
				et_dev, &bm_info,
				&regions[dir_mem_region->type]);
			if (rv) {
				dbg_msg.level = LEVEL_FATAL;
				dbg_msg.desc =
					"DIR discovered P2P region mapping failed!";
				sprintf(dbg_msg.syndrome,
					"\nDevice: %s\n"
					"Region type: %d\n",
					(is_mgmt) ? "Mgmt" : "Ops",
					dir_mem_region->type);
				et_print_event(et_dev->pdev, &dbg_msg);
				goto error_free_new_node;
			}
		}

		// Save other region information
		regions[dir_mem_region->type].size = dir_mem_region->bar_size;
		regions[dir_mem_region->type].dev_phys_addr =
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
				"\nDevice: %s\n"
				"Region type: %d\n",
				(is_mgmt) ? "Mgmt" : "Ops", i);
			et_print_event(et_dev->pdev, &dbg_msg);
			rv = -EINVAL;
			goto error_unmap_discovered_regions;
		}
	}

	if (compul_reg_count + non_compul_reg_count != num_reg_types) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"DIRs expected number doesn't match discovered number of memory regions!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: %s\n"
			"Expected regions: %d\n"
			"Discovered compulsory regions: %d\n"
			"Discovered non-compulsory regions: %d\n",
			(is_mgmt) ? "Mgmt" : "Ops", num_reg_types,
			compul_reg_count, non_compul_reg_count);
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	return (ssize_t)((u64)reg_pos - (u64)regs_data);

error_free_new_node:
	kfree(new_node);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, is_mgmt);

	return rv;
}

/**
 * et_mgmt_dev_init() - Initialize mgmt device
 * @et_dev: Pointer to struct et_pci_dev
 * @timeout_secs: Device discovery timeout
 * @miscdev_create: Create miscellaneous character device node
 *
 * Return: 0 on success, negative error on failure
 */
int et_mgmt_dev_init(struct et_pci_dev *et_dev, u32 timeout_secs,
		     bool miscdev_create)
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

	mutex_lock(&et_dev->mgmt.init_mutex);

	if (et_dev->mgmt.is_initialized) {
		dev_info(&et_dev->pdev->dev,
			 "Mgmt: Device is already initialized\n");
		rv = 0;
		goto error_unlock_init_mutex;
	}

	et_dev->mgmt.is_open = false;
	spin_lock_init(&et_dev->mgmt.open_lock);

	// Map DIR region
	rv = et_map_bar(et_dev, &DIR_MAPPINGS[IOMEM_R_DIR_MGMT],
			&et_dev->mgmt.dir);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Mgmt: DIR mapping failed\n");
		goto error_unlock_init_mutex;
	}

	dir_mgmt_mem = (struct et_mgmt_dir_header __iomem *)et_dev->mgmt.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for mgmt_discovery_timeout
	for (i = 0; !dir_ready && i <= timeout_secs; i++) {
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
			"\nDevice: Mgmt\n"
			"Boot status: %d\n",
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
			"\nDevice: Mgmt\n"
			"Size: (Max allowed: %llu, discovered: %zu)\n",
			DIR_MAPPINGS[IOMEM_R_DIR_MGMT].size, dir_size);
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

	// DIR version check
	if (dir_mgmt->version != MGMT_DIR_VERSION) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc = "DIR version mismatch found!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\n"
			"Region: DIR header\n"
			"Version: (expected: %u != discovered: %u)\n",
			MGMT_DIR_VERSION, dir_mgmt->version);
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	// BAR0 size check
	if (dir_mgmt->bar0_size !=
	    pci_resource_len(et_dev->pdev, 0 /* BAR0 */)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"BAR0 size doesn't match BAR0 size exposed by DIRs!";
		sprintf(dbg_msg.syndrome,
			"BAR0 size detected by host: 0x%llx\n"
			"BAR0 size exposed by DIRs: 0x%llx\n",
			pci_resource_len(et_dev->pdev, 0), dir_mgmt->bar0_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		// TODO: Enable back the following to stop discovery if BAR size
		// check fail. Currently setting it to optional with LEVEL_WARN
		// because silicon machines intermittently see failure reading
		// correct BAR sizes from PCI config space.
		//rv = -EINVAL;
		//goto error_unmap_dir_region;
	}

	// BAR2 size check
	if (dir_mgmt->bar2_size !=
	    pci_resource_len(et_dev->pdev, 2 /* BAR2 */)) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"BAR2 size doesn't match BAR2 size exposed by DIRs!";
		sprintf(dbg_msg.syndrome,
			"BAR2 size detected by host: 0x%llx\n"
			"BAR2 size exposed by DIRs: 0x%llx\n",
			pci_resource_len(et_dev->pdev, 2), dir_mgmt->bar2_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		// TODO: Enable back the following to stop discovery if BAR size
		// check fail. Currently setting it to optional with LEVEL_WARN
		// because silicon machines intermittently see failure reading
		// correct BAR sizes from PCI config space.
		//rv = -EINVAL;
		//goto error_unmap_dir_region;
	}

	// End of region check
	if (dir_pos + section_size > dir_data + dir_size) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region exceeded DIR total size!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\n"
			"Region: DIR header\n"
			"Region end - DIR end: %zd)\n",
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
			"\nDevice: Mgmt\n"
			"Region: DIR header\n"
			"Size: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_mgmt), section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_mgmt)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\n"
			"Region: DIR header\n"
			"Size: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_mgmt), section_size);
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
			"\nDevice: Mgmt\n"
			"CRC: (expected: %x, calculated: %x)\n",
			dir_mgmt->crc32, crc32_result);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	// Read the configuration values
	et_dev->cfg.form_factor =
		(enum dev_config_form_factor)dir_mgmt->form_factor;
	et_dev->cfg.tdp = dir_mgmt->device_tdp;
	et_dev->cfg.total_l3_size = dir_mgmt->l3_size;
	et_dev->cfg.total_l2_size = dir_mgmt->l2_size;
	et_dev->cfg.total_scp_size = dir_mgmt->scp_size;
	et_dev->cfg.cache_line_size = dir_mgmt->cache_line_size;
	et_dev->cfg.minion_boot_freq = dir_mgmt->minion_boot_freq;
	et_dev->cfg.cm_shire_mask = dir_mgmt->cm_shires_mask;
	et_dev->cfg.ddr_bandwidth = dir_mgmt->ddr_bandwidth;
	et_dev->cfg.num_l2_cache_banks = dir_mgmt->l2_shire_banks;
	et_dev->cfg.sync_min_shire_id = dir_mgmt->sync_min_shire_id;
	et_dev->cfg.arch_rev =
		(enum dev_config_arch_revision)dir_mgmt->arch_revision;
	et_dev->cfg.devnum = et_dev->devnum;

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
			"\nDevice: Mgmt\n"
			"Region: VQ Region\n"
			"Region end - DIR end: %zd)\n",
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
			"\nDevice: Mgmt\n"
			"Region: VQ Region\n"
			"Size: (expected: %zu < discovered: %zu)\n",
			sizeof(*dir_vq_mgmt), section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_vq_mgmt)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\n"
			"Region: VQ Region\n"
			"Size: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_vq_mgmt), section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	if (!valid_mgmt_vq_region(dir_vq_mgmt, dbg_msg.syndrome,
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

	/*
	 * Map all memory regions and save attributes
	 */
	regs_size = dir_size - ((u64)dir_pos - (u64)dir_data);
	rv = et_map_discovered_regions(et_dev, true /* mgmt_dev */, dir_pos,
				       regs_size, dir_mgmt->num_regions);
	if (rv < 0) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: DIR Memory Regions mapping failed!");
		goto error_free_dir_data;
	}

	et_print_mgmt_dir(&et_dev->pdev->dev, dir_data, dir_size,
			  et_dev->mgmt.regions);

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"Total DIR size doesn't match sum of respective sizes of attribute regions";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Mgmt\n"
			"Size: (DIRs: %zu, All Attribute Regions: %d)\n",
			dir_size, (int)(dir_pos - dir_data));
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	// SysFs error statistics initialization
	rv = et_sysfs_add_group(et_dev, ET_SYSFS_GID_ERR_STATS);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: et_sysfs_add_group() failed, group_id: %d\n",
			ET_SYSFS_GID_ERR_STATS);
		goto error_unmap_discovered_regions;
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
				"Mgmt: misc_register() failed!\n");
			goto error_vqueue_destroy_all;
		}
		et_dev->mgmt.miscdev_created = true;
	}

	kfree(dir_data);

	et_dev->mgmt.is_initialized = true;
	mutex_unlock(&et_dev->mgmt.init_mutex);

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

error_sysfs_remove_group:
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_ERR_STATS);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, true /* mgmt_dev */);

error_free_dir_data:
	kfree(dir_data);

error_unmap_dir_region:
	et_unmap_bar(et_dev->mgmt.dir);

error_unlock_init_mutex:
	mutex_unlock(&et_dev->mgmt.init_mutex);

	return rv;
}

/**
 * et_mgmt_dev_destroy() - Destroy/Uninitialize mgmt device
 * @et_dev: Pointer to struct et_pci_dev
 * @miscdev_destroy: miscellaneous character device node to be deleted
 *
 * Return: 0 on success, negative error on failure
 */
void et_mgmt_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy)
{
	mutex_lock(&et_dev->mgmt.init_mutex);

	if (miscdev_destroy && et_dev->mgmt.miscdev_created) {
		misc_deregister(&et_dev->mgmt.misc_dev);
		et_dev->mgmt.miscdev_created = false;
	}

	if (!et_dev->mgmt.is_initialized)
		goto unlock_init_mutex;

	et_dev->mgmt.is_initialized = false;
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_ERR_STATS);
	et_unmap_discovered_regions(et_dev, true /* mgmt_dev */);
	et_unmap_bar(et_dev->mgmt.dir);

unlock_init_mutex:
	mutex_unlock(&et_dev->mgmt.init_mutex);
}

/**
 * et_ops_dev_init() - Initialize ops device
 * @et_dev: Pointer to struct et_pci_dev
 * @timeout_secs: Device discovery timeout
 * @miscdev_create: Create miscellaneous device node
 *
 * Return: 0 on success, negative error on failure
 */
int et_ops_dev_init(struct et_pci_dev *et_dev, u32 timeout_secs,
		    bool miscdev_create)
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

	mutex_lock(&et_dev->ops.init_mutex);

	if (et_dev->ops.is_initialized) {
		dev_info(&et_dev->pdev->dev,
			 "Ops: Device is already initialized\n");
		rv = 0;
		goto error_unlock_init_mutex;
	}

	et_dev->ops.is_open = false;
	spin_lock_init(&et_dev->ops.open_lock);

	// Map DIR region
	rv = et_map_bar(et_dev, &DIR_MAPPINGS[IOMEM_R_DIR_OPS],
			&et_dev->ops.dir);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "Ops: DIR mapping failed\n");
		goto error_unlock_init_mutex;
	}

	dir_ops_mem = (struct et_ops_dir_header __iomem *)et_dev->ops.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for ops_discovery_timeout
	for (i = 0; !dir_ready && i <= timeout_secs; i++) {
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
		sprintf(dbg_msg.syndrome, "\nDevice: Ops\nBoot status: %d\n",
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
			DIR_MAPPINGS[IOMEM_R_DIR_OPS].size, dir_size);
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

	// DIR version check
	if (dir_ops->version != OPS_DIR_VERSION) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc = "DIR version mismatch found!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\n"
			"Region: DIR header\n"
			"Version: (expected: %u != discovered: %u)\n",
			OPS_DIR_VERSION, dir_ops->version);
		et_print_event(et_dev->pdev, &dbg_msg);
	}

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
			sizeof(*dir_ops), section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_ops)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: DIR header\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_ops), section_size);
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
			dir_ops->crc32, crc32_result);
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
			sizeof(*dir_vq_ops), section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
	} else if (section_size < sizeof(*dir_vq_ops)) {
		dbg_msg.level = LEVEL_FATAL;
		dbg_msg.desc = "DIR region does not have enough attributes!";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\nRegion: VQ Region\nSize: (expected: %zu > discovered: %zu)\n",
			sizeof(*dir_vq_ops), section_size);
		et_print_event(et_dev->pdev, &dbg_msg);
		rv = -EINVAL;
		goto error_free_dir_data;
	}

	if (!valid_ops_vq_region(dir_vq_ops, dbg_msg.syndrome,
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

	/*
	 * Map all memory regions and save attributes
	 */
	regs_size = dir_size - ((u64)dir_pos - (u64)dir_data);
	rv = et_map_discovered_regions(et_dev, false /* ops_dev */, dir_pos,
				       regs_size, dir_ops->num_regions);
	if (rv < 0) {
		dev_err(&et_dev->pdev->dev,
			"Ops: DIR Memory Regions mapping failed!");
		goto error_free_dir_data;
	}

	et_print_ops_dir(&et_dev->pdev->dev, dir_data, dir_size,
			 et_dev->ops.regions);

	dir_pos += rv;
	if (dir_pos != dir_data + dir_size) {
		dbg_msg.level = LEVEL_WARN;
		dbg_msg.desc =
			"Total DIR size doesn't match sum of respective sizes of attribute regions";
		sprintf(dbg_msg.syndrome,
			"\nDevice: Ops\n"
			"Size: (DIRs: %zu, All Attribute Regions: %d)\n",
			dir_size, (int)(dir_pos - dir_data));
		et_print_event(et_dev->pdev, &dbg_msg);
	}

	// SysFs memory statistics initialization
	rv = et_sysfs_add_group(et_dev, ET_SYSFS_GID_MEM_STATS);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops: et_sysfs_add_group() failed, group_id: %d\n",
			ET_SYSFS_GID_MEM_STATS);
		goto error_unmap_discovered_regions;
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
				"Ops: misc_register() failed!\n");
			goto error_vqueue_destroy_all;
		}
		et_dev->ops.miscdev_created = true;
	}

	kfree(dir_data);

	et_dev->ops.is_initialized = true;
	mutex_unlock(&et_dev->ops.init_mutex);

	return rv;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

error_sysfs_remove_group:
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_MEM_STATS);

error_unmap_discovered_regions:
	et_unmap_discovered_regions(et_dev, false /* ops_dev */);

error_free_dir_data:
	kfree(dir_data);

error_unmap_dir_region:
	et_unmap_bar(et_dev->ops.dir);

error_unlock_init_mutex:
	mutex_unlock(&et_dev->ops.init_mutex);

	return rv;
}

/**
 * et_ops_dev_destroy() - Destroy/Uninitialize ops device
 * @et_dev: Pointer to struct et_pci_dev
 * @miscdev_destroy: miscellaneous character device node to be deleted
 *
 * Return: 0 on success, negative error on failure
 */
void et_ops_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy)
{
	mutex_lock(&et_dev->ops.init_mutex);

	if (miscdev_destroy && et_dev->ops.miscdev_created) {
		misc_deregister(&et_dev->ops.misc_dev);
		et_dev->ops.miscdev_created = false;
	}

	if (!et_dev->ops.is_initialized)
		goto unlock_init_mutex;

	et_dev->ops.is_initialized = false;
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);
	et_sysfs_remove_group(et_dev, ET_SYSFS_GID_MEM_STATS);
	et_unmap_discovered_regions(et_dev, false /* ops_dev */);
	et_unmap_bar(et_dev->ops.dir);

unlock_init_mutex:
	mutex_unlock(&et_dev->ops.init_mutex);
}

/**
 * destroy_et_pci_dev() - Delete struct et_pci_dev object
 * @et_dev: Pointer to struct et_pci_dev
 */
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

/**
 * init_et_pci_dev() - Initialize ETSoC1 PCI device
 * @et_dev: Pointer to struct et_pci_dev
 * @miscdev_create: Create miscellaneous character device node
 *
 * Performs initialization steps generic to both mgmt and ops devices
 *
 * Return: 0 on success, negative error on failure
 */
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

	// Device Management:
	//  - Vector[0] - Mgmt SQ(s)
	//  - Vector[1] - Mgmt CQ(s)
	// Device Operations:
	//  - Vector[2] - Ops SQ(s)
	//  - Vector[3] - Ops CQ(s)
	rv = pci_alloc_irq_vectors(pdev, ET_MAX_MSI_VECS, ET_MAX_MSI_VECS,
				   PCI_IRQ_MSI);
	if (rv != ET_MAX_MSI_VECS) {
		dev_err(&pdev->dev, "msi vectors=%d alloc failed\n",
			ET_MAX_MSI_VECS);
		goto error_clear_master;
	}

	rv = pci_request_regions(pdev, DRIVER_NAME);
	if (rv) {
		dev_err(&pdev->dev, "request regions failed\n");
		goto error_free_irq_vectors;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) && (!defined(RHEL_MAJOR) || (RHEL_MAJOR < 9))
	rv = pci_enable_pcie_error_reporting(pdev);
	if (rv) {
		dev_warn(&pdev->dev,
			 "Couldn't enable PCI error reporting, errno: %d\n",
			 -rv);
		et_dev->is_err_reporting = false;
		rv = 0;
	} else {
		et_dev->is_err_reporting = true;
	}
#endif

	rv = et_mgmt_dev_init(et_dev, mgmt_discovery_timeout, miscdev_create);
	if (rv) {
		dev_err(&pdev->dev, "Mgmt device initialization failed\n");
		goto error_pci_disable_pcie_error_reporting;
	}

	rv = et_ops_dev_init(et_dev, ops_discovery_timeout, miscdev_create);
	if (rv) {
		dev_warn(&pdev->dev,
			 "Ops device initialization failed, errno: %d\n", -rv);
		rv = 0;
	}

	et_dev->is_initialized = true;

	return rv;

error_pci_disable_pcie_error_reporting:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) && (!defined(RHEL_MAJOR) || (RHEL_MAJOR < 9))
	if (et_dev->is_err_reporting) {
		pci_disable_pcie_error_reporting(pdev);
		et_dev->is_err_reporting = false;
	}
#endif

	pci_release_regions(pdev);

error_free_irq_vectors:
	pci_free_irq_vectors(pdev);

error_clear_master:
	pci_clear_master(pdev);

error_disable_dev:
	pci_disable_device(pdev);

	return rv;
}

/**
 * uninit_et_pci_dev() - Un-Initialize ETSoC1 PCI device
 * @et_dev: Pointer to struct et_pci_dev
 * @miscdev_destroy: miscellaneous character device node to be deleted
 *
 * Performs un-initialization steps generic to both mgmt and ops devices
 *
 * Return: 0 on success, negative error on failure
 */
static void uninit_et_pci_dev(struct et_pci_dev *et_dev, bool miscdev_destroy)
{
	struct pci_dev *pdev = et_dev->pdev;

	et_ops_dev_destroy(et_dev, miscdev_destroy);
	et_mgmt_dev_destroy(et_dev, miscdev_destroy);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) && (!defined(RHEL_MAJOR) || (RHEL_MAJOR < 9))
	if (et_dev->is_err_reporting) {
		pci_disable_pcie_error_reporting(pdev);
		et_dev->is_err_reporting = false;
	}
#endif

	if (!et_dev->is_initialized)
		return;

	et_dev->is_initialized = false;
	pci_release_regions(pdev);
	pci_free_irq_vectors(pdev);
	pci_clear_master(pdev);
	pci_disable_device(pdev);
}

/**
 * et_reset_isr_work() - ETSOC reset work ISR
 * @work: Pointer to struct work_struct
 */
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

/**
 * esperanto_pcie_probe() - Esperanto PCIe probe function
 * @pdev: Pointer to struct pci_dev
 *
 * Return: 0 on success, negative error on failure
 */
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

	if (!pci_device_is_present(et_dev->pdev)) {
		dev_err(&pdev->dev, "PCIe device is inaccessible\n");
		rv = -ENODEV;
		goto error_destroy_et_pci_dev;
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

error_destroy_et_pci_dev:
	destroy_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);

	return rv;
}

/**
 * esperanto_pcie_probe() - Esperanto PCIe remove function
 * @pdev: Pointer to struct pci_dev
 */
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
	.err_handler	= &et_pcie_err_handler,
};

// clang-format on

module_pci_driver(et_pcie_driver);
