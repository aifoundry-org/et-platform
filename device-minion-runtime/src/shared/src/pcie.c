/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

//Note: The DV version that some of this code was ported from is at
//esperanto-soc/dv/tests/ioshire/sw/inc/pshire_common.h

#include "pcie.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "etsoc_hal/rm_esr.h"
#include "hal_device.h"
#include "pcie_device.h"

static void pcie_init_pshire(void);
static void pcie_init_caps_list(void);
static void pcie_init_bars(void);
static void pcie_init_ints(void);
static void pcie_init_link(void);
static void pcie_init_atus(void);
static void pcie_wait_for_ints(void);

#define SMLH_LTSSM_STATE_LINK_UP 0x11

static int release_pshire_from_reset(void) {
    volatile Reset_Manager_t * reset_manager = (Reset_Manager_t*)R_SP_CRU_BASEADDR;
    reset_manager->rm_pshire_cold.R = (Reset_Manager_rm_pshire_cold_t){ .B = { .rstn = 1 }}.R;
    reset_manager->rm_pshire_warm.R = (Reset_Manager_rm_pshire_warm_t){ .B = { .rstn = 1 }}.R;
    return 0;
}

void PCIe_init(bool expect_link_up)
{  
    printf("Initializing PCIe\r\n");

    //If the PCIe link should already be up (e.x. ServiceProcessorROM should have ran
    //pcie_boot_config()), and it's not, try again anyways; however, the init timing may be
    //slow enough that the device will not be enumerated by the host properly.
    bool init_link = true;

    if (expect_link_up) {
        if (PCIE_CUST_SS->PE0_LINK_DBG_2.B.SMLH_LTSSM_STATE == SMLH_LTSSM_STATE_LINK_UP) {
            init_link = false;
        }
        else {
            printf("Warning: PCIe link not properly inited, trying again...\r\n");
        }
    }
    else {
        //Normally ServiceProcessorROM releases reset, however this codepath skips that
        release_pshire_from_reset();
    }

    if (init_link) {
        pcie_init_pshire();
        pcie_init_caps_list();
        pcie_init_bars();
        pcie_init_ints();
        pcie_init_link();
    }

    //These steps don't block the PCIe IP from responding to config space messages,
    //so their timing does not contribute to the PERST_n requirements. Always do 
    //them as late as possible (i.e. not in pcie_boot_config()) to simplfy things.
    pcie_init_atus();
    pcie_wait_for_ints();

    printf("PCIe link up at Gen %d\r\n", PCIE_CUST_SS->PE0_LINK_DBG_2.B.RATE + 1);
}

static void pcie_init_pshire(void)
{
    //TODO: Reset manager config for pshire

    //TODO FIXME JIRA SW-327: Use PShire PLL
    //For now, for emulation, we are bypassing the PShire PLL, and not configuring it at all
    //or waiting for it to lock.
    PCIE_ESR->pshire_ctrl.B.pll0_byp = 1;

    //TODO: ABP Reset deassert? In "PCIe Initialization Sequence" doc, TBD what state to check

    //Wait for PERST_N
    //TODO FIXME JIRA SW-330: Don't monopolize the HART to poll
    printf("Waiting for PCIe bus out of reset...");
    while (PCIE_ESR->pshire_stat.B.perst_n == 0);
    printf(" done\r\n");

    // Deassert PCIe cold reset
    PCIE_ESR->pshire_reset.B.pwr_up_rstn = 1;
    
    // Wait for CDM (controller, dual-mode; all of the "PCIE0" regs) to be ready
    while (PCIE_CUST_SS->PE0_LINK_DBG_2.B.CDM_IN_RESET != 0);
}

