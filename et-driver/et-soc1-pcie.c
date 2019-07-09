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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");  

#define TEST_MODE 1

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

static const char *MINOR_NAMES[] = {
#ifdef TEST_MODE
	"et%ddrct_dram",
	"et%dmbox_mm",
	"et%dmbox_sp",
	"et%dtrg_pcie",
	"et%dpcie_useresr",
#endif
	"et%dmb_to_sp",
	"et%dmb_from_sp",
	"et%dmb_to_mm",
	"et%dmb_from_mm",
	"et%dbulk"
};

#define MINORS_PER_SOC ARRAY_SIZE(MINOR_NAMES)

/* MAX_MINORS picked arbitrarily; each SoC needs 8x PCIe lanes, assuming no
   modern CPU will have 128 lanes. */
#define MIN_MINOR 0
#define MAX_MINORS MINORS_PER_SOC * 16

enum et_cdev_type {
#ifdef TEST_MODE
	et_cdev_type_drct_dram = 0,
	et_cdev_type_mbox_mm,
	et_cdev_type_mbox_sp,
	et_cdev_type_trg_pcie,
	et_cdev_type_pcie_useresr,
#endif
	et_cdev_type_mb_to_sp,
	et_cdev_type_mb_from_sp,
	et_cdev_type_mb_to_mm,
	et_cdev_type_mb_from_mm,
	et_cdev_type_bulk
};

static const enum et_cdev_type MINOR_TYPES[] = {
#ifdef TEST_MODE
	et_cdev_type_drct_dram,
	et_cdev_type_mbox_mm,
	et_cdev_type_mbox_sp,
	et_cdev_type_trg_pcie,
	et_cdev_type_pcie_useresr,
#endif
	et_cdev_type_mb_to_sp,
	et_cdev_type_mb_from_sp,
	et_cdev_type_mb_to_mm,
	et_cdev_type_mb_from_mm,
	et_cdev_type_bulk
};

#define R_DRCT_DRAM_OFFSET     0x0000000000
#define R_DRCT_DRAM_SIZE       0x0700000000

#define R_PU_MBOX_PC_MM_OFFSET 0x0000
#define R_PU_MBOX_PC_MM_SIZE   0x1000

#define R_PU_MBOX_PC_SP_OFFSET 0x1000
#define R_PU_MBOX_PC_SP_SIZE   0x1000

#define R_PU_TRG_PCIE_OFFSET   0x2000
#define R_PU_TRG_PCIE_SIZE     0x2000

#define R_PCIE_USRESR_OFFSET   0x4000
#define R_PCIE_USRESR_SIZE     0x1000

//TODO: mbox defines to separate file
typedef struct MESSAGE_PAYLOAD_s {
	u32 service_id;
	u32 command_id;
	u32 sender_tag_lo;
	u32 sender_tag_hi;
	u32 data[12];
} MESSAGE_PAYLOAD_t;

static const u64 MINOR_FIO_SIZES[] = {
#ifdef TEST_MODE
	R_DRCT_DRAM_SIZE, //et_cdev_type_drct_dram
	R_PU_MBOX_PC_MM_SIZE, //et_cdev_type_mbox_mm
	R_PU_MBOX_PC_SP_SIZE, //et_cdev_type_mbox_sp
	R_PU_TRG_PCIE_SIZE, //et_cdev_type_trg_pcie
	R_PCIE_USRESR_SIZE, //et_cdev_type_pcie_useresr
#endif
	sizeof(MESSAGE_PAYLOAD_t), //et_cdev_type_mb_to_sp
	sizeof(MESSAGE_PAYLOAD_t), //et_cdev_type_mb_from_sp
	sizeof(MESSAGE_PAYLOAD_t), //et_cdev_type_mb_to_mm
	sizeof(MESSAGE_PAYLOAD_t), //et_cdev_type_mb_from_mm
	R_DRCT_DRAM_SIZE, //et_cdev_type_bulk
};

static u8 rw_buff[8192];

#define RW_BUFF_DWORDS ARRAY_SIZE(rw_buff) / 4

