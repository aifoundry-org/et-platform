/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file mm_config.c
    \brief A C module that implements the MM configuration APIs

    Public interfaces:
        MM_Config_Init
        MM_Config_Get_DDR_Size
        MM_Config_Get_DRAM_End_Address
*/
/***********************************************************************/
#include <inttypes.h>
#include <stdio.h>

/* mm_rt_svcs */
#include <system/layout.h>

/* mm specific headers */
#include "services/log.h"
#include "services/sp_iface.h"
#include "config/mm_config.h"
#include "error_codes.h"

typedef struct mm_config_ {
    uint64_t ddr_size;
    uint64_t host_managed_dram_size;
    uint64_t host_managed_dram_end;
    // More config values in future
} mm_config_t;

static mm_config_t MM_Config_CB __attribute__((aligned(64))) = { 0 };

/************************************************************************
*
*   FUNCTION
*
*       MM_Config_Init
*
*   DESCRIPTION
*
*       This function retrieves DDR size from SP and saves it to a global to be used further.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t     Status success or error
*
***********************************************************************/
int32_t MM_Config_Init(void)
{
    uint64_t ddr_mem_size;
    int32_t status = DMA_DRIVER_ERROR_INVALID_ADDRESS;

    status = SP_Iface_Get_DDR_Memory_Info(&ddr_mem_size);
    if (status == STATUS_SUCCESS)
    {
        /* Validate DDR size limit */
        if (ddr_mem_size <= HOST_MANAGED_DRAM_SIZE_MAX)
        {
            /* Save DDR size and DRAM end based on size in CB */
            atomic_store_local_64(&MM_Config_CB.ddr_size, ddr_mem_size);
            atomic_store_local_64(&MM_Config_CB.host_managed_dram_size,
                (ddr_mem_size - (KERNEL_UMODE_ENTRY - LOW_MCODE_SUBREGION_BASE)));
            atomic_store_local_64(&MM_Config_CB.host_managed_dram_end,
                (HOST_MANAGED_DRAM_START +
                    (ddr_mem_size - (KERNEL_UMODE_ENTRY - LOW_MCODE_SUBREGION_BASE))));
        }
        else
        {
            status = MM_CONFIG_INVALID_DDR_SIZE;
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR:MM_Config_Init: unable to get ddr memory info\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Config_Get_DDR_Size
*
*   DESCRIPTION
*
*       This function returns DDR memory size.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t     DDR memory size
*
***********************************************************************/
uint64_t MM_Config_Get_DDR_Size(void)
{
    return atomic_load_local_64(&MM_Config_CB.ddr_size);
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Config_Get_Host_Managed_DRAM_Size
*
*   DESCRIPTION
*
*       This function returns host managed DRAM size.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t     host managed DRAM size
*
***********************************************************************/
uint64_t MM_Config_Get_Host_Managed_DRAM_Size(void)
{
    return atomic_load_local_64(&MM_Config_CB.host_managed_dram_size);
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Config_Get_DRAM_End_Address
*
*   DESCRIPTION
*
*       This function returns host managed DRAM end address.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t     host managed DRAM end address
*
***********************************************************************/
uint64_t MM_Config_Get_DRAM_End_Address(void)
{
    return atomic_load_local_64(&MM_Config_CB.host_managed_dram_end);
}