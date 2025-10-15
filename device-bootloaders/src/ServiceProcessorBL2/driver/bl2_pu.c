/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file bl2_pu.c
    \brief A C module that implements functions to interface with PU
    subsystem
*/
/***********************************************************************/
#include "bl2_pu.h"
#include "hwinc/hal_device.h"

int PU_Uart_Initialize(uint8_t uart_id)
{
    /* Peripheral unit initialization interface */

    (void)uart_id;
    return 0;
}

uint32_t PU_SRAM_Read(uint32_t *address)
{
    /* check if address range is valid */
    if ((((uintptr_t)address > (uintptr_t)R_PU_SRAM_LO_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_PU_SRAM_LO_BASEADDR + R_PU_SRAM_LO_SIZE))) ||
        (((uintptr_t)address > (uintptr_t)R_PU_SRAM_HI_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_PU_SRAM_HI_BASEADDR + R_PU_SRAM_HI_SIZE))) ||
        (((uintptr_t)address > (uintptr_t)R_PU_SRAM_MID_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_PU_SRAM_MID_BASEADDR + R_PU_SRAM_MID_SIZE))))
    {
        return *address;
    }

    return 0;
}

int PU_SRAM_Write(uint32_t *address, uint32_t data)
{
    if ((((uintptr_t)address > (uintptr_t)R_PU_SRAM_LO_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_PU_SRAM_LO_BASEADDR + R_PU_SRAM_LO_SIZE))) ||
        (((uintptr_t)address > (uintptr_t)R_PU_SRAM_HI_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_PU_SRAM_HI_BASEADDR + R_PU_SRAM_HI_SIZE))) ||
        (((uintptr_t)address > (uintptr_t)R_PU_SRAM_MID_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_PU_SRAM_MID_BASEADDR + R_PU_SRAM_MID_SIZE))))
    {
        *address = data;
    }

    return 0;
}
