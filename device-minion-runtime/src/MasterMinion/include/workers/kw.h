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
*       Header/Interface description for the Kernel Worker component's
*       public interface
*
***********************************************************************/
#ifndef KW_DEFS_H
#define KW_DEFS_H

#include "kernel_info.h"
#include "kernel_state.h"
#include "kernel_sync.h"

#define     KW_BASE_HART_ID     2054
#define     KW_NUM              4 /*TODO: why do we need 4 KWs ? */
#define     KW_MAX_HART_ID      \
                (KW_BASE_HART_ID + KW_NUM)

/* TODO: fix up the 1 suffix once the old implementation is removed */
typedef enum 
{ 
    KERNEL_ID1_0 = 0, 
    KERNEL_ID1_1, 
    KERNEL_ID1_2, 
    KERNEL_ID1_3, 
    KERNEL_ID1_NONE 
} kernel_id_t1;

/*! \fn void KW_Init(void)
    \brief Initialize Kernel Worker
    \param None
*/
void KW_Init(void);

/*! \fn void KW_Launch(uint32_t hart_id)
    \brief Launch the Kernel Worker thread
    \param [in] HART ID on which the Kernel Worker should be launched
*/
void KW_Launch(uint32_t hart_id);

#if 0

/*! \fn void KW_Dispatch_Kernel_Launch_Command
        (struct device_ops_kernel_launch_cmd_t *kernel_launch_cmd)
    \brief Kernel Worker's interface to dispatch a kernel launch command
    \param [in] Kernel Launch Command
*/
void KW_Dispatch_Kernel_Launch_Command
    (struct device_ops_kernel_launch_cmd_t *kernel_launch_cmd);

/*! \fn void KW_Dispatch_Kernel_Abort_Command
        (struct device_ops_kernel_abort_cmd_t *kernel_abort_cmd)
    \brief Kernel Worker's interface to dispatch a kernel state command
    \param [in] Kernel State Command
    \param [out] Kernel State Response 
*/
void KW_Dispatch_Kernel_Abort_Command
    (struct device_ops_kernel_abort_cmd_t *kernel_abort_cmd);

/*! \fn void KW_Dispatch_Kernel_State_Command
        (struct device_ops_kernel_state_cmd_t *kernel_state_cmd, 
        struct device_ops_kernel_state_rsp_t *kernel_state_rsp);
    \brief Kernel Worker's interface to dispatch a kernel abort command
    \param [in] Kernel Abort Command
*/
void KW_Dispatch_Kernel_State_Command
    (struct device_ops_kernel_state_cmd_t *kernel_state_cmd, 
    struct device_ops_kernel_state_rsp_t *kernel_state_rsp);

/*! \fn void KW_Update_Kernel_State
        (kernel_id_t kernel_id, kernel_state_t kernel_state)
    \brief Update kernel state machine for the kernel executing 
    \param [in] kernel identifier
    \param [in] kernel state to update the kernel state machine to
*/
void KW_Update_Kernel_State(kernel_id_t kernel_id, 
     kernel_state_t kernel_state);

#endif

#endif /* KW_DEFS_H */