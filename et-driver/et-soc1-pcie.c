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

#include "et_dma.h"
#include "et_ioctl.h"
#include "et_layout.h"
#include "et_mbox.h"
#include "et_mmio.h"
#include "et_pci_dev.h"
#include "et_ringbuffer.h"
#include "hal_device.h"

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

//The SoC supports MSI, MSI-X, and Legacy PCI IRQs. Legacy gives you exactly 1
//interrupt vector. MSI is up to 32 in powers of 2. MSI-X is up to 2048 in any
//step. We want two (one for each mbox).
#define MIN_VECS 1
#define REQ_VECS 2

/*
 * Timeout is 250ms. Picked because it's unlikley the driver will miss an IRQ,
 * so this is a contigency and does not need to be checked often.
 */
#define MISSED_IRQ_TIMEOUT (HZ / 4)

/*
 * Register offsets, per hardware implementation.
 */
#define IPI_TRIGGER_OFFSET 0
#define MMM_INT_INC_OFFSET 4

static u8 get_index(void)
{
	u8 index;

	if (dev_bitmap == ~0UL)
		return -EBUSY;
	index = ffz(dev_bitmap);
	set_bit(index, &dev_bitmap);
	return index;
}

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
}

//TODO: tune this value
#define DMA_THRESHOLD 1024

