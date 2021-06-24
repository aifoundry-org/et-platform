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
#include "spio_sram.h"
#include "hwinc/hal_device.h"

uint32_t SPIO_SRAM_Read_Word (uint64_t *address)
{
    /* check if address range is valid */
    if (((uintptr_t)address > (uintptr_t)R_SP_SRAM_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_SP_SRAM_BASEADDR + R_SP_SRAM_SIZE)))
    {

        return (uint32_t)*address;
    }

    return 0;

}

int SPIO_SRAM_Write_Word (uint64_t *address, uint32_t data)
{
    /* check if address range is valid */
    if (((uintptr_t)address > (uintptr_t)R_SP_SRAM_BASEADDR) &&
        ((uintptr_t)address < (uintptr_t)(R_SP_SRAM_BASEADDR + R_SP_SRAM_SIZE)))
    {

        *address = data;
    }

    return 0;
}