//The SoC supports MSI, MSI-X, and Legacy PCI IRQs. Legacy gives you exactly 1
//interrupt vector. MSI is up to 32 in powers of 2. MSI-X is up to 2048 in any
//step. We want two (one for each mbox).
#define MIN_VECS 1
#define REQ_VECS 2

struct et_pci_minor_dev {
	struct device *pdevice;
	struct et_pci_dev *et_dev;
	struct cdev cdev;
	struct mutex open_close_mutex;
	struct mutex read_write_mutex;
	enum et_cdev_type type;
	int ref_count;
};

struct et_pci_dev {
	struct pci_dev *pdev;
	void __iomem *r_drct_dram;
	void __iomem *r_pu_mbox_pc_mm;
	void __iomem *r_pu_mbox_pc_sp;
	void __iomem *r_pu_trg_pcie;
	void __iomem *r_pcie_usresr;
	int num_vecs;
	struct et_pci_minor_dev et_minor_devs[MINORS_PER_SOC];
};

static uint32_t int_cnt = 0;

static irqreturn_t et_mbox_isr(int irq, void *dev_id)
{
	//TODO
	printk("Got mbox irq %d\n", int_cnt);
	++int_cnt;
	
	return IRQ_HANDLED;
}

static int et_mmio_bounds_check(u64 max_size, size_t count, loff_t *pos)
{
	u64 last_off = *pos + count - 1;

	if (*pos >= last_off) {
		pr_err("read overflow\n");
		return -EIO;
	}

	if (last_off >= max_size) {
		pr_err("read out of bounds\n");
		return -EIO;
	}

	return 0;
}

static void et_ioread8_block(void __iomem *port, const void *dst, u64 count)
{
	while (count--) {
		*(u8 *)dst = ioread8(port);
		++port;
		++dst;
	}
}

static void et_ioread32_block(void __iomem *port, const void *dst, u64 count)
{
	while (count--) {
		*(u32 *)dst = ioread32(port);
		dst += 4;
		port += 4;
	}
}

static ssize_t et_mmio_read(void __iomem *mem, u64 max_size, char __user *buf,
	                    size_t count, loff_t *pos)
{
	int rc;
	u64 iocount, buf_dwords;
	ssize_t read_count = 0;
	
	rc = et_mmio_bounds_check(max_size, count, pos);
	if (rc < 0) {
		return rc;
	}

	iocount = *pos & 0x3;
	if (iocount) {
		et_ioread8_block(mem + *pos, rw_buff, iocount);

		rc = copy_to_user(buf, rw_buff, iocount);
		if (rc) {
			pr_err("failed to copy to user\n");
			*pos += iocount - rc;
			return iocount - rc;
		}

		*pos += iocount;
		buf += iocount;
		read_count += iocount;
	}

	//Copy 32-bit aligned values. The data set from user-mode might be quite
	//large (GB), so break up pulling it into smaller chunks.
	buf_dwords = (count - read_count) / 4;
	while(buf_dwords) {
		iocount = min((u64)RW_BUFF_DWORDS, buf_dwords);

		et_ioread32_block(mem + *pos, rw_buff, iocount);

		rc = copy_to_user(buf, rw_buff, iocount * 4);
		if (rc) {
			pr_err("failed to copy to user\n");
			*pos += (iocount * 4) - rc;
			return read_count + ((iocount * 4) - rc);
		}

		*pos += iocount * 4;
		buf += iocount * 4;
		read_count += iocount * 4;
		buf_dwords -= iocount;
	}

	//Remaining bytes smaller than a u32
	iocount = count - read_count;
	if (iocount) {
		et_ioread8_block(mem + *pos, rw_buff, iocount);

		rc = copy_to_user(buf, rw_buff, iocount);
		if (rc) {
			pr_err("failed to copy to user\n");
			*pos += iocount - rc;
			return iocount - rc;
		}

		*pos += iocount;
		read_count += iocount;
	}

	return read_count;
}

