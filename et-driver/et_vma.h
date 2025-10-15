/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 *-----------------------------------------------------------------------------
 */

#ifndef __ET_VMA_H
#define __ET_VMA_H

struct et_pci_dev;

struct vm_area_struct *et_find_vma(struct et_pci_dev *et_dev,
				   unsigned long vaddr);

#endif
