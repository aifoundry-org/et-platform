#include "et_pci_dev.h"

#define R_PU_DIR_PC_SP_BAR		2
#define R_PU_DIR_PC_SP_BAR_OFFSET	0x1000
#define R_PU_DIR_PC_SP_SIZE		0x0000000200

#define R_PU_DIR_PC_MM_BAR		2
#define R_PU_DIR_PC_MM_BAR_OFFSET	0x0
#define R_PU_DIR_PC_MM_SIZE		0x0000000200

//BAR2
//Name              Host Addr       Size     Notes
//R_PU_DIR_PC_MM    BAR2 + 0x0000   512B     MM DIR shared memory
//R_PU_DIR_PC_SP    BAR2 + 0x1000   512B     SP DIR shared memory
const struct et_bar_mapping DIR_MAPPINGS[] = {
	{
		.bar		= R_PU_DIR_PC_MM_BAR,
		.bar_offset	= R_PU_DIR_PC_MM_BAR_OFFSET,
		.size		= R_PU_DIR_PC_MM_SIZE
	},
	{
		.bar		= R_PU_DIR_PC_SP_BAR,
		.bar_offset	= R_PU_DIR_PC_SP_BAR_OFFSET,
		.size		= R_PU_DIR_PC_SP_SIZE
	}
};

int et_map_bar(struct et_pci_dev *et_dev, const struct et_bar_mapping *bm_info,
	       void __iomem **mapped_addr_ptr)
{
	u32 bar_cfg;

	switch (bm_info->bar) {
	case 0:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_0,
				      &bar_cfg);
		break;
	case 1:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_1,
				      &bar_cfg);
		break;
	case 2:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_2,
				      &bar_cfg);
		break;
	case 3:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_3,
				      &bar_cfg);
		break;
	case 4:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_4,
				      &bar_cfg);
		break;
	case 5:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_5,
				      &bar_cfg);
		break;
	default:
		dev_err(&et_dev->pdev->dev, "Invalid BAR number: %d\n",
			bm_info->bar);
		return -EINVAL;
	}

	if (bar_cfg & PCI_BASE_ADDRESS_MEM_PREFETCH)
		*mapped_addr_ptr = pci_iomap_wc_range(et_dev->pdev,
						      bm_info->bar,
						      bm_info->bar_offset,
						      bm_info->size);
	else
		*mapped_addr_ptr = pci_iomap_range(et_dev->pdev, bm_info->bar,
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