static ssize_t esperanto_pcie_read(struct file *fp, char __user *buf,
				   size_t count, loff_t *pos)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;
	struct et_pci_dev *et_dev = minor_dev->et_dev;
	ssize_t rv = 0;
 
	mutex_lock(&minor_dev->read_write_mutex);

	switch(minor_dev->type){
#ifdef TEST_MODE
	case et_cdev_type_drct_dram:
		rv = et_mmio_read(et_dev->r_drct_dram, R_DRCT_DRAM_SIZE, buf,
 			          count, pos);
		break;
	case et_cdev_type_mbox_mm:
		rv = et_mmio_read(et_dev->r_pu_mbox_pc_mm, R_PU_MBOX_PC_MM_SIZE,
				  buf, count, pos);
		break;
	case et_cdev_type_mbox_sp:
		rv = et_mmio_read(et_dev->r_pu_mbox_pc_sp, R_PU_MBOX_PC_SP_SIZE,
 				  buf, count, pos);
		break;
	case et_cdev_type_trg_pcie:
		rv = et_mmio_read(et_dev->r_pu_trg_pcie, R_PU_TRG_PCIE_SIZE,
 				  buf, count, pos);
		break;
	case et_cdev_type_pcie_useresr:
		rv = et_mmio_read(et_dev->r_pcie_usresr, R_PCIE_USRESR_SIZE,
 			          buf, count, pos);
		break;
#endif
	case et_cdev_type_mb_to_sp:
		//TODO (fall though intentionally for now)
	case et_cdev_type_mb_from_sp:
		//TODO (fall though intentionally for now)
	case et_cdev_type_mb_to_mm:
		//TODO (fall though intentionally for now)
	case et_cdev_type_mb_from_mm:
		//TODO (fall though intentionally for now)
	case et_cdev_type_bulk:
		//TODO (fall though intentionally for now)
	default:
		pr_err("dev type invalid");
		return -EINVAL;
	}
 
  	mutex_unlock(&minor_dev->read_write_mutex);

	return rv;
}

static void et_iowrite8_block(void __iomem *port, const void *src, u64 count)
{
	while (count--) {
		iowrite8(*(u8 *)src, port);
		++src;
		++port;
	}
}

static void et_iowrite32_block(void __iomem *port, const void *src, u64 count)
{
	while (count--) {
		iowrite32(*(u32 *)src, port);
		src += 4;
		port += 4;
	}
}

static ssize_t et_mmio_write(void __iomem *mem, u64 max_size, const char __user *buf,
	                    size_t count, loff_t *pos)
{
	int rc;
	u64 iocount, buf_dwords;
	ssize_t write_count = 0;
	
	rc = et_mmio_bounds_check(max_size, count, pos);
	if (rc < 0) {
		return rc;
	}

	//Handle unaligned bytes at beginning of write
	iocount = *pos & 0x3;
	if (iocount) {
		rc = copy_from_user(rw_buff, buf, iocount);
		if (rc) {
			pr_err("failed to copy from user\n");
			return write_count;
		}

		et_iowrite8_block(mem + *pos, rw_buff, iocount);

		*pos += iocount;
		buf += iocount;
		write_count += iocount;
	}

	//Copy 32-bit aligned values. The data set from user-mode might be quite
	//large (GB), so break up pulling it into smaller chunks.
	buf_dwords = (count - write_count) / 4;

	while (buf_dwords) {
		iocount = min((u64)RW_BUFF_DWORDS, buf_dwords);

		rc = copy_from_user(rw_buff, buf, iocount * 4);
		if (rc) {
			pr_err("failed to copy from user\n");
			return write_count;
		}

		et_iowrite32_block(mem + *pos, rw_buff, iocount);

		*pos += iocount * 4;
		buf += iocount * 4;
		write_count += iocount * 4;
		buf_dwords -= iocount;
	}

	//Remaining bytes smaller than a u32
	iocount = count - write_count;
	if (iocount) {
		rc = copy_from_user(rw_buff, buf, iocount);
		if (rc) {
			pr_err("failed to copy from user\n");
			return write_count;
		}

		et_iowrite8_block(mem + *pos, rw_buff, iocount);

		*pos += iocount;
		write_count += iocount;
	}

	return write_count;
}

