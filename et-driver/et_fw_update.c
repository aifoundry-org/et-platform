// SPDX-License-Identifier: GPL-2.0

/***********************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 **********************************************************************/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "et_dev_iface_regs.h"
#include "et_fw_update.h"
#include "et_io.h"
#include "et_pci_dev.h"

/*
 * et_mmio_write_fw_image - Writes FW Image on MGMT_MEM_REGION_TYPE_SCRATCH
 *
 * @et_dev: et-soc device pointer
 * @buf: user virtual address for firmware image
 * @count: count of bytes of firmware image
 *
 * Return: the number of bytes written on success, or a negative value
 * on failure.
 */
ssize_t et_mmio_write_fw_image(struct et_pci_dev *et_dev,
			       const char __user *buf, size_t count)
{
	ssize_t rv;
	u8 *kern_buf;

	// Check region validity
	if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH].is_valid) {
		pr_err("scratch region not found!");
		return -EINVAL;
	}

	// Check if scratch region is large enough for FW image
	if (count > et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH].size) {
		pr_err("image size (0x%lx) exceeded scratch region size (0x%llx)!",
		       count,
		       et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH].size);
		return -ENOMEM;
	}

	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf)
		return -ENOMEM;

	rv = copy_from_user(kern_buf, buf, count);
	if (rv) {
		pr_err("failed to copy from user!");
		goto error;
	}

	et_iowrite(et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SCRATCH]
			   .io.mapped_baseaddr,
		   0, kern_buf, count);

	rv = count;

error:
	kfree(kern_buf);

	return rv;
}
