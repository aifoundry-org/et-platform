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
#include <etsoc/isa/syscall.h>
#include <system/layout.h>

/* mm specific headers */
#include "config/mm_config.h"
#include "services/log.h"
#include "services/sp_iface.h"

/* Common headers */
#include "common_utils.h"
#include "error_codes.h"
#include "syscall_internal.h"

#define BYTES_IN_GB(x) (x * 1024UL * 1024UL * 1024UL)

#define MPROT_ENCODE_DDR_SIZE(size_in_bytes, encoded_value) \
    {                                                       \
        /* Check DRAM size and set the value */             \
        if (size_in_bytes <= BYTES_IN_GB(8))                \
        {                                                   \
            encoded_value = 0;                              \
        }                                                   \
        else if (size_in_bytes <= BYTES_IN_GB(16))          \
        {                                                   \
            encoded_value = 1;                              \
        }                                                   \
        else if (size_in_bytes <= BYTES_IN_GB(24))          \
        {                                                   \
            encoded_value = 2;                              \
        }                                                   \
        else                                                \
        {                                                   \
            encoded_value = 3;                              \
        }                                                   \
    }

typedef struct mm_config_ {
    uint64_t ddr_size;
    uint64_t host_managed_dram_size;
    uint64_t host_managed_dram_end;
    uint64_t cm_shire_mask;
    uint8_t lvdpll_strap;
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
*       This function initializes the MM configuration related information.
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
    uint64_t shire_mask = 0;
    uint8_t lvdpll_strap = 0;
    int32_t status = DMA_DRIVER_ERROR_INVALID_ADDRESS;

    status = SP_Iface_Get_DDR_Memory_Info(&ddr_mem_size);
    if (status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG, "MM_Config_Init:From SP:DDR size: 0x%lx\r\n", ddr_mem_size);

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

    /* Obtain the number of shires from SP and lvdpll strap value */
    status = SP_Iface_Get_Shire_Mask_And_Strap(&shire_mask, &lvdpll_strap);
    if (status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG, "MM_Config_Init:From SP:shire_mask: 0x%lx lvdpll_strap: %d\r\n",
            shire_mask, lvdpll_strap);

        atomic_store_local_64(&MM_Config_CB.cm_shire_mask, shire_mask);
        atomic_store_local_8(&MM_Config_CB.lvdpll_strap, lvdpll_strap);
    }

    /* Configure the DDR size in mprot */
    if (status == STATUS_SUCCESS)
    {
        uint64_t value;

        /* Set the bit for MM shire */
        shire_mask = MASK_SET_BIT(shire_mask, MASTER_SHIRE);

        /* Encode the DDR size */
        MPROT_ENCODE_DDR_SIZE(ddr_mem_size, value)

        Log_Write(LOG_LEVEL_INFO,
            "MM_Config_Init:Setting mprot:DRAM size:%ld:Encoded value:%ld\r\n", ddr_mem_size,
            value);

        /* Perform the syscall for mprot */
        syscall(SYSCALL_MPROT_CONFIG, MPROT_FIELD_DRAM_SIZE, value, shire_mask);
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

/************************************************************************
*
*   FUNCTION
*
*       MM_Config_Get_CM_Shire_Mask
*
*   DESCRIPTION
*
*       This function returns CM shire mask.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t     CM shire mask
*
***********************************************************************/
uint64_t MM_Config_Get_CM_Shire_Mask(void)
{
    return atomic_load_local_64(&MM_Config_CB.cm_shire_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Config_Get_Lvdpll_Strap
*
*   DESCRIPTION
*
*       This function returns LVDPLL strap value.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint8_t     lvdpll strap value
*
***********************************************************************/
uint8_t MM_Config_Get_Lvdpll_Strap(void)
{
    return atomic_load_local_8(&MM_Config_CB.lvdpll_strap);
}
