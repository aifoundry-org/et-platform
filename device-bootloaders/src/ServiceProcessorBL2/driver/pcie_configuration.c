/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "log.h"
#include "io.h"
#include "layout.h"
#include "pcie_device.h"
#include "pcie_configuration.h"

#include "hwinc/sp_cru_reset.h"
#include "hwinc/hal_device.h"

static void pcie_init_pshire(void);
static void pcie_init_caps_list(void);
static void pcie_init_bars(void);
static void pcie_init_ints(void);
static void pcie_init_link(void);
static void pcie_init_atus(void);
static void pcie_wait_for_ints(void);

/*! \def BAR2_SIZE
    \brief BAR Sizes must be powers of 2 per the PCIe spec. Round up to next biggest power of 2.
*/
#define BAR2_SIZE 0x000000000003FFFFULL /*256kb*/

/*! \def BAR_IN_MEM_SPACE
    \brief BARs in memory space
*/
#define BAR_IN_MEM_SPACE 0

/*! \def BAR_TYPE_64BIT
    \brief Define for 64-bit type
*/
#define BAR_TYPE_64BIT   2

/*! \def BAR_ENABLE
    \brief BAr Enable mask
*/
#define BAR_ENABLE       1

/*! \def BAR_DISABLE
    \brief BAR disable mask
*/
#define BAR_DISABLE      0

/*! \def FB_MODE_FIGURE_OF_MERIT
    \brief Set FB MODE
*/
#define FB_MODE_FIGURE_OF_MERIT  1

/*! \def RATE_SHADOW_SEL_GEN4
    \brief RATE_SHADOW_SEL_GEN4
*/
#define RATE_SHADOW_SEL_GEN4     1

/*! \def SMLH_LTSSM_STATE_LINK_UP
    \brief Link up mask
*/
#define SMLH_LTSSM_STATE_LINK_UP 0x11


/*! \def MSI_ENABLED
    \brief  MSI enabled value */
#define MSI_ENABLED 0x1U

/*! \def MSI_TWO_VECTORS
    \brief MSI two verctor enable
*/
#define MSI_TWO_VECTORS 1

/* The driver can populate this structure with the defaults that will be used during the init
    phase.*/

static struct pcie_event_control_block event_control_block __attribute__((section(".data")));

/*!
 * @struct struct pcie_event_control_block
 * @brief PCIE driver error mgmt control block
 */
struct pcie_event_control_block
{
    uint32_t ce_count; /**< Correctable error count. */
    uint32_t uce_count; /**< Un-Correctable error count. */
    uint32_t ce_threshold; /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \var static uint32_t PCIE_SPEED[5]
    \brief PCIE Speed Generation
*/
static uint32_t PCIE_SPEED[5] = { PCIE_GEN_1, PCIE_GEN_2, PCIE_GEN_3, PCIE_GEN_4, PCIE_GEN_5 };

int32_t pcie_error_control_init(dm_event_isr_callback event_cb)
{
    event_control_block.event_cb = event_cb;
    return 0;
}

/*! \brief This step is to release PShire out of reset,
    Both Cold/Warm reset are initialized
    \param[in] none */

void PCIe_release_pshire_from_reset(void)
{
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_PSHIRE_COLD_ADDRESS,
              RESET_MANAGER_RM_PSHIRE_COLD_RSTN_SET(1));
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_PSHIRE_WARM_ADDRESS,
              RESET_MANAGER_RM_PSHIRE_WARM_RSTN_SET(1));
}

/*! \brief This step is to initiate PShire, and PCIE SS, and if link is not up,
   kick off the link training process. Expectation Host will ink training successfully
   \param[in] none  */
void PCIe_init(bool expect_link_up)
{
    uint32_t tmp;

    /* If the PCIe link should already be up (e.x. ServiceProcessorROM should have ran
       pcie_boot_config()), and it's not, try again anyways; however, the init timing may be
      slow enough that the device will not be enumerated by the host properly. */
    bool init_link = true;

    if (expect_link_up)
    {
        tmp = ioread32(PCIE_CUST_SS +
                       DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);
        if (DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_SMLH_LTSSM_STATE_GET(
                tmp) == SMLH_LTSSM_STATE_LINK_UP)
        {
            init_link = false;
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "Warning: PCIe link not properly inited, trying again...\r\n");
        }
    }

    if (init_link)
    {
        pcie_init_pshire();
        pcie_init_caps_list();
        pcie_init_bars();
        pcie_init_ints();
        pcie_init_link();
    }

}

