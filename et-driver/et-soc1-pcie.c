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
#include <linux/cdev.h>
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

static struct class *pclass;
static int major;
static int curr_minor;
static int curr_dev = 0;

//The SoC supports MSI, MSI-X, and Legacy PCI IRQs. Legacy gives you exactly 1
//interrupt vector. MSI is up to 32 in powers of 2. MSI-X is up to 2048 in any
//step. We want two (one for each mbox).
#define MIN_VECS 1
#define REQ_VECS 2

/* MAX_MINORS picked arbitrarily */
#define MIN_MINOR 0
#define MAX_MINORS (MINORS_PER_SOC * 16)

static irqreturn_t et_pcie_isr(int irq, void *dev_id)
{
	struct et_pci_dev *et_dev = (struct et_pci_dev *)dev_id;

	//TODO: if multi-vector setup, dispatch without broadcasting to everyone
	et_mbox_isr(&et_dev->mbox_sp);
	et_mbox_isr(&et_dev->mbox_mm);

	return IRQ_HANDLED;
}

//TODO: tune this value
#define DMA_THRESHOLD 1024

static ssize_t esperanto_pcie_read(struct file *fp, char __user *buf,
				   size_t count, loff_t *pos)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;
	struct et_pci_dev *et_dev = minor_dev->et_dev;
	ssize_t rv;
	bool use_mmio = false;

	mutex_lock(&minor_dev->read_write_mutex);

	switch(minor_dev->type){
	case et_cdev_type_mb_sp:
		rv = et_mbox_read_to_user(&et_dev->mbox_sp, buf, count);
		break;
	case et_cdev_type_mb_mm:
		rv = et_mbox_read_to_user(&et_dev->mbox_mm, buf, count);
		break;
	case et_cdev_type_bulk:
		if (et_dev->bulk_cfg == BULK_CFG_AUT0) {
			if (count < DMA_THRESHOLD) {
				use_mmio = true;
			}
		} else if (et_dev->bulk_cfg == BULK_CFG_MMIO) {
			use_mmio = true;
		}

		if (use_mmio) {
			rv = et_mmio_read_to_user(buf, count, pos, et_dev);
		} else {
			rv = et_dma_push_to_user(buf, count, pos,
						 et_dma_chan_write_0, et_dev);
		}

		break;
	default:
		pr_err("dev type invalid");
		rv = -EINVAL;
	}
 
  	mutex_unlock(&minor_dev->read_write_mutex);

	return rv;
}

static ssize_t esperanto_pcie_write(struct file *fp, const char __user *buf,
				    size_t count, loff_t *pos)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;
	struct et_pci_dev *et_dev = minor_dev->et_dev;
	ssize_t rv = 0;
	bool use_mmio = false;
 
	mutex_lock(&minor_dev->read_write_mutex);

	switch(minor_dev->type) {
	case et_cdev_type_mb_sp:
		rv = et_mbox_write_from_user(&et_dev->mbox_sp, buf, count);
		break;
	case et_cdev_type_mb_mm:
		rv = et_mbox_write_from_user(&et_dev->mbox_mm, buf, count);
		break;
	case et_cdev_type_bulk:
		//TODO: pick DMA channel from an ioctl
		//TODO: allow multiple bulk reads/writes at once; change locking
		//TODO: async I/O api
		if (et_dev->bulk_cfg == BULK_CFG_AUT0) {
			if (count < DMA_THRESHOLD) {
				use_mmio = true;
			}
		}
		else if (et_dev->bulk_cfg == BULK_CFG_MMIO) {
			use_mmio = true;
		}

		if (use_mmio) {
			rv = et_mmio_write_from_user(buf, count, pos, et_dev);
		} else {
			rv = et_dma_pull_from_user(buf, count, pos,
						   et_dma_chan_read_0, et_dev);
		}

		break;
	default:
		pr_err("dev type invalid");
		rv = -EINVAL;
	}

 	mutex_unlock(&minor_dev->read_write_mutex);

	return rv;
}

