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

#include "et_dma.h"
#include "et_ioctl.h"
#include "et_layout.h"
#include "et_mbox.h"
#include "et_vqueue.h"
#include "et_mmio.h"
#include "et_pci_dev.h"
#include "hal_device.h"

/* Header file is currently not part of driver code. It will be added once
 * implementation becomes stable
 */
#include "mm_dev_intf_reg.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");

#define DRIVER_NAME "esperanto"

#define ET_PCIE_VENDOR_ID 0x1e0a
#define ET_PCIE_TEST_DEVICE_ID 0x9038
#define ET_PCIE_SOC1_ID 0xeb01

#define MBOX_TIMEOUT_SEC 60

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_TEST_DEVICE_ID) },
	{ PCI_DEVICE(ET_PCIE_VENDOR_ID, ET_PCIE_SOC1_ID) },
	{}
};

static unsigned long dev_bitmap;

/*
 * Timeout is 250ms. Picked because it's unlikley the driver will miss an IRQ,
 * so this is a contigency and does not need to be checked often.
 */
#define MISSED_IRQ_TIMEOUT (HZ / 4)
#define REQ_IRQ_NUM	1
/*
 * Register offsets, per hardware implementation.
 */
#define IPI_TRIGGER_OFFSET 0
#define MMM_INT_INC_OFFSET 4

static u64 et_ioread64(void __iomem *addr)
{
	u64 low, high;

	low = ioread32(addr);
	high = ioread32(addr + sizeof(u32));
	return low | (high << 32);
}

static u8 get_index(void)
{
	u8 index;

	if (dev_bitmap == ~0UL)
		return -EBUSY;
	index = ffz(dev_bitmap);
	set_bit(index, &dev_bitmap);
	return index;
}

/* TODO: will be used when we switch from MBox to VQs */
#if 0
static void et_isr_work(struct work_struct *work)
{
	struct et_vqueue *vqueue;

	vqueue = container_of(work, struct et_vqueue, isr_work);

	et_vqueue_isr_bottom(vqueue);
}
#endif

static void et_isr_work(struct work_struct *work);

static void et_unmap_bar_dynamic(void *bar_map_ptr)
{
	if (bar_map_ptr)
		iounmap(bar_map_ptr);
}

static int et_map_bar_dynamic(struct et_pci_dev *et_dev,
			      struct et_bar_mapping *bm_info,
			      void __iomem **bar_map_ptr)
{
	if (bm_info->strictly_order_access)
		*bar_map_ptr = pci_iomap_range(et_dev->pdev, bm_info->bar,
					       bm_info->bar_offset,
					       bm_info->size);
	else
		*bar_map_ptr = pci_iomap_wc_range(et_dev->pdev, bm_info->bar,
						  bm_info->bar_offset,
						  bm_info->size);

	if (IS_ERR(*bar_map_ptr)) {
		dev_err(&et_dev->pdev->dev, "dynamic bar mapping failed\n");
		return PTR_ERR(*bar_map_ptr);
	}

	return 0;
}

static bool is_bar_prefetchable(struct et_pci_dev *et_dev, u8 bar_no)
{
	u32 bar;

	switch (bar_no) {
	case 0:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_0, &bar);
		break;
	case 1:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_1, &bar);
		break;
	case 2:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_2, &bar);
		break;
	case 3:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_3, &bar);
		break;
	case 4:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_4, &bar);
		break;
	case 5:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_5, &bar);
		break;
	default:
		pr_err("Invalid BAR number: %d\n", bar_no);
		return -EINVAL;
	}

	if (bar & PCI_BASE_ADDRESS_MEM_PREFETCH) {
		pr_info("BAR[%d]: prefetchable memory\n", bar_no);
		return true;
	} else {
		pr_info("BAR[%d]: non-prefetchable memory\n", bar_no);
		return false;
	}
}

/* Static device data discovery is just a temporary stop gap solution and
 * this code will change in futurue to support proper PCI device discovery
 */
