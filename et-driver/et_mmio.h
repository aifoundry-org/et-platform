#ifndef __ET_MMIO_H
#define __ET_MMIO_H

#include "et_pci_dev.h"

int et_mmio_iomem_idx(uint64_t soc_addr, uint64_t count);

ssize_t et_mmio_write_from_user(const char __user *buf, size_t count,
				loff_t *pos, struct et_pci_dev *et_dev);

ssize_t et_mmio_read_to_user(char __user *buf, size_t count, loff_t *pos,
			     struct et_pci_dev *et_dev);

ssize_t et_mmio_write_to_device(struct et_pci_dev *et_dev,
				const char __user *buf, size_t count,
				u64 soc_addr);

ssize_t et_mmio_read_from_device(struct et_pci_dev *et_dev, char __user *buf,
				 size_t count, u64 soc_addr);
#endif
