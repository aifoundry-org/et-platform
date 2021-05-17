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
#include "et_fw_update.h"
#include "et_pci_dev.h"
#include "et_event_handler.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe loopback device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME "Esperanto"

#define PCI_VENDOR_ID_REDHAT		0x1b36
#define PCI_DEVICE_ID_REDHAT_TEST	0x0005
/* Define Vendor ID and Device ID to utilize QEMU PCI test device */
#define ET_PCIE_VENDOR_ID		PCI_VENDOR_ID_REDHAT
#define ET_PCIE_TEST_DEVICE_ID		PCI_DEVICE_ID_REDHAT_TEST
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

#define DMA_MAX_ALLOC_SIZE	(BIT(31)) /* 2GB */

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

static long esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd,
				     unsigned long arg)
{
	struct et_pci_dev *et_dev;
	struct et_ops_dev *ops;
	struct dram_info user_dram;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
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

		user_dram.base = ops->regions
			[OPS_MEM_REGION_TYPE_HOST_MANAGED].soc_addr;
		user_dram.size = ops->regions
			[OPS_MEM_REGION_TYPE_HOST_MANAGED].size;
		user_dram.dma_max_alloc_size = DMA_MAX_ALLOC_SIZE;
		// TODO: Discover from DIRs
		user_dram.dma_max_elem_size = BIT(27); /* 128MB */

		switch (ops->regions
			[OPS_MEM_REGION_TYPE_HOST_MANAGED].access.dma_align) {
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
		    copy_to_user((struct dram_info *)arg, &user_dram, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_INFO: failed to copy to user\n");
			return -ENOMEM;
		}

		return 0;

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (size >= sizeof(u16) &&
		    copy_to_user((u16 *)arg, &ops->vq_common.dir_vq.sq_count,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = ops->vq_common.dir_vq.per_sq_size -
			   sizeof(struct et_circbuffer);
		if (size >= sizeof(u16) &&
		    copy_to_user((u16 *)arg, &max_size, size)) {
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
			return et_dma_move_data(et_dev, cmd_info.sq_index,
						(void __user *)cmd_info.cmd,
						cmd_info.size);
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
		if (size >= sizeof(u64) &&
		    copy_to_user((u64 *)arg, ops->vq_common.sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (size >= sizeof(u64) &&
		    copy_to_user((u64 *)arg, ops->vq_common.cq_bitmap,
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

static void esperanto_pcie_vm_open(struct vm_area_struct *vma)
{
        struct et_dma_mapping *map = vma->vm_private_data;

        dev_dbg(&map->pdev->dev, "vm_open: %p, [size=%lu,vma=%08lx-%08lx]\n",
		map, map->size, vma->vm_start, vma->vm_end);

        map->ref_count++;
}

static void esperanto_pcie_vm_close(struct vm_area_struct *vma)
{
	struct et_dma_mapping *map = vma->vm_private_data;

	if (!map)
		return;

	dev_dbg(&map->pdev->dev, "vm_close: %p, [size=%lu,vma=%08lx-%08lx]\n",
		map, map->size, vma->vm_start, vma->vm_end);

	map->ref_count--;
	if (map->ref_count == 0) {
		dma_free_coherent(&map->pdev->dev, map->size, map->kern_vaddr,
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

	kern_vaddr = dma_alloc_coherent(&et_dev->pdev->dev, size, &dma_addr,
					GFP_USER);
	if (!kern_vaddr) {
		dev_err(&et_dev->pdev->dev, "dma_alloc_coherent() failed!\n");
		return -ENOMEM;
	}

	rv = remap_pfn_range(vma, vma->vm_start,
			     page_to_pfn(virt_to_page(kern_vaddr)),
			     vma_pages(vma) << PAGE_SHIFT, vma->vm_page_prot);
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

static long esperanto_pcie_mgmt_ioctl(struct file *fp, unsigned int cmd,
				      unsigned long arg)
{
	struct et_mgmt_dev *mgmt;
	struct et_pci_dev *et_dev;
	struct cmd_desc cmd_info;
	struct rsp_desc rsp_info;
	struct sq_threshold sq_threshold_info;
	struct fw_update_desc fw_update_info;
	u16 sq_idx;
	size_t size;
	u16 max_size;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_FW_UPDATE:
		if (copy_from_user(&fw_update_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;

		if (!fw_update_info.ubuf || !fw_update_info.size)
			return -EINVAL;

		return et_mmio_write_fw_image
			(et_dev, (void __user *)fw_update_info.ubuf,
			 fw_update_info.size);

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (size >= sizeof(u16) &&
		    copy_to_user((u16 *)arg, &mgmt->vq_common.dir_vq.sq_count,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = mgmt->vq_common.dir_vq.per_sq_size -
			   sizeof(struct et_circbuffer);
		if (size >= sizeof(u16) &&
		    copy_to_user((u16 *)arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_ACTIVE_SHIRE:
		if (size >= sizeof(u64) &&
		    copy_to_user((u64 *)arg, &mgmt->minion_shires, size)) {
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
		if (size >= sizeof(u64) &&
		    copy_to_user((u64 *)arg, mgmt->vq_common.sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (size >= sizeof(u64) &&
		    copy_to_user((u64 *)arg, mgmt->vq_common.cq_bitmap,
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
	.poll = esperanto_pcie_ops_poll,
	.unlocked_ioctl = esperanto_pcie_ops_ioctl,
	.mmap = esperanto_pcie_ops_mmap,
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

	return 0;
}

static int et_mgmt_dev_init(struct et_pci_dev *et_dev)
{
	int rv;

	et_dev->mgmt.is_mgmt_open = false;
	spin_lock_init(&et_dev->mgmt.mgmt_open_lock);

	et_dev->mgmt.minion_shires = 0;
	et_dev->mgmt.vq_common.dir_vq.sq_count = 1;
	et_dev->mgmt.vq_common.dir_vq.per_sq_size = 0x700UL;
	et_dev->mgmt.vq_common.dir_vq.sq_offset = 0;
	et_dev->mgmt.vq_common.dir_vq.cq_count = 1;
	et_dev->mgmt.vq_common.dir_vq.per_cq_size = 0x700UL;
	et_dev->mgmt.vq_common.dir_vq.cq_offset =
		et_dev->mgmt.vq_common.dir_vq.sq_offset +
		et_dev->mgmt.vq_common.dir_vq.sq_count *
		et_dev->mgmt.vq_common.dir_vq.per_sq_size;

	et_dev->mgmt.regions
		[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = true;
	et_dev->mgmt.regions
		[MGMT_MEM_REGION_TYPE_VQ_BUFFER].access.priv_mode =
		MEM_REGION_PRIVILEGE_MODE_KERNEL;
	et_dev->mgmt.regions
		[MGMT_MEM_REGION_TYPE_VQ_BUFFER].access.node_access =
		MEM_REGION_NODE_ACCESSIBLE_NONE;
	et_dev->mgmt.regions
		[MGMT_MEM_REGION_TYPE_VQ_BUFFER].access.dma_align =
		MEM_REGION_DMA_ALIGNMENT_NONE;
	et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER].size =
		et_dev->mgmt.vq_common.dir_vq.sq_count *
		et_dev->mgmt.vq_common.dir_vq.per_sq_size +
		et_dev->mgmt.vq_common.dir_vq.cq_count *
		et_dev->mgmt.vq_common.dir_vq.per_cq_size;
	et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER].mapped_baseaddr =
		kzalloc(et_dev->mgmt.regions
			[MGMT_MEM_REGION_TYPE_VQ_BUFFER].size, GFP_KERNEL);

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt: VQs initialization failed\n");
		goto error_free_vq_buffer;
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

error_free_vq_buffer:
	kfree(et_dev->mgmt.regions
	      [MGMT_MEM_REGION_TYPE_VQ_BUFFER].mapped_baseaddr);
	et_dev->mgmt.regions
		[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

	return rv;
}

static void et_mgmt_dev_destroy(struct et_pci_dev *et_dev)
{
	misc_deregister(&et_dev->mgmt.misc_mgmt_dev);
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

	kfree(et_dev->mgmt.regions
	      [MGMT_MEM_REGION_TYPE_VQ_BUFFER].mapped_baseaddr);
	et_dev->mgmt.regions
		[MGMT_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;
}

static int et_ops_dev_init(struct et_pci_dev *et_dev)
{
	int rv;

	et_dev->ops.is_ops_open = false;
	spin_lock_init(&et_dev->ops.ops_open_lock);

	// Init DMA rbtree
	mutex_init(&et_dev->ops.dma_rbtree_mutex);
	et_dev->ops.dma_rbtree = RB_ROOT;

	et_dev->ops.vq_common.dir_vq.sq_count = 2;
	et_dev->ops.vq_common.dir_vq.per_sq_size = 0x400UL;
	et_dev->ops.vq_common.dir_vq.sq_offset = 0;
	et_dev->ops.vq_common.dir_vq.cq_count = 1;
	et_dev->ops.vq_common.dir_vq.per_cq_size = 0x600UL;
	et_dev->ops.vq_common.dir_vq.cq_offset =
		et_dev->ops.vq_common.dir_vq.sq_offset +
		et_dev->ops.vq_common.dir_vq.sq_count *
		et_dev->ops.vq_common.dir_vq.per_sq_size;

	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = true;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].access.priv_mode =
		MEM_REGION_PRIVILEGE_MODE_KERNEL;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].access.node_access =
		MEM_REGION_NODE_ACCESSIBLE_NONE;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].access.dma_align =
		MEM_REGION_DMA_ALIGNMENT_NONE;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].size =
		et_dev->ops.vq_common.dir_vq.sq_count *
		et_dev->ops.vq_common.dir_vq.per_sq_size +
		et_dev->ops.vq_common.dir_vq.cq_count *
		et_dev->ops.vq_common.dir_vq.per_cq_size;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].mapped_baseaddr =
		kzalloc(et_dev->ops.regions
			[OPS_MEM_REGION_TYPE_VQ_BUFFER].size, GFP_KERNEL);

	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].is_valid = true;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].access.priv_mode =
		MEM_REGION_PRIVILEGE_MODE_USER;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].access.node_access =
		MEM_REGION_NODE_ACCESSIBLE_OPS;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].access.dma_align =
		MEM_REGION_DMA_ALIGNMENT_64BIT;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].soc_addr = 0x8100600000ULL;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].size = 0x2fba00000ULL;
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_HOST_MANAGED].mapped_baseaddr = NULL;

	// VQs initialization
	rv = et_vqueue_init_all(et_dev, false /* ops_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops device VQs initialization failed\n");
		goto error_free_vq_buffer;
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

error_free_vq_buffer:
	kfree(et_dev->ops.regions
	      [OPS_MEM_REGION_TYPE_VQ_BUFFER].mapped_baseaddr);
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

	return rv;
}

static void et_ops_dev_destroy(struct et_pci_dev *et_dev)
{
	misc_deregister(&et_dev->ops.misc_ops_dev);
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

	kfree(et_dev->ops.regions
	      [OPS_MEM_REGION_TYPE_VQ_BUFFER].mapped_baseaddr);
	et_dev->ops.regions
		[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid = false;

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
		dev_err(&pdev->dev,
			"Mgmt device initialization failed\n");
		goto error_clear_master;
	}

	rv = et_ops_dev_init(et_dev);
	if (rv) {
		dev_err(&pdev->dev, "Ops device initialization failed\n");
		goto error_mgmt_dev_destroy;
	}
	printk("\n------------ET PCIe loop back driver!-------------\n\n");
	return 0;

error_mgmt_dev_destroy:
	et_mgmt_dev_destroy(et_dev);

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