static void pcie_init_caps_list(void)
{
    //Init capabilities list. The compiled-in, default capabilities list looks like this:
    //HEAD (0x34) -> Power Mgmt (0x40) -> MSI (0x50) -> PCIe (0x70) -> MSI-X (0xB0) -> NULL

    //For now, the Linux host driver is not implementing the power management API,
    //so don't advertise the power management capability.

    //The config registers are protected by a write-enable bit
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_t miscControl1;
    miscControl1.R = PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R;
    miscControl1.B.DBI_RO_WR_EN = 1;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;

    //Make HEAD point to MSI cap, skipping Power Mgmt Cap
    PCIE0->PF0_TYPE0_HDR.PCI_CAP_PTR_REG.R = (PE0_DWC_pcie_ctl_DBI_Slave_PF0_TYPE0_HDR_PCI_CAP_PTR_REG_t){ .B = {
        .CAP_POINTER = 0x50
    }}.R;

    miscControl1.B.DBI_RO_WR_EN = 0;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;
}

//BAR Sizes must be powers of 2 per the PCIe spec. Round up to next biggest power of 2.
#define BAR0_SIZE 0x00000007FFFFFFFFULL /*32gb*/
#define BAR2_SIZE 0x000000000000FFFFULL /*64kb*/

#define BAR_IN_MEM_SPACE 0
#define BAR_TYPE_64BIT 2
#define BAR_ENABLE 1

static void pcie_init_bars(void)
{
    //The BAR config registers are protected by a write-enable bit
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_t miscControl1;
    miscControl1.R = PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R;
    miscControl1.B.DBI_RO_WR_EN = 1;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;

    //Write the BAR mask registers first. A BAR must be enabled for the other configuration
    //writes to take effect.

    //When BARs are used in 64-bit mode, BAR1 becomes the high 32-bits of BAR0, and so on
    //(BAR3 for BAR2, BAR5 for BAR4)
    PCIE0->PF0_TYPE0_HDR_DBI2.BAR0_MASK_REG.R = (uint32_t)(BAR0_SIZE & 0xFFFFFFFFULL) | BAR_ENABLE;
    PCIE0->PF0_TYPE0_HDR_DBI2.BAR1_MASK_REG.R = (uint32_t)((BAR0_SIZE >> 32) & 0xFFFFFFFFULL);

    PCIE0->PF0_TYPE0_HDR_DBI2.BAR2_MASK_REG.R = (uint32_t)(BAR2_SIZE & 0xFFFFFFFFUL) | BAR_ENABLE;
    PCIE0->PF0_TYPE0_HDR_DBI2.BAR3_MASK_REG.R = (uint32_t)((BAR2_SIZE >> 32) & 0xFFFFFFFFULL);

    PCIE0->PF0_TYPE0_HDR_DBI2.BAR4_MASK_REG.B.PCI_TYPE0_BAR4_ENABLED = 0;
    PCIE0->PF0_TYPE0_HDR_DBI2.BAR5_MASK_REG.B.PCI_TYPE0_BAR5_ENABLED = 0;

    PCIE0->PF0_TYPE0_HDR_DBI2.EXP_ROM_BAR_MASK_REG.B.ROM_BAR_ENABLED = 0;

    //BAR0 (maps DRAM)
    PCIE0->PF0_TYPE0_HDR.BAR0_REG = (PE0_DWC_pcie_ctl_DBI_Slave_PF0_TYPE0_HDR_BAR0_REG_t){ .B = {
        .BAR0_MEM_IO = BAR_IN_MEM_SPACE,
        .BAR0_TYPE = BAR_TYPE_64BIT,
        .BAR0_PREFETCH = 1 //IMPORTANT: Many hosts do not tolerate > 1 gb BARs that are not prefetchable
    }};

    //BAR2 (maps registers)
    PCIE0->PF0_TYPE0_HDR.BAR2_REG = (PE0_DWC_pcie_ctl_DBI_Slave_PF0_TYPE0_HDR_BAR2_REG_t){ .B = {
        .BAR2_MEM_IO = BAR_IN_MEM_SPACE,
        .BAR2_TYPE = BAR_TYPE_64BIT,
        .BAR2_PREFETCH = 0 //IMPORTANT - not prefetchable (registers with read side effects mapped here)
    }};

    //Wait to init iATUs until BAR addresses are assigned - see PCIe_initATUs.

    miscControl1.B.DBI_RO_WR_EN = 0;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;
}

#define MSI_TWO_VECTORS 1

