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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file implements services to initialize and manage
*       Service Processor's Device Interface Registers
*
*   FUNCTIONS
*
*       DIR_Init
*       DIR_Set_Service_Processor_Status
*       DIR_Set_Minion_Shires
*
***********************************************************************/
#include "config/mgmt_dir_regs.h"
#include "minion_configuration.h"
#include "bl2_cache_control.h"
#include "thermal_pwr_mgmt.h"
#include "perf_mgmt.h"
#include "crc32.h"

/*! \var Gbl_SP_DIRs
    \brief Global static instance of Service Processors
    Device Interface Registers
    \warning Not thread safe!
*/
static SP_DEV_INTF_REG_s *Gbl_SP_DIRs = (void *)SP_DEV_INTF_BASE_ADDR;

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
    Gbl_SP_DIRs->generic_attr.version = SP_DEV_INTF_REG_VERSION;
    Gbl_SP_DIRs->generic_attr.total_size = sizeof(SP_DEV_INTF_REG_s);
    Gbl_SP_DIRs->generic_attr.attributes_size = sizeof(SP_DEV_INTF_GENERIC_ATTR_s);
    Gbl_SP_DIRs->generic_attr.num_mem_regions = SP_DEV_INTF_MEM_REGION_TYPE_NUM;
    Gbl_SP_DIRs->generic_attr.minion_shires_mask = Minion_Get_Active_Compute_Minion_Mask();
    Gbl_SP_DIRs->generic_attr.form_factor = SP_DEV_CONFIG_FORM_FACTOR_PCIE;
    Gbl_SP_DIRs->generic_attr.cache_line_size = CACHE_LINE_SIZE;

    /* Populate the SP VQs attributes */
    Gbl_SP_DIRs->vq_attr.sq_offset = SP_SQ_OFFSET;
    Gbl_SP_DIRs->vq_attr.sq_count = SP_SQ_COUNT;
    Gbl_SP_DIRs->vq_attr.per_sq_size = SP_SQ_SIZE;
    Gbl_SP_DIRs->vq_attr.cq_offset = SP_CQ_OFFSET;
    Gbl_SP_DIRs->vq_attr.cq_count = SP_CQ_COUNT;
    Gbl_SP_DIRs->vq_attr.per_cq_size = SP_CQ_SIZE;
    Gbl_SP_DIRs->vq_attr.int_trg_offset = SP_INTERRUPT_TRG_OFFSET;
    Gbl_SP_DIRs->vq_attr.int_trg_size = SP_INTERRUPT_TRG_SIZE;
    Gbl_SP_DIRs->vq_attr.int_id = SP_INTERRUPT_TRG_ID;
    Gbl_SP_DIRs->vq_attr.attributes_size = sizeof(SP_DEV_INTF_VQ_ATTR_s);

    /* Populate the SP VQ Buffer memory region attributes */
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .type = SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .bar = SP_VQ_BAR;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .bar_offset = SP_PC_MAILBOX_BAR_OFFSET + SP_VQ_OFFSET;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .bar_size = SP_VQ_BAR_SIZE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .dev_address = 0U;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .attributes_size = sizeof(SP_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_NONE) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Populate the VQ interrupt trigger memory region attributes */
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .type = SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .bar = SP_DEV_INTF_INTERRUPT_TRG_REGION_BAR;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .bar_offset = SP_DEV_INTF_INTERRUPT_TRG_REGION_OFFSET;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .bar_size = SP_DEV_INTF_INTERRUPT_TRG_REGION_SIZE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .dev_address = 0U;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .attributes_size = sizeof(SP_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_INT_TRIGGER]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_NONE) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Populate the Device Management scratch memory region attributes */
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .type = SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .bar = SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_BAR;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .bar_offset = SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_OFFSET;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .bar_size = SP_DEV_INTF_DEV_MANAGEMENT_SCRATCH_SIZE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .dev_address = 0U;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .attributes_size = sizeof(SP_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SCRATCH]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_MANAGEMENT) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Populate the SP FW Trace buffer memory region attributes */
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .type = SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .bar = SP_DEV_INTF_TRACE_BUFFER_BAR;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .bar_offset = SP_DEV_INTF_TRACE_BUFFER_OFFSET;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .bar_size = SP_DEV_INTF_TRACE_BUFFER_SIZE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .dev_address = 0U;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .attributes_size = sizeof(SP_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_SPFW_TRACE]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_NONE) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Populate the MM FW Trace buffer memory region attributes */
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .type = SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .bar = SP_DEV_INTF_MM_TRACE_BUFFER_BAR;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .bar_offset = SP_DEV_INTF_MM_TRACE_BUFFER_OFFSET;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .bar_size = SP_DEV_INTF_MM_TRACE_BUFFER_SIZE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .dev_address = 0U;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .attributes_size = sizeof(SP_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_NONE) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Populate the MM FW Trace buffer memory region attributes */
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .type = SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .bar = SP_DEV_INTF_CM_TRACE_BUFFER_BAR;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .bar_offset = SP_DEV_INTF_CM_TRACE_BUFFER_OFFSET;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .bar_size = SP_DEV_INTF_CM_TRACE_BUFFER_SIZE;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .dev_address = 0U;
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .attributes_size = sizeof(SP_DEV_INTF_MEM_REGION_ATTR_s);
    Gbl_SP_DIRs->mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE]
        .access_attr = MEM_REGION_PRIVILEDGE_MODE_SET(MEM_REGION_PRIVILEDGE_MODE_KERNEL) |
        MEM_REGION_NODE_ACCESSIBLE_SET(MEM_REGION_NODE_ACCESSIBLE_NONE) |
        MEM_REGION_DMA_ALIGNMENT_SET(MEM_REGION_DMA_ALIGNMENT_NONE);

    /* Calculate CRC32 of the DIRs excluding generic attributes
    NOTE: CRC32 checksum should be calculated at the end */
    crc32((void*)&Gbl_SP_DIRs->vq_attr,
        (uint32_t)(Gbl_SP_DIRs->generic_attr.total_size - Gbl_SP_DIRs->generic_attr.attributes_size),
        &dir_crc32);

    Gbl_SP_DIRs->generic_attr.crc32 = dir_crc32;

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       DIR_Set_Service_Processor_Status
*
*   DESCRIPTION
*
*       Set Service Processor ready status
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
void DIR_Set_Service_Processor_Status(int16_t status)
{
    Gbl_SP_DIRs->generic_attr.status = status;
    if(status == SP_DEV_INTF_SP_BOOT_STATUS_MM_FW_LAUNCHED)
    {
      // Add Magic marker to know when to load PCIE Driver
      ISSUE_MAGIC_MARKER();
    }
}

/************************************************************************
*
*   FUNCTION
*
*       DIR_Cache_Size_Init
*
*   DESCRIPTION
*
*       Initialize Device Cache Size Registers
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
void DIR_Cache_Size_Init(void)
{
    /* Populate the device generic attributes */
    Gbl_SP_DIRs->generic_attr.l3_size =
                            Cache_Control_L3_size(Gbl_SP_DIRs->generic_attr.minion_shires_mask);
    Gbl_SP_DIRs->generic_attr.l2_size =
                            Cache_Control_L2_size(Gbl_SP_DIRs->generic_attr.minion_shires_mask);
    Gbl_SP_DIRs->generic_attr.scp_size =
                            Cache_Control_SCP_size(Gbl_SP_DIRs->generic_attr.minion_shires_mask);

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       DIR_Generic_Attributes_Init
*
*   DESCRIPTION
*
*       Initialize Device Generic Attribute
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
void DIR_Generic_Attributes_Init(void)
{
    /* Populate the device generic attributes */
    Gbl_SP_DIRs->generic_attr.minion_boot_freq = (uint32_t)Get_Minion_Frequency();
    get_module_tdp_level((uint8_t *)&Gbl_SP_DIRs->generic_attr.device_tdp);

    return;
}
