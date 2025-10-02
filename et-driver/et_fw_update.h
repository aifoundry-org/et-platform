/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (C) 2020 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ******************************************************************************/

#ifndef __ET_FW_UPDATE_H
#define __ET_FW_UPDATE_H

struct et_pci_dev;

ssize_t et_mmio_write_fw_image(struct et_pci_dev *et_dev,
			       const char __user *buf, size_t count);

#endif