/*! \brief This is the last step to enable PCIe link so Host/Device can communicate.
    \param[in] none
    These steps don't block the PCIe IP from responding to config space messages,
    so their timing does not contribute to the PERST_n requirements. Always do
    them as late as possible (i.e. not in pcie_boot_config()) to simplfy things. */
void pcie_enable_link(void)
{
    uint32_t tmp;

    pcie_init_atus();
    pcie_wait_for_ints();

    tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);
    Log_Write(LOG_LEVEL_CRITICAL, "PCIe link up at Gen %ld\r\n",
           DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp) + 1);
}

static void pcie_init_pshire(void)
{
    uint32_t tmp;
    /* TODO: ABP Reset deassert? In "PCIe Initialization Sequence" doc, TBD what state to check */

    /* Wait for PERST_N */
    Log_Write(LOG_LEVEL_CRITICAL, "Waiting for PCIe bus out of reset...");
    do
    {
        tmp = ioread32(PCIE_ESR + PSHIRE_PSHIRE_STAT_ADDRESS);
    } while (PSHIRE_PSHIRE_STAT_PERST_N_GET(tmp) == 0);
    Log_Write(LOG_LEVEL_CRITICAL, " done\r\n");

    /* Deassert PCIe cold reset */
    tmp = ioread32(PCIE_ESR + PSHIRE_PSHIRE_RESET_ADDRESS);
    tmp = PSHIRE_PSHIRE_RESET_PWR_UP_RSTN_MODIFY(tmp, 1);
    iowrite32(PCIE_ESR + PSHIRE_PSHIRE_RESET_ADDRESS, tmp);

    /* Wait for CDM (controller, dual-mode; all of the "PCIE0" regs) to be ready */
    do
    {
        tmp = ioread32(PCIE_CUST_SS +
                       DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);
    } while (DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_CDM_IN_RESET_GET(tmp) !=
             0);
}

static void pcie_init_caps_list(void)
{
    uint32_t misc_control;

    /* Init capabilities list. The compiled-in, default capabilities list looks like this:
      HEAD (0x34) -> Power Mgmt (0x40) -> MSI (0x50) -> PCIe (0x70) -> MSI-X (0xB0) -> NULL

      For now, the Linux host driver is not implementing the power management API,
      so don't advertise the power management capability. */

    /* The config registers are protected by a write-enable bit */
    misc_control =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS);
    misc_control =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
        misc_control, 1);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              misc_control);

    /* Make HEAD point to MSI cap, skipping Power Mgmt Cap */
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_PCI_CAP_PTR_REG_ADDRESS,
              PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_PCI_CAP_PTR_REG_CAP_POINTER_SET(0x50));

    misc_control =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
        misc_control, 0);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              misc_control);
}

