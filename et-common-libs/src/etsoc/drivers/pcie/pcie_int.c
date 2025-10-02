/***********************************************************************
*
* Copyright (C) 2019 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#include "etsoc/isa/io.h"
#include "etsoc/drivers/pcie/pcie_int.h"
#include "etsoc/drivers/pcie/pcie_device.h"
#include "etsoc/isa/atomic.h"
#include "etsoc/isa/etsoc_rt_memory.h"

/*! \enum pcie_int_t
    \brief Enum which specifies the PCI interrupt types
*/
enum pcie_int_t {
    pcie_int_none = 0,
    pcie_int_legacy,
    pcie_int_msi,
    pcie_int_msix
};

typedef uint32_t pcie_int_t;

/*! \struct pcie_cb
    \brief struct which represent the pcie interrupt control block
*/
typedef struct pcie_cb {
    pcie_int_t int_type;
    uint32_t int_vecs;
    uint32_t initialized;
} pcie_cb_t;

/*! \var pcie_cb_t PCIE_CB
    \brief Global control block for PCIE interrupts
*/
static pcie_cb_t PCIE_CB __attribute__((aligned(64))) = {0};

/* Local Functions */

static pcie_int_t pcie_get_int_type(void)
{
    uint32_t msi;
    uint32_t msix;
    uint32_t status;

    /* The PCI spec defines 3 different interrupt mechanisms. Per the PCI spec, the host system
    software will enable exactly one of them at a time. */

    msi = ioread32(PCIE0 +
                   PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_ADDRESS);
    if (PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_PCI_MSI_ENABLE_GET(
        msi))
    {
        return pcie_int_msi;
    }

    msix = ioread32(PCIE0 +
                    PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSIX_CAP_PCI_MSIX_CAP_ID_NEXT_CTRL_REG_ADDRESS);
    if (PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSIX_CAP_PCI_MSIX_CAP_ID_NEXT_CTRL_REG_PCI_MSIX_ENABLE_GET(
        msix))
    {
        return pcie_int_msix;
    }

    status = ioread32(PCIE0 + PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_ADDRESS);
    /* PCI_TYPE0_INT_EN bit is named "disable" in PCIe spec. Sigh Synopsis.
    0 = legacy ints enabled. */
    if (PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_PCI_TYPE0_INT_EN_GET(status) ==
        0)
    {
        return pcie_int_legacy;
    }

    return pcie_int_none;
}

static uint32_t pcie_get_int_vecs(pcie_int_t int_type)
{
    switch (int_type)
    {
    case pcie_int_legacy:
        return 1;
    case pcie_int_msi:
    {
        /* The host dynamically determines how many vectors to give you based
        on it's resources. You'll get between 1 and the number requested. */
        uint32_t msi = ioread32(
            PCIE0 + PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_ADDRESS);
        return (1U <<
                PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_PCI_MSI_MULTIPLE_MSG_EN_GET(
                      msi));
    }
    case pcie_int_msix:
    {
        uint32_t msix = ioread32(
            PCIE0 + PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSIX_CAP_PCI_MSIX_CAP_ID_NEXT_CTRL_REG_ADDRESS);
        return (uint32_t)
                PE0_DWC_EP_PCIE_CTL_DBI_SLAVE_PF0_MSIX_CAP_PCI_MSIX_CAP_ID_NEXT_CTRL_REG_PCI_MSIX_TABLE_SIZE_GET(
                msix) + 1U;
    }
    case pcie_int_none:
        /* Intentional fall through */
    default:
        return 0;
    }
}

static inline pcie_int_t pcie_cb_get_int_type(void)
{
    return ETSOC_RT_MEM_READ_32(&PCIE_CB.int_type);
}

static inline uint32_t pcie_cb_get_int_vecs(void)
{
    return ETSOC_RT_MEM_READ_32(&PCIE_CB.int_vecs);
}

static void pcie_cb_init(void)
{
    uint32_t temp32 = pcie_get_int_type();
    ETSOC_RT_MEM_WRITE_32(&PCIE_CB.int_type, temp32);
    temp32 = pcie_get_int_vecs(temp32);
    ETSOC_RT_MEM_WRITE_32(&PCIE_CB.int_vecs, temp32);
    temp32 = 1;
    ETSOC_RT_MEM_WRITE_32(&PCIE_CB.initialized, temp32);
}

int pcie_interrupt_host(uint32_t vec)
{
    /* The first time this function is called, only then initialize the PCIE CB */
    if(ETSOC_RT_MEM_READ_32(&PCIE_CB.initialized) == 0)
    {
        pcie_cb_init();
    }

    /* Get and verify the number of interrupt vector from PCIE CB */
    if (vec >= pcie_cb_get_int_vecs())
    {
        return -1;
    }

    uint32_t msi_mask;

    /* Get the interrupt type supported from PCIE CB */
    switch (pcie_cb_get_int_type())
    {
    case pcie_int_msi:
        msi_mask = 1U << vec;

        /* TODO: this can bit-bang faster than the synopsis IP can
        handle if you call pcie_interrupt_host() back-to-back */
        /* Need to read-modify-write to this register. But writing the
        value directly should work as well */
        iowrite32(PCIE_NOPCIESR + PCIE_NOPCIESR_MSI_TX_VEC_ADDRESS, msi_mask);
        msi_mask &= ~msi_mask;
        iowrite32(PCIE_NOPCIESR + PCIE_NOPCIESR_MSI_TX_VEC_ADDRESS, msi_mask);

        break;
    case pcie_int_msix:
        iowrite64(MSIX_TRIG_REG, (uint64_t)vec);
        break;
    case pcie_int_legacy:
        /* Intentional fall through. Even though hardware supports legacy, the sw stack is
        relying on PCIe write ordering for MSI/MSI-X (i.e. data written before a MSI is
        assured to arrive before the IRQ). With legacy, the IRQ can pass data in flight.
        Don't allow using legacy. */
    case pcie_int_none:
        /* Intentional fall through */
    default:
        return -1;
    }

    return 0;
}
