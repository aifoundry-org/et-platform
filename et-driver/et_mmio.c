#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

#include "et_io.h"
#include "et_mmio.h"
#include "et_pci_dev.h"

static struct et_ddr_region *lookup_ddr_region(struct et_pci_dev *et_dev,
					       bool is_mgmt, u64 soc_addr,
					       size_t count)
{
	int i;
	u64 reg_begin, reg_end;
	u64 soc_end = soc_addr + count;
	struct et_ddr_region **ddr_regions;
	u32 num_regions;

	// Integer overflow check
	if (soc_end < soc_addr) {
		pr_err("Invalid soc_addr + size (0x%010llx + 0x%lx)\n",
		       soc_addr, count);
		return NULL;
	}

	if (is_mgmt) {
		ddr_regions = et_dev->mgmt.ddr_regions;
		num_regions = et_dev->mgmt.num_regions;
	} else {
		ddr_regions = et_dev->ops.ddr_regions;
		num_regions = et_dev->ops.num_regions;
	}

	if (!ddr_regions)
		return NULL;

	for (i = 0; i < num_regions; ++i) {
		reg_begin = ddr_regions[i]->soc_addr;
		reg_end = reg_begin + ddr_regions[i]->size;

		if (soc_addr >= reg_begin && soc_end <= reg_end)
			return ddr_regions[i];
	}

	// Didn't find any region matching parameters
	return NULL;
}

ssize_t et_mmio_write_to_device(struct et_pci_dev *et_dev, bool is_mgmt,
				const char __user *buf, size_t count,
				u64 soc_addr)
{
	ssize_t rv;
	loff_t offset;
	u8 *kern_buf;
	struct et_ddr_region *ddr_region;

	// Lookup DDR region that matches parameters
	ddr_region = lookup_ddr_region(et_dev, is_mgmt, soc_addr, count);
	if (!ddr_region) {
		pr_err("lookup_ddr_region failed, out of bound soc_addr: 0x%010llx",
		       soc_addr);
		return -EINVAL;
	}
	offset = soc_addr - ddr_region->soc_addr;

	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	// Copy user's buffer to kernel
	rv = copy_from_user(kern_buf, buf, count);
	if (rv) {
		pr_err("failed to copy from user\n");
		goto error;
	}

	et_iowrite(ddr_region->mapped_baseaddr, offset, kern_buf, count);

	rv = count;

error:
	kfree(kern_buf);

	return rv;
}

ssize_t et_mmio_read_from_device(struct et_pci_dev *et_dev, bool is_mgmt,
				 char __user *buf, size_t count, u64 soc_addr)
{
	ssize_t rv;
	loff_t offset;
	u8 *kern_buf;
	struct et_ddr_region *ddr_region;

	// Lookup DDR region that matches parameters
	ddr_region = lookup_ddr_region(et_dev, is_mgmt, soc_addr, count);
	if (!ddr_region) {
		pr_err("lookup_ddr_region failed, out of bound soc_addr: 0x%010llx",
		       soc_addr);
		return -EINVAL;
	}
	offset = soc_addr - ddr_region->soc_addr;

	// Buffer incoming data
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	et_ioread(ddr_region->mapped_baseaddr, offset, kern_buf, count);

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