static int et_vqueues_discover(struct et_pci_dev *et_dev)
{
	int i, rc;
	char wq_name[32];
	void __iomem *vq_baseaddr_mm;
	struct et_bar_mapping bm_info;
	size_t aligned_queue_size;
	struct et_vqueue *vqs_mm;
	struct et_vqueue_buf *vqs_buf_mm;
	struct et_vqueue_common *vq_common_mm;
	MM_DEV_INTF_REG_s *dir_mm_ptr;
	u32 vq_info_size_mm;

	dir_mm_ptr = (MM_DEV_INTF_REG_s *)et_dev->iomem[IOMEM_R_PU_DIR_PC_MM];

	rc = ioread32(&dir_mm_ptr->status);
	if (rc < MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY_INITIALIZED) {
		dev_err(&et_dev->pdev->dev, "Dev interface not initialized\n");
		return -EBUSY;
	}

	bm_info.bar = ioread8(&dir_mm_ptr->ddr_region
			[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].bar);
	bm_info.bar_offset = et_ioread64(&dir_mm_ptr->ddr_region
			[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].offset);
	bm_info.size            = et_ioread64(&dir_mm_ptr->ddr_region
			[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].size);
	et_dev->hm_dram_base    = et_ioread64(&dir_mm_ptr->ddr_region
			[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].devaddr);
	et_dev->hm_dram_size    = bm_info.size;
	bm_info.strictly_order_access =
		is_bar_prefetchable(et_dev, bm_info.bar);

	/* uncomment once MBox retires, current DRAM mapping is in use by
	 * MBox and DMA implementation.
	 */
	//rc =
	//et_map_bar_dynamic(et_dev, &bm_info, &et_dev->iomem[IOMEM_R_DRCT_DRAM]);
	//if (rc)
	//	return rc;

	bm_info.bar             = ioread8(&dir_mm_ptr->mm_vq.bar);
	bm_info.bar_offset      = et_ioread64(&dir_mm_ptr->mm_vq.bar_offset);
	bm_info.size            = et_ioread64(&dir_mm_ptr->mm_vq.bar_size);
	bm_info.strictly_order_access =
		is_bar_prefetchable(et_dev, bm_info.bar);

	rc = et_map_bar_dynamic(et_dev, &bm_info, &vq_baseaddr_mm);
	if (rc)
		/* uncomment once MBox retires */
		//goto error_unmap_dram;
		return rc;

	vq_common_mm = kmalloc(sizeof(*vq_common_mm), GFP_KERNEL);
	if (!vq_common_mm) {
		rc = -ENOMEM;
		goto error_unmap_vq_baseaddr_mm;
	}

	vq_common_mm->sq_bitmap		= 0;
	vq_common_mm->cq_bitmap		= 0;
	vq_common_mm->queue_count	= ioread8(&dir_mm_ptr->mm_vq.vq_count);
	vq_common_mm->queue_buf_count	=
		ioread16(&dir_mm_ptr->mm_vq.size_info.element_count);
	vq_common_mm->queue_buf_size	=
		ioread16(&dir_mm_ptr->mm_vq.size_info.element_size);
	vq_common_mm->queue_alignment	=
		ioread16(&dir_mm_ptr->mm_vq.size_info.element_alignment);
	vq_common_mm->mapped_baseaddr	= vq_baseaddr_mm;
	vq_common_mm->interrupt_addr	=
		et_dev->iomem[IOMEM_R_PU_TRG_PCIE] + MMM_INT_INC_OFFSET;
	init_waitqueue_head(&vq_common_mm->vqueue_wq);

	snprintf(wq_name, sizeof(wq_name), "%s_mm_wq%d",
		 dev_name(&et_dev->pdev->dev), et_dev->index);
	vq_common_mm->workqueue = create_singlethread_workqueue(wq_name);
	if (!vq_common_mm->workqueue) {
		rc = -ENOMEM;
		goto error_free_vq_common_mm;
	}
	mutex_init(&vq_common_mm->dma_rbtree_mutex);
	vq_common_mm->dma_rbtree = RB_ROOT;

	aligned_queue_size =
	(((vq_common_mm->queue_buf_count * vq_common_mm->queue_buf_size - 1) /
	vq_common_mm->queue_alignment) + 1) * vq_common_mm->queue_alignment;

	vq_info_size_mm = ioread16(&dir_mm_ptr->mm_vq.size_info.control_size);

	vqs_mm = kmalloc_array(vq_common_mm->queue_count,
			       sizeof(struct et_vqueue), GFP_KERNEL);
	if (!vqs_mm) {
		rc = -ENOMEM;
		goto error_free_workqueue_mm;
	}

	vqs_buf_mm = kmalloc_array(vq_common_mm->queue_count,
				   sizeof(struct et_vqueue_buf), GFP_KERNEL);
	if (!vqs_buf_mm) {
		rc = -ENOMEM;
		goto error_free_vqs_mm;
	}

	et_dev->vqueue_mm_pptr =
		kmalloc_array(vq_common_mm->queue_count,
			      sizeof(struct et_vqueue *), GFP_KERNEL);
	if (!et_dev->vqueue_mm_pptr) {
		rc = -ENOMEM;
		goto error_free_vqs_buf_mm;
	}

	for (i = 0; i < vq_common_mm->queue_count; i++) {
		et_dev->vqueue_mm_pptr[i] = &vqs_mm[i];
		et_dev->vqueue_mm_pptr[i]->vqueue_common = vq_common_mm;
		et_dev->vqueue_mm_pptr[i]->vqueue_info =
			(struct et_vqueue_info *)
			(vq_baseaddr_mm + (i * sizeof(struct et_vqueue_info)));
		et_dev->vqueue_mm_pptr[i]->vqueue_buf = &vqs_buf_mm[i];
		et_dev->vqueue_mm_pptr[i]->vqueue_buf->sq_buf =
			(void *)((vq_baseaddr_mm + vq_info_size_mm) +
			(2 * i * aligned_queue_size));
		et_dev->vqueue_mm_pptr[i]->vqueue_buf->cq_buf =
			(void *)((vq_baseaddr_mm + vq_info_size_mm) +
			(2 * i + 1) * aligned_queue_size);
		et_dev->vqueue_mm_pptr[i]->available_buf_count =
			vq_common_mm->queue_buf_count;
		et_dev->vqueue_mm_pptr[i]->index = i;

		INIT_WORK(&et_dev->vqueue_mm_pptr[i]->isr_work, et_isr_work);

		et_vqueue_init(et_dev->vqueue_mm_pptr[i]);
	}

	/* This line will be uncommented when we enable device discovery flows
	 * that involve SP
	 */
	//et_vqueue_init(&et_dev->vqueue_sp);
	//INIT_WORK(&et_dev->vqueue_sp.isr_work, et_isr_work);
	//init_waitqueue_head(&et_dev->vqueue_sp.vqueue_common->vqueue_wq);

	pr_err("VQueues discovery successful\n");

	return 0;

error_free_vqs_buf_mm:
	kfree(vqs_buf_mm);
error_free_vqs_mm:
	kfree(vqs_mm);
error_free_workqueue_mm:
	destroy_workqueue(vq_common_mm->workqueue);
error_free_vq_common_mm:
	kfree(vq_common_mm);
error_unmap_vq_baseaddr_mm:
	et_unmap_bar_dynamic(vq_baseaddr_mm);
/* uncomment once MBox retires */
//error_unmap_dram:
//	et_unmap_bar_dynamic(et_dev->iomem[IOMEM_R_DRCT_DRAM]);

	return rc;
}

