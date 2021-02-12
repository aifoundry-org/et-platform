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
#include "et_layout.h"
#include "et_vqueue.h"
#include "et_mmio.h"
#include "et_pci_dev.h"
#include "hal_device.h"
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

static unsigned long dev_bitmap;

// TODO SW-4210: Remove when MSIx is enabled
/*
 * Timeout is 250ms. Picked because it's unlikley the driver will miss an IRQ,
 * so this is a contigency and does not need to be checked often.
 */
#define MISSED_IRQ_TIMEOUT (HZ / 4)

static u8 get_index(void)
{
	u8 index;

	if (dev_bitmap == ~0UL)
		return -EBUSY;
	index = ffz(dev_bitmap);
	set_bit(index, &dev_bitmap);
	return index;
}

// TODO SW-4210: Remove when MSIx is enabled
static void et_isr_work(struct work_struct *work);

// TODO SW-4210: Remove when MSIx is enabled
static irqreturn_t et_pcie_isr(int irq, void *dev_id)
{
	struct et_pci_dev *et_dev = (struct et_pci_dev *)dev_id;

	//Push off next missed IRQ check since we just got one
	//TODO: be careful about this with mutlivector. Don't let one IRQ
	//keep firing mean we fail to check for missed IRQs on the other
	//one. JIRA SW-953.
	mod_timer(&et_dev->missed_irq_timer, jiffies + MISSED_IRQ_TIMEOUT);

	queue_work(et_dev->workqueue, &et_dev->isr_work);

	return IRQ_HANDLED;
}

