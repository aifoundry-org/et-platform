#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

#include "et_io.h"
#include "et_mmio.h"
#include "et_pci_dev.h"
#include "et_dev_iface_regs.h"

static struct et_mapped_region *lookup_mem_region(struct et_pci_dev *et_dev,
						  bool is_mgmt, u64 soc_addr,
						  size_t count)
{
	int reg_idx;
	u64 reg_begin, reg_end;
	u64 soc_end = soc_addr + count;

	// Integer overflow check
	if (soc_end < soc_addr) {
		pr_err("Invalid soc_addr + size (0x%010llx + 0x%lx)\n",
		       soc_addr, count);
		return NULL;
	}

	if (is_mgmt) {
		for (reg_idx = 0; reg_idx < MGMT_MEM_REGION_TYPE_NUM;
		     reg_idx++) {
			if (!et_dev->mgmt.regions[reg_idx].is_valid)
				continue;

			if (et_dev->mgmt.regions[reg_idx].access.priv_mode
			    != MEM_REGION_PRIVILEGE_MODE_USER ||
			    et_dev->mgmt.regions[reg_idx].access.node_access
			    == MEM_REGION_NODE_ACCESSIBLE_OPS)
				continue;

			reg_begin = et_dev->mgmt.regions[reg_idx].soc_addr;
			reg_end = reg_begin +
				  et_dev->mgmt.regions[reg_idx].size;

			if (soc_addr >= reg_begin && soc_end <= reg_end)
				return &et_dev->mgmt.regions[reg_idx];
		}
	} else {
		for (reg_idx = 0; reg_idx < OPS_MEM_REGION_TYPE_NUM;
		     reg_idx++) {
			if (!et_dev->ops.regions[reg_idx].is_valid)
				continue;

			if (et_dev->ops.regions[reg_idx].access.priv_mode
			    != MEM_REGION_PRIVILEGE_MODE_USER ||
			    et_dev->ops.regions[reg_idx].access.node_access
			    == MEM_REGION_NODE_ACCESSIBLE_MGMT)
				continue;

			reg_begin = et_dev->ops.regions[reg_idx].soc_addr;
			reg_end = reg_begin +
				  et_dev->ops.regions[reg_idx].size;

			if (soc_addr >= reg_begin && soc_end <= reg_end)
				return &et_dev->ops.regions[reg_idx];
		}
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
	struct et_mapped_region *region;

	// Lookup memory region that matches parameters
	region = lookup_mem_region(et_dev, is_mgmt, soc_addr, count);
	if (!region) {
		pr_err("lookup_mem_region failed, out of bound soc_addr: 0x%010llx",
		       soc_addr);
		return -EINVAL;
	}
	offset = soc_addr - region->soc_addr;

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

	et_iowrite(region->mapped_baseaddr, offset, kern_buf, count);

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
	struct et_mapped_region *region;

	// Lookup memory region that matches parameters
	region = lookup_mem_region(et_dev, is_mgmt, soc_addr, count);
	if (!region) {
		pr_err("lookup_mem_region failed, out of bound soc_addr: 0x%010llx",
		       soc_addr);
		return -EINVAL;
	}
	offset = soc_addr - region->soc_addr;

	// Buffer incoming data
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}

	et_ioread(region->mapped_baseaddr, offset, kern_buf, count);

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
