/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file maxion_configuration.c
    \brief A C module that implements the maxion shire configuration services.

    Public interfaces:
    Maxion_Shire_Channel_Enable
    Maxion_Set_BootVector
    Maxion_Reset_Cold_Deassert
    Maxion_Reset_Warm_Uncore_Deassert
    Maxion_Reset_PLL_Uncore_Deassert
    Maxion_Reset_PLL_Core_Deassert
    Maxion_Reset_Warm_Core_Deassert
    Maxion_Reset_Warm_Core_All_Deassert
*/
/***********************************************************************/
#include <stdio.h>

#include "hwinc/etsoc_shire_other_esr.h"
#include "esr.h"
//#include "minion_esr_defines.h"
#include "maxion_configuration.h"

void Maxion_Shire_Channel_Enable(void) {
   /* Maxion Shire Channel enable*/

    #define IOSHIRE_ID 0xFE
    #define ALL_NEIGH_MASK 0xF

    #define SUB_REGION_ADDR 0x02

    volatile uint64_t *p = esr_address_new(PP_MACHINE, IOSHIRE_ID, REGION_OTHER, SUB_REGION_ADDR,
                                           ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_ADDRESS,0x0);

    *p =    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_RESET_VALUE |
            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(ALL_NEIGH_MASK) |
            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(IOSHIRE_ID);
}

void Maxion_Set_BootVector(uint64_t address) {

    iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_LO_OFFSET,
                              (uint32_t)(address & 0xFFFFFFFF));
    iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_HI_OFFSET,
                              (uint32_t)((address>>32) & 0xFFFFFFFF));

}

void Maxion_Reset_Cold_Deassert(void) {
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_COLD_OFFSET,
                  RESET_MANAGER_RM_MAX_COLD_RSTN_SET(1));

}

void Maxion_Reset_Warm_Uncore_Deassert(void) {

    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_WARM_OFFSET,
                  RESET_MANAGER_RM_MAX_WARM_UNCORE_RSTN_SET(1));
}

void Maxion_Reset_PLL_Uncore_Deassert(void) {

    uint32_t rwVal = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_OFFSET);
    rwVal |= RESET_MANAGER_RM_MAX_PLL_UNCORE_RSTN_SET(1);

    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_OFFSET, rwVal);
}

void Maxion_Reset_PLL_Core_Deassert(void) {

    uint32_t rwVal = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_OFFSET);
    rwVal |= RESET_MANAGER_RM_MAX_PLL_CORE_RSTN_SET(1);

    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_OFFSET, rwVal);

}

int Maxion_Reset_Warm_Core_Deassert(uint8_t coreNumber) {

    switch (coreNumber)
      {
        case 0:
          iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_WARM_OFFSET,
                    RESET_MANAGER_RM_MAX_WARM_CORE_RSTN_SET(0x1));
          break;
        case 1:
          iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_WARM_OFFSET,
                    RESET_MANAGER_RM_MAX_WARM_CORE_RSTN_SET(0x2));
          break;
        case 2:
          iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_WARM_OFFSET,
                    RESET_MANAGER_RM_MAX_WARM_CORE_RSTN_SET(0x4));
          break;
        case 3:
          iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_WARM_OFFSET,
                    RESET_MANAGER_RM_MAX_WARM_CORE_RSTN_SET(0x8));
          break;
        default:
          return (int)coreNumber;
      }

    return 0;
}

void Maxion_Reset_Warm_Core_All_Deassert(void) {
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MAX_WARM_OFFSET,
                RESET_MANAGER_RM_MAX_WARM_CORE_RSTN_SET(0xF));
}