static ssize_t esperanto_pcie_write(struct file *fp, const char __user *buf,
				    size_t count, loff_t *pos)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;
	struct et_pci_dev *et_dev = minor_dev->et_dev;
	ssize_t rv = 0;
	u64 max_size = MINOR_FIO_SIZES[minor_dev->type];
 
	mutex_lock(&minor_dev->read_write_mutex);

	switch(minor_dev->type) {
#ifdef TEST_MODE
	case et_cdev_type_drct_dram:
		rv = et_mmio_write(et_dev->r_drct_dram, max_size, buf, count,
			           pos);
		break;
	case et_cdev_type_mbox_mm:
		rv = et_mmio_write(et_dev->r_pu_mbox_pc_mm, max_size, buf, 
			           count, pos);
		break;
	case et_cdev_type_mbox_sp:
		rv = et_mmio_write(et_dev->r_pu_mbox_pc_sp, max_size, buf,
			           count, pos);
		break;
	case et_cdev_type_trg_pcie:
		rv = et_mmio_write(et_dev->r_pu_trg_pcie, max_size, buf, count,
			           pos);
		break;
	case et_cdev_type_pcie_useresr:
		rv = et_mmio_write(et_dev->r_pcie_usresr, max_size, buf, count,
			           pos);
		break;
#endif
	case et_cdev_type_mb_to_sp:
		//TODO (fall though intentionally for now)
	case et_cdev_type_mb_from_sp:
		//TODO (fall though intentionally for now)
	case et_cdev_type_mb_from_mm:
		//TODO (fall though intentionally for now)
	case et_cdev_type_bulk:
		//TODO (fall though intentionally for now)
	default:
		pr_err("dev type invalid");
		return -EINVAL;
	}

 	mutex_unlock(&minor_dev->read_write_mutex);

	return rv;
}

static loff_t esperanto_pcie_llseek(struct file *fp, loff_t pos, int whence)
{
	struct et_pci_minor_dev *minor_dev = fp->private_data;

	loff_t new_pos = 0;

	mutex_lock(&minor_dev->read_write_mutex);

	switch(minor_dev->type) {
	case et_cdev_type_mb_to_sp:
	case et_cdev_type_mb_from_sp:
	case et_cdev_type_mb_to_mm:
	case et_cdev_type_mb_from_mm:
		pr_err("dev does not support lseek");
		break;
	default:
		//Other char devices support lseek
		break;
	}

	switch (whence) {
	case SEEK_SET:
		new_pos = pos;
		break;
	case SEEK_CUR:
		new_pos = fp->f_pos + pos;
		break;
	case SEEK_END:
		new_pos = MINOR_FIO_SIZES[minor_dev->type] + pos;
		break;
	}

	if (new_pos > MINOR_FIO_SIZES[minor_dev->type]) {
		pr_err("pos > bounds");
		return -EINVAL;
	}
	if (new_pos < 0) {
		pr_err("pos < bounds");
		return -EINVAL;
	}

	fp->f_pos = new_pos;

	mutex_unlock(&minor_dev->read_write_mutex);

	return new_pos;
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
	if (et_dev->r_drct_dram)
		iounmap(et_dev->r_drct_dram);
	if (et_dev->r_pu_mbox_pc_mm)
		iounmap(et_dev->r_pu_mbox_pc_mm);
	if (et_dev->r_pu_mbox_pc_sp)
		iounmap(et_dev->r_pu_mbox_pc_sp);
	if (et_dev->r_pu_trg_pcie)
		iounmap(et_dev->r_pu_trg_pcie);
	if (et_dev->r_pcie_usresr)
		iounmap(et_dev->r_pcie_usresr);

	pci_release_regions(et_dev->pdev);
}

