// SPDX-License-Identifier: GPL-2.0

/***********************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 **********************************************************************/

#include "et_pci_dev.h"

/* DIR Management BAR mapping */
#define R_DIR_MGMT_BAR	      2
#define R_DIR_MGMT_BAR_OFFSET 0x1000
#define R_DIR_MGMT_SIZE	      0x0000000200

/* DIR Operations BAR mapping */
#define R_DIR_OPS_BAR	     2
#define R_DIR_OPS_BAR_OFFSET 0x0
#define R_DIR_OPS_SIZE	     0x0000000200

// clang-format off

/**
 * ET BAR MAP
 * Name		Host Addr	Size	Notes
 * R_DIR_OPS	BAR2 + 0x0000	512B	DIR Ops shared memory
 * R_DIR_MGMT	BAR2 + 0x1000	512B	DIR Mgmt shared memory
 */
const struct et_bar_mapping DIR_MAPPINGS[] = {
	{
		.bar		= R_DIR_OPS_BAR,
		.bar_offset	= R_DIR_OPS_BAR_OFFSET,
		.size		= R_DIR_OPS_SIZE
	},
	{
		.bar		= R_DIR_MGMT_BAR,
		.bar_offset	= R_DIR_MGMT_BAR_OFFSET,
		.size		= R_DIR_MGMT_SIZE
	}
};

// clang-format on

/**
 * et_map_bar() - Maps a BAR region
 * @et_dev: pointer to device structure of type struct et_pci_dev
 * @bm_info: BAR mapping information
 * @mapped_addr_ptr: Mapped IOMEM base address will be returned in it
 *
 * Return: 0 on success, negative on error
 */
int et_map_bar(struct et_pci_dev *et_dev, const struct et_bar_mapping *bm_info,
	       void __iomem **mapped_addr_ptr)
{
	u32 bar_cfg;

	if (bm_info->bar >= 6) {
		dev_err(&et_dev->pdev->dev, "Invalid BAR number: %d\n",
			bm_info->bar);
		return -EINVAL;
	}

	pci_read_config_dword(et_dev->pdev,
			      PCI_BASE_ADDRESS_0 + bm_info->bar * 4, &bar_cfg);

	if (bar_cfg & PCI_BASE_ADDRESS_MEM_PREFETCH)
		*mapped_addr_ptr =
			pci_iomap_wc_range(et_dev->pdev, bm_info->bar,
					   bm_info->bar_offset, bm_info->size);
	else
		*mapped_addr_ptr =
			pci_iomap_range(et_dev->pdev, bm_info->bar,
					bm_info->bar_offset, bm_info->size);

	if (IS_ERR(*mapped_addr_ptr)) {
		dev_err(&et_dev->pdev->dev, "bar mapping failed\n");
		return PTR_ERR(*mapped_addr_ptr);
	}

	return 0;
}

/**
 * et_unmap_bar() - Unmaps the mapped BAR region
 * @et_dev: pointer to device structure of type struct et_pci_dev
 * @bm_info: BAR mapping information
 * @mapped_addr: Mapped IOMEM base address of the region to be unmapped
 */
void et_unmap_bar(void __iomem *mapped_addr)
{
	if (mapped_addr)
		iounmap(mapped_addr);
}
