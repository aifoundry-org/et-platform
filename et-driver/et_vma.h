/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-----------------------------------------------------------------------------
 */

#ifndef __ET_VMA_H
#define __ET_VMA_H

struct et_pci_dev;

struct vm_area_struct *et_find_vma(struct et_pci_dev *et_dev,
				   unsigned long vaddr);

#endif