static void pcie_init_bars(void)
{
    uint32_t bar0, bar2;
    uint32_t misc_control;

    /* The BAR config registers are protected by a write-enable bit */
    misc_control =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS);
    misc_control =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
        misc_control, 1);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              misc_control);

    /* When BARs are used in 64-bit mode, BAR1 becomes the high 32-bits of BAR0, and so on
       (BAR3 for BAR2, BAR5 for BAR4) */

    /* BAR0 has the resizeable BAR capability setup. It ignores the mask register (except for the
       enable bit), and instead bases it's size on RESBAR_CTRL_REG_BAR_SIZE. */
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_DBI2_BAR0_MASK_REG_ADDRESS,
              BAR_ENABLE);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_DBI2_BAR1_MASK_REG_ADDRESS,
              BAR_DISABLE);

    /* All other BARs are not resizeable, and use the mask register to set size */
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_DBI2_BAR2_MASK_REG_ADDRESS,
              (uint32_t)(BAR2_SIZE & 0xFFFFFFFFUL) | BAR_ENABLE);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_DBI2_BAR3_MASK_REG_ADDRESS,
              (uint32_t)((BAR2_SIZE >> 32) & 0xFFFFFFFFULL));

    /* Leave the BAR 4/5 pair in it's default configuration. This pair is set in hardware
       to only be accessable on the SoC from within the Synopsis PCIe IP block. It is used
      for accessing things like the MSI-X table, which is in RAM contained in the IP block. */

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_DBI2_EXP_ROM_BAR_MASK_REG_ADDRESS,
              BAR_DISABLE);

    /* Only allow the host to size BAR0 to 32GB */
    iowrite32(
        PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_RESBAR_CAP_RESBAR_CAP_REG_0_REG_ADDRESS,
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_RESBAR_CAP_RESBAR_CAP_REG_0_REG_RESBAR_CAP_REG_0_32GB_SET(
            1));

    /* Default BAR0 to 32GB. 32GB = 2^35. BAR size is encoded as
       2^(RESBAR_CTRL_REG_BAR_SIZE + 20) - set to 15 to get 32GB. */
    iowrite32(
        PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_RESBAR_CAP_RESBAR_CTRL_REG_0_REG_ADDRESS,
     PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_RESBAR_CAP_RESBAR_CTRL_REG_0_REG_RESBAR_CTRL_REG_BAR_SIZE_SET(
            15));

    /* BAR0 config (maps DRAM) */
    bar0 = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR0_REG_ADDRESS);
    bar0 =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR0_REG_BAR0_MEM_IO_MODIFY(
            bar0, BAR_IN_MEM_SPACE);
    bar0 =
        (uint32_t)PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR0_REG_BAR0_TYPE_MODIFY(
            bar0, BAR_TYPE_64BIT);
    bar0 =
        /* IMPORTANT: Many hosts do not tolerate > 1 gb BARs that are not prefetchable */
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR0_REG_BAR0_PREFETCH_MODIFY(bar0, 1);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR0_REG_ADDRESS, bar0);

    /* BAR2 config (maps registers) */
    bar2 = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR2_REG_ADDRESS);
    bar2 = PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR2_REG_BAR2_MEM_IO_MODIFY(
                bar2, BAR_IN_MEM_SPACE);
    bar2 = (uint32_t)PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR2_REG_BAR2_TYPE_MODIFY(
                bar2, BAR_TYPE_64BIT);

    /* IMPORTANT - not prefetchable (registers with read side effects mapped here) */
    bar2 = PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR2_REG_BAR2_PREFETCH_MODIFY(bar2, 0);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR2_REG_ADDRESS, bar2);

    /* Wait to init iATUs until BAR addresses are assigned - see PCIe_initATUs. */

    misc_control =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
                misc_control, 0);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              misc_control);
}

static void pcie_init_ints(void)
{
    uint32_t misc_control;
    uint32_t msi_ctrl;

    /* Open access to MSI Capability Register */
    misc_control =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS);
    misc_control =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
            misc_control, 1);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              misc_control);

    /* Configure MSI */
    msi_ctrl = ioread32(
        PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_ADDRESS);
    /* Enable MSI (PCIE CFG 0x50) */
    msi_ctrl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_PCI_MSI_ENABLE_MODIFY(
            msi_ctrl, MSI_TWO_VECTORS);
    /* Request 2 interrupt vectors (1 for Management and Ops node) */
    msi_ctrl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_PCI_MSI_MULTIPLE_MSG_CAP_MODIFY(
            msi_ctrl, MSI_TWO_VECTORS);
    msi_ctrl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_PCI_MSI_MULTIPLE_MSG_EN_MODIFY(
            msi_ctrl, MSI_TWO_VECTORS);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_ADDRESS,
              msi_ctrl);
    /* Close access to MSI Capability Register */
    misc_control =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
            misc_control, 0);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              misc_control);
}

