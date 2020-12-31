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
/*! \file host_iface.c
    \brief A C module that implements the Host Interface services

    Public interfaces:
        SP_Iface_SQs_Init
        SP_Iface_CQs_Init
        SP_Iface_SQ_Pop_Cmd
        SP_Iface_CQ_Push_Cmd
        SP_Iface_Interrupt_Status
        SP_Iface_Processing
        SP_Iface_SQs_Deinit
        SP_Iface_CQs_Deinit
*/
/***********************************************************************/
#include "config/mm_config.h"
#include "services/sp_iface.h"
#include "services/sp_cmd_hdlr.h"
#include "services/log1.h"
#include "drivers/interrupts.h"
#include "circbuff.h"
#include "pcie_int.h"

/*! \var iface_q_cb_t SP_SQs
    \brief Global MM to SP submission queue
    \warning Not thread safe!
*/
static iface_cb_t SP_SQs = {0};

/*! \var iface_q_cb_t SP_CQs
    \brief Global SP to MM completion queue
    \warning Not thread safe!
*/
static iface_cb_t SP_CQs = {0};

/*! \var bool SP_Iface_Interrupt_Flag
    \brief Global SP Interface Interrupt flag
    \warning Not thread safe!
*/
static bool SP_Iface_Interrupt_Flag = false;

/* Interrupt Handler for Submission Queue post notification events from 
SP, uncomment and use skeleton as needed */
/* Local fn proptotypes */
//static void sp_iface_rxisr(void);
//static void sp_iface_push_notify(void);

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_SQs_Init
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
int8_t SP_Iface_SQs_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Submission Queues control block 
    based on build configuration mm_config.h */
    SP_SQs.vqueue_base = SP_SQ_BASE;
    SP_SQs.vqueue_size = SP_SQ_SIZE;

    /* Initialize the SQ circular buffer */
    status = VQ_Init(&SP_SQs.vqueue, SP_SQs.vqueue_base,
    SP_SQs.vqueue_size, 0, sizeof(cmd_size_t),
    SP_SQ_MEM_TYPE);

    if (status == STATUS_SUCCESS) 
    {
        /* TODO: Register SP ISR here */
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
        "ERROR: Unable to initialize SQs. (Error code: )", status, "\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_CQs_Init
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
int8_t SP_Iface_CQs_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Completion Queues control block 
    based on build configuration mm_config.h */
    SP_CQs.vqueue_size = SP_CQ_SIZE;
    SP_CQs.vqueue_base = SP_CQ_BASE;

    /* Initialize the CQ circular buffer */
    status = VQ_Init(&SP_CQs.vqueue, SP_CQs.vqueue_base,
    SP_CQs.vqueue_size, 0, sizeof(cmd_size_t),
    SP_CQ_MEM_TYPE);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Interrupt_Status
*  
*   DESCRIPTION
*
*       Returns the status of SP to master minion notifiy interrupt 
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       bool            True is interrupt is active, else false
*
***********************************************************************/
bool SP_Iface_Interrupt_Status(void) 
{
    return SP_Iface_Interrupt_Flag;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_CQ_Push_Command
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
int8_t SP_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size)
{
    int8_t status;

    /* Pop the command from circular buffer */
    status = VQ_Push(&SP_CQs.vqueue, p_cmd, cmd_size);

    if (status == STATUS_SUCCESS) 
    {
        /* TODO: Notify SP using appropriate interrupt here */
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
            "SP interface CQ push failed. (Error code: )", status, "\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_SQ_Pop_Cmd
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
uint32_t SP_Iface_SQ_Pop_Cmd(void* rx_buff)
{
    uint32_t return_val = 0;
    uint32_t command_size;

    /* Pop the command from circular buffer */
    command_size = VQ_Pop(&SP_CQs.vqueue, rx_buff);

    if (command_size) 
    {
        return_val = command_size;
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s",
        "SP interface SQ pop failed \r\n");
    }

    return return_val;
}


/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Processing
*  
*   DESCRIPTION
*
*       Processes commands in SP <-> MM  submission queues
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
void SP_Iface_Processing(void)
{
    /* TODO: Implement processing for SP > MM VQ command post
    event here */
    
    return;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_SQs_Deinit
*  
*   DESCRIPTION
*
*       This function deinitializes the SP Interface SQs.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t SP_Iface_SQs_Deinit(void) 
{
    /* TODO: Perform deinit activities for the SQs */
    
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_CQs_Deinit
*  
*   DESCRIPTION
*
*       This function deinitializes the SP Interface SQs.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t SP_Iface_CQs_Deinit(void) 
{
    /* TODO: Perform deinit activities for the CQs */
    
    return 0;
}