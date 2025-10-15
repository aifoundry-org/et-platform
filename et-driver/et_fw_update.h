/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 ******************************************************************************/

#ifndef __ET_FW_UPDATE_H
#define __ET_FW_UPDATE_H

struct et_pci_dev;

ssize_t et_mmio_write_fw_image(struct et_pci_dev *et_dev,
			       const char __user *buf, size_t count);

#endif
