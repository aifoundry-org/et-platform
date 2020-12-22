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
#include "config/dir_regs.h"
#include "config/bl2_config.h"

/*! \var Gbl_SP_Dev_Intf_Regs
    \brief Global static instance of Service Processors 
    Device Interface Registers
    \warning Not thread safe!
*/
static volatile SP_DEV_INTF_REG_s *Gbl_SP_Dev_Intf_Regs = 
                    (void *)SP_DEV_INTF_BASE_ADDR;

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
    Gbl_SP_Dev_Intf_Regs->version     = SP_DEV_INTF_REG_VERSION;
    Gbl_SP_Dev_Intf_Regs->size        = sizeof(SP_DEV_INTF_REG_s);
    
    /* Populate the SP VQs information */
    Gbl_SP_Dev_Intf_Regs->sp_vq.bar         = SP_VQ_BAR;
    Gbl_SP_Dev_Intf_Regs->sp_vq.bar_size    = SP_VQ_BAR_SIZE;
    Gbl_SP_Dev_Intf_Regs->sp_vq.sq_offset   = SP_SQ_OFFSET;
    Gbl_SP_Dev_Intf_Regs->sp_vq.sq_count    = SP_SQ_COUNT;
    Gbl_SP_Dev_Intf_Regs->sp_vq.per_sq_size = SP_SQ_SIZE;
    Gbl_SP_Dev_Intf_Regs->sp_vq.cq_offset   = SP_CQ_OFFSET;
    Gbl_SP_Dev_Intf_Regs->sp_vq.cq_count    = SP_CQ_COUNT;
    Gbl_SP_Dev_Intf_Regs->sp_vq.per_cq_size = SP_CQ_SIZE;

    /* Populate the SP DDR regions information */
    Gbl_SP_Dev_Intf_Regs->ddr_regions.num_regions = SP_DEV_INTF_DDR_REGION_MAP_NUM;
    /* User kernel space reion */
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].attr = 
        SP_DEV_INTF_DDR_REGION_ATTR_READ_WRITE;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].bar = 
        SP_DEV_INTF_USER_KERNEL_SPACE_BAR;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].offset = 
        SP_DEV_INTF_USER_KERNEL_SPACE_OFFSET;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].size = 
        SP_DEV_INTF_USER_KERNEL_SPACE_SIZE;

    /* Firmware update region */
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_FIRMWARE_UPDATE_SCRATCH].attr = 
        SP_DEV_INTF_DDR_REGION_ATTR_WRITE_ONLY;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_FIRMWARE_UPDATE_SCRATCH].bar = 
        SP_DEV_INTF_FIRMWARE_UPDATE_SCRATCH_BAR;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_FIRMWARE_UPDATE_SCRATCH].offset = 
        SP_DEV_INTF_FIRMWARE_UPDATE_SCRATCH_OFFSET;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_FIRMWARE_UPDATE_SCRATCH].size = 
        SP_DEV_INTF_FIRMWARE_UPDATE_SCRATCH_SIZE;

    /* MSIX table region */
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_MSIX_TABLE].attr = 
        SP_DEV_INTF_DDR_REGION_ATTR_READ_ONLY;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_MSIX_TABLE].bar = 
        SP_DEV_INTF_MSIX_TABLE_BAR;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_MSIX_TABLE].offset = 
        SP_DEV_INTF_MSIX_TABLE_OFFSET;
    Gbl_SP_Dev_Intf_Regs->ddr_regions.
        regions[SP_DEV_INTF_DDR_REGION_MAP_MSIX_TABLE].size = 
        SP_DEV_INTF_MSIX_TABLE_SIZE;

    /* Update Status to indicate SP Device Interface Registers 
    are ready */
    Gbl_SP_Dev_Intf_Regs->status = SP_DEV_INTF_SP_BOOT_STATUS_DEV_INTF_READY;
    
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
*       uint8_t     status
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DIR_Set_Service_Processor_Status(uint8_t status)
{
    Gbl_SP_Dev_Intf_Regs->status = status;
}

/************************************************************************
*
*   FUNCTION
*
*       DIR_Set_Minion_Shires
*  
*   DESCRIPTION
*
*       Set the avaialble minion shires
*
*   INPUTS
*
*       uint64_t     minion_shires
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DIR_Set_Minion_Shires(uint64_t minion_shires)
{
    Gbl_SP_Dev_Intf_Regs->minion_shires = minion_shires;
}