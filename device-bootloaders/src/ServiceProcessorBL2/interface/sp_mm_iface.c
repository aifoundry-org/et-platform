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
*       This file implements the Service Processor to Master Minion 
*       Interface Services.
*
*   FUNCTIONS
*
*       SP_MM_Iface_SQ_Init
*       SP_MM_Iface_CQ_Init
*       SP_MM_Iface_SQ_Pop_Cmd
*       SP_MM_Iface_CQ_Push_Cmd
*
***********************************************************************/
#include "sp_vq_build_config.h"
#include "sp_mm_iface.h"
#include "vq.h"

/*! \var iface_q_cb_t SP_MM_SQ
    \brief Global MM to SP submission queue
*/
static iface_cb_t SP_MM_SQ = {0};

/*! \var iface_q_cb_t SP_MM_CQ
    \brief Global SP to MM completion queue
*/
static iface_cb_t SP_MM_CQ = {0};

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_SQ_Init
*  
*   DESCRIPTION
*
*       This function initializes SP interface. i.e., the Submission
*       Queues that enable SP to MM communications
*
*   INPUTS
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
int8_t SP_MM_Iface_SQ_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Submission Queues control block 
    based on build configuration */
    SP_MM_SQ.vqueue_base = SP_MM_SQ_BASE_ADDRESS;
    SP_MM_SQ.vqueue_size = SP_MM_SQ_SIZE;

    /* Initialize the SQ circular buffer */
    status = VQ_Init(&SP_MM_SQ.vqueue, SP_MM_SQ.vqueue_base,
                      SP_MM_SQ.vqueue_size, 0, sizeof(cmd_size_t),
                      SP_MM_SQ_MEM_TYPE);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_CQs_Init
*  
*   DESCRIPTION
*
*       This function initializes SP interface. i.e., the completion
*       Queues that enable SP to MM communications
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
int8_t SP_MM_Iface_CQ_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Completion Queues control block 
    based on build configuration */
    SP_MM_CQ.vqueue_size = SP_MM_CQ_SIZE;
    SP_MM_CQ.vqueue_base = SP_MM_CQ_BASE_ADDRESS;

    /* Initialize the CQ circular buffer */
    status = VQ_Init(&SP_MM_CQ.vqueue, SP_MM_CQ.vqueue_base,
                     SP_MM_CQ.vqueue_size, 0, sizeof(cmd_size_t),
                     SP_MM_CQ_MEM_TYPE);

    return status;
}

/**********************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_CQ_Push_Command
*  
*   DESCRIPTION
*
*       This function is used to push a command to SP completion queue.
*
*   INPUTS
*
*       p_cmd      Pointer to command to be pushed to Completion queue
*       cmd_size   Size of command to be pused
*
*   OUTPUTS
*
*       uint8_t    Returns status of command push operation
*
***********************************************************************/
int8_t SP_MM_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size)
{
    int8_t status;

    /* Pop the command from circular buffer */
    status = VQ_Push(&SP_MM_CQ.vqueue, p_cmd, cmd_size);

    if (status == STATUS_SUCCESS) 
    {
        /* TODO: Notify SP using appropriate interrupt here */
    } 
    else 
    {
        // Common logging TODO: MESSAGE_ERROR("SP interface CQ push failed. (Error code: )", status, "\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_SQ_Pop_Cmd
*  
*   DESCRIPTION
*
*       This function is used to pop a command from SP submission queue.
*
*   INPUTS
*
*       rx_buff    Pointer to the RX buffer.
*
*   OUTPUTS
*
*       uint32_t    Command size popped from Submission queue
*
***********************************************************************/
uint32_t SP_MM_Iface_SQ_Pop_Cmd(void* rx_buff)
{
    uint32_t return_val = 0;
    int32_t pop_ret_val;

    /* Pop the command from circular buffer */
    pop_ret_val = VQ_Pop(&SP_MM_CQ.vqueue, rx_buff);

    if (pop_ret_val > 0)
    {
        return_val = (uint32_t)pop_ret_val;
    }
    else if (pop_ret_val < 0)
    {
        // Common logging TODO: MESSAGE_ERROR("SP interface SQ pop failed \r\n");
    }

    return return_val;
}
