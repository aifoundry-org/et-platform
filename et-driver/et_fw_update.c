#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "et_dev_iface_regs.h"
#include "et_fw_update.h"
#include "et_io.h"
#include "et_pci_dev.h"

ssize_t et_mmio_write_fw_image(struct et_pci_dev *et_dev,
			       const char __user *buf,
			       size_t count)
{
	ssize_t rv;
	u8 *kern_buf;

	// Check region validity
	if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH].is_valid) {
		pr_err("scratch region not found!");
		return -EINVAL;
	}

	// Check if scratch region is large enough for FW image, where count is
	// in bytes
	if (count > et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH].size) {
		pr_err("image size (0x%lx) exceeded scratch region size (0x%llx)!",
		       count,
		       et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH].size);
		return -ENOMEM;
	}

	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc!");
		return -ENOMEM;
	}

	// Copy user's buffer to kernel
	rv = copy_from_user(kern_buf, buf, count);
	if (rv) {
		pr_err("failed to copy from user!");
		goto error;
	}

	et_iowrite(et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH]
			   .io.mapped_baseaddr,
		   0,
		   kern_buf,
		   count);

	rv = count;

error:
	kfree(kern_buf);

	return rv;
}