static void et_vqueue_cleanup(struct et_pci_dev *et_dev, bool is_vqueue_sp)
{
	int i, queue_count_mm;
	struct et_vqueue_common *vq_common;

	if (is_vqueue_sp) {
		vq_common = et_dev->vqueue_sp.vqueue_common;
		if (vq_common->workqueue)
			destroy_workqueue(vq_common->workqueue);

		/* TODO: call wake_up_interruptible_all() for SP wait queue */

		et_vqueue_destroy(&et_dev->vqueue_sp);

	} else {
		vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;
		queue_count_mm = vq_common->queue_count;

		mutex_lock(&vq_common->dma_rbtree_mutex);
		et_dma_delete_all_info(&vq_common->dma_rbtree);
		mutex_unlock(&vq_common->dma_rbtree_mutex);

		mutex_destroy(&vq_common->dma_rbtree_mutex);

		wake_up_interruptible_all(&vq_common->vqueue_wq);

		if (vq_common->workqueue)
			destroy_workqueue(vq_common->workqueue);

		for (i = 0; i < queue_count_mm; i++)
			et_vqueue_destroy(et_dev->vqueue_mm_pptr[i]);

		/* uncomment once MBox retires */
		//if (et_dev->iomem[IOMEM_R_DRCT_DRAM])
		//	et_unmap_bar_dynamic(et_dev->iomem[IOMEM_R_DRCT_DRAM]);
		if (vq_common->mapped_baseaddr)
			et_unmap_bar_dynamic(vq_common->mapped_baseaddr);

		kfree(et_dev->vqueue_mm_pptr[0]->vqueue_common);
		kfree(et_dev->vqueue_mm_pptr[0]->vqueue_buf);
		kfree(et_dev->vqueue_mm_pptr[0]);
		kfree(et_dev->vqueue_mm_pptr);
	}
}

#if 1
/* This one will be removed when we switch from MBox to VQs
 * and the function below this function will be used
 */
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
#else
static irqreturn_t et_pcie_isr(int irq, void *vqueue_id)
{
	struct et_vqueue *vqueue = (struct et_vqueue *)vqueue_id;

	queue_work(vqueue->vqueue_common->workqueue, &vqueue->isr_work);

	return IRQ_HANDLED;
}
#endif

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

static void et_isr_work(struct work_struct *work)
{
	struct et_pci_dev *et_dev =
		container_of(work, struct et_pci_dev, isr_work);

	//TODO: if multi-vector setup, dispatch without broadcasting to everyone - JIRA SW-953
	et_mbox_isr_bottom(&et_dev->mbox_sp, et_dev);
	et_mbox_isr_bottom(&et_dev->mbox_mm, et_dev);
	et_vqueue_isr_bottom(et_dev->vqueue_mm_pptr[0]);
}

//TODO: tune this value
#define DMA_THRESHOLD 1024

static ssize_t esperanto_pcie_ops_read(struct file *fp, char __user *buf,
				   size_t count, loff_t *pos)
{
	struct et_pci_dev *et_dev = container_of(fp->private_data,
					struct et_pci_dev, misc_ops_dev);
	ssize_t rv;
	bool use_mmio = false;

	//TODO: JIRA SW-948: Support Multi-Channel DMA
	if (et_dev->bulk_cfg == BULK_CFG_AUTO) {
		if (count < DMA_THRESHOLD)
			use_mmio = true;
	} else if (et_dev->bulk_cfg == BULK_CFG_MMIO) {
		use_mmio = true;
	}

	if (use_mmio) {
		rv = et_mmio_read_to_user(buf, count, pos, et_dev);
	} else {
		rv = et_dma_push_to_user(buf, count, pos,
					 ET_DMA_CHAN_ID_WRITE_0, et_dev);
	}

	return rv;
}

static ssize_t esperanto_pcie_ops_write(struct file *fp, const char __user *buf,
				    size_t count, loff_t *pos)
{
	struct et_pci_dev *et_dev = container_of(fp->private_data,
					struct et_pci_dev, misc_ops_dev);
	ssize_t rv = 0;
	bool use_mmio = false;

	//TODO: JIRA SW-948: Support Multi-Channel DMA
	if (et_dev->bulk_cfg == BULK_CFG_AUTO) {
		if (count < DMA_THRESHOLD)
			use_mmio = true;
	} else if (et_dev->bulk_cfg == BULK_CFG_MMIO) {
		use_mmio = true;
	}

	if (use_mmio) {
		rv = et_mmio_write_from_user(buf, count, pos, et_dev);
	} else {
		rv = et_dma_pull_from_user(buf, count, pos,
					   ET_DMA_CHAN_ID_READ_0, et_dev);
	}

	return rv;
}

static ssize_t esperanto_pcie_mgmt_read(struct file *fp, char __user *buf,
				   size_t count, loff_t *pos)
{
	struct et_pci_dev *et_dev = container_of(fp->private_data,
					struct et_pci_dev, misc_mgmt_dev);
	ssize_t rv;
	bool use_mmio = false;

	//TODO: JIRA SW-948: Support Multi-Channel DMA
	if (et_dev->bulk_cfg == BULK_CFG_AUTO) {
		if (count < DMA_THRESHOLD)
			use_mmio = true;
	} else if (et_dev->bulk_cfg == BULK_CFG_MMIO) {
		use_mmio = true;
	}

	if (use_mmio) {
		rv = et_mmio_read_to_user(buf, count, pos, et_dev);
	} else {
		rv = et_dma_push_to_user(buf, count, pos,
					 ET_DMA_CHAN_ID_WRITE_0, et_dev);
	}

	return rv;
}

