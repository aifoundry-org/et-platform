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

#include "et_io.h"
#include "et_dma.h"
#include "et_ioctl.h"
#include "et_vqueue.h"
#include "et_mmio.h"
#include "et_pci_dev.h"
#include "et_mgmt_dir.h"
#include "et_ops_dir.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME "esperanto"

#define ET_PCIE_VENDOR_ID 0x1e0a
#define ET_PCIE_TEST_DEVICE_ID 0x9038
#define ET_PCIE_SOC1_ID 0xeb01

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

#define ET_MAX_DEVS	64
DECLARE_BITMAP(dev_bitmap, ET_MAX_DEVS);

// TODO SW-4210: Remove when MSIx is enabled
/*
 * Timeout is 250ms. Picked because it's unlikley the driver will miss an IRQ,
 * so this is a contigency and does not need to be checked often.
 */
#define MISSED_IRQ_TIMEOUT (HZ / 4)

static u8 get_index(void)
{
	u8 index;

	if (bitmap_full(dev_bitmap, ET_MAX_DEVS))
		return -EBUSY;
	index = find_first_zero_bit(dev_bitmap, ET_MAX_DEVS);
	set_bit(index, dev_bitmap);
	return index;
}

// TODO SW-4210: Remove when MSIx is enabled
static void et_isr_work(struct work_struct *work);

// TODO SW-4210: Remove when MSIx is enabled
static irqreturn_t et_pcie_isr(int irq, void *dev_id)
{
	struct et_pci_dev *et_dev = (struct et_pci_dev *)dev_id;

	queue_work(et_dev->workqueue, &et_dev->isr_work);

	return IRQ_HANDLED;
}

// TODO SW-4210: Remove when MSIx is enabled
static void et_isr_work(struct work_struct *work)
{
	struct et_pci_dev *et_dev =
		container_of(work, struct et_pci_dev, isr_work);

	// TODO: if multi-vector setup, dispatch without broadcasting to
	// everyone - JIRA SW-953
	et_cqueue_isr_bottom(et_dev->mgmt.cq_pptr[0]);
	et_cqueue_isr_bottom(et_dev->ops.cq_pptr[0]);
}

static __poll_t esperanto_pcie_ops_poll(struct file *fp, poll_table *wait)
{
	__poll_t mask = 0;
	struct et_ops_dev *ops;
	int i;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);

	// waitqueue is wake up whenever a message is received on CQ which
	// indicates that either CQ msg list has got message for userspace
	// (EPOLLIN event) or SQ is freed up to be used by userspace
	// (EPOLLOUT event)
	poll_wait(fp, &ops->vq_common.waitqueue, wait);

	// Update sq_bitmap for all SQs, set corresponding bit when space
	// available is more than threshold
	for (i = 0; i < ops->vq_common.sq_count; i++) {
		if (test_bit(i, ops->vq_common.sq_bitmap))
			continue;

		// Sync SQ circbuffer
		et_squeue_sync_cb_for_host(ops->sq_pptr[i]);

		if (et_circbuffer_free(&ops->sq_pptr[i]->cb) >=
		    atomic_read(&ops->sq_pptr[i]->sq_threshold))
			set_bit(i, ops->vq_common.sq_bitmap);
	}

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(ops->vq_common.sq_bitmap, ops->vq_common.sq_count))
		mask |= EPOLLOUT;

	// Update cq_bitmap for all CQs, set corresponding bit when msg is
	// available for userspace
	for (i = 0; i < ops->vq_common.cq_count; i++) {
		if (test_bit(i, ops->vq_common.cq_bitmap))
			continue;

		// Sync CQ circbuffer
		et_cqueue_sync_cb_for_host(ops->cq_pptr[i]);

		if (et_cqueue_msg_available(ops->cq_pptr[i]))
			set_bit(i, ops->vq_common.cq_bitmap);
	}

	// Generate EPOLLIN event if any CQ msg list has message for userspace
	if (!bitmap_empty(ops->vq_common.cq_bitmap, ops->vq_common.cq_count))
		mask |= EPOLLIN;

	return mask;
}

