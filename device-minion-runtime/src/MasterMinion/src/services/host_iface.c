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
*       Host_Iface_SQs_Init
*       Host_Iface_CQs_Init
*       Host_Iface_Peek_SQ_Cmd_Size
*       Host_Iface_Peek_SQ_Cmd
*       Host_Iface_SQ_Pop_Cmd
*       Host_Iface_CQ_Push_Cmd
*       Host_Iface_Interrupt_Status
*       Host_Iface_Processing
*       Host_Iface_SQs_Deinit
*       Host_Iface_CQs_Deinit
*
***********************************************************************/
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/log1.h"
#include "drivers/interrupts.h"
#include "vq.h"
#include "pcie_int.h"

/*! \struct host_iface_sqs_cb_t;
    \brief Host interface control block that manages 
    submissions queues
*/
typedef struct host_iface_sqs_cb_ {
    uint32_t vqueues_base; /* This is a 32 bit offset from 64 dram base */
    uint32_t per_vqueue_size;
    vq_cb_t vqueues[MM_SQ_COUNT]; 
} host_iface_sqs_cb_t;

/*! \struct host_iface_cqs_cb_t;
    \brief Host interface control block that manages 
    completion queues
*/
typedef struct host_iface_cqs_cb_ {
    uint32_t vqueues_base; /* This is a 32 bit offset from 64 dram base */
    uint32_t per_vqueue_size;
    vq_cb_t vqueues[MM_CQ_COUNT]; 
} host_iface_cqs_cb_t;


/*! \var host_iface_sqs_cb_t Host_SQs
    \brief Global Host to MM submission
    queues interface
    \warning Not thread safe!
*/
static host_iface_sqs_cb_t Host_SQs = {0};

/*! \var host_iface_cqs_cb_t Host_CQs
    \brief Global MM to Host Minion completion 
    queues interface
    \warning Not thread safe!
*/
static host_iface_cqs_cb_t Host_CQs = {0};


/*! \var bool Host_Iface_Interrupt_Flag
    \brief Global Submission vqueues Control Block
    \warning Not thread safe!
*/
static bool Host_Iface_Interrupt_Flag = false;

/* Local fn proptotypes */
static void host_iface_rxisr(void);