static ssize_t esperanto_pcie_read(struct file *fp, char __user *buf,
				   size_t count, loff_t *pos)
{
	struct et_pci_dev *et_dev = container_of(fp->private_data,
					struct et_pci_dev, misc_ops_dev);
	ssize_t rv;
	bool use_mmio = false;

	//TODO: JIRA SW-948: Support Multi-Channel DMA
	if (et_dev->bulk_cfg == BULK_CFG_AUT0) {
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

static ssize_t esperanto_pcie_write(struct file *fp, const char __user *buf,
				    size_t count, loff_t *pos)
{
	struct et_pci_dev *et_dev = container_of(fp->private_data,
					struct et_pci_dev, misc_ops_dev);
	ssize_t rv = 0;
	bool use_mmio = false;

	//TODO: JIRA SW-948: Support Multi-Channel DMA
	if (et_dev->bulk_cfg == BULK_CFG_AUT0) {
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

static long esperanto_pcie_ops_ioctl(struct file *fp, unsigned int cmd,
				     unsigned long arg)
{
	uint64_t dram_base, dram_size;
	uint64_t mbox_rdy, mbox_max_msg;
	u32 bulk_cfg;
	struct et_mbox *ops_mbox_ptr;
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_ops_dev_ptr = fp->private_data;
	size_t size;

	et_dev = container_of(misc_ops_dev_ptr,
			      struct et_pci_dev, misc_ops_dev);
	ops_mbox_ptr = &et_dev->mbox_mm;

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
		mbox_rdy = (uint64_t)et_mbox_ready(ops_mbox_ptr);
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
	u64 mbox_rdy, mbox_max_msg;
	struct et_mbox *mgmt_mbox_ptr;
	struct et_pci_dev *et_dev;
	struct miscdevice *misc_mgmt_dev_ptr = fp->private_data;
	size_t size;

	et_dev = container_of(misc_mgmt_dev_ptr,
			      struct et_pci_dev, misc_mgmt_dev);
	mgmt_mbox_ptr = &et_dev->mbox_sp;

	size = _IOC_SIZE(cmd);

	switch (cmd & ~IOCSIZE_MASK) {
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
		mbox_rdy = (uint64_t)et_mbox_ready(mgmt_mbox_ptr);
		if (copy_to_user((uint64_t *)arg, &mbox_rdy, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_GET_MBOX_READY: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case ETSOC1_IOCTL_PUSH_MBOX(0):
		if (et_mbox_write_from_user(mgmt_mbox_ptr,
					    (char __user *)arg, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_PUSH_MBOX: failed to write mbox from user\n");
			return -ENOMEM;
		}
	return 0;

	case ETSOC1_IOCTL_POP_MBOX(0):
		if (et_mbox_read_to_user(mgmt_mbox_ptr,
					 (char __user *)arg, size)) {
			pr_err("ioctl: ETSOC1_IOCTL_POP_MBOX: failed to read mbox to user\n");
			return -ENOMEM;
		}
		return 0;

	default:
		pr_err("%s: unknown cmd: 0x%x\n", __func__, cmd);
		return -EINVAL;
	}
	return -EINVAL;
}

static int esperanto_pcie_open(struct inode *inode, struct file *filp)
{
	//int rc;

 	/*mutex_lock(&minor_dev->open_close_mutex);

 	if (minor_dev->ref_count == 0) { //TODO: is there anything in inode I can use - does it keep refcnt somewhere and does that refcnt work like I want?
 		minor_dev->ref_count++;
 		rc = 0;
 	}
 	else {
 		pr_err("Tried to open same device multiple times\n");
 		rc = -ENODEV;
 	}

 	mutex_unlock(&minor_dev->open_close_mutex);*/

	//return rc;
	return 0;
}

static int esperanto_pcie_release(struct inode *inode, struct file *filp)
{
	/*struct et_pci_minor_dev *minor_dev = filp->private_data;

	mutex_lock(&minor_dev->open_close_mutex);

 	if (minor_dev->ref_count) {
 		minor_dev->ref_count--;
 	}

 	mutex_unlock(&minor_dev->open_close_mutex);*/

	return 0;
}

static const struct file_operations et_pcie_ops_fops = {
	.owner = THIS_MODULE,
	.read = esperanto_pcie_read,
	.write = esperanto_pcie_write,
	.llseek = esperanto_pcie_llseek,
	.unlocked_ioctl = esperanto_pcie_ops_ioctl,
	.open = esperanto_pcie_open,
	.release = esperanto_pcie_release,
};

static const struct file_operations et_pcie_mgmt_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = esperanto_pcie_mgmt_ioctl,
	.open = esperanto_pcie_open,
	.release = esperanto_pcie_release,
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

	mutex_init(&et_dev->dev_mutex);

	snprintf(wq_name, sizeof(wq_name), "%s_wq%d",
		 dev_name(&et_dev->pdev->dev), et_dev->index);
	et_dev->workqueue = create_singlethread_workqueue(wq_name);

	if (!et_dev->workqueue)
		return -ENOMEM;

	INIT_WORK(&et_dev->isr_work, et_isr_work);

	timer_setup(&et_dev->missed_irq_timer, et_missed_irq_timeout, 0);

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
	int rc;
	struct et_pci_dev *et_dev;
	int irq_vec;
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

	et_mbox_init(&et_dev->mbox_mm, et_dev->iomem[IOMEM_R_PU_MBOX_PC_MM],
		     et_dev->iomem[IOMEM_R_PU_TRG_PCIE], interrupt_mbox_mm);
	et_mbox_init(&et_dev->mbox_sp, et_dev->iomem[IOMEM_R_PU_MBOX_PC_SP],
		     et_dev->iomem[IOMEM_R_PU_TRG_PCIE], interrupt_mbox_sp);

	et_dma_init(et_dev);

	rc = pci_alloc_irq_vectors(pdev, MIN_VECS, REQ_VECS, PCI_IRQ_MSI | PCI_IRQ_MSIX);
	if (rc < 0) {
		dev_err(&pdev->dev, "alloc irq vectors failed\n");
		goto error_unmap_bars;
	}
	else {
		et_dev->num_irq_vecs = rc;
	}

	//TODO: For now, only using one vec. In the future, take advantage of multi vec. JIRA SW-953
	irq_vec = pci_irq_vector(pdev, 0);
	if (irq_vec < 0) {
		rc = -ENODEV;
		dev_err(&pdev->dev, "finding irq vector failed\n");
		goto error_free_irq_vecs;
	}

	rc = request_irq(irq_vec, et_pcie_isr, IRQF_SHARED, DRIVER_NAME,
			 (void*)et_dev);
	if (rc) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto error_free_irq_vecs;
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

	free_irq(irq_vec, (void*)et_dev);

error_free_irq_vecs:
	pci_free_irq_vectors(pdev);


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
	int irq_vec;
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

	irq_vec = pci_irq_vector(pdev, 0);

	if (irq_vec >= 0) {
		free_irq(irq_vec, (void*)et_dev);
	}

	pci_free_irq_vectors(pdev);

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
