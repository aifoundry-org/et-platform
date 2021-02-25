#include <stdio.h>

#include "io.h"
#include "bl2_reset.h"

#include "etsoc_hal/inc/cm_esr.h"
#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/hal_device.h"

int release_memshire_from_reset(void)
{
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MEMSHIRE_COLD_ADDRESS,
              RESET_MANAGER_RM_MEMSHIRE_COLD_RSTN_SET(0x01));
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MEMSHIRE_WARM_ADDRESS,
              RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN_SET(0xFF));
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
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MINION_WARM_A_ADDRESS,
              RESET_MANAGER_RM_MINION_WARM_A_RSTN_SET(0xFFFFFFFF));
    iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_MINION_WARM_B_ADDRESS,
              RESET_MANAGER_RM_MINION_WARM_B_RSTN_SET(0x3));
    return 0;
}

void release_etsoc_reset(void)
{
   printf("Reseting ETSOC\n");
   iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_SYS_RESET_CTRL_ADDRESS,
             RESET_MANAGER_RM_SYS_RESET_CTRL_ENABLE_SET(0x1));
}