static long esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd,
				     unsigned long arg)
{
	struct et_pci_dev *et_dev;
	struct et_ops_dev *ops;
	struct mmio_desc mmio_info;
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
	case ETSOC1_IOCTL_GET_USER_DRAM_BASE:
		if (copy_to_user
		    ((u64 *)arg, &ops->ddr_regions
		     [OPS_DDR_REGION_MAP_USER_KERNEL_SPACE]->soc_addr,
		     size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_USER_DRAM_SIZE:
		if (copy_to_user
		    ((u64 *)arg, &ops->ddr_regions
		     [OPS_DDR_REGION_MAP_USER_KERNEL_SPACE]->size,
		     size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_MMIO_WRITE:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_mmio_write_to_device(et_dev, false /* ops_dev */,
					       (void __user *)mmio_info.ubuf,
					       mmio_info.size,
					       mmio_info.devaddr);

	case ETSOC1_IOCTL_MMIO_READ:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_mmio_read_from_device(et_dev, false /* ops_dev */,
						(void __user *)mmio_info.ubuf,
						mmio_info.size,
						mmio_info.devaddr);

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (copy_to_user((u64 *)arg, &ops->vq_common.sq_count, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = ops->vq_common.sq_size -
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

		if (!sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
		    (ops->vq_common.sq_size - sizeof(struct et_circbuffer)))
			return -EINVAL;

		sq_idx = sq_threshold_info.sq_index;

		atomic_set(&ops->sq_pptr[sq_idx]->sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		et_squeue_sync_cb_for_host(ops->sq_pptr[sq_idx]);

		if (et_circbuffer_free(&ops->sq_pptr[sq_idx]->cb) >=
		    atomic_read(&ops->sq_pptr[sq_idx]->sq_threshold))
			set_bit(sq_idx, ops->vq_common.sq_bitmap);

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

	// waitqueue is wake up whenever a message is received on CQ which
	// indicates that either CQ msg list has got message for userspace
	// (EPOLLIN event) or SQ is freed up to be used by userspace
	// (EPOLLOUT event)
	poll_wait(fp, &mgmt->vq_common.waitqueue, wait);

	// Update sq_bitmap for all SQs, set corresponding bit when space
	// available is more than threshold
	for (i = 0; i < mgmt->vq_common.sq_count; i++) {
		if (test_bit(i, mgmt->vq_common.sq_bitmap))
			continue;

		// Sync SQ circbuffer
		et_squeue_sync_cb_for_host(mgmt->sq_pptr[i]);

		if (et_circbuffer_free(&mgmt->sq_pptr[i]->cb) >=
		    atomic_read(&mgmt->sq_pptr[i]->sq_threshold))
			set_bit(i, mgmt->vq_common.sq_bitmap);
	}

	// Generate EPOLLOUT event if any SQ has space more than its threshold
	if (!bitmap_empty(mgmt->vq_common.sq_bitmap, mgmt->vq_common.sq_count))
		mask |= EPOLLOUT;

	// Update cq_bitmap for all CQs, set corresponding bit when msg is
	// available for userspace
	for (i = 0; i < mgmt->vq_common.cq_count; i++) {
		if (test_bit(i, mgmt->vq_common.cq_bitmap))
			continue;

		// Sync CQ circbuffer
		et_cqueue_sync_cb_for_host(mgmt->cq_pptr[i]);

		if (et_cqueue_msg_available(mgmt->cq_pptr[i]))
			set_bit(i, mgmt->vq_common.cq_bitmap);
	}

	// Generate EPOLLIN event if any CQ msg list has message for userspace
	if (!bitmap_empty(mgmt->vq_common.cq_bitmap, mgmt->vq_common.cq_count))
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
	struct mmio_desc mmio_info;
	struct sq_threshold sq_threshold_info;
	u16 sq_idx;
	size_t size;
	u16 max_size;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);
	et_dev = container_of(mgmt, struct et_pci_dev, mgmt);

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_USER_DRAM_BASE:
		if (copy_to_user
		    ((u64 *)arg, &mgmt->ddr_regions
		     [MGMT_DDR_REGION_MAP_TRACE_BUFFER]->soc_addr, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_USER_DRAM_SIZE:
		if (copy_to_user
		    ((u64 *)arg, &mgmt->ddr_regions
		     [MGMT_DDR_REGION_MAP_TRACE_BUFFER]->size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_DEV_MGMT_SCRATCH_DRAM_BASE:
		if (copy_to_user((uint64_t *)arg, &mgmt->ddr_regions
		    [MGMT_DDR_REGION_MAP_DEV_MANAGEMENT_SCRATCH]->soc_addr,
		    size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DEV_MGMT_SCRATCH_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_DEV_MGMT_SCRATCH_DRAM_SIZE:
		if (copy_to_user((uint64_t *)arg, &mgmt->ddr_regions
		    [MGMT_DDR_REGION_MAP_DEV_MANAGEMENT_SCRATCH]->size,
		    size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DEV_MGMT_SCRATCH_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_MMIO_WRITE:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_mmio_write_to_device(et_dev, true /* mgmt_dev */,
					       (void __user *)mmio_info.ubuf,
					       mmio_info.size,
					       mmio_info.devaddr);

	case ETSOC1_IOCTL_MMIO_READ:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_mmio_read_from_device(et_dev, true /* mgmt_dev */,
						(void __user *)mmio_info.ubuf,
						mmio_info.size,
						mmio_info.devaddr);

	case ETSOC1_IOCTL_GET_SQ_COUNT:
		if (copy_to_user((u64 *)arg, &mgmt->vq_common.sq_count, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE:
		max_size = mgmt->vq_common.sq_size - sizeof(struct et_circbuffer);
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

		if (!sq_threshold_info.bytes_needed ||
		    sq_threshold_info.bytes_needed >
		    (mgmt->vq_common.sq_size - sizeof(struct et_circbuffer)))
			return -EINVAL;

		sq_idx = sq_threshold_info.sq_index;

		atomic_set(&mgmt->sq_pptr[sq_idx]->sq_threshold,
			   sq_threshold_info.bytes_needed);

		// Update sq_bitmap w.r.t new threshold
		et_squeue_sync_cb_for_host(mgmt->sq_pptr[sq_idx]);

		if (et_circbuffer_free(&mgmt->sq_pptr[sq_idx]->cb) >=
		    atomic_read(&mgmt->sq_pptr[sq_idx]->sq_threshold))
			set_bit(sq_idx, mgmt->vq_common.sq_bitmap);

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
	char wq_name[32];

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
	et_dev->aborting = false;
	spin_lock_init(&et_dev->abort_lock);

	// TODO SW-4210: Remove when MSIx is enabled
	snprintf(wq_name, sizeof(wq_name), "%s_wq%d",
		 dev_name(&et_dev->pdev->dev), et_dev->dev_index);
	et_dev->workqueue = create_singlethread_workqueue(wq_name);
	if (!et_dev->workqueue)
		return -ENOMEM;

	// TODO SW-4210: Remove when MSIx is enabled
	INIT_WORK(&et_dev->isr_work, et_isr_work);

	return 0;
}

static int et_mgmt_dev_init(struct et_pci_dev *et_dev)
{
	int rv, i, ddr_cnt_map;
	struct et_mgmt_dir *dir_mgmt;
	struct et_mgmt_ddr_regions ddr_mgmt;
	struct et_mgmt_intrpt_region intrpt_mgmt;
	struct et_bar_mapping bm_info;
	bool ddr_ready = false;
	u8 *mem;

	et_dev->mgmt.is_mgmt_open = false;
	spin_lock_init(&et_dev->mgmt.mgmt_open_lock);

	// Map DIR region
	rv = et_map_bar(et_dev, &DIR_MAPPINGS[IOMEM_R_PU_DIR_PC_SP],
			&et_dev->mgmt.dir);
	if (rv)
		return rv;

	dir_mgmt = (struct et_mgmt_dir *)et_dev->mgmt.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 300 secs
	for (i = 0; !ddr_ready && i < 30; i++) {
		rv = ioread32(&dir_mgmt->status);
		if (rv >= MGMT_BOOT_STATUS_DEV_READY) {
			pr_debug("Mgmt device DIRs ready, status: %d", rv);
			ddr_ready = true;
		} else {
			pr_debug("Mgmt device DIRs not ready, status: %d, waiting...",
				 rv);
			msleep(10000);
		}
	}

	if (!ddr_ready) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt DIRs not ready; discovery timeout\n");
		rv = -EBUSY;
		goto error_unmap_dir_region;
	}

	if (ioread32(&dir_mgmt->size) != sizeof(*dir_mgmt)) {
		dev_err(&et_dev->pdev->dev, "Mgmt device DIRs size mismatch!");
		rv = -EINVAL;
		goto error_unmap_dir_region;
	}

	// Perform optimized read of DDR fields from DIRs
	et_ioread(dir_mgmt, offsetof(struct et_mgmt_dir, ddr_regions),
		  (u8 *)&ddr_mgmt, sizeof(ddr_mgmt));

	et_dev->mgmt.num_regions = ddr_mgmt.num_regions;
	mem = kmalloc_array(et_dev->mgmt.num_regions,
			    sizeof(*et_dev->mgmt.ddr_regions) +
			    sizeof(**et_dev->mgmt.ddr_regions), GFP_KERNEL);
	if (!mem) {
		rv = -ENOMEM;
		goto error_unmap_dir_region;
	}

	et_dev->mgmt.ddr_regions = (struct et_ddr_region **)mem;
	mem += et_dev->mgmt.num_regions * sizeof(*et_dev->mgmt.ddr_regions);

	for (i = 0, ddr_cnt_map = 0; i < et_dev->mgmt.num_regions; i++,
	     ddr_cnt_map++) {
		et_dev->mgmt.ddr_regions[i] = (struct et_ddr_region *)mem;
		mem += sizeof(**et_dev->mgmt.ddr_regions);

		bm_info.bar		= ddr_mgmt.regions[i].bar;
		bm_info.bar_offset	= ddr_mgmt.regions[i].offset;
		bm_info.size		= ddr_mgmt.regions[i].size;
		rv = et_map_bar(et_dev, &bm_info,
				&et_dev->mgmt.ddr_regions[i]->mapped_baseaddr);
		if (rv) {
			dev_err(&et_dev->pdev->dev,
				"Mgmt device DDR region mapping failed\n");
			goto error_unmap_ddr_regions;
		}

		et_dev->mgmt.ddr_regions[i]->size =
			ddr_mgmt.regions[i].size;
		et_dev->mgmt.ddr_regions[i]->soc_addr =
			ddr_mgmt.regions[i].devaddr;
	}

	et_dev->mgmt.minion_shires = ioread64(&dir_mgmt->minion_shires);

	// Read PU_TRG_PCIE region mapping details and map the region
	et_ioread(dir_mgmt, offsetof(struct et_mgmt_dir, intrpt_region),
		  (u8 *)&intrpt_mgmt, sizeof(intrpt_mgmt));

	bm_info.bar		= intrpt_mgmt.bar;
	bm_info.bar_offset	= intrpt_mgmt.offset;
	bm_info.size		= intrpt_mgmt.size;
	rv = et_map_bar(et_dev, &bm_info, &et_dev->r_pu_trg_pcie);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"pu_trg_pcie region mapping failed!");
		goto error_unmap_ddr_regions;
	}

	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt device VQs initialization failed\n");
		goto error_unmap_trg_pcie_region;
	}

	et_dev->mgmt.misc_mgmt_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->mgmt.misc_mgmt_dev.fops  = &et_pcie_mgmt_fops;
	et_dev->mgmt.misc_mgmt_dev.name  = devm_kasprintf(&et_dev->pdev->dev,
							GFP_KERNEL,
							"et%d_mgmt",
							et_dev->dev_index);
	rv = misc_register(&et_dev->mgmt.misc_mgmt_dev);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "misc mgmt register failed\n");
		goto error_vqueue_destroy_all;
	}

	return 0;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

error_unmap_trg_pcie_region:
	et_unmap_bar(et_dev->r_pu_trg_pcie);

error_unmap_ddr_regions:
	for (i = 0; i < ddr_cnt_map; i++)
		et_unmap_bar(et_dev->mgmt.ddr_regions[i]->mapped_baseaddr);
	kfree(et_dev->mgmt.ddr_regions);

error_unmap_dir_region:
	et_unmap_bar(et_dev->mgmt.dir);

	return rv;
}

static void et_mgmt_dev_destroy(struct et_pci_dev *et_dev)
{
	int i;

	misc_deregister(&et_dev->mgmt.misc_mgmt_dev);

	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

	et_unmap_bar(et_dev->r_pu_trg_pcie);

	for (i = 0; i < et_dev->mgmt.num_regions; i++)
		et_unmap_bar(et_dev->mgmt.ddr_regions[i]->mapped_baseaddr);
	kfree(et_dev->mgmt.ddr_regions);

	et_unmap_bar(et_dev->mgmt.dir);
}

static int et_ops_dev_init(struct et_pci_dev *et_dev)
{
	int rv, i, ddr_cnt_map;
	struct et_ops_dir *dir_ops;
	struct et_ops_ddr_regions ddr_ops;
	struct et_bar_mapping bm_info;
	bool ddr_ready = false;
	u8 *mem;

	et_dev->ops.is_ops_open = false;
	spin_lock_init(&et_dev->ops.ops_open_lock);

	// Init DMA rbtree
	mutex_init(&et_dev->ops.dma_rbtree_mutex);
	et_dev->ops.dma_rbtree = RB_ROOT;

	// Map DIR region
	rv = et_map_bar(et_dev, &DIR_MAPPINGS[IOMEM_R_PU_DIR_PC_MM],
			&et_dev->ops.dir);
	if (rv)
		return rv;

	dir_ops = (struct et_ops_dir *)et_dev->ops.dir;

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 300 secs
	for (i = 0; !ddr_ready && i < 30; i++) {
		rv = ioread32(&dir_ops->status);
		if (rv >= OPS_BOOT_STATUS_MM_READY) {
			pr_debug("Ops device DIRs ready, status: %d", rv);
			ddr_ready = true;
		} else {
			pr_debug("Ops device not ready, status: %d, waiting...",
				 rv);
			msleep(10000);
		}
	}

	if (!ddr_ready) {
		dev_err(&et_dev->pdev->dev,
			"Ops device DIRs not ready; discovery timeout\n");
		rv = -EBUSY;
		goto error_unmap_dir_region;
	}

	if (ioread32(&dir_ops->size) != sizeof(*dir_ops)) {
		dev_err(&et_dev->pdev->dev, "Ops device DIRs size mismatch!");
		rv = -EINVAL;
		goto error_unmap_dir_region;
	}

	// Perform optimized read of DDR fields from DIRs
	et_ioread(dir_ops, offsetof(struct et_ops_dir, ddr_regions),
		  (u8 *)&ddr_ops, sizeof(ddr_ops));

	et_dev->ops.num_regions = ddr_ops.num_regions;

	mem = kmalloc_array(et_dev->ops.num_regions,
			    sizeof(*et_dev->ops.ddr_regions) +
			    sizeof(**et_dev->ops.ddr_regions), GFP_KERNEL);
	if (!mem) {
		rv = -ENOMEM;
		goto error_unmap_dir_region;
	}

	et_dev->ops.ddr_regions = (struct et_ddr_region **)mem;
	mem += et_dev->ops.num_regions * sizeof(*et_dev->ops.ddr_regions);

	for (i = 0, ddr_cnt_map = 0; i < et_dev->ops.num_regions; i++,
	     ddr_cnt_map++) {
		et_dev->ops.ddr_regions[i] = (struct et_ddr_region *)mem;
		mem += sizeof(**et_dev->ops.ddr_regions);

		bm_info.bar		= ddr_ops.regions[i].bar;
		bm_info.bar_offset	= ddr_ops.regions[i].offset;
		bm_info.size		= ddr_ops.regions[i].size;
		rv = et_map_bar(et_dev, &bm_info,
				&et_dev->ops.ddr_regions[i]->mapped_baseaddr);
		if (rv) {
			dev_err(&et_dev->pdev->dev,
				"Ops device DDR region mapping failed\n");
			goto error_unmap_ddr_regions;
		}

		et_dev->ops.ddr_regions[i]->size =
			ddr_ops.regions[i].size;
		et_dev->ops.ddr_regions[i]->soc_addr =
			ddr_ops.regions[i].devaddr;
	}

	rv = et_vqueue_init_all(et_dev, false /* ops_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Ops device VQs initialization failed\n");
		goto error_unmap_ddr_regions;
	}

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

	return 0;

error_vqueue_destroy_all:
	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

error_unmap_ddr_regions:
	for (i = 0; i < ddr_cnt_map; i++)
		et_unmap_bar(et_dev->ops.ddr_regions[i]->mapped_baseaddr);
	kfree(et_dev->ops.ddr_regions);

error_unmap_dir_region:
	et_unmap_bar(et_dev->ops.dir);

	return rv;
}

static void et_ops_dev_destroy(struct et_pci_dev *et_dev)
{
	int i;

	misc_deregister(&et_dev->ops.misc_ops_dev);

	et_vqueue_destroy_all(et_dev, false /* ops_dev */);

	for (i = 0; i < et_dev->ops.num_regions; i++)
		et_unmap_bar(et_dev->ops.ddr_regions[i]->mapped_baseaddr);
	kfree(et_dev->ops.ddr_regions);

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

	// TODO SW-4210: Remove when MSIx is enabled
	if (et_dev->workqueue)
		destroy_workqueue(et_dev->workqueue);
}

static int esperanto_pcie_probe(struct pci_dev *pdev,
				const struct pci_device_id *pci_id)
{
	int rv;
	struct et_pci_dev *et_dev;
	unsigned long flags;

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

	// TODO SW-4210: Uncomment when MSIx is enabled
//	et_dev->num_irq_vecs = pci_msix_vec_count(pdev);
	et_dev->num_irq_vecs = 1;
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

	// TODO SW-4210: Remove when MSIx is enabled
	rv = request_irq(pci_irq_vector(pdev, 0), et_pcie_isr, 0, "common_irq",
			 (void *)et_dev);
	if (rv) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto error_master_minion_destroy;
	}

	return 0;

error_master_minion_destroy:
	// TODO SW-4210: Remove when MSIx is enabled
	spin_lock_irqsave(&et_dev->abort_lock, flags);
	et_dev->aborting = true;
	spin_unlock_irqrestore(&et_dev->abort_lock, flags);

	// TODO SW-4210: Remove when MSIx is enabled
	// Disable anything that could trigger additional calls to isr_work
	// in another core before canceling it
	cancel_work_sync(&et_dev->isr_work);

	et_ops_dev_destroy(et_dev);

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
	unsigned long flags;

	et_dev = pci_get_drvdata(pdev);
	if (!et_dev)
		return;

	// TODO SW-4210: Remove when MSIx is enabled
	spin_lock_irqsave(&et_dev->abort_lock, flags);
	et_dev->aborting = true;
	spin_unlock_irqrestore(&et_dev->abort_lock, flags);

	// TODO SW-4210: Remove when MSIx is enabled
	// Disable anything that could trigger additional calls to isr_work
	// in another core before canceling it
	cancel_work_sync(&et_dev->isr_work);
	free_irq(pci_irq_vector(pdev, 0), (void *)et_dev);

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
