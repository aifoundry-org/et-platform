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
*       This file implements the Host Interface Services.
*
*   FUNCTIONS
*
*       SP_Host_Iface_SQs_Init
*       SP_Host_Iface_CQs_Init
*       SP_Host_Iface_SQ_Pop_Cmd
*       SP_Host_Iface_CQ_Push_Cmd
*
***********************************************************************/
#include <stdio.h>
#include "config/mgmt_build_config.h"
#include "sp_host_iface.h"
#include "pcie_int.h"
#include "vq.h"

/*! \struct host_iface_sqs_cb_t;
    \brief Host interface control block that manages
    submissions queues
*/
typedef struct host_iface_sqs_cb_ {
    uint32_t vqueue_base; /* This is a 32 bit offset from base */
    uint32_t vqueue_size;
    vq_cb_t vqueue;
} sp_host_iface_sqs_cb_t;

/*! \struct host_iface_cqs_cb_t;
    \brief Host interface control block that manages
    completion queues
*/
typedef struct host_iface_cqs_cb_ {
    uint32_t vqueue_base;
    uint32_t vqueue_size;
    vq_cb_t vqueue;
} sp_host_iface_cqs_cb_t;


/*! \var host_iface_sqs_cb_t Host_SQs
    \brief Global Host to SP submission
    queues interface
*/
static sp_host_iface_sqs_cb_t SP_Host_SQ = {0};

/*! \var host_iface_cqs_cb_t Host_CQs
    \brief Global SP to Host Minion completion
    queues interface
*/
static sp_host_iface_cqs_cb_t SP_Host_CQ = {0};

/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_SQs_Init
*
*   DESCRIPTION
*
*       Initiliaze SQs used by Host to post commands to SP
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t      status success or failure of Interface initialization
*
***********************************************************************/
int8_t SP_Host_Iface_SQ_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Submission vqueues control block
    based on build configuration */
    SP_Host_SQ.vqueue_base = SP_SQ_BASE_ADDRESS;
    SP_Host_SQ.vqueue_size = SP_SQ_SIZE;

    /* Initialize the SQ circular buffer */
    status = VQ_Init(&SP_Host_SQ.vqueue,
                      SP_Host_SQ.vqueue_base,
                      SP_Host_SQ.vqueue_size,
                      0,sizeof(cmd_size_t),SP_SQ_MEM_TYPE);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_CQ_Init
*
*   DESCRIPTION
*
*       Initiliaze CQs used by SP to post command responses to host
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t      status success or failure of Interface initialization
*
***********************************************************************/
int8_t SP_Host_Iface_CQ_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Submission vqueues control block
    based on build configuration */
    SP_Host_CQ.vqueue_base = SP_CQ_BASE_ADDRESS;
    SP_Host_CQ.vqueue_size = SP_CQ_SIZE;

    /* Initialize the SQ circular buffer */
    status = VQ_Init(&SP_Host_CQ.vqueue,
                      SP_Host_CQ.vqueue_base,
                      SP_Host_CQ.vqueue_size,
                      0,sizeof(cmd_size_t),SP_CQ_MEM_TYPE);
    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_CQ_Push_Command
*
*   DESCRIPTION
*
*       This function is used to push a command to host completion queue.
*
*   INPUTS
*
*       rx_buff    Pointer to the RX buffer.
*
*   OUTPUTS
*
*       uint16_t    Returns SQ command size read or zero for error.
*
***********************************************************************/
int8_t SP_Host_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size)
{
    int8_t status;

    /* Push the command to circular buffer */
    status = VQ_Push(&SP_Host_CQ.vqueue, p_cmd, cmd_size);

    if (status == STATUS_SUCCESS)
    {
      pcie_interrupt_host(SP_CQ_NOTIFY_VECTOR);
    }
    else
    {
       //MESSAGE_ERROR("SP_Host_Iface_CQ_Push_Cmd: ERROR: Circbuff Push Failed. (Error code: )", status, "\r\n");
    }


    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_SQ_Pop_Cmd
*
*   DESCRIPTION
*
*       This function is used to pop a command from host submission queue.
*
*   INPUTS
*
*       rx_buff    Pointer to the RX buffer.
*
*   OUTPUTS
*
*       uint32_t    Returns SQ command size read or zero for error.
*
***********************************************************************/
uint32_t SP_Host_Iface_SQ_Pop_Cmd(void* rx_buff)
{
    uint32_t return_val = 0;
    int32_t pop_ret_val;

    /* Pop the command from circular buffer */
    pop_ret_val = VQ_Pop(&SP_Host_SQ.vqueue, rx_buff);

    if (pop_ret_val > 0)
    {
        return_val = (uint32_t)pop_ret_val;
    }
    else if (pop_ret_val < 0)
    {
        printf("SP_Host_Iface:VQ pop failed.(Error code: %d)\r\n", pop_ret_val);
    }

    return return_val;
}