static ssize_t esperanto_pcie_mgmt_write(struct file *fp, const char __user *buf,
				    size_t count, loff_t *pos)
{
	struct et_pci_dev *et_dev = container_of(fp->private_data,
					struct et_pci_dev, misc_mgmt_dev);
	ssize_t rv = 0;
	bool use_mmio = false;

	//TODO: JIRA SW-948: Support Multi-Channel DMA
	if (et_dev->bulk_cfg == BULK_CFG_AUTO) {
		if (count < DMA_THRESHOLD)
			use_mmio = true;
	} else if (et_dev->bulk_cfg == BULK_CFG_MMIO) {
		use_mmio = true;
	}

	if (use_mmio) {
		rv = et_mmio_write_from_user(buf, count, pos, et_dev);
	} else {
		rv = et_dma_pull_from_user(buf, count, pos,
					   ET_DMA_CHAN_ID_READ_0, et_dev);
	}

	return rv;
}

static loff_t esperanto_pcie_llseek(struct file *fp, loff_t pos, int whence)
{
	loff_t new_pos = 0;

	switch (whence) {
	case SEEK_SET:
		new_pos = pos;
		break;
	case SEEK_CUR:
		new_pos = fp->f_pos + pos;
		break;
	case SEEK_END:
	default:
		pr_err("whence %d not supported\n", whence);
		return -EINVAL;
	}

	//Could do MMIO or DMA based on the size; so, either bounds check
	//passing is sufficent.
	if (et_mmio_iomem_idx((uint64_t)new_pos, 1) < 0) {
		if (et_dma_bounds_check((uint64_t)new_pos, 1)) {
			return -EINVAL;
		}
	}

	fp->f_pos = new_pos;

	return new_pos;
}

static __poll_t esperanto_pcie_ops_poll(struct file *fp, poll_table *wait)
{
	int i;
	__poll_t mask = 0;
	struct et_msg_node *msg;
	struct et_pci_dev *et_dev;
	struct et_vqueue_common *vq_common;
	struct miscdevice *misc_ops_dev_ptr = fp->private_data;

	et_dev = container_of(misc_ops_dev_ptr,
			      struct et_pci_dev, misc_ops_dev);

	vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;

	poll_wait(fp, &vq_common->vqueue_wq, wait);

	for (i = 0; i < vq_common->queue_count; i++) {
		mutex_lock(&et_dev->vqueue_mm_pptr[i]->buf_count_mutex);
		mutex_lock(&et_dev->vqueue_mm_pptr[i]->threshold_mutex);
		mutex_lock(&et_dev->vqueue_mm_pptr[i]->sq_bitmap_mutex);

		if (et_dev->vqueue_mm_pptr[i]->available_buf_count >=
		    et_dev->vqueue_mm_pptr[i]->available_threshold) {
			if (!(vq_common->sq_bitmap & (1 << i)) &&
			    et_dev->vqueue_mm_pptr[i]->is_ready) {
				mask |= EPOLLOUT;
				vq_common->sq_bitmap |= (1 << i);
			}
		} else {
			vq_common->sq_bitmap &= ~((u32)1 << i);
		}

		mutex_unlock(&et_dev->vqueue_mm_pptr[i]->sq_bitmap_mutex);
		mutex_unlock(&et_dev->vqueue_mm_pptr[i]->threshold_mutex);
		mutex_unlock(&et_dev->vqueue_mm_pptr[i]->buf_count_mutex);

		mutex_lock(&et_dev->vqueue_mm_pptr[i]->cq_bitmap_mutex);

		if (usr_message_available(et_dev->vqueue_mm_pptr[i], &msg)) {
			if (!(vq_common->cq_bitmap & (1 << i))) {
				mask |= EPOLLIN;
				vq_common->cq_bitmap |= (1 << i);
			}
		} else {
			vq_common->cq_bitmap &= ~((u32)1 << i);
		}

		mutex_unlock(&et_dev->vqueue_mm_pptr[i]->cq_bitmap_mutex);
	}

	return mask;
}