static void pcie_init_link(void)
{
    uint32_t tmp;

    /*  Setup FSM tracker per Synopsis testbench */
    iowrite32(
        PCIE_CUST_SS + DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_FSM_TRACK_1_ADDRESS,
        0xCC);
        /*TODO in "PCIe Initialization Sequence" doc, this comes before polling CDM_IN_REST.
           Matching order from DV code. */

    /* Enable Fast Link Mode - Trains link faster for simulation/emulation
     Note: The PCIe transactor (Phy replacement) in ZeBu requires this to be set. */
    tmp = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_PORT_LINK_CTRL_OFF_ADDRESS);
    tmp = PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_PORT_LINK_CTRL_OFF_FAST_LINK_MODE_MODIFY(
        tmp, 1); /* TODO: should this be disabled in production? */
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_PORT_LINK_CTRL_OFF_ADDRESS, tmp);

    /* When FAST_LINK_MODE is on, scale LTSSM timer x1024 */
    tmp = ioread32(PCIE0 +
                   PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_TIMER_CTRL_MAX_FUNC_NUM_OFF_ADDRESS);
    tmp =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_TIMER_CTRL_MAX_FUNC_NUM_OFF_FAST_LINK_SCALING_FACTOR_MODIFY(
            tmp, 0);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_TIMER_CTRL_MAX_FUNC_NUM_OFF_ADDRESS,
              tmp);

    /* Configure PCIe LTSSM phase 2 equalization behavior.
       All values below provided by James Yu 2019-06-19 */

    /* First, configure PCIe Gen 3.0 LTSSM behavior */
    uint32_t gen3EqControl;
    gen3EqControl =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_ADDRESS);

    gen3EqControl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_GEN3_EQ_FB_MODE_MODIFY(
            gen3EqControl, FB_MODE_FIGURE_OF_MERIT);
    gen3EqControl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_GEN3_EQ_PHASE23_EXIT_MODE_MODIFY(
            gen3EqControl, 1);
    gen3EqControl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_GEN3_EQ_EVAL_2MS_DISABLE_MODIFY(
            gen3EqControl, 1);
    gen3EqControl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_GEN3_EQ_FOM_INC_INITIAL_EVAL_MODIFY(
            gen3EqControl, 0);
    gen3EqControl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_GEN3_EQ_PSET_REQ_VEC_MODIFY(
            gen3EqControl, 4); /* preset 2, arbitrarily chosen */

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_ADDRESS,
              gen3EqControl); /* write Gen 3.0 config */

    /* Then, configure PCIe Gen 4.0 LTSSM behavior */

    /* By setting RATE_SHADOW_SEL; writes to GEN3_EQ_CONTROL_OFF actually configure Gen 4.0 */
    tmp = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_RELATED_OFF_ADDRESS);
    tmp =
        (uint32_t)PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_RELATED_OFF_RATE_SHADOW_SEL_MODIFY(
            tmp, RATE_SHADOW_SEL_GEN4);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_RELATED_OFF_ADDRESS, tmp);

    gen3EqControl = (uint32_t)
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_GEN3_EQ_PSET_REQ_VEC_MODIFY(
            gen3EqControl, 8); /* preset 3, arbitrarily chosen */
    /* all other gen3EqControl values the same as Gen 3.0 configuration */

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_ADDRESS,
              gen3EqControl); /* write Gen 4.0 config */

    /* Enable LTSSM */
    tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_GEN_CTRL_3_ADDRESS);
    tmp = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_GEN_CTRL_3_LTSSM_EN_MODIFY(tmp, 1);
    iowrite32(PCIE_CUST_SS + DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_GEN_CTRL_3_ADDRESS,
              tmp);

    /* Wait for link training to finish*/
    Log_Write(LOG_LEVEL_CRITICAL, "Link training...");
    do {
        tmp = ioread32(PCIE_CUST_SS +
                       DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);
    } while (DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_SMLH_LTSSM_STATE_GET(
                 tmp) != SMLH_LTSSM_STATE_LINK_UP);
    Log_Write(LOG_LEVEL_CRITICAL, "done\r\n");
}
/*! \def CONFIG_INBOUND_IATU
    \brief  See DWC_pcie_ctl_dm_databook section 3.10.11
            Since the regmap codegen does not make an array of iATU registers, parameterize with macros */
#define CONFIG_INBOUND_IATU(num)                                                                                        \
    static void config_inbound_iatu_##num(uint64_t baseAddr, uint64_t targetAddr, uint64_t size)                        \
    {                                                                                                                   \
        uint64_t limitAddr = baseAddr + size - 1;                                                                       \
                                                                                                                        \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_##num##_ADDRESS,                  \
            PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_##num##_TYPE_SET(                     \
                0) | /*Watch to TLPs with TYPE field 0 (mem space)*/                                                    \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_##num##_INCREASE_REGION_SIZE_SET( \
                    1) /*Use the UPPR_LIMIT_ADDR reg*/                                                                  \
        );                                                                                                              \
                                                                                                                        \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_LWR_BASE_ADDR_OFF_INBOUND_##num##_ADDRESS,                  \
            (uint32_t)baseAddr);                                                                                        \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_UPPER_BASE_ADDR_OFF_INBOUND_##num##_ADDRESS,                \
            (uint32_t)(baseAddr >> 32));                                                                                \
                                                                                                                        \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_LIMIT_ADDR_OFF_INBOUND_##num##_ADDRESS,                     \
            (uint32_t)limitAddr);                                                                                       \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_UPPR_LIMIT_ADDR_OFF_INBOUND_##num##_ADDRESS,                \
            (uint32_t)(limitAddr >> 32));                                                                               \
                                                                                                                        \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_LWR_TARGET_ADDR_OFF_INBOUND_##num##_ADDRESS,                \
            (uint32_t)targetAddr);                                                                                      \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_##num##_ADDRESS,              \
            (uint32_t)(targetAddr >> 32));                                                                              \
                                                                                                                        \
        iowrite32(                                                                                                      \
            PCIE0 +                                                                                                     \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_##num##_ADDRESS,                  \
            PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_##num##_MATCH_MODE_SET(               \
                0) | /*Address match mode. Do NOT use BAR match mode.*/                                                 \
                PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_##num##_REGION_EN_SET(            \
                    (uint32_t)1));                                                                                      \
    }

