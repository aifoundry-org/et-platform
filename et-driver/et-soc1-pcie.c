/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <uapi/linux/pci_regs.h>
#include <asm/uaccess.h>

//#define ESPERANTO_PCIE_VENDOR_ID 0x1e0a  // official PCI-SIG assignment
//#define ESPERANTO_PCIE_DEVICE_ID 0x9038  // official device ID TBD
#define ESPERANTO_PCIE_VENDOR_ID 0x10ee // official PCI-SIG assignment
#define ESPERANTO_PCIE_DEVICE_ID 0x9038 // official device ID TBD

#define DRIVER_NAME "et-soc1-cheetah"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esperanto <esperanto@gmail.com or admin@esperanto.com>");
MODULE_DESCRIPTION("PCIe device driver for esperanto soc-1");
MODULE_VERSION("1.0");
MODULE_SUPPORTED_DEVICE("Esperanto soc-1");

static dev_t first_cdev; // Global variable for the first device number
static u64 num_cdevs = 0; // Count number of /dev entries created so far
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
static u64 dev_no; // Global for mbox number
static u8 rw_buffer =
	'a'; // Global buffer for communcating between read <-> write
#define MAXOPENFPS 16
static struct file *open_fps[MAXOPENFPS]; // Remember the open files
static u64 num_open_fps; // Remember number of open files

static u8 kargs[256]; // Kernel copy of args

struct bar {
	u64 begin;
	u64 end;
	u64 len;
	void* bar;
};
struct bar bars[6]; // we know there are max 6 bars

// static DEFINE_MUTEX(open_close_lock);
static struct mutex open_close_lock;

#define ESPERANTO_PCIE_IOCTL_MAGIC 0x67879

#define ESPERANTO_PCIE_IOCTL_REGRD _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 1, int)
#define ESPERANTO_PCIE_IOCTL_REGWR _IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 2, int)
#define ESPERANTO_PCIE_IOCTL_CNFRD _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 3, int)
#define ESPERANTO_PCIE_IOCTL_CNFWR _IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 4, int)
#define ESPERANTO_PCIE_IOCTL_DEBUG _IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 5, int)
#define ESPERANTO_PCIE_IOCTL_GET_BARS _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 6, int)
#define ESPERANTO_PCIE_IOCTL_GET_OPEN_FILES                                    \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 7, int)

static const struct pci_device_id esperanto_pcie_tbl[] = {
	{ PCI_DEVICE(ESPERANTO_PCIE_VENDOR_ID, ESPERANTO_PCIE_DEVICE_ID) },
	{}
};

MODULE_DEVICE_TABLE(pci, esperanto_pcie_tbl);

static int esperanto_pcie_open(struct inode *ip, struct file *fp)
{
	u64 i;

	printk(KERN_INFO
	       "Enter: esperanto_pcie_open, ip: 0x%016llx, fp: 0x%016llx\n",
	       (u64)ip, (u64)fp);

	mutex_lock(&open_close_lock);
	for (i = 0; i < MAXOPENFPS; i += 1) {
		if (open_fps[i] == NULL) {
			open_fps[i] = fp;
			num_open_fps += 1;
			mutex_unlock(&open_close_lock);
			return 0;
		}
	}
	mutex_unlock(&open_close_lock);

	printk(KERN_INFO "No more room for opening a file pointer.\n");
	return -1;
}

static int esperanto_pcie_close(struct inode *ip, struct file *fp)
{
	u64 i;

	printk(KERN_INFO
	       "Enter: esperanto_pcie_close, ip: 0x%016llx, fp: 0x%016llx\n",
	       (u64)ip, (u64)fp);

	mutex_lock(&open_close_lock);
	for (i = 0; i < MAXOPENFPS; i += 1) {
		if (open_fps[i] == fp) {
			open_fps[i] = NULL;
			num_open_fps -= 1;
			mutex_unlock(&open_close_lock);
			return 0;
		}
	}
	mutex_unlock(&open_close_lock);

	printk(KERN_INFO
	       "esperanto_pcie_close: fp 0x%016llx not found in list of open files.\n",
	       (u64)fp);

	return -1;
}

