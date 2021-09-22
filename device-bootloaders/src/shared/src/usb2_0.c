/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

// Note: This code was ported from DV located at: esperanto-soc/dv/common/sw/ip/src/usb_phy.c

#include <stdint.h>
#include <stdio.h>

#include "hwinc/hal_device.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_misc.h"
#include "hwinc/sp_u0esr.h"
#include "etsoc/isa/mem-access/io.h"
#include "log.h"
#include "usb2_0.h"

static void usb2_0_phy_reset(bool release)
{
    uint32_t tmp;

    tmp = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_USB2_0_ADDRESS);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_ESR_RSTN_MODIFY(tmp, release);
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_USB2_0_ADDRESS, tmp);
}

static void usb2_0_por_reset(bool release)
{
    uint32_t tmp;

    tmp = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_USB2_0_ADDRESS);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_PHY_PO_RSTN_MODIFY(tmp, release);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_PHY_PORT0_RSTN_MODIFY(tmp, release);
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_USB2_0_ADDRESS, tmp);
}

static void usb2_0_controller_reset(bool release)
{
    uint32_t tmp;

    tmp = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_USB2_0_ADDRESS);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_RELOC_RSTN_MODIFY(tmp, release);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_AHB2AXI_RSTN_MODIFY(tmp, release);
    // tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_CTRL_PHY_RSTN_MODIFY(tmp, release);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_DBG_RSTN_MODIFY(tmp, release);
    tmp = (uint32_t)RESET_MANAGER_RM_USB2_0_CTRL_AHB_RSTN_MODIFY(tmp, release);
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_USB2_0_ADDRESS, tmp);
}

static bool usb_phy_clk_frequency_cfg(uint32_t refclk_sel, uint32_t fsel, uint32_t pll_btune)
{
    // Cfg ref clock with allowed values
    if ((((fsel < 0x3 || fsel == 0x7) && pll_btune == 0x1) || (fsel == 0x7 && pll_btune == 0x0)) &&
        refclk_sel == 0x2) {
        iowrite32(R_SP_U0ESR_BASEADDR + USB2_PHY_CONFIG_ESR_CONFIG_REFCLKSEL_ADDRESS, refclk_sel);
        iowrite32(R_SP_U0ESR_BASEADDR + USB2_PHY_CONFIG_ESR_CONFIG_PLLBTUNE_ADDRESS, pll_btune);
        iowrite32(R_SP_U0ESR_BASEADDR + USB2_PHY_CONFIG_ESR_CONFIG_FSEL_ADDRESS, fsel);
        asm volatile("fence" ::: "memory");
        return true;
    } else {
        return false;
    }
}

static void set_ust_ip_pin(bool ust_en)
{
    iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_PU_USB20_UST_EN_ADDRESS, ust_en);
}

void usb2_0_init(bool enable_ultrasoc)
{
    // Enable UltraSoc USB controller if needed
    set_ust_ip_pin(enable_ultrasoc);

    Log_Write(LOG_LEVEL_INFO,"USB2 1: Release PHY reset...\n");
    usb2_0_phy_reset(1);

    Log_Write(LOG_LEVEL_INFO,"USB2 1: Cfg clocks...\n");
    usb_phy_clk_frequency_cfg(0x02, 0x02, 0x01);

    Log_Write(LOG_LEVEL_INFO,"USB2 1: Release POR reset...\n");
    usb2_0_por_reset(1);

    // TODO: Wait UTMI and PHY clk at least 50us (at least 45us according the spec)
    // usleep(50);

    Log_Write(LOG_LEVEL_INFO,"USB2 1: Release controller resets...\n");
    usb2_0_controller_reset(1);

    // ctrl_cfg -> GOTGCTL = ctrl_cfg -> GUSBCFG & (~(0x01 << 6));
}
