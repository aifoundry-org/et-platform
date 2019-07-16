/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "pcie_int.h"

#include "hal_device.h"
#include "pcie_device.h"

//#include <stdio.h>

pcie_int_t pcie_get_int_type(void)
{
    //The PCI spec defines 3 different interrupt mechanisms. This SoC supports
    //all of them. Per the PCI spec, the host system software will enable
    //exactly one of them at a time per device.

    //Per the PCI spec, the board comes out of reset with only legacy
    //interrupts on, and MSIs are configured later. Until (or if)
    //MSIs are up, the device should use legacy ints to signal.
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_t msi_ctrl;
    msi_ctrl.R = PCIE0->PF0_MSI_CAP.PCI_MSI_CAP_ID_NEXT_CTRL_REG.R;

    if (msi_ctrl.B.PCI_MSI_ENABLE) return pcie_int_msi;

    PE0_DWC_pcie_ctl_DBI_Slave_PF0_MSIX_CAP_PCI_MSIX_CAP_ID_NEXT_CTRL_REG_t msix_ctrl;
    msix_ctrl.R = PCIE0->PF0_MSIX_CAP.PCI_MSIX_CAP_ID_NEXT_CTRL_REG.R;

    if (msix_ctrl.B.PCI_MSIX_ENABLE) return pcie_int_msix;

    PE0_DWC_pcie_ctl_DBI_Slave_PF0_TYPE0_HDR_STATUS_COMMAND_REG_t status_command;
    status_command.R = PCIE0->PF0_TYPE0_HDR.STATUS_COMMAND_REG.R;

    //PCI_TYPE0_INT_EN bit is named "disable" in PCIe spec. Sigh Synopsis. 0 = legacy ints enabled.
    if (status_command.B.PCI_TYPE0_INT_EN == 0) return pcie_int_legacy;

    return pcie_int_none;
}

uint32_t pcie_get_int_vecs(pcie_int_t int_type)
{
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_t msi_ctrl;

    switch (int_type)
    {
        case pcie_int_legacy:
            return 1;
        case pcie_int_msi:
            //The host dynamically determines how many vectors to give you based
            //on it's resources. You'll get between 1 and the number requested.
            msi_ctrl.R = PCIE0->PF0_MSI_CAP.PCI_MSI_CAP_ID_NEXT_CTRL_REG.R;

            return 1U << msi_ctrl.B.PCI_MSI_MULTIPLE_MSG_EN;
        case pcie_int_msix:
            //TODO. Fall through for now.
        case pcie_int_none:
            //Intentional fall through
        default:
            //printf("ERROR: int type %d not supported\r\n", (int32_t)int_type);
            return 0;
    }
}

int pcie_interrupt_host(uint32_t vec)
{
    const pcie_int_t int_type = pcie_get_int_type();

    if (vec >= pcie_get_int_vecs(int_type))
    {
        //printf("ERROR: int vec %d unsupported\r\n", vec);
        return -1;
    }

    uint32_t msi_mask;

    switch (int_type)
    {
        case pcie_int_msi:
            msi_mask = 1U << vec;

            PCIE_NOPCIESR->msi_tx_vec.R |= msi_mask;
            PCIE_NOPCIESR->msi_tx_vec.R &= ~msi_mask;
            //TODO: this can bit-bang faster than the synopsis IP can
            //handle if you call pcie_interrupt_host() back-to-back
            break;
        case pcie_int_legacy:
            //TODO. Fall through for now.
        case pcie_int_msix:
            //TODO. Fall through for now.
        case pcie_int_none:
            //Intentional fall through
        default:
            //printf("ERROR: int type %d not supported\r\n", (int32_t)int_type);
            return -1;
    }

    return 0;
}