static long esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd,
				     unsigned long arg)
{
	uint64_t dram_base, dram_size;
	uint64_t mbox_rdy, mbox_max_msg;
	u32 bulk_cfg;
	struct et_mbox *ops_mbox_ptr;
	struct et_pci_dev *et_dev;
	u32 reset_bitmap;
	mmio_info_t mmio_info;
	cmd_info_t cmd_info;
	rsp_info_t rsp_info;
	sq_availability_threshold_t sq_threshold;
	struct miscdevice *misc_ops_dev_ptr = fp->private_data;
	size_t size;
	u16 max_size;
	int i, rc;
	struct et_vqueue_common *vq_common;

	et_dev = container_of(misc_ops_dev_ptr,
			      struct et_pci_dev, misc_ops_dev);
	ops_mbox_ptr = &et_dev->mbox_mm;

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case ETSOC1_IOCTL_GET_USER_DRAM_BASE:
		if (copy_to_user((u64 *)arg, &et_dev->hm_dram_base, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_USER_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_USER_DRAM_SIZE:
		if (copy_to_user((u64 *)arg, &et_dev->hm_dram_size, size)) {
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

	case ETSOC1_IOCTL_GET_VQ_COUNT:
		vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;

		if (copy_to_user((u64 *)arg, &vq_common->queue_count, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_VQ_COUNT: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_VQ_MAX_MSG_SIZE:
		vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;

		max_size = vq_common->queue_buf_size - ET_VQUEUE_HEADER_SIZE;
		if (copy_to_user((u64 *)arg, &max_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_VQ_MAX_MSG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_RESET_VQ:
		vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;
		reset_bitmap = (u32)arg;
		for (i = 0; i < vq_common->queue_count; i++) {
			if (reset_bitmap & (0x1U << i))
				et_vqueue_reset(et_dev->vqueue_mm_pptr[i]);
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_SQ_MSG:
		if (copy_from_user(&cmd_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		if (cmd_info.flags & CMD_INFO_FLAG_DMA) {
			return et_dma_move_data(et_dev, cmd_info.sq_index,
						(void __user *)cmd_info.cmd,
						cmd_info.size);
		} else {
			return et_vqueue_write_from_user
				(et_dev->vqueue_mm_pptr[cmd_info.sq_index],
				(char *)cmd_info.cmd, cmd_info.size);
		}

	case ETSOC1_IOCTL_POP_CQ_MSG:
		if (copy_from_user(&rsp_info, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		return et_vqueue_read_to_user
				(et_dev->vqueue_mm_pptr[rsp_info.cq_index],
				(char __user *)rsp_info.rsp, rsp_info.size);

	case ETSOC1_IOCTL_GET_SQ_AVAILABILITY_BITMAP:
		vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;

		if (copy_to_user((u64 *)arg, &vq_common->sq_bitmap,
				 size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_SQ_AVAILABILITY_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_CQ_AVAILABILITY_BITMAP:
		vq_common = et_dev->vqueue_mm_pptr[0]->vqueue_common;

		if (copy_to_user((u64 *)arg, &vq_common->cq_bitmap, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_CQ_AVAILABILITY_BITMAP: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_SQ_AVAILABILITY_THRESHOLD:
		if (copy_from_user(&sq_threshold, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EINVAL;
		mutex_lock
		(&et_dev->vqueue_mm_pptr[sq_threshold.index]->threshold_mutex);

		et_dev->vqueue_mm_pptr[sq_threshold.index]
		->available_threshold = sq_threshold.count;

		mutex_unlock
		(&et_dev->vqueue_mm_pptr[sq_threshold.index]->threshold_mutex);
		return 0;
	}

	switch (cmd & ~IOCSIZE_MASK) {
	case ETSOC1_IOCTL_GET_DRAM_BASE & ~IOCSIZE_MASK:
		dram_base = DRAM_MEMMAP_BEGIN;
		if (copy_to_user((uint64_t *)arg, &dram_base, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_DRAM_SIZE & ~IOCSIZE_MASK:
		dram_size = DRAM_MEMMAP_SIZE;
		if (copy_to_user((uint64_t *)arg, &dram_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_BULK_CFG & ~IOCSIZE_MASK:
		bulk_cfg = (uint32_t)arg;

		if (arg > BULK_CFG_DMA) {
			pr_err("Invalid bulk cfg %d\n", bulk_cfg);
			return -EINVAL;
		}

		mutex_lock(&et_dev->dev_mutex);
		et_dev->bulk_cfg = bulk_cfg;
		mutex_unlock(&et_dev->dev_mutex);

		return 0;

	case ETSOC1_IOCTL_GET_MBOX_MAX_MSG & ~IOCSIZE_MASK:
		mbox_max_msg = ET_MBOX_MAX_MSG_LEN;
		if (copy_to_user((uint64_t *)arg, &mbox_max_msg, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_MBOX_MAX_MSG: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_RESET_MBOX:
		et_mbox_reset(ops_mbox_ptr);
		return 0;

	case ETSOC1_IOCTL_GET_MBOX_READY & ~IOCSIZE_MASK:
		rc =
		wait_event_interruptible_timeout(ops_mbox_ptr->user_msg_wq,
						 et_mbox_ready(ops_mbox_ptr),
						 MBOX_TIMEOUT_SEC * HZ);
		if (rc == -ERESTARTSYS)
			return rc;
		else if (!rc)
			return -ETIMEDOUT;

		mbox_rdy = !!et_mbox_ready(ops_mbox_ptr);
		if (copy_to_user((uint64_t *)arg, &mbox_rdy, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_MBOX_READY: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_MBOX(0):
		return et_mbox_write_from_user(ops_mbox_ptr,
					       (char __user *)arg, size);

	case ETSOC1_IOCTL_POP_MBOX(0):
		return et_mbox_read_to_user(ops_mbox_ptr,
					    (char __user *)arg, size);

	default:
		pr_err("%s: unknown cmd: 0x%x\n", __func__, cmd);
		return -EINVAL;
	}
	return -EINVAL;
}

static long esperanto_pcie_mgmt_ioctl(struct file *fp, unsigned int cmd,
				      unsigned long arg)
{
	uint64_t dram_base, dram_size;
	uint64_t fw_update_reg_base;
	uint32_t fw_update_reg_size;
	u64 mbox_rdy, mbox_max_msg;
	u32 bulk_cfg;
	struct et_mbox *mgmt_mbox_ptr;
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_mgmt_dev_ptr = fp->private_data;
	size_t size;
	int rc;

	et_dev = container_of(misc_mgmt_dev_ptr,
			      struct et_pci_dev, misc_mgmt_dev);
	mgmt_mbox_ptr = &et_dev->mbox_sp;

	size = _IOC_SIZE(cmd);

	switch (cmd & ~IOCSIZE_MASK) {
	case ETSOC1_IOCTL_GET_DRAM_BASE & ~IOCSIZE_MASK:
		dram_base = DRAM_MEMMAP_BEGIN;
		if (copy_to_user((uint64_t *)arg, &dram_base, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_DRAM_SIZE & ~IOCSIZE_MASK:
		dram_size = DRAM_MEMMAP_SIZE;
		if (copy_to_user((uint64_t *)arg, &dram_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_FW_UPDATE_REG_BASE & ~IOCSIZE_MASK:
		fw_update_reg_base = FW_UPDATE_REGION_BEGIN;
		if (copy_to_user((uint64_t *)arg, &fw_update_reg_base, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_FW_UPDATE_REG_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_GET_FW_UPDATE_REG_SIZE & ~IOCSIZE_MASK:
		fw_update_reg_size = FW_UPDATE_REGION_SIZE;
		if (copy_to_user((uint32_t *)arg, &fw_update_reg_size, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_FW_UPDATE_REG_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_SET_BULK_CFG & ~IOCSIZE_MASK:
		bulk_cfg = (uint32_t)arg;

		if (arg > BULK_CFG_DMA) {
			pr_err("Invalid bulk cfg %d\n", bulk_cfg);
			return -EINVAL;
		}

		mutex_lock(&et_dev->dev_mutex);
		et_dev->bulk_cfg = bulk_cfg;
		mutex_unlock(&et_dev->dev_mutex);

		return 0;

	case ETSOC1_IOCTL_GET_MBOX_MAX_MSG & ~IOCSIZE_MASK:
		mbox_max_msg = ET_MBOX_MAX_MSG_LEN;
		if (copy_to_user((uint64_t *)arg, &mbox_max_msg, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_MBOX_MAX_MSG: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_RESET_MBOX:
		et_mbox_reset(mgmt_mbox_ptr);
		return 0;

	case ETSOC1_IOCTL_GET_MBOX_READY & ~IOCSIZE_MASK:
		rc =
		wait_event_interruptible_timeout(mgmt_mbox_ptr->user_msg_wq,
						 et_mbox_ready(mgmt_mbox_ptr),
						 MBOX_TIMEOUT_SEC * HZ);
		if (rc == -ERESTARTSYS)
			return rc;
		else if (!rc)
			return -ETIMEDOUT;

		mbox_rdy = !!et_mbox_ready(mgmt_mbox_ptr);
		if (copy_to_user((uint64_t *)arg, &mbox_rdy, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_MBOX_READY: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_MBOX(0):
		return et_mbox_write_from_user(mgmt_mbox_ptr,
					       (char __user *)arg, size);

	case ETSOC1_IOCTL_POP_MBOX(0):
		return et_mbox_read_to_user(mgmt_mbox_ptr,
					    (char __user *)arg, size);

	default:
		pr_err("%s: unknown cmd: 0x%x\n", __func__, cmd);
		return -EINVAL;
	}
	return -EINVAL;
}

static int esperanto_pcie_ops_open(struct inode *inode, struct file *filp)
{
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_ops_dev_ptr = filp->private_data;

	/* TODO: In order to support Edge Triggered EPOLL event distribution,
	 * device node should be opened in non-blocking mode to avoid
	 * starvation due to blocking read or write. But this can be done
	 * only after MBox is retired since MBox implementation performs
	 * blocking calls.
	 */
	et_dev = container_of(misc_ops_dev_ptr,
			      struct et_pci_dev, misc_ops_dev);

	spin_lock(&et_dev->ops_open_lock);
	if (et_dev->is_ops_open) {
		spin_unlock(&et_dev->ops_open_lock);
		pr_err("Tried to open same device multiple times\n");
		return -EBUSY; /* already open */
	}
	et_dev->is_ops_open = true;
	spin_unlock(&et_dev->ops_open_lock);

	return 0;
}

static int esperanto_pcie_mgmt_open(struct inode *inode, struct file *filp)
{
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_mgmt_dev_ptr = filp->private_data;

	et_dev = container_of(misc_mgmt_dev_ptr,
			      struct et_pci_dev, misc_mgmt_dev);

	spin_lock(&et_dev->mgmt_open_lock);
	if (et_dev->is_mgmt_open) {
		spin_unlock(&et_dev->mgmt_open_lock);
		pr_err("Tried to open same device multiple times\n");
		return -EBUSY; /* already open */
	}
	et_dev->is_mgmt_open = true;
	spin_unlock(&et_dev->mgmt_open_lock);

	return 0;
}

static int esperanto_pcie_ops_release(struct inode *inode, struct file *filp)
{
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_ops_dev_ptr = filp->private_data;

	et_dev = container_of(misc_ops_dev_ptr,
			      struct et_pci_dev, misc_ops_dev);
	et_dev->is_ops_open = false;

	return 0;
}

static int esperanto_pcie_mgmt_release(struct inode *inode, struct file *filp)
{
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_mgmt_dev_ptr = filp->private_data;

	et_dev = container_of(misc_mgmt_dev_ptr,
			      struct et_pci_dev, misc_mgmt_dev);
	et_dev->is_mgmt_open = false;

	return 0;
}

static const struct file_operations et_pcie_ops_fops = {
	.owner = THIS_MODULE,
	.read = esperanto_pcie_ops_read,
	.write = esperanto_pcie_ops_write,
	.llseek = esperanto_pcie_llseek,
	.poll = esperanto_pcie_ops_poll,
	.unlocked_ioctl = esperanto_pcie_ops_ioctl,
	.open = esperanto_pcie_ops_open,
	.release = esperanto_pcie_ops_release,
};

static const struct file_operations et_pcie_mgmt_fops = {
	.owner = THIS_MODULE,
	.read = esperanto_pcie_mgmt_read,
	.write = esperanto_pcie_mgmt_write,
	.llseek = esperanto_pcie_llseek,
	.unlocked_ioctl = esperanto_pcie_mgmt_ioctl,
	.open = esperanto_pcie_mgmt_open,
	.release = esperanto_pcie_mgmt_release,
};

static int create_et_pci_dev(struct et_pci_dev **new_dev, struct pci_dev *pdev)
{
	struct et_pci_dev *et_dev;
	char wq_name[32];

	et_dev = devm_kzalloc(&pdev->dev, sizeof(struct et_pci_dev),
			      GFP_KERNEL);
	*new_dev = et_dev;

	if (!et_dev) return -ENOMEM;

	et_dev->pdev = pdev;

	et_dev->index = get_index();
	if (et_dev->index < 0) {
		dev_err(&pdev->dev, "get index failed\n");
		return -ENODEV;
	}

	et_dev->is_ops_open = false;
	et_dev->is_mgmt_open = false;
	et_dev->aborting = false;

	mutex_init(&et_dev->dev_mutex);

	snprintf(wq_name, sizeof(wq_name), "%s_wq%d",
		 dev_name(&et_dev->pdev->dev), et_dev->index);
	et_dev->workqueue = create_singlethread_workqueue(wq_name);

	if (!et_dev->workqueue)
		return -ENOMEM;

	INIT_WORK(&et_dev->isr_work, et_isr_work);

	timer_setup(&et_dev->missed_irq_timer, et_missed_irq_timeout, 0);

	spin_lock_init(&et_dev->ops_open_lock);
	spin_lock_init(&et_dev->mgmt_open_lock);
	spin_lock_init(&et_dev->abort_lock);

	return 0;
}

static void destroy_et_pci_dev(struct et_pci_dev *et_dev)
{
	if (et_dev->workqueue) {
		destroy_workqueue(et_dev->workqueue);
	}

	mutex_destroy(&et_dev->dev_mutex);
}

static void et_unmap_bars(struct et_pci_dev *et_dev)
{
	int i;

	for (i = 0; i < IOMEM_REGIONS; ++i) {
		if (et_dev->iomem[i]) {
			iounmap(et_dev->iomem[i]);
		}
	}

	pci_release_regions(et_dev->pdev);
}

static int et_map_bars(struct et_pci_dev *et_dev)
{
	int rc, i;

	struct pci_dev *pdev = et_dev->pdev;

	rc = pci_request_regions(pdev, DRIVER_NAME);
	if (rc) {
		dev_err(&pdev->dev, "request regions failed\n");
		return rc;
	}

	for (i = 0; i < IOMEM_REGIONS; ++i) {
		//Map all regions that don't care about ordering with wc (write
		//combining) for perfomance
		if (BAR_MAPPINGS[i].strictly_order_access) {
			et_dev->iomem[i] =
				pci_iomap_range(pdev, BAR_MAPPINGS[i].bar,
						BAR_MAPPINGS[i].bar_offset,
						BAR_MAPPINGS[i].size);
		} else {
			et_dev->iomem[i] =
				pci_iomap_wc_range(pdev, BAR_MAPPINGS[i].bar,
						   BAR_MAPPINGS[i].bar_offset,
						   BAR_MAPPINGS[i].size);
		}

		if (IS_ERR(et_dev->iomem[i])) {
			dev_err(&pdev->dev, "mapping BAR_MAPPINGS[%d] failed\n",
				i);
			rc = PTR_ERR(et_dev->iomem[i]);
			goto error;
		}
	}

	return 0;
error:
	et_unmap_bars(et_dev);
	return rc;
}

static void interrupt_mbox_sp(void __iomem *r_pu_trg_pcie)
{
	//TODO: this could drop interrupts if we write to fast. Cycle through mask bits?
	iowrite32(1, r_pu_trg_pcie + IPI_TRIGGER_OFFSET);
}

static void interrupt_mbox_mm(void __iomem *r_pu_trg_pcie)
{
	iowrite32(1, r_pu_trg_pcie + MMM_INT_INC_OFFSET);
	return;
}

static int esperanto_pcie_probe(struct pci_dev *pdev,
				const struct pci_device_id *pci_id)
{
	int rc, i, irq_vec, irq_cnt_init;
	/* TODO: enable after addition of MSI-X support */
	//char irq_name[16];
	struct et_pci_dev *et_dev;
	unsigned long flags;

	//Create instance data for this device, save it to drvdata
	rc = create_et_pci_dev(&et_dev, pdev);
	pci_set_drvdata(pdev, et_dev); //Set even if NULL
	if (rc < 0) {
		dev_err(&pdev->dev, "create_et_pci_dev failed\n");
		return rc;
	}

	rc = pci_enable_device_mem(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "enable device failed\n");
		goto error_free_dev;
	}

	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (rc) {
		dev_err(&pdev->dev, "set dma mask failed\n");
		goto error_disable_dev;
	}

	pci_set_master(pdev);

	rc = et_map_bars(et_dev);
	if (rc) {
		dev_err(&pdev->dev, "mapping bars failed\n");
		goto error_disable_dev;
	}

	et_dev->r_pu_trg_pcie = et_dev->iomem[IOMEM_R_PU_TRG_PCIE];

	if (et_vqueues_discover(et_dev) != 0) {
		dev_err(&pdev->dev, "VQs setup failed\n");
		goto error_unmap_bars;
	}

	et_mbox_init(&et_dev->mbox_sp, et_dev->iomem[IOMEM_R_PU_MBOX_PC_SP],
		     et_dev->iomem[IOMEM_R_PU_TRG_PCIE], interrupt_mbox_sp);

	et_mbox_init(&et_dev->mbox_mm, et_dev->iomem[IOMEM_R_PU_MBOX_PC_MM],
		     et_dev->iomem[IOMEM_R_PU_TRG_PCIE], interrupt_mbox_mm);

	et_dma_init(et_dev);

	rc = pci_alloc_irq_vectors(pdev, REQ_IRQ_NUM, REQ_IRQ_NUM, PCI_IRQ_MSI);
	if (rc < 0) {
		dev_err(&pdev->dev, "alloc irq vectors failed -- %d/%d\n",
			REQ_IRQ_NUM, rc);
		goto error_vqueues_cleanup;
	}
	else {
		et_dev->num_irq_vecs = rc;
	}

	for (i = 0, irq_cnt_init = 0;
	     i < et_dev->num_irq_vecs; i++, irq_cnt_init++) {
		irq_vec = pci_irq_vector(pdev, i);
		if (irq_vec < 0) {
			rc = -ENODEV;
			dev_err(&pdev->dev, "finding irq vector failed\n");
			goto error_free_irq_vecs;
		}

		if (i == 0) {
			rc = request_irq(irq_vec, et_pcie_isr, 0, "common_irq",
					 (void *)et_dev);
			/* TODO: enable after addition of MSI-X support */
			//rc = request_irq(irq_vec, et_pcie_sp_isr, 0,
			//		   "irq_sp_cq",
			//		   (void *)&et_dev->vqueue_sp);
		}
		/* TODO: enable after addition of MSI-X support */
		//else {
		//	snprintf(irq_name, sizeof(irq_name), "irq_mm_cq_%d",
		//		 i - 1);
		//	rc = request_irq(irq_vec, et_pcie_isr, 0, irq_name,
		//			 (void *)et_dev->vqueue_mm_pptr[i]);
		//}

		if (rc) {
			dev_err(&pdev->dev, "request irq failed\n");
			goto error_free_irq_vecs;
		}
	}

	mod_timer(&et_dev->missed_irq_timer, jiffies + MISSED_IRQ_TIMEOUT);

	et_dev->misc_ops_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->misc_ops_dev.fops  = &et_pcie_ops_fops;
	et_dev->misc_ops_dev.name  = devm_kasprintf(&pdev->dev, GFP_KERNEL,
						    "et%d_ops",
						    et_dev->index);

	et_dev->misc_mgmt_dev.minor = MISC_DYNAMIC_MINOR;
	et_dev->misc_mgmt_dev.fops  = &et_pcie_mgmt_fops;
	et_dev->misc_mgmt_dev.name  = devm_kasprintf(&pdev->dev, GFP_KERNEL,
						     "et%d_mgmt",
						     et_dev->index);
	rc = misc_register(&et_dev->misc_ops_dev);
	if (rc) {
		dev_err(&pdev->dev, "misc ops register failed\n");
		goto error_disable_irq;
	}

	rc = misc_register(&et_dev->misc_mgmt_dev);
	if (rc) {
		dev_err(&pdev->dev, "misc mgmt register failed\n");
		goto error_disable_irq;
	}

	return 0;

error_disable_irq:
	spin_lock_irqsave(&et_dev->abort_lock, flags);
	et_dev->aborting = true;
	spin_unlock_irqrestore(&et_dev->abort_lock, flags);

	//Disable anything that could trigger additional calls to isr_work
	//in another core before canceling it
	disable_irq(et_dev->num_irq_vecs);
	del_timer_sync(&et_dev->missed_irq_timer);

	cancel_work_sync(&et_dev->isr_work);

	et_dma_destroy(et_dev);

	et_mbox_destroy(&et_dev->mbox_sp);
	et_mbox_destroy(&et_dev->mbox_mm);

	/* TODO: enable when we switch from MBox to VQs */
	//et_vqueue_cleanup(et_dev, VQUEUE_SP);
	et_vqueue_cleanup(et_dev, VQUEUE_MM);

error_free_irq_vecs:
	for (i = 0; i < irq_cnt_init; i++)
		free_irq(pci_irq_vector(pdev, i), (void *)et_dev);

	pci_free_irq_vectors(pdev);

error_vqueues_cleanup:
	/* TODO: enable when we switch from MBox to VQs */
	//et_vqueue_cleanup(et_dev, VQUEUE_SP);
	et_vqueue_cleanup(et_dev, VQUEUE_MM);

error_unmap_bars:
	et_unmap_bars(et_dev);

error_disable_dev:
	pci_clear_master(pdev);
	pci_disable_device(pdev);

error_free_dev:
	destroy_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);

	return rc;
}

static void esperanto_pcie_remove(struct pci_dev *pdev)
{
	struct et_pci_dev *et_dev;
	int i;
	unsigned long flags;
	u8 index;

	et_dev = pci_get_drvdata(pdev);
	if (!et_dev) return;

	index = et_dev->index;

	misc_deregister(&et_dev->misc_ops_dev);
	misc_deregister(&et_dev->misc_mgmt_dev);

	clear_bit(index, &dev_bitmap);

	spin_lock_irqsave(&et_dev->abort_lock, flags);
	et_dev->aborting = true;
	spin_unlock_irqrestore(&et_dev->abort_lock, flags);

	//Disable anything that could trigger additional calls to isr_work
	//in another core before canceling it
	disable_irq(et_dev->num_irq_vecs);
	del_timer_sync(&et_dev->missed_irq_timer);

	cancel_work_sync(&et_dev->isr_work);

	et_dma_destroy(et_dev);

	et_mbox_destroy(&et_dev->mbox_sp);
	et_mbox_destroy(&et_dev->mbox_mm);

	for (i = 0; i < et_dev->num_irq_vecs; i++)
		free_irq(pci_irq_vector(pdev, i), (void *)et_dev);

	pci_free_irq_vectors(pdev);

	/* TODO: enable when we switch from MBox to VQs */
	//et_vqueue_cleanup(et_dev, VQUEUE_SP);
	et_vqueue_cleanup(et_dev, VQUEUE_MM);

	et_unmap_bars(et_dev);

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