static ssize_t esperanto_pcie_read(struct file *fp, char __user *buf,
				   size_t count, loff_t *pos)
{
	printk(KERN_INFO
	       "Enter: esperanto_pcie_read, fp: 0x%016llx, count %zd\n",
	       (u64)fp, count);

	if (copy_to_user(buf, &rw_buffer, 1)) {
		return 0;
	}

	return count;
}

static ssize_t esperanto_pcie_write(struct file *fp, const char __user *buf,
				    size_t count, loff_t *pos)
{
	printk(KERN_INFO
	       "Enter: esperanto_pcie_write, fp: 0x%016llx, count %zd\n",
	       (u64)fp, count);

	if (copy_from_user(&rw_buffer, buf, 1)) {
		return 0;
	}

	return count;
}

static long esperanto_pcie_ioctl(struct file *fp, unsigned int cmd,
				 unsigned long arg)
{
	u64 local_num_open_fps;

	printk(KERN_INFO "Enter: esperanto_pcie_ioctl\n");
	printk(KERN_INFO "    fp: 0x%016llx\n", (u64)fp);
	printk(KERN_INFO "   cmd: 0x%016llx\n", (u64)fp);
	printk(KERN_INFO "   arg: 0x%016llx\n", (u64)arg);

	switch (cmd) {
	case ESPERANTO_PCIE_IOCTL_GET_BARS:
		pr_info("ESPERANTO_PCIE_IOCTL_GET_BARS\n");

		if (copy_to_user((uint8_t*)arg, (uint8_t*)&bars, sizeof(bars)))
		{
			pr_info("copy to user error\n");
			return -EFAULT;
		}
		break;

	case ESPERANTO_PCIE_IOCTL_GET_OPEN_FILES:
		pr_info("Enter: ESPERANTO_PCIE_IOCTL_GET_OPEN_FILES\n");

		mutex_lock(&open_close_lock);
		local_num_open_fps = num_open_fps;
		mutex_unlock(&open_close_lock);
		if (copy_to_user((u64 *)arg, &local_num_open_fps, sizeof(u64))) {
			return -EINVAL;
		}
		break;

	case ESPERANTO_PCIE_IOCTL_DEBUG:
		pr_info("Enter: ESPERANTO_PCIE_IOCTL_DEBUG: arg: 0x%p\n",
			(void *)arg);
		if (copy_from_user(&kargs, (u8 *)arg, sizeof(kargs))) {
			pr_info("ioctl ESPERANTO_PCIE_IOCTL_DEBUG: copy_from_user error\n");
			return -EFAULT;
		}
		break;

	default:
		pr_info("Unknown IOCTL command\n");
		return -EINVAL;
	}

	return 0;
}

static struct file_operations esperanto_pcie_fops = {
	.owner = THIS_MODULE,
	.read = esperanto_pcie_read,
	.write = esperanto_pcie_write,
	.unlocked_ioctl = esperanto_pcie_ioctl,
	.open = esperanto_pcie_open,
	.release = esperanto_pcie_close,
};

static int esperanto_pcie_map_bars(struct pci_dev *pdev)
{
	u32 i, ret;

	ret = 0;
	for (i = 0; i < 6; i++) {
		bars[i].begin = pci_resource_start(pdev, i);
		bars[i].end = pci_resource_end(pdev, i);

		if (!bars[i].begin || !bars[i].end) {
			ret = -1;
			printk(KERN_INFO "Didn't get start/end for BAR%u\n", i);
			continue;
		}

		bars[i].len = pci_resource_len(pdev, i);

		if ((bars[i].bar = pci_iomap(pdev, i, bars[i].len)) == NULL) {
			printk(KERN_INFO "Could not map BAR%u\n", i);
			ret = -1;
			goto failed;
		}
		printk(KERN_INFO "BAR%d start 0x%016llx\n", i, bars[i].begin);
		printk(KERN_INFO "BAR%d end   0x%016llx\n", i, bars[i].end);

		printk(KERN_INFO "BAR%d mapped at 0x%p with length %llu\n", i,
		       bars[i].bar, (uint64_t)bars[i].len);
		printk(KERN_INFO "Trying bar as llx: 0x%llx\n",
		       (unsigned long long)bars[i].bar);
	}
	goto succeeded;

failed:

succeeded:
	return ret;
}

