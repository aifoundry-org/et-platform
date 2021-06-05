/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file minion_configuration.c
    \brief A C module that implements the minion shire configuration services. 

    Public interfaces:
        Enable_Minion_Neighborhoods
        Enable_Master_Shire_Threads
        Enable_Compute_Minion
        Get_Active_Compute_Minion_Mask
        Load_Autheticate_Minion_Firmware 
*/
/***********************************************************************/
#include <stdio.h>

#include <etsoc_hal/inc/etsoc_shire_other_esr.h>
#include <etsoc_hal/inc/sp_minion_cold_reset_sequence.h>

#include "minion_esr_defines.h"
// #include "dvfs_lvdpll_prog.h"
#include "minion_configuration.h"

/*==================== Function Separator =============================*/

/*! \def TIMEOUT_PLL_CONFIG
    \brief PLL timeout configuration
*/
#define TIMEOUT_PLL_CONFIG 100000

/*! \def TIMEOUT_PLL_LOCK
    \brief lock timeout for PLL
*/
#define TIMEOUT_PLL_LOCK   100000

/*==================== Function Separator =============================*/

// Fixme: SW-8063  Replace with version from HAL
static int pll_config(uint8_t shire_id)
{
    uint64_t reg_value;

    /* Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output */
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xb);

    /* Auto-config register set dll_enable and get reset deasserted of the DLL */
    reg_value = SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL |
                (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);

    /* Wait until DLL is locked to change clock mux */
    while (!(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x20000));
   return 0;
}

static int configure_minion_plls_and_dlls(uint64_t shire_mask)
{
    int status = MINION_PLL_DLL_CONFIG_ERROR; 
    for (uint8_t i = 0; i <= 32; i++) 
    {
        if (shire_mask & 1) {
          status = pll_config(i);
        }
        shire_mask >>= 1;
    }
   return status;
}

int Enable_Minion_Neighborhoods(uint64_t shire_mask)
{
    for (uint8_t i = 0; i <= 32; i++) 
    {
        if (shire_mask & 1) 
        {
            /* Set Shire ID, enable cache and all Neighborhoods */
            const uint64_t config = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(i) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(0xF);
            write_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_OTHER_CONFIG, config);
        }
        shire_mask >>= 1;
    }
    return 0;
}

int Enable_Master_Shire_Threads(uint8_t mm_id)
{
    /* Enable only Device Runtime Management thread on Master Shire */
    write_esr(PP_MACHINE, mm_id, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, ~(MM_RT_THREADS));
    return 0;
}

int Enable_Compute_Minion(uint64_t minion_shires_mask)
{
    /* Configure Minon Step clock to 650 Mhz */
    if (0 != configure_sp_pll_4(3)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_4() failed!\n");
        return MINION_STEP_CLOCK_CONFIGURE_ERROR;
    }

    if (0 != release_minions_from_cold_reset()) {
        Log_Write(LOG_LEVEL_ERROR, "release_minions_from_cold_reset() failed!\n");
        return MINION_COLD_RESET_CONFIG_ERROR;
    }

    if (0 != release_minions_from_warm_reset()) {
        Log_Write(LOG_LEVEL_ERROR, "release_minions_from_warm_reset() failed!\n");
        return MINION_WARM_RESET_CONFIG_ERROR ;
    }

    if (0 != configure_minion_plls_and_dlls(minion_shires_mask)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_minion_plls_and_dlls() failed!\n");
        return MINION_PLL_DLL_CONFIG_ERROR;
    }

   return SUCCESS;
}

uint64_t Get_Active_Compute_Minion_Mask(void)
{
    int ret;
    OTP_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER_t status_other;
    uint64_t enable_mask = 0;

    // 32 Worker Shires: There are 4 OTP entries containing the status of their Neighboorhods
    for (uint32_t entry = 0; entry < 4; entry++) {
        uint32_t status;

        ret = sp_otp_get_neighborhood_status_mask(entry, &status);
        if (ret < 0) {
            // If the OTP read fails, assume we have to enable all Neighboorhods
            status = 0xFFFFFFFF;
        }

        // Each Neighboorhod status OTP entry contains information for 8 Shires
        for (uint32_t i = 0; i < 8; i++) {
            // Only enable a Shire if *ALL* its Neighboorhods are Functional
            if ((status & 0xF) == 0xF) {
                enable_mask |= 1ULL << (entry * 8 + i);
            }
            status >>= 4;
        }
    }

    // Master Shire Neighboorhods status
    ret = sp_otp_get_neighborhood_status_nh128_nh135_other(&status_other);
    if ((ret < 0) || ((status_other.B.neighborhood_status & 0xF) == 0xF)) {
        enable_mask |= 1ULL << 32;
    }

    return enable_mask;
}

int Load_Autheticate_Minion_Firmware(void)
{
    if (0 != load_sw_certificates_chain()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load SW ROOT/Issuing Certificate chain!\n");
        return FW_SW_CERTS_LOAD_ERROR;
    }

    // In fast-boot mode we skip loading from flash, and assume everything is already pre-loaded
#if !FAST_BOOT
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MACHINE_MINION)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Machine Minion firmware!\n");
        return FW_MACH_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "MACH FW loaded.\n");

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MASTER_MINION)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Master Minion firmware!\n");
        return FW_MM_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "MM FW loaded.\n");

    // TODO: Update the following to Log macro - set to INFO/DEBUG
    //Log_Write(LOG_LEVEL_ERROR, "Attempting to load Worker Minion firmware...\n");
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_WORKER_MINION)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Worker Minion firmware!\n");
        return FW_CM_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "WM FW loaded.\n");
#endif

   return SUCCESS;
}
