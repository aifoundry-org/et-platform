#ifndef __ET_IOCTL_H
#define __ET_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ESPERANTO_PCIE_IOCTL_MAGIC 0x67879

#define GET_DRAM_BASE       _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 1, uint64_t)
#define GET_DRAM_SIZE       _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 2, uint64_t)
#define GET_MBOX_MAX_MSG    _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 3, uint64_t)
#define RESET_MBOX           _IO(ESPERANTO_PCIE_IOCTL_MAGIC, 4)
#define GET_MBOX_READY      _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 5, uint64_t)
#define SET_BULK_CFG        _IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 6, uint32_t)

#define BULK_CFG_AUT0 0
#define BULK_CFG_MMIO 1
#define BULK_CFG_DMA 2

#endif