static loff_t esperanto_pcie_llseek(struct file *fp, loff_t pos, int whence)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;

	loff_t new_pos = 0;

	mutex_lock(&minor_dev->read_write_mutex);

	if (minor_dev->type == et_cdev_type_mb_sp ||
	    minor_dev->type == et_cdev_type_mb_mm) {
		pr_err("dev does not support lseek\n");
		new_pos = -EINVAL;
		goto error;
	}

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
		new_pos = -EINVAL;
		goto error;
	}

	//Could do MMIO or DMA based on the size; so, either bounds check
	//passing is sufficent.
	if (et_mmio_iomem_idx((uint64_t)new_pos, 1) < 0) {
		if (et_dma_bounds_check((uint64_t)new_pos, 1)) {
			new_pos = -EINVAL;
			goto error;
		}
	}

	fp->f_pos = new_pos;

error:
	mutex_unlock(&minor_dev->read_write_mutex);

	return new_pos;
}

static long esperanto_pcie_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;
	struct et_pci_dev *et_dev = minor_dev->et_dev;
	unsigned long dram_base, dram_size;
	unsigned long mbox_rdy, mbox_max_msg;

	switch (cmd) {
	case GET_DRAM_BASE:
		if (minor_dev->type != et_cdev_type_bulk) {
			// Allowed on DRAM devices only
			return -EINVAL;
		}
		dram_base = R_L3_DRAM_BASEADDR;
		if (copy_to_user((u64 *)arg, &dram_base, sizeof(u64))) {
			pr_err("ioctl: GET_DRAM_BASE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case GET_DRAM_SIZE:
		if (minor_dev->type != et_cdev_type_bulk) {
			// Allowed on DRAM devices only
			return -EINVAL;
		}
		dram_size = R_L3_DRAM_SIZE;
		if (copy_to_user((u64 *)arg, &dram_size, sizeof(u64))) {
			pr_err("ioctl: GET_DRAM_SIZE: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case GET_MBOX_MAX_MSG:
		if (minor_dev->type != et_cdev_type_mb_mm &&
		    minor_dev->type != et_cdev_type_mb_sp) {
			// Allowed on mailbox devices only
			return -EINVAL;
		}
		mbox_max_msg = ET_MBOX_MAX_MSG_SIZE;
		if (copy_to_user((u64 *)arg, &mbox_max_msg, sizeof(u64))) {
			pr_err("ioctl: GET_BOX_MAX_MSG: failed to copy to user\n");
			return -ENOMEM;
		}
		return 0;

	case RESET_MBOX:
		if (minor_dev->type == et_cdev_type_mb_mm) {
			mutex_lock(&minor_dev->read_write_mutex);
			et_mbox_reset(&et_dev->mbox_mm);
 			mutex_unlock(&minor_dev->read_write_mutex);
			return 0;
		} else if (minor_dev->type == et_cdev_type_mb_sp) {
			mutex_lock(&minor_dev->read_write_mutex);
			et_mbox_reset(&et_dev->mbox_sp);
 			mutex_unlock(&minor_dev->read_write_mutex);
			return 0;
		}
		// Allowed on mailbox devices only
		return -EINVAL;

	case GET_MBOX_READY:
		if (minor_dev->type == et_cdev_type_mb_mm) {
			mbox_rdy = (unsigned long)et_mbox_ready(&et_dev->mbox_mm);
			if (copy_to_user((u64 *)arg, &mbox_rdy, sizeof(u64))) {
				pr_err("ioctl: GET_MBOX_READY: failed to copy to user\n");
				return -ENOMEM;
			}
			return 0;
		} else if (minor_dev->type == et_cdev_type_mb_sp) {
			mbox_rdy = (unsigned long)et_mbox_ready(&et_dev->mbox_sp);
			if (copy_to_user((u64 *)arg, &mbox_rdy, sizeof(u64))) {
				pr_err("ioctl: GET_MBOX_READY: failed to copy to user\n");
				return -ENOMEM;
			}
			return 0;
		}
		// Allowed on mailbox devices only
		return -EINVAL;

	case SET_BULK_CFG:
	{
		uint32_t bulk_cfg = (uint32_t)arg;
		
		if (minor_dev->type != et_cdev_type_bulk) {
			pr_err("Tried to set bulk cfg on non-bulk device\n");
			return -EINVAL;
		}

		
		if (arg > BULK_CFG_DMA) {
			pr_err("Invalid bulk cfg %d\n", bulk_cfg);
			return -EINVAL;
		}

		mutex_lock(&minor_dev->read_write_mutex);
		et_dev->bulk_cfg = bulk_cfg;
		mutex_unlock(&minor_dev->read_write_mutex);

		return 0;
	}

	default:
		pr_err("esperanto_pcie_ioctl: unknown cmd: 0x%x\n", cmd);
		return -EINVAL;
	}
	return -EINVAL;
}

static int esperanto_pcie_open(struct inode *inode, struct file *filp)
{
	//int rc;

	//Find container of cdev, save for other file i/o calls
	struct et_pci_minor_dev *minor_dev;
 	minor_dev = container_of(inode->i_cdev, struct et_pci_minor_dev, cdev);
 	filp->private_data = minor_dev;

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

static struct file_operations et_pcie_fops = {
	.owner = THIS_MODULE,
	.read = esperanto_pcie_read,
	.write = esperanto_pcie_write,
	.llseek = esperanto_pcie_llseek,
	.unlocked_ioctl = esperanto_pcie_ioctl,
	.open = esperanto_pcie_open,
	.release = esperanto_pcie_release,
};

static int create_et_pci_dev(struct et_pci_dev **new_dev)
{
	struct et_pci_dev *et_dev;
	int i;

	et_dev = kzalloc(sizeof(struct et_pci_dev), GFP_KERNEL);
	*new_dev = et_dev;

	if (!et_dev) return -ENOMEM;

	//Initialize data for minors
	for(i = 0; i < MINORS_PER_SOC; ++i) {
		struct et_pci_minor_dev *minor_dev = &et_dev->et_minor_devs[i];

		minor_dev->et_dev = et_dev;

		mutex_init(&minor_dev->open_close_mutex);
		mutex_init(&minor_dev->read_write_mutex);

		minor_dev->type = MINOR_TYPES[i];
	}

	return 0;
}

static void destory_et_pci_dev(struct et_pci_dev *et_dev)
{
	int i;

	for(i = 0; i < MINORS_PER_SOC; ++i) {
		struct et_pci_minor_dev *minor_dev = &et_dev->et_minor_devs[i];

		mutex_destroy(&minor_dev->read_write_mutex);
		mutex_destroy(&minor_dev->open_close_mutex);

		minor_dev->et_dev = NULL;
	}

	kfree(et_dev);
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
		//Map all regions that don't map registers with wc (write
		//combining) for perfomance
		if (BAR_MAPPINGS[i].maps_regs) {
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
	//Write ipi_trigger register
	//TODO: use register map structs
	//TODO: this could drop interrupts if we write to fast. Cycle through mask bits?
	iowrite32(1, r_pu_trg_pcie);
}

static void interrupt_mbox_mm(void __iomem *r_pu_trg_pcie)
{
	//Write mmm_int_inc register
	//TODO: use register map structs
	iowrite32(1, r_pu_trg_pcie + 4);
	return;
}

static int esperanto_pcie_probe(struct pci_dev *pdev,
				const struct pci_device_id *pci_id)
{
	int rc, i;
	struct et_pci_dev *et_dev;
	int irq_vec;

	//Create instance data for this device, save it to drvdata
	rc = create_et_pci_dev(&et_dev);
	pci_set_drvdata(pdev, et_dev); //Set even if NULL
	if (rc < 0) {
		dev_err(&pdev->dev, "create_et_pci_dev failed\n");
		return rc;
	}

	et_dev->pdev = pdev;

	rc = pci_enable_device_mem(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "enable device failed\n");
		goto error_free_dev;
	}

	rc = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (rc < 0) {
		dev_err(&pdev->dev, "set dma mask failed\n");
		goto error_disable_dev;
	}

	pci_set_master(pdev);

	rc = et_map_bars(et_dev);
	if (rc) {
		dev_err(&pdev->dev, "mapping bars failed\n");
		goto error_disable_dev;
	}

	rc = pci_alloc_irq_vectors(pdev, MIN_VECS, REQ_VECS, PCI_IRQ_MSI /*TODO: MSIX*/);
	if (rc < 0) {
		dev_err(&pdev->dev, "alloc irq vectors failed\n");
		goto error_unmap_bars;
	}
	else {
		et_dev->num_irq_vecs = rc;
	}

	//TODO: For now, only using one vec. In the future, take advantage of multi vec
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

	et_mbox_init(&et_dev->mbox_mm, et_dev->iomem[IOMEM_R_PU_MBOX_PC_MM],
		     et_dev->iomem[IOMEM_R_PU_TRG_PCIE], interrupt_mbox_mm);
	et_mbox_init(&et_dev->mbox_sp, et_dev->iomem[IOMEM_R_PU_MBOX_PC_SP],
		     et_dev->iomem[IOMEM_R_PU_TRG_PCIE], interrupt_mbox_sp);

	//Create minor character devices for this device
	for (i = 0; i < MINORS_PER_SOC; ++i) {
		struct et_pci_minor_dev *minor_dev = &et_dev->et_minor_devs[i];

		dev_t dev = MKDEV(major, curr_minor);
		struct cdev *pcdev = &minor_dev->cdev;

		//Add the device to the system
		cdev_init(pcdev, &et_pcie_fops);
		pcdev->owner = THIS_MODULE;

		rc = cdev_add(pcdev, dev, 1);
		if (rc) {
			dev_err(&pdev->dev, "cdev_add failed\n");
			goto error_destory_devs;
		}

		//Make udev enumerate it to user mode
		minor_dev->pdevice = device_create(pclass, NULL, dev, NULL,
						   MINOR_NAMES[i], curr_dev);
		if (IS_ERR(minor_dev->pdevice)) {
			dev_err(&pdev->dev, "device_create failed\n");
			rc = PTR_ERR(minor_dev->pdevice);
			goto error_destory_devs;
		}

		++curr_minor;
	}
	++curr_dev;

	return 0;

error_destory_devs:
	for (; i >= 0; --i) {
		struct et_pci_minor_dev *minor_dev = &et_dev->et_minor_devs[i];

		if (minor_dev->pdevice) {
			device_destroy(minor_dev->pdevice->class,
					minor_dev->pdevice->devt);
		}
		cdev_del(&minor_dev->cdev);
	}

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
	destory_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);

	return rc;
}

static void esperanto_pcie_remove(struct pci_dev *pdev)
{
	struct et_pci_dev *et_dev;
	int i;
	int irq_vec;

	et_dev = pci_get_drvdata(pdev);
	if (!et_dev) return;

	for (i = 0; i < MINORS_PER_SOC; ++i) {
		struct et_pci_minor_dev *minor_dev = &et_dev->et_minor_devs[i];

		if (minor_dev->pdevice) {
			device_destroy(minor_dev->pdevice->class,
					minor_dev->pdevice->devt);
		}
		cdev_del(&minor_dev->cdev);
	}

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

	destory_et_pci_dev(et_dev);
	pci_set_drvdata(pdev, NULL);
}

static struct pci_driver et_pcie_driver = {
	.name = DRIVER_NAME,
	.id_table = esperanto_pcie_tbl,
	.probe = esperanto_pcie_probe,
	.remove = esperanto_pcie_remove,
};

static int __init esperanto_pcie_init(void)
{
	int rc;
	dev_t dev;

	pr_info("Enter: esperanto_pcie_init\n");

	//Dynamically alloc a major device number
	rc = alloc_chrdev_region(&dev, MIN_MINOR, MAX_MINORS, DRIVER_NAME);
	if (rc < 0) {
		pr_err("alloc_chrdev_region failed\n");
		return rc;
	}
	major = MAJOR(dev);
	curr_minor = MINOR(dev);

	//Create a class so udev can expose char devices to user mode
	pclass = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(pclass)) {
		pr_err("class_create failed\n");
		rc = PTR_ERR(pclass);
		goto class_create_fail;
	}

	rc = pci_register_driver(&et_pcie_driver);
	if (rc) {
		pr_err("failed to register pci device\n");
		goto pci_register_driver_fail;
	}

	pr_info("esperanto driver loaded\n");

	return 0;

pci_register_driver_fail:
	class_destroy(pclass);
class_create_fail:
	unregister_chrdev_region(dev, MAX_MINORS);
	return rc;
}

static void __exit esperanto_pcie_exit(void)
{
	pci_unregister_driver(&et_pcie_driver);
	class_destroy(pclass);
	unregister_chrdev_region(MKDEV(major, 0), MAX_MINORS);
}

module_init(esperanto_pcie_init);
module_exit(esperanto_pcie_exit);
