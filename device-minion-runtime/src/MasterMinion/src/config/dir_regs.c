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
*       Master Minion's Device Interface Registers
*
*   FUNCTIONS
*
*       DIR_Init
*
***********************************************************************/
#include "config/dir_regs.h"
#include "config/mm_config.h"
#include "layout.h"

/*! \var log_level_t Gbl_MM_Dev_Intf_Regs
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
    Gbl_MM_Dev_Intf_Regs->mm_vq.sq_count   = MM_SQ_COUNT;
    Gbl_MM_Dev_Intf_Regs->mm_vq.cq_count   = MM_CQ_COUNT;
    Gbl_MM_Dev_Intf_Regs->mm_vq.sq_size    = MM_SQ_SIZE;
    Gbl_MM_Dev_Intf_Regs->mm_vq.cq_size    = MM_CQ_SIZE;
    Gbl_MM_Dev_Intf_Regs->mm_vq.bar        = MM_VQ_BAR;
    Gbl_MM_Dev_Intf_Regs->mm_vq.bar_offset = MM_VQ_OFFSET;
    Gbl_MM_Dev_Intf_Regs->mm_vq.bar_size   = MM_SQ_SIZE; // TODO: DO we need this?

    for (uint8_t i = 0; i < MM_CQ_COUNT; i++) {
        /* TODO: SW-4597: Start from vector one when MSI-X and 
        SP VQ is available. Vector zero is reserved for SP CQ. */
        Gbl_MM_Dev_Intf_Regs->mm_vq.cq_interrupt_vector[i] = MM_CQ_NOTIFY_VECTOR + i;
    }

    Gbl_MM_Dev_Intf_Regs->
        ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].attr = 
        MM_DEV_INTF_DDR_REGION_ATTR_READ_WRITE;
    Gbl_MM_Dev_Intf_Regs->
        ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].bar = 
        MM_DEV_INTF_USER_KERNEL_SPACE_BAR;
    Gbl_MM_Dev_Intf_Regs->
        ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].offset = 
        MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET;
    Gbl_MM_Dev_Intf_Regs->
        ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].devaddr = 
        HOST_MANAGED_DRAM_START;
    Gbl_MM_Dev_Intf_Regs->
        ddr_region[MM_DEV_INTF_DDR_REGION_MAP_USER_KERNEL_SPACE].size = 
        MM_DEV_INTF_USER_KERNEL_SPACE_SIZE;

    /* Update Status to indicate MM Device Interface Registers 
    are initialized */
    Gbl_MM_Dev_Intf_Regs->status = 
        MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY_INITIALIZED;
    
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