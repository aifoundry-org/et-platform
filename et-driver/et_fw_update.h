#ifndef __ET_FW_UPDATE_H
#define __ET_FW_UPDATE_H

struct et_pci_dev;

/*
 * Writes FW Image on MGMT_MEM_REGION_TYPE_SCRATCH
 *
 * et_dev:	et-soc device pointer
 * buf:		user virtual address for firmware image
 * count:	count of bytes of firmware image
 *
 * Returns the number of bytes written on success, or a negative value
 * on failure.
 */
ssize_t et_mmio_write_fw_image(struct et_pci_dev *et_dev,
			       const char __user *buf, size_t count);

#endif