static int et_map_bars(struct et_pci_dev *et_dev)
{
	int rc;

	struct pci_dev *pdev = et_dev->pdev;

	rc = pci_request_regions(pdev, DRIVER_NAME);
	if (rc) {
		dev_err(&pdev->dev, "request regions failed\n");
		return rc;
	}

	//Map all regions that are RAM with wc (write combining) for performance

	//Setup BAR0
	//Name        Host Addr       Size   Notes
	//R_DRCT_DRAM BAR0 + 0x0000   28G    DRAM with PCIe access permissions

	et_dev->r_drct_dram = pci_iomap_wc(pdev, 0, R_DRCT_DRAM_SIZE);

	if (IS_ERR(et_dev->r_drct_dram)) {
		dev_err(&pdev->dev, "mapping r_drct_dram failed\n");
		rc = PTR_ERR(et_dev->r_drct_dram);
		goto error;
	}
    
	//Setup BAR2
	//Name              Host Addr       Size   Notes
	//R_PU_MBOX_PC_MM   BAR2 + 0x0000   4k     Mailbox shared memory
	//R_PU_MBOX_PC_SP   BAR2 + 0x1000   4k     Mailbox shared memory
	//R_PU_TRG_PCIE     BAR2 + 0x2000   8k     Mailbox interrupts
	//R_PCIE_USRESR     BAR2 + 0x4000   4k     DMA control registers

	et_dev->r_pu_mbox_pc_mm = pci_iomap_wc_range(pdev, 2,
		                                     R_PU_MBOX_PC_MM_OFFSET,
						     R_PU_MBOX_PC_MM_SIZE);

	if (IS_ERR(et_dev->r_pu_mbox_pc_mm)) {
		dev_err(&pdev->dev, "mapping r_pu_mbox_pc_mm failed\n");
		rc = PTR_ERR(et_dev->r_pu_mbox_pc_mm);
		goto error;
	}

	et_dev->r_pu_mbox_pc_sp = pci_iomap_wc_range(pdev, 2,
		                                     R_PU_MBOX_PC_SP_OFFSET,
						     R_PU_MBOX_PC_SP_SIZE);

	if (IS_ERR(et_dev->r_pu_mbox_pc_sp)) {
		dev_err(&pdev->dev, "mapping r_pu_mbox_pc_sp failed\n");
		rc = PTR_ERR(et_dev->r_pu_mbox_pc_mm);
		goto error;
	}

	//Important: This region maps registers. Do NOT mark as
	//write-combinable, writes should not trigger writes to adjacent regs
	et_dev->r_pu_trg_pcie = pci_iomap_range(pdev, 2,
		                                R_PU_TRG_PCIE_OFFSET,
						R_PU_TRG_PCIE_SIZE);

	if (IS_ERR(et_dev->r_pu_trg_pcie)) {
		dev_err(&pdev->dev, "mapping r_pu_trg_pcie failed\n");
		rc = PTR_ERR(et_dev->r_pu_trg_pcie);
		goto error;
	}

	//Important: This region maps registers. Do NOT mark as
	//write-combinable, writes should not trigger writes to adjacent regs
	et_dev->r_pcie_usresr = pci_iomap_range(pdev, 2,
		                                R_PCIE_USRESR_OFFSET,
					        R_PCIE_USRESR_SIZE);

	if (IS_ERR(et_dev->r_pcie_usresr)) {
		dev_err(&pdev->dev, "mapping r_pcie_usresr failed\n");
		rc = PTR_ERR(et_dev->r_pcie_usresr);
		goto error;
	}

	return 0;
error:
	et_unmap_bars(et_dev);
	return rc;
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
		et_dev->num_vecs = rc;
	}

	//TODO: For now, only using one vec. In the future, take advantage of multi vec
	irq_vec = pci_irq_vector(pdev, 0);
	if (irq_vec < 0) {
		rc = -ENODEV;
		dev_err(&pdev->dev, "finding irq vector failed\n");
		goto error_free_irq_vecs;
	}

	rc = request_irq(irq_vec, et_mbox_isr, IRQF_SHARED, DRIVER_NAME, 
			 (void*)et_dev);
	if (rc) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto error_free_irq_vecs;
	}

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

	printk(KERN_INFO "Enter: esperanto_pcie_init\n");

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

	printk(KERN_INFO "esperanto driver loaded\n");

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
