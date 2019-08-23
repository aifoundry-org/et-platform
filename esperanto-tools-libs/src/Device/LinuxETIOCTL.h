//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_LINUX_ET_IOCTL_H
#define ET_RUNTIME_DEVICE_LINUX_ET_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

// clang-format off

#define ESPERANTO_PCIE_IOCTL_MAGIC 0x67879

#define GET_DRAM_BASE       _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 1, int)
#define GET_DRAM_SIZE       _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 2, int)
#define GET_MM_MBOX_MAX_MSG _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 3, int)
#define GET_SP_MBOX_MAX_MSG _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 4, int)
#define RESET_MBOXES         _IO(ESPERANTO_PCIE_IOCTL_MAGIC, 5)
#define GET_MM_MBOX_READY   _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 6, int)
#define GET_SP_MBOX_READY   _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 7, int)
#define SET_BULK_CFG        _IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 8, uint32_t)

#define BULK_CFG_AUT0 0
#define BULK_CFG_MMIO 1
#define BULK_CFG_DMA 2

// clang-format on

#endif // ET_RUNTIME_DEVICE_LINUX_ET_IOCTL_H