CONFIG_INBOUND_IATU(0)
CONFIG_INBOUND_IATU(1)
CONFIG_INBOUND_IATU(2)
CONFIG_INBOUND_IATU(3)

static void pcie_init_atus(void)
{
    /* The config registers are protected by a write-enable bit */
    uint32_t miscControl1;
    miscControl1 =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS);
    miscControl1 =
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
        miscControl1, 1);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              miscControl1);

    /* The iATU has a "BAR Match Mode" feature where it can track BARs, but that mode does not allow
       any offset from the BAR, so we can't map multiple iATUs into one BAR. So, instead of using BAR
       match mode, wait until the BARs are programmed by the host, and program the iATUs manually.

      Note the initial BAR values assigned by the BIOS may be reassigned by the OS, so just waiting
      for the BARs to be assigned is not sufficient. However, the OS, not the BIOS, enables PCI
      memory space. Once the memory space is enabled, the BAR values cannot change anymore. So, it's
      critical to wait to program the iATUs until after memory space is enabled. */

    /* This wait could be long (tens of seconds), depending on when the OS enables PCIe */
    Log_Write(LOG_LEVEL_CRITICAL, "Waiting for host to enable memory space...");
    uint32_t status_command_reg;
    do
    {
        status_command_reg =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_ADDRESS);
    } while (PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_PCI_TYPE0_MEM_SPACE_EN_GET(
                 status_command_reg) == 0);
    Log_Write(LOG_LEVEL_CRITICAL, " done\r\n");

    /* TODO: I need to ensure the host does not try and send Mem Rd / Mem Wr before the iATUs
       are configured. The latency of a PCIe transaction (1-10s of uS) is probably long enough
       that the iATUs will always be programmed between the PCIe config TLP to enable mem
       space and the first PCIe MRd/MWr. However, we should make sure. Send the host an interrupt
      (once interrupts are implemented), make the  kernel driver block on receiving the first
      interrupt before doing MRd/MWr to these regions. */

    /* Setup BAR0
       Name        Host Addr       SoC Addr      Size   Notes
       R_L3_DRAM   BAR0 + 0x0000   0x8005000000  ~32G   SoC DRAM */

    uint32_t bar0_lo = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR0_REG_ADDRESS);
    uint32_t bar0_hi = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR1_REG_ADDRESS);
    uint64_t bar0 = ((uint64_t)bar0_hi << 32) | ((uint64_t)bar0_lo & 0xFFFFFFF0ULL);

    config_inbound_iatu_0(bar0, /* baseAddr */
                          DRAM_MEMMAP_BEGIN, /* targetAddr */
                          DRAM_MEMMAP_SIZE); /* size */

    /* Setup BAR2
       Name              Host Addr       SoC Addr      Size   Notes
       R_PU_MBOX_PC_MM   BAR2 + 0x0000   0x0020007000  4k     Mailbox shared memory
       R_PU_MBOX_PC_SP   BAR2 + 0x1000   0x0030003000  4k     Mailbox shared memory
       R_PU_TRG_PCIE     BAR2 + 0x2000   0x0030008000  8k     Mailbox interrupts */

    /* start on BAR2 */
    uint32_t baseAddr_lo =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR2_REG_ADDRESS);
    uint32_t baseAddr_hi =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_TYPE0_HDR_BAR3_REG_ADDRESS);
    uint64_t baseAddr = ((uint64_t)baseAddr_hi << 32) | ((uint64_t)baseAddr_lo & 0xFFFFFFF0ULL);

    config_inbound_iatu_1(baseAddr,
                          R_PU_MBOX_PC_MM_BASEADDR, /* targetAddr */
                          R_PU_MBOX_PC_MM_SIZE); /* size */

    baseAddr += R_PU_MBOX_PC_MM_SIZE;

    config_inbound_iatu_2(baseAddr,
                          R_PU_MBOX_PC_SP_BASEADDR, /* targetAddr */
                          R_PU_MBOX_PC_SP_SIZE); /* size */

    baseAddr += R_PU_MBOX_PC_SP_SIZE;

    config_inbound_iatu_3(baseAddr,
                          R_PU_TRG_PCIE_BASEADDR, //targetAddr
                          R_PU_TRG_PCIE_SIZE); //size

    baseAddr += R_PU_TRG_PCIE_SIZE;

    miscControl1 = PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN_MODIFY(
        miscControl1, 0);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_ADDRESS,
              miscControl1);
}

