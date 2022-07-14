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
/*! \file pmic_controller.c
    \brief A C module that implements the PMIC controller's functionality. It
    provides functions to set/get voltage of different components and set/clear
    gpio bits. It also configures different error thresholds and report error
    events to host.

    Public interfaces:
        release_memshire_from_reset
        release_minions_from_cold_reset
        release_minions_from_warm_reset
        release_etsoc_reset
        pcie_reset_flr
        pcie_reset_warm

*/
/***********************************************************************/
#include <stdio.h>
#include "log.h"
#include "etsoc/isa/io.h"
#include "bl2_reset.h"

#include "hwinc/sp_cru.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/hal_device.h"
#include "delays.h"

int release_memshire_from_reset(void)
{
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MEMSHIRE_COLD_ADDRESS,
              RESET_MANAGER_RM_MEMSHIRE_COLD_RSTN_SET(0x01));
#if !(FAST_BOOT || TEST_FRAMEWORK)
    msdelay(200);
#endif

    /* Serially bring up each MemShire */
    for (int memshire = 0; memshire < 8; ++memshire)
    {
        Log_Write(LOG_LEVEL_INFO, "CRU:[txt]RESET_MANAGER_RM_MEMSHIRE_WARM_ADDRESS, memshire[%d]\n",
                  memshire);
        iormw32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MEMSHIRE_WARM_ADDRESS,
                (uint32_t)0x1 << memshire);
#if (FAST_BOOT || TEST_FRAMEWORK)
        usdelay(3);
#else
        msdelay(200);
#endif
    }

    return 0;
}

int release_minions_from_cold_reset(void)
{
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MINION_ADDRESS,
              RESET_MANAGER_RM_MINION_COLD_RSTN_SET(1) | RESET_MANAGER_RM_MINION_WARM_RSTN_SET(1));
    return 0;
}

int release_minions_from_warm_reset(void)
{
    /* Serially bring up each Minion Shire */
    for (int minion = 0; minion < 32; ++minion)
    {
        Log_Write(LOG_LEVEL_INFO, "CRU:[txt]RESET_MANAGER_RM_MINION_WARM_A_ADDRESS, minion[%d]\n",
                  minion);
        iormw32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MINION_WARM_A_ADDRESS,
                (uint32_t)(0x1 << minion));
#if (FAST_BOOT || TEST_FRAMEWORK)
        usdelay(3);
#else
        msdelay(200);
#endif
    }
    for (int minion = 0; minion < 2; ++minion)
    {
        Log_Write(LOG_LEVEL_INFO, "CRU:[txt]RESET_MANAGER_RM_MINION_WARM_B_ADDRESS, minion[%d]\n",
                  32 + minion);
        iormw32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MINION_WARM_B_ADDRESS,
                (uint32_t)(0x1 << minion));
#if (FAST_BOOT || TEST_FRAMEWORK)
        usdelay(3);
#else
        msdelay(200);
#endif
    }

    Log_Write(LOG_LEVEL_INFO, "CRU:[txt]Release minions from reset completed\n");
    return 0;
}

void release_etsoc_reset(void)
{
    Log_Write(LOG_LEVEL_INFO, "Reseting ETSOC\n");
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_SYS_RESET_CTRL_ADDRESS,
              RESET_MANAGER_RM_SYS_RESET_CTRL_ENABLE_SET(0x1));
}

void pcie_reset_flr(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6606 */
}

void pcie_reset_warm(void)
{
    /* TODO: https://esperantotech.atlassian.net/browse/SW-6606 */
}

uint8_t get_hpdpll_strap_value(void)
{
    uint32_t rm_status2;
    rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
    return (uint8_t)(RESET_MANAGER_RM_STATUS2_STRAP_IN_GET(rm_status2) & 0x3u);
}

uint8_t get_lvdpll_strap_value(void)
{
    uint32_t rm_status2;
    rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
    return (uint8_t)((RESET_MANAGER_RM_STATUS2_STRAP_IN_GET(rm_status2) >> 2) & 0x3u);
}
