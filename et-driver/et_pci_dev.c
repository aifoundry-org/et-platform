#include "et_pci_dev.h"

#include "et_layout.h"
#include "hal_device.h"

//BAR2
//Name              Host Addr       Size   Notes
//R_PU_DIR_PC_MM    BAR2 + 0x0400   1k     MM DIR shared memory
//R_PU_DIR_PC_SP    BAR2 + 0x1400   1k     SP DIR shared memory
//R_PU_TRG_PCIE     BAR2 + 0x2000   8k     Mailbox interrupts
//R_PCIE_USRESR     BAR2 + 0x4000   4k     DMA control registers
//R_PU_SRAM_HI      BAR2 + 0x5000   128k   Mailbox shared memory (VQ)
const struct et_bar_mapping BAR_MAPPINGS[] = {
	{
		.soc_addr =              R_PU_DIR_PC_MM_BASEADDR,
		.size =                  R_PU_DIR_PC_MM_SIZE,
		.bar_offset =            0x400,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PU_DIR_PC_SP_BASEADDR,
		.size =                  R_PU_DIR_PC_SP_SIZE,
		.bar_offset =            0x1400,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PU_TRG_PCIE_BASEADDR,
		.size =                  R_PU_TRG_PCIE_SIZE,
		.bar_offset =            0x2000,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PCIE_USRESR_BASEADDR,
		.size =                  R_PCIE_USRESR_SIZE,
		.bar_offset =            0x4000,
		.bar =                   2,
		.strictly_order_access = true
	},
	{
		.soc_addr =              R_PU_SRAM_HI_BASEADDR,
		.size =                  R_PU_SRAM_HI_SIZE,
		.bar_offset =            0x5000,
		.bar =                   2,
		.strictly_order_access = true
	}
};

bool is_bar_prefetchable(struct et_pci_dev *et_dev, u8 bar_no)
{
	u32 bar;

	switch (bar_no) {
	case 0:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_0, &bar);
		break;
	case 1:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_1, &bar);
		break;
	case 2:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_2, &bar);
		break;
	case 3:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_3, &bar);
		break;
	case 4:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_4, &bar);
		break;
	case 5:
		pci_read_config_dword(et_dev->pdev, PCI_BASE_ADDRESS_5, &bar);
		break;
	default:
		pr_err("Invalid BAR number: %d\n", bar_no);
		return -EINVAL;
	}

	if (bar & PCI_BASE_ADDRESS_MEM_PREFETCH)
		return true;

	return false;
}

int et_map_bar(struct et_pci_dev *et_dev, const struct et_bar_mapping *bm_info,
	       void __iomem **bar_map_ptr)
{
	if (bm_info->strictly_order_access)
		*bar_map_ptr = pci_iomap_range(et_dev->pdev, bm_info->bar,
					       bm_info->bar_offset,
					       bm_info->size);
	else
		*bar_map_ptr = pci_iomap_wc_range(et_dev->pdev, bm_info->bar,
						  bm_info->bar_offset,
						  bm_info->size);

	if (IS_ERR(*bar_map_ptr)) {
		dev_err(&et_dev->pdev->dev, "bar mapping failed\n");
		return PTR_ERR(*bar_map_ptr);
	}

	return 0;
}

void et_unmap_bar(void __iomem *bar_map_ptr)
{
	if (bar_map_ptr)
		iounmap(bar_map_ptr);
}
