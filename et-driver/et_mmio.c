#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

#include "et_io.h"
#include "et_mmio.h"
#include "et_pci_dev.h"

int et_mmio_iomem_idx(uint64_t soc_addr, uint64_t count)
{
	int i;
	uint64_t reg_begin, reg_end;
	uint64_t soc_end = soc_addr + count;

	//integer overflow check
	if (soc_end < soc_addr) {
		pr_err("Invalid soc_addr + size (0x%010llx + 0x%llx)\n",
		       soc_addr, count);
		return -EINVAL;
	}

	for (i = 0; i < IOMEM_REGIONS; ++i) {
		reg_begin = BAR_MAPPINGS[i].soc_addr;
		reg_end = reg_begin + BAR_MAPPINGS[i].size;

		if (soc_addr >= reg_begin && soc_end <= reg_end) {
			return i;
		}
	}

	//Didn't find any regions matching parameters
	return -EINVAL;
}

ssize_t et_mmio_write_from_user(const char __user *buf, size_t count,
				loff_t *pos, struct et_pci_dev *et_dev)
{
	int iomem_idx;
	ssize_t rv;
	void __iomem *iomem;
	uint64_t soc_addr = (uint64_t)*pos;
	uint64_t off;
	uint8_t *kern_buf;

	//Bounds check and lookup BAR mapping
	iomem_idx = et_mmio_iomem_idx(soc_addr, count);
	if (iomem_idx < 0) {
		return iomem_idx;
	}
	iomem = et_dev->iomem[iomem_idx];
	off = soc_addr - BAR_MAPPINGS[iomem_idx].soc_addr;

	//Pull user's buffer to kernel
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	rv = copy_from_user(kern_buf, buf, count);
	if (rv) {
		pr_err("failed to copy from user\n");
		goto error;
	}

	et_iowrite(iomem, off, kern_buf, count);

	*pos += count;
	rv = count;

error:
	kfree(kern_buf);

	return rv;
}

ssize_t et_mmio_read_to_user(char __user *buf, size_t count, loff_t *pos,
			     struct et_pci_dev *et_dev)
{
	int iomem_idx;
	ssize_t rv;
	void __iomem *iomem;
	uint64_t soc_addr = (uint64_t)*pos;
	uint64_t off;
	uint8_t *kern_buf;

	//Bounds check and lookup BAR mapping
	iomem_idx = et_mmio_iomem_idx(soc_addr, count);
	if (iomem_idx < 0) {
		return iomem_idx;
	}
	iomem = et_dev->iomem[iomem_idx];
	off = soc_addr - BAR_MAPPINGS[iomem_idx].soc_addr;

	//Buffer incoming data
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	et_ioread(iomem, off, kern_buf, count);

	rv = copy_to_user(buf, kern_buf, count);
	if (rv) {
		pr_err("failed to copy to user\n");
		goto error;
	}

	rv = count;

error:
	kfree(kern_buf);

	return rv;
}

ssize_t et_mmio_write_to_device(struct et_pci_dev *et_dev,
				const char __user *buf, size_t count,
				u64 soc_addr)
{
	int iomem_idx;
	ssize_t rv;
	void __iomem *iomem;
	u64 off;
	u8 *kern_buf;

	// Bounds check and lookup BAR mapping
	iomem_idx = et_mmio_iomem_idx(soc_addr, count);
	if (iomem_idx < 0) {
		return iomem_idx;
	}
	iomem = et_dev->iomem[iomem_idx];
	off = soc_addr - BAR_MAPPINGS[iomem_idx].soc_addr;

	//Pull user's buffer to kernel
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	rv = copy_from_user(kern_buf, buf, count);
	if (rv) {
		pr_err("failed to copy from user\n");
		goto error;
	}

	et_iowrite(iomem, off, kern_buf, count);

	rv = count;

error:
	kfree(kern_buf);

	return rv;
}

ssize_t et_mmio_read_from_device(struct et_pci_dev *et_dev, char __user *buf,
				 size_t count, u64 soc_addr)
{
	int iomem_idx;
	ssize_t rv;
	void __iomem *iomem;
	u64 off;
	u8 *kern_buf;

	//Bounds check and lookup BAR mapping
	iomem_idx = et_mmio_iomem_idx(soc_addr, count);
	if (iomem_idx < 0) {
		return iomem_idx;
	}
	iomem = et_dev->iomem[iomem_idx];
	off = soc_addr - BAR_MAPPINGS[iomem_idx].soc_addr;

	//Buffer incoming data
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	et_ioread(iomem, off, kern_buf, count);

	rv = copy_to_user(buf, kern_buf, count);
	if (rv) {
		pr_err("failed to copy to user\n");
		goto error;
	}

	rv = count;

error:
	kfree(kern_buf);

	return rv;
}
