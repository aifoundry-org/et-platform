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
/*! \file dir_regs.c
    \brief A C module that implements the DIR configuration APIs

    Public interfaces:
        DIR_Init
        DIR_Set_Master_Minion_Status
*/
/***********************************************************************/
/* mm specific headers */
#include "config/dir_regs.h"
#include "config/mm_config.h"
#include "services/log.h"

/* mm-rt-svcs (shared across minion rt) */
#include "layout.h"
#include "services/log.h"

/*! \var MM_DEV_INTF_REG_s *Gbl_MM_DIRs
    \brief Global static instance of Master Minions
    Device Interface Registers
    \warning Not thread safe!
*/
static MM_DEV_INTF_REG_s *Gbl_MM_DIRs = (void *)MM_DEV_INTF_BASE_ADDR;

/* Local functions */
static inline uint32_t crc32_for_byte(uint32_t r)
{
    /* Standard CRC32 polynomial value reversed (LSB-first) */
    uint32_t crc32_polynomial = 0xEDB88320;

    for (uint8_t j = 0; j < 8; j++)
    {
        r = (r & 1 ? 0 : crc32_polynomial) ^ (r >> 1);
    }

    return (r ^ (uint32_t)0xFF000000L);
}

static void crc32(const void *data, uint32_t len, uint32_t *crc)
{
    static uint32_t table[0x100] = { 0 };

    /* Initialize the table once */
    if (!(*table))
    {
        for (uint32_t i = 0; i < 0x100; i++)
        {
            table[i] = crc32_for_byte(i);
        }
    }


    /* Calculate CRC */
    for (uint32_t i = 0; i < len; ++i)
    {
        *crc = table[(const uint8_t)*crc ^ ((const uint8_t *)data)[i]] ^ (*crc >> 8);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       DIR_Init
*
*   DESCRIPTION
*
*       Initialize Device Interface Registers
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DIR_Init(void)
{
    uint32_t dir_crc32 = 0U; /* Initial value should be zero */

    /* Populate the device generic attributes */
    Gbl_MM_DIRs->generic_attr.version = MM_DEV_INTF_REG_VERSION;
    Gbl_MM_DIRs->generic_attr.total_size = sizeof(MM_DEV_INTF_REG_s);
    Gbl_MM_DIRs->generic_attr.attributes_size = sizeof(MM_DEV_INTF_GENERIC_ATTR_s);
    Gbl_MM_DIRs->generic_attr.num_mem_regions = MM_DEV_INTF_MEM_REGION_TYPE_NUM;

    /* Populate the MM VQs attributes */
    Gbl_MM_DIRs->vq_attr.sq_offset = MM_SQ_OFFSET;
    Gbl_MM_DIRs->vq_attr.sq_count = MM_SQ_COUNT;
    Gbl_MM_DIRs->vq_attr.per_sq_size = MM_SQ_SIZE;
    Gbl_MM_DIRs->vq_attr.sq_hp_offset = MM_SQ_HP_OFFSET;
    Gbl_MM_DIRs->vq_attr.sq_hp_count = MM_SQ_HP_COUNT;
    Gbl_MM_DIRs->vq_attr.per_sq_hp_size = MM_SQ_HP_SIZE;
    Gbl_MM_DIRs->vq_attr.cq_offset = MM_CQ_OFFSET;
    Gbl_MM_DIRs->vq_attr.cq_count = MM_CQ_COUNT;
    Gbl_MM_DIRs->vq_attr.per_cq_size = MM_CQ_SIZE;
    Gbl_MM_DIRs->vq_attr.int_trg_offset = MM_INTERRUPT_TRG_OFFSET;
    Gbl_MM_DIRs->vq_attr.int_trg_size = MM_INTERRUPT_TRG_SIZE;
    Gbl_MM_DIRs->vq_attr.int_id = MM_INTERRUPT_TRG_ID;
    Gbl_MM_DIRs->vq_attr.attributes_size = sizeof(MM_DEV_INTF_VQ_ATTR_s);

    /* Populate the MM VQ Buffer memory region attributes */
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .type = MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .bar = MM_VQ_BAR;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .bar_offset = MM_VQ_OFFSET;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .bar_size = MM_VQ_SIZE;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .dev_address = 0U;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .attributes_size = sizeof(MM_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_NONE) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Populate the OPS Host Managed memory region attributes */
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .type = MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .bar = MM_DEV_INTF_USER_KERNEL_SPACE_BAR;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .bar_offset = MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .bar_size = MM_DEV_INTF_USER_KERNEL_SPACE_SIZE;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .dev_address = HOST_MANAGED_DRAM_START;
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .attributes_size = sizeof(MM_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_MM_DIRs->mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_USER) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_OPS) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_64_BIT)  |
        MEM_REGION_DMA_ELEMENT_COUNT_SET(MEM_REGION_DMA_ELEMENT_COUNT) |
        MEM_REGION_DMA_ELEMENT_SIZE_SET(MEM_REGION_DMA_ELEMENT_SIZE);

    /* Calculate CRC32 of the DIRs excluding generic attributes
    NOTE: CRC32 checksum should be calculated at the end */
    crc32((void*)&Gbl_MM_DIRs->vq_attr,
        (uint32_t)(Gbl_MM_DIRs->generic_attr.total_size - Gbl_MM_DIRs->generic_attr.attributes_size),
        &dir_crc32);

    Gbl_MM_DIRs->generic_attr.crc32 = dir_crc32;

    /* Update Status to indicate MM Device Interface Registers are ready */
    Gbl_MM_DIRs->generic_attr.status = MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY;

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       DIR_Set_Master_Minion_Status
*
*   DESCRIPTION
*
*       Set Master Minion ready status
*
*   INPUTS
*
*       int16_t     status
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DIR_Set_Master_Minion_Status(int16_t status)
{
    Gbl_MM_DIRs->generic_attr.status = status;
}