// TODO SW-4210: Remove when MSIx is enabled
static void et_missed_irq_timeout(struct timer_list *timer)
{
	unsigned long flags;
	struct et_pci_dev *et_dev = from_timer(et_dev, timer, missed_irq_timer);

	//The isr_work method and et_mbox_isr methods are tolerant of spurrious
	//interrupts; call them unconditionally in case of missed IRQs.
	queue_work(et_dev->workqueue, &et_dev->isr_work);

	spin_lock_irqsave(&et_dev->abort_lock, flags);

	//Rescheudle timer; run timer as long as module is active
	if (!et_dev->aborting) {
		mod_timer(&et_dev->missed_irq_timer,
			  jiffies + MISSED_IRQ_TIMEOUT);
	}

	spin_unlock_irqrestore(&et_dev->abort_lock, flags);
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
	struct et_circbuffer cb;
	int i;

	ops = container_of(fp->private_data, struct et_ops_dev, misc_ops_dev);

	poll_wait(fp, &ops->vq_common.waitqueue, wait);

	for (i = 0; i < ops->vq_common.sq_count; i++) {
		mutex_lock(&ops->sq_pptr[i]->push_mutex);
		et_ioread(&ops->sq_pptr[i]->cb, 0, (u8 *)&cb, sizeof(cb));
		mutex_unlock(&ops->sq_pptr[i]->push_mutex);

		if (et_circbuffer_free(&cb) >=
		    atomic_read(&ops->sq_pptr[i]->sq_threshold)) {
			if (!test_and_set_bit
			    (i, (unsigned long *)&ops->vq_common.sq_bitmap))
				mask |= EPOLLOUT;
		} else {
			clear_bit(i,
				  (unsigned long *)&ops->vq_common.sq_bitmap);
		}

		if (et_cqueue_msg_available(ops->cq_pptr[i])) {
			if (!test_and_set_bit
			    (i, (unsigned long *)&ops->vq_common.cq_bitmap))
				mask |= EPOLLIN;
		} else {
			clear_bit(i,
				  (unsigned long *)&ops->vq_common.cq_bitmap);
		}
	}

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
		return et_mmio_write_to_device(et_dev,
					       (void __user *)mmio_info.ubuf,
					       mmio_info.size,
					       mmio_info.devaddr);

	case ETSOC1_IOCTL_MMIO_READ:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_mmio_read_from_device(et_dev,
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
		if (copy_to_user((u64 *)arg, &ops->vq_common.sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (copy_to_user((u64 *)arg, &ops->vq_common.cq_bitmap,
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

		atomic_set
		(&ops->sq_pptr[sq_threshold_info.sq_index]->sq_threshold,
		 sq_threshold_info.bytes_needed);

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
	struct et_circbuffer cb;
	int i;

	mgmt = container_of(fp->private_data, struct et_mgmt_dev,
			    misc_mgmt_dev);

	poll_wait(fp, &mgmt->vq_common.waitqueue, wait);

	for (i = 0; i < mgmt->vq_common.sq_count; i++) {
		mutex_lock(&mgmt->sq_pptr[i]->push_mutex);
		et_ioread(&mgmt->sq_pptr[i]->cb, 0, (u8 *)&cb, sizeof(cb));
		mutex_unlock(&mgmt->sq_pptr[i]->push_mutex);

		if (et_circbuffer_free(&cb) >=
		    atomic_read(&mgmt->sq_pptr[i]->sq_threshold)) {
			if (!test_and_set_bit
			    (i, (unsigned long *)&mgmt->vq_common.sq_bitmap))
				mask |= EPOLLOUT;
		} else {
			clear_bit(i,
				  (unsigned long *)&mgmt->vq_common.sq_bitmap);
		}

		if (et_cqueue_msg_available(mgmt->cq_pptr[i])) {
			if (!test_and_set_bit
			    (i, (unsigned long *)&mgmt->vq_common.cq_bitmap))
				mask |= EPOLLIN;
		} else {
			clear_bit(i,
				  (unsigned long *)&mgmt->vq_common.cq_bitmap);
		}
	}

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
		return et_mmio_write_to_device(et_dev,
					       (void __user *)mmio_info.ubuf,
					       mmio_info.size,
					       mmio_info.devaddr);

	case ETSOC1_IOCTL_MMIO_READ:
		if (copy_from_user(&mmio_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_mmio_read_from_device(et_dev,
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
		if (copy_to_user((u64 *)arg, &mgmt->vq_common.sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP:
		if (copy_to_user((u64 *)arg, &mgmt->vq_common.cq_bitmap,
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

		atomic_set
		(&mgmt->sq_pptr[sq_threshold_info.sq_index]->sq_threshold,
		 sq_threshold_info.bytes_needed);

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

	/* TODO: In order to support Edge Triggered EPOLL event distribution,
	 * device node should be opened in non-blocking mode to avoid
	 * starvation due to blocking read or write. But this can be done
	 * only after MBox is retired since MBox implementation performs
	 * blocking calls.
	 */
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

	// TODO SW-4210: Remove when MSIx is enabled
	timer_setup(&et_dev->missed_irq_timer, et_missed_irq_timeout, 0);

	return 0;
}

static int et_mgmt_dev_init(struct et_pci_dev *et_dev)
{
	int rv, i, ddr_cnt_map;
	struct et_mgmt_dir *dir_mgmt;
	struct et_mgmt_ddr_regions ddr_mgmt;
	struct et_bar_mapping bm_info;
	bool ddr_ready = false;
	u8 *mem;

	et_dev->mgmt.is_mgmt_open = false;
	spin_lock_init(&et_dev->mgmt.mgmt_open_lock);

	dir_mgmt = (struct et_mgmt_dir *)et_dev->iomem[IOMEM_R_PU_DIR_PC_SP];

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 100 secs
	for (i = 0; !ddr_ready && i < 10; i++) {
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
		return -EBUSY;
	}

	if (ioread32(&dir_mgmt->size) != sizeof(*dir_mgmt)) {
		dev_err(&et_dev->pdev->dev, "Mgmt device DIRs size mismatch!");
		return -EINVAL;
	}

	// Perform optimized read of DDR fields from DIRs
	et_ioread(dir_mgmt, offsetof(struct et_mgmt_dir, ddr_regions),
		  (u8 *)&ddr_mgmt, sizeof(ddr_mgmt));

	et_dev->mgmt.num_regions = ddr_mgmt.num_regions;
	mem = kmalloc_array(et_dev->mgmt.num_regions,
			    sizeof(*et_dev->mgmt.ddr_regions) +
			    sizeof(**et_dev->mgmt.ddr_regions), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	et_dev->mgmt.ddr_regions = (struct et_ddr_region **)mem;
	mem += et_dev->mgmt.num_regions * sizeof(*et_dev->mgmt.ddr_regions);

	for (i = 0, ddr_cnt_map = 0; i < et_dev->mgmt.num_regions; i++,
	     ddr_cnt_map++) {
		et_dev->mgmt.ddr_regions[i] = (struct et_ddr_region *)mem;
		mem += sizeof(**et_dev->mgmt.ddr_regions);

		bm_info.bar			= ddr_mgmt.regions[i].bar;
		bm_info.bar_offset		= ddr_mgmt.regions[i].offset;
		bm_info.size			= ddr_mgmt.regions[i].size;
		bm_info.strictly_order_access	=
			is_bar_prefetchable(et_dev, bm_info.bar);

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

	rv = et_vqueue_init_all(et_dev, true /* mgmt_dev */);
	if (rv) {
		dev_err(&et_dev->pdev->dev,
			"Mgmt device VQs initialization failed\n");
		goto error_unmap_ddr_regions;
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

error_unmap_ddr_regions:
	for (i = 0; i < ddr_cnt_map; i++)
		et_unmap_bar(et_dev->mgmt.ddr_regions[i]->mapped_baseaddr);
	kfree(et_dev->mgmt.ddr_regions);

	return rv;
}

static void et_mgmt_dev_destroy(struct et_pci_dev *et_dev)
{
	int i;

	misc_deregister(&et_dev->mgmt.misc_mgmt_dev);

	et_vqueue_destroy_all(et_dev, true /* mgmt_dev */);

	for (i = 0; i < et_dev->mgmt.num_regions; i++)
		et_unmap_bar(et_dev->mgmt.ddr_regions[i]->mapped_baseaddr);
	kfree(et_dev->mgmt.ddr_regions);
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

	dir_ops = (struct et_ops_dir *)et_dev->iomem[IOMEM_R_PU_DIR_PC_MM];

	// TODO: Improve device discovery
	// Waiting for device to be ready, wait for 100 secs
	for (i = 0; !ddr_ready && i < 10; i++) {
		rv = ioread32(&dir_ops->status);
		if (rv >= OPS_BOOT_STATUS_VQ_READY) {
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
		return -EBUSY;
	}

	if (ioread32(&dir_ops->size) != sizeof(*dir_ops)) {
		dev_err(&et_dev->pdev->dev, "Ops device DIRs size mismatch!");
		return -EINVAL;
	}

	// Perform optimized read of DDR fields from DIRs
	et_ioread(dir_ops, offsetof(struct et_ops_dir, ddr_regions),
		  (u8 *)&ddr_ops, sizeof(ddr_ops));

	et_dev->ops.num_regions = ddr_ops.num_regions;

	mem = kmalloc_array(et_dev->ops.num_regions,
			    sizeof(*et_dev->ops.ddr_regions) +
			    sizeof(**et_dev->ops.ddr_regions), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	et_dev->ops.ddr_regions = (struct et_ddr_region **)mem;
	mem += et_dev->ops.num_regions * sizeof(*et_dev->ops.ddr_regions);

	for (i = 0, ddr_cnt_map = 0; i < et_dev->ops.num_regions; i++,
	     ddr_cnt_map++) {
		et_dev->ops.ddr_regions[i] = (struct et_ddr_region *)mem;
		mem += sizeof(**et_dev->ops.ddr_regions);

		bm_info.bar			= ddr_ops.regions[i].bar;
		bm_info.bar_offset		= ddr_ops.regions[i].offset;
		bm_info.size			= ddr_ops.regions[i].size;
		bm_info.strictly_order_access	=
			is_bar_prefetchable(et_dev, bm_info.bar);

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
	clear_bit(dev_index, &dev_bitmap);

	// TODO SW-4210: Remove when MSIx is enabled
	if (et_dev->workqueue)
		destroy_workqueue(et_dev->workqueue);
}

static void et_unmap_bars(struct et_pci_dev *et_dev)
{
	int i;

	for (i = 0; i < IOMEM_REGIONS; i++)
		et_unmap_bar(et_dev->iomem[i]);

	pci_release_regions(et_dev->pdev);
}

static int et_map_bars(struct et_pci_dev *et_dev)
{
	int i, rv;

	rv = pci_request_regions(et_dev->pdev, DRIVER_NAME);
        if (rv) {
                dev_err(&et_dev->pdev->dev, "request regions failed\n");
                return rv;
        }

	for (i = 0; i < IOMEM_REGIONS; i++) {
		rv = et_map_bar(et_dev, &BAR_MAPPINGS[i], &et_dev->iomem[i]);
		if (rv)
			goto error_unmap_bars;
	}

	return 0;

error_unmap_bars:
	et_unmap_bars(et_dev);
	return rv;
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

	rv = et_map_bars(et_dev);
	if (rv) {
		dev_err(&pdev->dev, "mapping BAR regions failed\n");
		goto error_free_irq_vectors;
	}

	rv = et_mgmt_dev_init(et_dev);
	if (rv) {
		dev_err(&pdev->dev,
			"Mgmt device initialization failed\n");
		goto error_unmap_bars;
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

	// TODO SW-4210: Remove when MSIx is enabled
	mod_timer(&et_dev->missed_irq_timer, jiffies + MISSED_IRQ_TIMEOUT);

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

error_unmap_bars:
	et_unmap_bars(et_dev);

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
	del_timer_sync(&et_dev->missed_irq_timer);
	cancel_work_sync(&et_dev->isr_work);
	free_irq(pci_irq_vector(pdev, 0), (void *)et_dev);

	et_ops_dev_destroy(et_dev);
	et_mgmt_dev_destroy(et_dev);

	et_unmap_bars(et_dev);

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