static int esperanto_pcie_probe(struct pci_dev *pdev,
				const struct pci_device_id *pci_id)
{
#define DEVNAMELEN 32
	char devname[DEVNAMELEN];

	printk(KERN_INFO "Enter: esperanto_pcie_probe\n");
	printk(KERN_INFO "pci_id->vendor 0x%x, pci_id->device 0x%x\n",
	       pci_id->vendor, pci_id->device);
	printk(KERN_INFO "pci_id: 0x%016llx\n", (u64)pci_id);
	printk(KERN_INFO "pdev:   0x%016llx\n", (u64)pdev);

	esperanto_pcie_map_bars(pdev);

	printk(KERN_INFO "esperanto_pcie_probe: before alloc_chrdev_region\n");
	if (alloc_chrdev_region(&first_cdev, 0, 1, "etsoc1") < 0) {
		printk(KERN_INFO "alloc_chrdev_region failed\n");
		return -1;
	}

	printk(KERN_INFO "esperanto_pcie_probe: is cl NULL?\n");
	if (cl == NULL) {
		printk(KERN_INFO "esperanto_pcie_probe: before class_create\n");
		if ((cl = class_create(THIS_MODULE, "etsoc1class")) == NULL) {
			printk(KERN_INFO "class_create failed\n");
			unregister_chrdev_region(first_cdev, 1);
			return -1;
		}
	}

	snprintf(devname, DEVNAMELEN, "etsoc1mbox%llx", dev_no++);
	printk(KERN_INFO "esperanto_pcie_probe: calling device_create\n");
	if ((device_create(cl, NULL, first_cdev, NULL, devname)) == NULL) {
		printk(KERN_INFO "device_create failed\n");
		class_destroy(cl);
		unregister_chrdev_region(first_cdev, 1);
		return -1;
	}

	printk(KERN_INFO "esperanto_pcie_probe: before cdev_init\n");
	cdev_init(&c_dev, &esperanto_pcie_fops);
	printk(KERN_INFO "esperanto_pcie_probe: before cdev_add\n");
	if (cdev_add(&c_dev, first_cdev, 1) == -1) {
		printk(KERN_INFO "cdev_add failed\n");
		device_destroy(cl, first_cdev);
		class_destroy(cl);
		unregister_chrdev_region(first_cdev, 1);
		return -1;
	}

	printk(KERN_INFO "esperanto_pcie_probe: num_cdevs += 1\n");
	num_cdevs += 1;

	return 0;
}

static void esperanto_pcie_remove(struct pci_dev *pdev)
{
	unregister_chrdev_region(first_cdev, 1);
	device_destroy(cl, first_cdev);
	class_destroy(cl); // delete class created by us
}

static struct pci_driver esperanto_pcie_driver = {
	.name = DRIVER_NAME,
	.id_table = esperanto_pcie_tbl,
	.probe = esperanto_pcie_probe,
	.remove = esperanto_pcie_remove,
};

static int __init esperanto_pcie_init(void)
{
	u32 i;
	int err;

	dev_no = 0;

	for (i = 0; i < MAXOPENFPS; i += 1) {
		open_fps[i] = NULL;
	}

	mutex_init(&open_close_lock);

	printk(KERN_INFO "Enter: esperanto_pcie_init\n");

	err = pci_register_driver(&esperanto_pcie_driver);

	if (err < 0) {
		printk(KERN_INFO "Exit: esperanto_pcie_init: error -EPERM\n");
		return -EPERM;
	}

	cl = NULL;

	return err;
}

static void __exit esperanto_pcie_exit(void)
{
	pci_unregister_driver(&esperanto_pcie_driver);
	mutex_destroy(&open_close_lock);
}

module_init(esperanto_pcie_init);
module_exit(esperanto_pcie_exit);