static void host_iface_rxisr(void)
{
    Host_Iface_Interrupt_Flag = true;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_SQs_Init
*  
*   DESCRIPTION
*
*       Initiliaze SQs used by Host to post commands to MM
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
int8_t Host_Iface_SQs_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Submission vqueues control block 
    based on build configuration mm_config.h */
    Host_SQs.vqueues_base = MM_SQS_BASE_ADDRESS;
    Host_SQs.per_vqueue_size = MM_SQ_SIZE;

    for (uint32_t i = 0; (i < MM_SQ_COUNT) && 
        (status == STATUS_SUCCESS); i++) 
    {
        /* Initialize the SQ circular buffer */
        status = VQ_Init(&Host_SQs.vqueues[i], 
        VQ_CIRCBUFF_BASE_ADDR(Host_SQs.vqueues_base, i, 
        Host_SQs.per_vqueue_size),
        Host_SQs.per_vqueue_size, 0, sizeof(cmd_size_t), 
        MM_SQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS) 
    {
        /* Register host interface interrupt service routine to the host 
        PCIe interrupt that is used to notify MM runtime of host's push 
        to any oft the submission vqueues */
        Interrupt_Enable(PU_PLIC_PCIE_MESSAGE_INTR, HIFACE_INT_PRIORITY, 
            host_iface_rxisr);
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
        "ERROR: Unable to initialize Host to MM SQs. (Error code: )", 
        status, "\r\n");
    }

    return status;    
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_CQs_Init
*  
*   DESCRIPTION
*
*       Initiliaze CQs used by MM to post command responses to host
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
int8_t Host_Iface_CQs_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the Submission vqueues control block 
    based on build configuration mm_config.h */
    Host_CQs.vqueues_base = MM_SQS_BASE_ADDRESS;
    Host_CQs.per_vqueue_size = MM_SQ_SIZE;

    for (uint32_t i = 0; (i < MM_CQ_COUNT) && 
        (status == STATUS_SUCCESS); i++) 
    {
        /* Initialize the SQ circular buffer */
        status = VQ_Init(&Host_CQs.vqueues[i], 
        VQ_CIRCBUFF_BASE_ADDR(Host_CQs.vqueues_base, i, 
        Host_CQs.per_vqueue_size),
        Host_CQs.per_vqueue_size, 0, sizeof(cmd_size_t), 
        MM_CQ_MEM_TYPE);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_Interrupt_Status
*  
*   DESCRIPTION
*
*       Returns the status of host to master minion notifiy interrupt 
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       bool            True if interrupt is active, else false
*
***********************************************************************/
bool Host_Iface_Interrupt_Status(void) 
{
    bool retval = false;

    retval = Host_Iface_Interrupt_Flag; 

    if(Host_Iface_Interrupt_Flag)
        Host_Iface_Interrupt_Flag = false;

    return retval;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_Peek_Cmd_Size
*  
*   DESCRIPTION
*
*       This function is used to peek into command header and obtain 
*       its size.
*
*   INPUTS
*
*       sq_id      ID of the SQ to pop command from.
*
*   OUTPUTS
*
*       uint16_t   Returns SQ command size read or zero for error.
*
***********************************************************************/
uint32_t Host_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
{
    cmd_size_t command_size;

    /* Peek the command size to pop from SQ */
    VQ_Peek(&Host_SQs.vqueues[sq_id], 
            (void *)&command_size, 0, 
            sizeof(cmd_size_t));

    return command_size;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_Peek_Cmd
*  
*   DESCRIPTION
*
*       This function is used to peek into SQ and obtain the command
*
*   INPUTS
*
*       sq_id      ID of the SQ to pop command from.
*
*   OUTPUTS
*
*       uint16_t   Returns SQ command size read or zero for error.
*
***********************************************************************/
uint32_t Host_Iface_Peek_SQ_Cmd(uint8_t sq_id, void* cmd)
{
    cmd_size_t command_size = 0;

    (void) sq_id;
    (void) cmd;

 
    return command_size;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_CQ_Push_Command
*  
*   DESCRIPTION
*
*       This function is used to push a command to host completion queue.
*
*   INPUTS
*
*       sq_id      ID of the SQ to pop command from.
*       rx_buff    Pointer to the RX buffer.
*
*   OUTPUTS
*
*       uint16_t    Returns SQ command size read or zero for error.
*
***********************************************************************/
int8_t Host_Iface_CQ_Push_Cmd(uint8_t cq_id, void* p_cmd, uint32_t cmd_size)
{
    int8_t status;

    /* Pop the command from circular buffer */
    status = VQ_Push(&Host_CQs.vqueues[cq_id], p_cmd, cmd_size);

    if (status == STATUS_SUCCESS) 
    {
        /* TODO: Using MSI idx 0 for single CQ model */
        asm volatile("fence");
        status = (int8_t)pcie_interrupt_host(0);

        if (status != STATUS_SUCCESS) 
        {
            Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
            "CQ:ERROR: Host notification Failed. (Error code: )", status, "\r\n");
        }
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
            "CQ:ERROR: Circbuff Push Failed. (Error code: )", status, "\r\n");
    }


    if (!status) 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s",
        "ERROR: Circbuff Pop Failed \r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_SQ_Pop_Cmd
*  
*   DESCRIPTION
*
*       This function is used to pop a command from host submission queue.
*
*   INPUTS
*
*       sq_id      ID of the SQ to pop command from.
*       rx_buff    Pointer to the RX buffer.
*
*   OUTPUTS
*
*       uint16_t    Returns SQ command size read or zero for error.
*
***********************************************************************/
uint32_t Host_Iface_SQ_Pop_Cmd(uint8_t sq_id, void* rx_buff)
{
    uint32_t return_val = 0;
    uint32_t command_size;

    /* Pop the command from circular buffer */
    command_size = VQ_Pop(&Host_SQs.vqueues[sq_id], rx_buff);

    if (command_size) 
    {
        return_val = command_size;
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s",
        "ERROR: Circbuff Pop Failed \r\n");
    }

    return return_val;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_Processing
*  
*   DESCRIPTION
*
*       Processes commands in Host <-> MM  submission vqueues
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
void Host_Iface_Processing(void)
{
    /* TODO: Before processing SQs, we need to ensure that CQ is not full 
       and we can push response to it. */

    /* MM_SQ_HP_INDEX is Higher priority Submission Queue. Remaining SQs 
       are of normal priority. All available commands in MM_SQ_HP_INDEX 
       will be processed first, then each command from remaining SQs will 
       be processed in Round-Robin fashion with a single command processing 
       at a time. */
    bool sq_pending = true;
    uint8_t sq_id;
    int8_t status;
    uint16_t cmd_length;
    static uint8_t command_buffer[MM_CMD_MAX_SIZE] 
        __attribute__((aligned(8))) = { 0 };

    while (sq_pending) 
    {
        /* Scan all SQs for available command */
        for (sq_id = 0, sq_pending = false; sq_id < MM_SQ_COUNT; 
            sq_id++) 
        {
            do {
                /* Pop the command from current SQ */
                cmd_length = (uint16_t) VQ_Pop(&Host_CQs.vqueues[sq_id], 
                    command_buffer);

                if (cmd_length > 0)
                {
                    /* Handle the host command */
                    status = Host_Command_Handler(command_buffer);

                    if (status != STATUS_SUCCESS)
                    {
                        Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
                        "ERROR: Host command processing failed. (Error code: )", 
                        status, "\r\n");
                    }
                }
                /* Only process all commands from high priority SQ.
                   This while loop will continue to run until there 
                   are no more cmds in high priority SQ */
            } while ((sq_id == MM_SQ_HP_INDEX) && (cmd_length > 0));

            if (cmd_length > 0)
            {
                /* Goto next SQ and set the flag to indicate that we 
                   might need to process the same normal priority SQ again */
                sq_pending = true;
            }
        }
    }
 
    return;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_SQs_Deinit
*  
*   DESCRIPTION
*
*       This function deinitializes the Host Interface SQs.
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
int8_t Host_Iface_SQs_Deinit(void) 
{
    /* TODO: Perform deinit activities for the SQs */
    Interrupt_Disable(PU_PLIC_PCIE_MESSAGE_INTR);
    
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_CQs_Deinit
*  
*   DESCRIPTION
*
*       This function deinitializes the Host Interface SQs.
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
int8_t Host_Iface_CQs_Deinit(void) 
{
    /* TODO: Perform deinit activities for the CQs */
    Interrupt_Disable(PU_PLIC_PCIE_MESSAGE_INTR);
    
    return 0;
}