static void pcie_init_ints(void)
{
    //The config registers are protected by a write-enable bit
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_t miscControl1;
    miscControl1.R = PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R;
    miscControl1.B.DBI_RO_WR_EN = 1;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;

    //Configure MSI

    //Request 2 interrupt vectors (1 per pcie mailbox)
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_t msi_ctrl;
    msi_ctrl.R = PCIE0->PF0_MSI_CAP.PCI_MSI_CAP_ID_NEXT_CTRL_REG.R;
    msi_ctrl.B.PCI_MSI_MULTIPLE_MSG_CAP = MSI_TWO_VECTORS;
    PCIE0->PF0_MSI_CAP.PCI_MSI_CAP_ID_NEXT_CTRL_REG.R = msi_ctrl.R;

    //Configure MSI-X
    //TODO

    miscControl1.B.DBI_RO_WR_EN = 0;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;
}

#define FB_MODE_FIGURE_OF_MERIT 1
#define RATE_SHADOW_SEL_GEN4 1
#define SMLH_LTSSM_STATE_LINK_UP 0x11

static void pcie_init_link(void)
{
    // Setup FSM tracker per Synopsis testbench
    PCIE_CUST_SS->PE0_FSM_TRACK_1.R = 0xCC; //TODO in "PCIe Initialization Sequence" doc, this comes before polling CDM_IN_REST. Matching order from DV code.

    // Enable Fast Link Mode - Trains link faster for simulation/emulation
    // Note: The PCIe transactor (Phy replacement) in ZeBu requires this to be set.
    PCIE0->PF0_PORT_LOGIC.PORT_LINK_CTRL_OFF.B.FAST_LINK_MODE = 1; // TODO: should this be disabled in production?

    // When FAST_LINK_MODE is on, scale LTSSM timer x1024
    PCIE0->PF0_PORT_LOGIC.TIMER_CTRL_MAX_FUNC_NUM_OFF.B.FAST_LINK_SCALING_FACTOR = 0;

    // Configure PCIe LTSSM phase 2 equalization behavior.
    // All values below provided by James Yu 2019-06-19

    // First, configure PCIe Gen 3.0 LTSSM behavior
    PE0_DWC_pcie_ctl_AXI_Slave_PF0_PORT_LOGIC_GEN3_EQ_CONTROL_OFF_t gen3EqControl;
    gen3EqControl.R = PCIE0->PF0_PORT_LOGIC.GEN3_EQ_CONTROL_OFF.R;

    gen3EqControl.B.GEN3_EQ_FB_MODE = FB_MODE_FIGURE_OF_MERIT;
    gen3EqControl.B.GEN3_EQ_PHASE23_EXIT_MODE = 1;
    gen3EqControl.B.GEN3_EQ_EVAL_2MS_DISABLE = 1;
    gen3EqControl.B.GEN3_EQ_FOM_INC_INITIAL_EVAL = 0;
    gen3EqControl.B.GEN3_EQ_PSET_REQ_VEC = 4; //preset 2, arbitrarily chosen

    PCIE0->PF0_PORT_LOGIC.GEN3_EQ_CONTROL_OFF.R = gen3EqControl.R; //write Gen 3.0 config

    // Then, configure PCIe Gen 4.0 LTSSM behavior

    // By setting RATE_SHADOW_SEL; writes to GEN3_EQ_CONTROL_OFF actually configure Gen 4.0
    PCIE0->PF0_PORT_LOGIC.GEN3_RELATED_OFF.B.RATE_SHADOW_SEL = RATE_SHADOW_SEL_GEN4;

    gen3EqControl.B.GEN3_EQ_PSET_REQ_VEC = 8; //preset 3, arbitrarily chosen
    // all other gen3EqControl values the same as Gen 3.0 configuration

    PCIE0->PF0_PORT_LOGIC.GEN3_EQ_CONTROL_OFF.R = gen3EqControl.R; //write Gen 4.0 config

    // Enable LTSSM
    PCIE_CUST_SS->PE0_GEN_CTRL_3.B.LTSSM_EN = 1;

    // Wait for link training to finish
    // TODO FIXME JIRA SW-330: Don't monopolize the HART to poll, add a 100ms timeout
    printf("Link training...");
    while (PCIE_CUST_SS->PE0_LINK_DBG_2.B.SMLH_LTSSM_STATE != SMLH_LTSSM_STATE_LINK_UP);
    printf(" done\r\n");
}

