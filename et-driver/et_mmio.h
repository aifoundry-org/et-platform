#ifndef __ET_MMIO_H
#define __ET_MMIO_H

struct et_pci_dev;

ssize_t et_mmio_write_to_device(struct et_pci_dev *et_dev, bool is_mgmt,
				const char __user *buf, size_t count,
				u64 soc_addr);

ssize_t et_mmio_read_from_device(struct et_pci_dev *et_dev, bool is_mgmt,
				 char __user *buf, size_t count, u64 soc_addr);
#endif