static void pcie_wait_for_ints(void)
{
    /* Wait until the x86 host driver comes up and enables MSI
       See pci_alloc_irq_vectors on the host driver. */

    uint32_t msi_ctrl;

    Log_Write(LOG_LEVEL_CRITICAL, "Waiting for host to enable MSI...");
    do
    {
        msi_ctrl = ioread32(
            PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_ADDRESS);
    } while (
        PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_PCI_MSI_ENABLE_GET(
            msi_ctrl) != MSI_ENABLED);
    Log_Write(LOG_LEVEL_CRITICAL, " done\r\n");
}

int32_t pcie_error_control_deinit(void)
{
    return 0;
}

int32_t pcie_enable_uce_interrupt(void)
{
    return 0;
}

int32_t pcie_disable_ce_interrupt(void)
{
    return 0;
}

int32_t pcie_disable_uce_interrupt(void)
{
    return 0;
}

int32_t pcie_set_ce_threshold(uint32_t ce_threshold)
{
    /* set countable errors threshold */
    event_control_block.ce_threshold = ce_threshold;
    return 0;
}

int32_t pcie_get_ce_count(uint32_t *ce_count)
{
    /* get correctable errors count */
    *ce_count = event_control_block.ce_count;
    return 0;
}

int32_t pcie_get_uce_count(uint32_t *uce_count)
{
    /* get un-correctable errors count */
    *uce_count = event_control_block.uce_count;
    return 0;
}

void pcie_error_threshold_isr(void)
{
    /* TODO: This is just an example implementation.
       The final driver implementation will read these values from the
       hardware, create a message and invoke call back with message and error type as parameters.
    */

    if (++event_control_block.ce_count > event_control_block.ce_threshold)
    {
        struct event_message_t message;

        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, PCIE_UCE,
                          sizeof(struct event_message_t) - sizeof(struct cmn_header_t));
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 1024, 1, 0);

        /* call the callback function and post message */
        event_control_block.event_cb(CORRECTABLE, &message);
    }
}

int32_t setup_pcie_gen3_link_speed(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6607 */
    return 0;
}

int32_t setup_pcie_gen4_link_speed(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6607 */
    return 0;
}

int32_t setup_pcie_lane_width_x4(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6607 */
    return 0;
}

int32_t setup_pcie_lane_width_x8(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6607 */
    return 0;
}

int32_t pcie_retrain_phy(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6607 */
    return 0;
}

int pcie_get_speed(char *pcie_speed)
{
    uint32_t pcie_gen, tmp;

    tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);

    /* Get the PCIE Gen */
    pcie_gen = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp);

    snprintf(pcie_speed, 8, "%d", PCIE_SPEED[pcie_gen - 1]);

    return 0;
}

int PShire_Initialize(void)
{
    PCIe_release_pshire_from_reset();
    /*Configure PShire PLL to 1010 Mhz */
    configure_pshire_pll(6);
    return 0;
}

int PCIE_Controller_Initialize(void)
{
    PCIe_init(true /* expect_link_up*/);
    return 0;
}

int PCIE_Phy_Initialize(void)
{
    /* interface to initialize PCIe phy */
    return 0;
}

int PShire_Voltage_Update(uint8_t voltage)
{
    return pmic_set_voltage(PCIE, voltage);
}

int Pshire_PLL_Program(uint8_t mode)
{
    return  configure_pshire_pll(mode);
}

int Pshire_NOC_update_routing_table(void)
{
    /* interface to update routing table */
    return 0;
}

int PCIe_Phy_Firmware_Update (uint64_t* image)
{
    /* interface to initialize PCIe phy */
    (void)image;
    return 0;
}