//See DWC_pcie_ctl_dm_databook section 3.10.11
//Since the regmap codegen does not make an array of iATU registers, parameterize with macros
#define CONFIG_INBOUND_IATU(num) static void config_inbound_iatu_##num(uint64_t baseAddr, uint64_t targetAddr, uint64_t size) { \
    uint64_t limitAddr = baseAddr + size - 1; \
\
    PCIE0->PF0_ATU_CAP.IATU_REGION_CTRL_1_OFF_INBOUND_##num.R = \
        (PE0_DWC_pcie_ctl_DBI_Slave_PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_ ## num ## _t){ .B = { \
            .TYPE = 0, /*Watch to TLPs with TYPE field 0 (mem space)*/ \
            .INCREASE_REGION_SIZE = 1 /*Use the UPPR_LIMIT_ADDR reg*/ \
    }}.R; \
\
    PCIE0->PF0_ATU_CAP.IATU_LWR_BASE_ADDR_OFF_INBOUND_##num.R = (uint32_t)baseAddr; \
    PCIE0->PF0_ATU_CAP.IATU_UPPER_BASE_ADDR_OFF_INBOUND_##num.R = (uint32_t)(baseAddr >> 32); \
\
    PCIE0->PF0_ATU_CAP.IATU_LIMIT_ADDR_OFF_INBOUND_##num.R = (uint32_t)limitAddr; \
    PCIE0->PF0_ATU_CAP.IATU_UPPR_LIMIT_ADDR_OFF_INBOUND_##num.R = (uint32_t)(limitAddr >> 32); \
\
    PCIE0->PF0_ATU_CAP.IATU_LWR_TARGET_ADDR_OFF_INBOUND_##num.R = (uint32_t)targetAddr; \
    PCIE0->PF0_ATU_CAP.IATU_UPPER_TARGET_ADDR_OFF_INBOUND_##num.R = (uint32_t)(targetAddr >> 32); \
\
    PCIE0->PF0_ATU_CAP.IATU_REGION_CTRL_2_OFF_INBOUND_##num.R = \
        (PE0_DWC_pcie_ctl_DBI_Slave_PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_## num ## _t){ .B = { \
            .MATCH_MODE = 0, /*Address match mode. Do NOT use BAR match mode.*/ \
            .REGION_EN = 1 \
    }}.R; \
}

CONFIG_INBOUND_IATU(0)
CONFIG_INBOUND_IATU(1)
CONFIG_INBOUND_IATU(2)
CONFIG_INBOUND_IATU(3)
CONFIG_INBOUND_IATU(4)

