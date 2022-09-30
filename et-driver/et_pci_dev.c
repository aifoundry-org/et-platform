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
/*
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

void et_save_bars(struct et_pci_dev *et_dev)
{
	int i, err = 0;

	for (i = 0; i < sizeof(et_dev->bar_cfgs) / sizeof(et_dev->bar_cfgs[0]);
	     i++) {
		err |= pci_read_config_dword(et_dev->pdev,
					     PCI_BASE_ADDRESS_0 + i * 4,
					     &et_dev->bar_cfgs[i]);
	}

	WARN_ON(err);
}

void et_restore_bars(struct et_pci_dev *et_dev)
{
	int i, err = 0;

	for (i = 0; i < sizeof(et_dev->bar_cfgs) / sizeof(et_dev->bar_cfgs[0]);
	     i++) {
		err |= pci_write_config_dword(et_dev->pdev,
					      PCI_BASE_ADDRESS_0 + i * 4,
					      et_dev->bar_cfgs[i]);
	}

	WARN_ON(err);
}

int et_map_bar(struct et_pci_dev *et_dev,
	       const struct et_bar_mapping *bm_info,
	       void __iomem **mapped_addr_ptr)
{
	u32 bar_cfg;

	if (bm_info->bar >= 6) {
		dev_err(&et_dev->pdev->dev,
			"Invalid BAR number: %d\n",
			bm_info->bar);
		return -EINVAL;
	}

	pci_read_config_dword(et_dev->pdev,
			      PCI_BASE_ADDRESS_0 + bm_info->bar * 4,
			      &bar_cfg);

	if (bar_cfg & PCI_BASE_ADDRESS_MEM_PREFETCH)
		*mapped_addr_ptr = pci_iomap_wc_range(et_dev->pdev,
						      bm_info->bar,
						      bm_info->bar_offset,
						      bm_info->size);
	else
		*mapped_addr_ptr = pci_iomap_range(et_dev->pdev,
						   bm_info->bar,
						   bm_info->bar_offset,
						   bm_info->size);

	if (IS_ERR(*mapped_addr_ptr)) {
		dev_err(&et_dev->pdev->dev, "bar mapping failed\n");
		return PTR_ERR(*mapped_addr_ptr);
	}

	return 0;
}

void et_unmap_bar(void __iomem *mapped_addr)
{
	if (mapped_addr)
		iounmap(mapped_addr);
}
