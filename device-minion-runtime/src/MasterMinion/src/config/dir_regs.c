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
#include "config/dir_regs.h"
#include "config/mm_config.h"
#include "layout.h"

/*! \var Gbl_MM_Dev_Intf_Regs
    \brief Global static instance of Master Minions 
    Device Interface Registers
    \warning Not thread safe!
*/
static volatile MM_DEV_INTF_REG_s *Gbl_MM_Dev_Intf_Regs = 
                    (void *)MM_DEV_INTF_BASE_ADDR;

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
    Gbl_MM_Dev_Intf_Regs->version     = MM_DEV_INTF_REG_VERSION;
    Gbl_MM_Dev_Intf_Regs->size        = sizeof(MM_DEV_INTF_REG_s);
    
    /* Populate the MM VQs information */
    Gbl_MM_Dev_Intf_Regs->mm_vq.bar         = MM_VQ_BAR;
    Gbl_MM_Dev_Intf_Regs->mm_vq.bar_size    = MM_VQ_SIZE;
    Gbl_MM_Dev_Intf_Regs->mm_vq.sq_offset   = MM_SQ_OFFSET;
    Gbl_MM_Dev_Intf_Regs->mm_vq.sq_count    = MM_SQ_COUNT;
    Gbl_MM_Dev_Intf_Regs->mm_vq.per_sq_size = MM_SQ_SIZE;
    Gbl_MM_Dev_Intf_Regs->mm_vq.cq_offset   = MM_CQ_OFFSET;
    Gbl_MM_Dev_Intf_Regs->mm_vq.cq_count    = MM_CQ_COUNT;
    Gbl_MM_Dev_Intf_Regs->mm_vq.per_cq_size = MM_CQ_SIZE;

    /* Populate the MM DDR regions information */
    Gbl_MM_Dev_Intf_Regs->ddr_regions.num_regions = MM_DEV_INTF_DDR_REGION_MAP_NUM;
    Gbl_MM_Dev_Intf_Regs->ddr_regions.
        regions[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].attr = 
        MM_DEV_INTF_DDR_REGION_ATTR_READ_WRITE;
    Gbl_MM_Dev_Intf_Regs->ddr_regions.
        regions[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].bar = 
        MM_DEV_INTF_USER_KERNEL_SPACE_BAR;
    Gbl_MM_Dev_Intf_Regs->ddr_regions.
        regions[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].offset = 
        MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET;
    Gbl_MM_Dev_Intf_Regs->ddr_regions.
        regions[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].devaddr = 
        HOST_MANAGED_DRAM_START;
    Gbl_MM_Dev_Intf_Regs->ddr_regions.
        regions[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].size = 
        MM_DEV_INTF_USER_KERNEL_SPACE_SIZE;

    /* Update Status to indicate MM Device Interface Registers 
    are ready */
    Gbl_MM_Dev_Intf_Regs->status = MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY;
    
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
*       uint8_t     status
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DIR_Set_Master_Minion_Status(uint8_t status)
{
    Gbl_MM_Dev_Intf_Regs->status = status;
}