static void pcie_init_atus(void)
{
    //The config registers are protected by a write-enable bit
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_t miscControl1;
    miscControl1.R = PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R;
    miscControl1.B.DBI_RO_WR_EN = 1;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;

    //The iATU has a "BAR Match Mode" feature where it can track BARs, but that mode does not allow
    //any offset from the BAR, so we can't map multiple iATUs into one BAR. So, instead of using BAR
    //match mode, wait until the BARs are programmed by the host, and program the iATUs manually.

    //Note the initial BAR values assigned by the BIOS may be reassigned by the OS, so just waiting
    //for the BARs to be assigned is not sufficient. However, the OS, not the BIOS, enables PCI
    //memory space. Once the memory space is enabled, the BAR values cannot change anymore. So, it's
    //critical to wait to program the iATUs until after memory space is enabled.

    //TODO FIXME JIRA SW-330: Don't monopolize the HART to poll
    //This wait could be long (tens of seconds), depending on when the OS enables PCIe
    printf("Waiting for host to enable memory space...");
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_TYPE0_HDR_STATUS_COMMAND_REG_t status_command_reg;
    do {
        status_command_reg.R = PCIE0->PF0_TYPE0_HDR.STATUS_COMMAND_REG.R;
    }
    while (status_command_reg.B.PCI_TYPE0_MEM_SPACE_EN == 0);
    printf(" done\r\n");

    //TODO: I need to ensure the host does not try and send Mem Rd / Mem Wr before the iATUs
    //are configured. The latency of a PCIe transaction (1-10s of uS) is probably long enough
    //that the iATUs will always be programmed between the PCIe config TLP to enable mem 
    //space and the first PCIe MRd/MWr. However, we should make sure. Send the host an interrupt 
    //(once interrupts are implemented), make the  kernel driver block on receiving the first
    //interrupt before doing MRd/MWr to these regions.

    //Setup BAR0
    //Name        Host Addr       SoC Addr      Size   Notes
    //R_L3_DRAM   BAR0 + 0x0000   0x8100000000  28G    DRAM with PCIe access permissions
    
    uint64_t bar0 = 
        ((uint64_t)PCIE0->PF0_TYPE0_HDR.BAR1_REG.R << 32) |
        ((uint64_t)PCIE0->PF0_TYPE0_HDR.BAR0_REG.R & 0xFFFFFFF0ULL);

    config_inbound_iatu_0(
        bar0,               //baseAddr
        R_L3_DRAM_BASEADDR, //targetAddr
        R_L3_DRAM_SIZE);    //size

    //Setup BAR2
    //Name              Host Addr       SoC Addr      Size   Notes
    //R_PU_MBOX_PC_MM   BAR2 + 0x0000   0x0020007000  4k     Mailbox shared memory
    //R_PU_MBOX_PC_SP   BAR2 + 0x1000   0x0030003000  4k     Mailbox shared memory
    //R_PU_TRG_PCIE     BAR2 + 0x2000   0x0030008000  8k     Mailbox interrupts
    //R_PCIE_USRESR     BAR2 + 0x4000   0x7f80000000  4k     DMA control registers

    //start on BAR2
    uint64_t baseAddr = 
        ((uint64_t)PCIE0->PF0_TYPE0_HDR.BAR3_REG.R << 32) |
        ((uint64_t)PCIE0->PF0_TYPE0_HDR.BAR2_REG.R & 0xFFFFFFF0ULL); 

    config_inbound_iatu_1(
        baseAddr,
        R_PU_MBOX_PC_MM_BASEADDR, //targetAddr
        R_PU_MBOX_PC_MM_SIZE);    //size

    baseAddr += R_PU_MBOX_PC_MM_SIZE;

    config_inbound_iatu_2(
        baseAddr, 
        R_PU_MBOX_PC_SP_BASEADDR, //targetAddr
        R_PU_MBOX_PC_SP_SIZE);    //size

    baseAddr += R_PU_MBOX_PC_SP_SIZE;

    config_inbound_iatu_3(
        baseAddr,
        R_PU_TRG_PCIE_BASEADDR, //targetAddr
        R_PU_TRG_PCIE_SIZE);    //size

    baseAddr += R_PU_TRG_PCIE_SIZE;

    config_inbound_iatu_4(
        baseAddr,
        R_PCIE_USRESR_BASEADDR, //targetAddr
        R_PCIE_USRESR_SIZE);    //size

    miscControl1.B.DBI_RO_WR_EN = 0;
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.R = miscControl1.R;
}

static void pcie_wait_for_ints(void)
{
    // Wait until the x86 host driver comes up and enables MSI
    // See pci_alloc_irq_vectors on the host driver.
    // TODO FIXME JIRA SW-330: Don't monopolize the HART to poll
    // TODO Intentionally not supporting legacy ints at this time. Future feature?

    PE0_DWC_pcie_ctl_DBI_Slave_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_t msi_ctrl;

    printf("Waiting for host to enable MSI/MSI-X...");
    do
    {
        msi_ctrl.R = PCIE0->PF0_MSI_CAP.PCI_MSI_CAP_ID_NEXT_CTRL_REG.R;
        //TODO: MSI-X
    }
    while(msi_ctrl.B.PCI_MSI_ENABLE == 0);
    printf(" done\r\n");
}
