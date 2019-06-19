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

#include "hal_device.h"
#include "pcie_device.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

static void PCIe_initPShire(void);
static void PCIe_initBars(void);
static void PCIe_initLink(void);

void PCIe_init(void)
{  
    printf("Initializing PCIe\r\n");

    PCIe_initPShire();
    PCIe_initBars();
    PCIe_initLink(); 

    printf("PCIe link up at Gen %d\r\n", PCIE_CUST_SS->PE0_LINK_DBG_2.B.RATE + 1);
}

static void PCIe_initPShire(void)
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

//BAR Sizes must be powers of 2 per the PCIe spec. Round up to next biggest power of 2.
#define BAR0_SIZE 0x00000007FFFFFFFFULL /*32gb*/
#define BAR2_SIZE 0x000000000000FFFFULL /*64kb*/

#define BAR_IN_MEM_SPACE 0
#define BAR_TYPE_64BIT 2
#define BAR_ENABLE 1

static void PCIe_initBars(void)
{
    //The BAR config registers are protected by a write-enable bit
    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.B.DBI_RO_WR_EN = 1;

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

    PCIE0->PF0_PORT_LOGIC.MISC_CONTROL_1_OFF.B.DBI_RO_WR_EN = 0;
}

#define FB_MODE_FIGURE_OF_MERIT 1
#define RATE_SHADOW_SEL_GEN4 1
#define SMLH_LTSSM_STATE_LINK_UP 0x11

static void PCIe_initLink(void)
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
