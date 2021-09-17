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
#include "etsoc/drivers/pcie/pcie_int.h"
#include "transports/vq/vq.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portmacro.h"


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
    /* Local copy globals */
    circ_buff_cb_t circ_buff_local;
    /* Shared copy globals */
    uint32_t vqueue_base;
    uint32_t vqueue_size;
    vq_cb_t vqueue;
} sp_host_iface_cqs_cb_t;


/*! \var host_iface_sqs_cb_t Host_SQs
    \brief Global Host to SP submission
    queues interface
*/
static sp_host_iface_sqs_cb_t SP_Host_SQ __attribute__((aligned(64))) = {0};

/*! \var host_iface_cqs_cb_t Host_CQs
    \brief Global SP to Host Minion completion
    queues interface
*/
static sp_host_iface_cqs_cb_t SP_Host_CQ __attribute__((aligned(64))) = {0};

/*! \var SemaphoreHandle_t Host_CQ_Lock
    \brief Host CQ lock
*/
static SemaphoreHandle_t Host_CQ_Lock = NULL;

/*! \var StaticSemaphore_t Host_CQ_Mutex_Buffer
    \brief Host CQ lock buffer
*/

static StaticSemaphore_t Host_CQ_Mutex_Buffer;

/*! \var SemaphoreHandle_t Host_SQ_Lock
    \brief Host SQ lock
*/
static SemaphoreHandle_t Host_SQ_Lock = NULL;

/*! \var StaticSemaphore_t Host_SQ_Mutex_Buffer
     \brief Host SQ lock buffer
*/
static StaticSemaphore_t Host_SQ_Mutex_Buffer;
/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_Init
*
*   DESCRIPTION
*
*       Initialiaze SQs and CQs used for Host to SP communications
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
int8_t SP_Host_Iface_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    status = SP_Host_Iface_SQ_Init();

    if(status == STATUS_SUCCESS)
    {
        status = SP_Host_Iface_CQ_Init();
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_SQs_Init
*
*   DESCRIPTION
*
*       Initialize SQs used by Host to post commands to SP
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

    if (status == STATUS_SUCCESS)
    {
        /* Init SQ lock to released state */
        Host_SQ_Lock = xSemaphoreCreateMutexStatic(&Host_SQ_Mutex_Buffer);
        if (!Host_SQ_Lock)
        {
            status = GENERAL_ERROR;
        }
    }

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

    /* Populate data in local copy globals */
    if(status == STATUS_SUCCESS)
    {
        /* Make a copy of the Circular Buffer CB in shared SRAM to global variable */
        memcpy(&SP_Host_CQ.circ_buff_local, SP_Host_CQ.vqueue.circbuff_cb,
            sizeof(SP_Host_CQ.circ_buff_local));
    }

    if (status == STATUS_SUCCESS)
    {
        /* Init CQ lock to released state */
        Host_CQ_Lock = xSemaphoreCreateMutexStatic(&Host_CQ_Mutex_Buffer);
        if (!Host_CQ_Lock)
        {
            status = GENERAL_ERROR;
        }
    }

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
    int8_t status = GENERAL_ERROR;

    /* Obtain the mutex to serialize access to the VQs.
       The mutex includes the priority inheritance and hence
       avoids the priority inversion problem
       In case mutex is already locked, the calling task will have to wait until
       it is released by the prior task. The max wait time is defined by the
       HOST_VQ_MAX_TIMEOUT.
       If wait time exceeds the HOST_VQ_MAX_TIMEOUT, the call will return with
       the error.
    */
    if (xSemaphoreTake(Host_CQ_Lock, (TickType_t)HOST_VQ_MAX_TIMEOUT ) == pdTRUE)
    {
        /* Verify that the head value read from shared memory is equal to previous head value */
        if(SP_Host_CQ.circ_buff_local.head_offset != VQ_Get_Head_Offset(&SP_Host_CQ.vqueue))
        {
            uint64_t local_head_offset =  SP_Host_CQ.circ_buff_local.head_offset;
            uint64_t reference_head_offset = VQ_Get_Head_Offset(&SP_Host_CQ.vqueue);

            /* Fallback mechanism: use the cached copy of SQ tail */
            SP_Host_CQ.vqueue.circbuff_cb->head_offset = SP_Host_CQ.circ_buff_local.head_offset;

            /* If this condition occurs, there's definitely some corruption in VQs */
            Log_Write(LOG_LEVEL_WARNING,
            "SP_Host_Iface_CQ_Push_Cmd:FATAL_ERROR:Tail Mismatch:Local: %ld, Shared Memory: %ld Using local value as fallback mechanism\r\n",
            local_head_offset, reference_head_offset);
        }

        /* Push the command to circular buffer */
        status = VQ_Push(&SP_Host_CQ.vqueue, p_cmd, cmd_size);

        /* Get the updated head pointer in local copy */
        SP_Host_CQ.circ_buff_local.head_offset = VQ_Get_Head_Offset(&SP_Host_CQ.vqueue);

        if(status == STATUS_SUCCESS)
        {
            pcie_interrupt_host(SP_CQ_NOTIFY_VECTOR);
        }

        xSemaphoreGive(Host_CQ_Lock);
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
    int32_t pop_ret_val = 0;

   /*  Obtain the mutex to serialize access to the VQs.
       The mutex includes the priority inheritance and hence
       avoids the priority inversion problem
       In case mutex is already locked, the calling task will have to wait until
       it is released by the prior task. The max wait time is defined by the
       HOST_VQ_MAX_TIMEOUT.
       If wait time exceeds the HOST_VQ_MAX_TIMEOUT, the call will return with
       the error.
    */
    if (xSemaphoreTake(Host_SQ_Lock, (TickType_t)HOST_VQ_MAX_TIMEOUT ) == pdTRUE)
    {
        /* Pop the command from circular buffer */
        pop_ret_val = VQ_Pop(&SP_Host_SQ.vqueue, rx_buff);

        xSemaphoreGive(Host_SQ_Lock);
    }

    if (pop_ret_val > 0)
    {
        return_val = (uint32_t)pop_ret_val;
    }
    else if (pop_ret_val < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "SP_Host_Iface:VQ pop failed.(Error code: %d)\r\n", pop_ret_val);
    }

    return return_val;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Host_Iface_Get_VQ_Base_Addr
*
*   DESCRIPTION
*
*       Obtain the Virtual Queue base address
*
*   INPUTS
*
*       vq_type     Virtuql Queue Type
*
*   OUTPUTS
*
*       vq_cb_t*    Pointer to Virtual queue base
*
***********************************************************************/
vq_cb_t* SP_Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type)
{
    vq_cb_t* retval = 0;

    if(vq_type == SQ)
    {
        retval = &SP_Host_SQ.vqueue;
    }
    else if(vq_type == CQ)
    {
        retval = &SP_Host_CQ.vqueue;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR,
            "HostIface:ERROR:Obtaining VQ base address, bad vq_type: %d\r\n",
            vq_type);
    }

    return retval;
}
