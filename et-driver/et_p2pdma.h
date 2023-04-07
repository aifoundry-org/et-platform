/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ******************************************************************************
 */

#ifndef __ET_P2PDMA_H
#define __ET_P2PDMA_H

#include <linux/kernel.h>
#include <linux/pci-p2pdma.h>
#include <linux/rwsem.h>

#include "et_dev_iface_regs.h"
#include "et_pci_dev.h"

struct et_p2pdma_region {
	struct list_head list;
	struct et_mapped_region *region;
};

struct et_p2pdma_mapping {
	struct pci_dev *pdev;
	struct rw_semaphore rwsem; /*
				    * serializes changes to this struct fields
				    * (e.g. during ETSOC reset) vs read access
				    * by other devices
				    */
	struct list_head region_list; /* list of pointer of P2P regions */
	DECLARE_BITMAP(dev_compat_bitmap,
		       ET_MAX_DEVS); /* compatibility with other devices */
};

void et_p2pdma_init(u8 devnum);
u64 et_p2pdma_get_compat_bitmap(u8 devnum);
int et_p2pdma_add_resource(struct et_pci_dev *et_dev,
			   const struct et_bar_mapping *bm_info,
			   struct et_mapped_region *region);
void et_p2pdma_release_resource(struct et_pci_dev *et_dev,
				struct et_mapped_region *region);
ssize_t et_p2pdma_move_data(struct et_pci_dev *et_dev,
			    u16 queue_index,
			    char __user *ucmd,
			    size_t ucmd_size);

#endif
