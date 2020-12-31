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
        Host_Iface_SQs_Init
        Host_Iface_CQs_Init
        Host_Iface_Peek_SQ_Cmd_Size
        Host_Iface_Peek_SQ_Cmd
        Host_Iface_SQ_Pop_Cmd
        Host_Iface_CQ_Push_Cmd
        Host_Iface_Interrupt_Status
        Host_Iface_Processing
        Host_Iface_SQs_Deinit
        Host_Iface_CQs_Deinit
*/
/***********************************************************************/
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/log1.h"
#include "workers/sqw.h"
#include "drivers/interrupts.h"
#include "vq.h"
#include "pcie_int.h"
#include "hal_device.h"

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
static volatile bool Host_Iface_Interrupt_Flag = false;


/* Local fn proptotypes */
static void host_iface_rxisr(void);

static void host_iface_rxisr(void)
{
    Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "Dispatcher: PCIe interrupt! \r\n");

    Host_Iface_Interrupt_Flag = true;

    /* TODO: Move this to a interrupt ack API within
    the driver abstraction, if ack mechanism is generic
    across interrupts this ack API can go to interrupts.c */
    volatile uint32_t *const pcie_int_dec_ptr = 
        (uint32_t *)(R_PU_TRG_MMIN_BASEADDR + 0x8);
    volatile uint32_t *const pcie_int_cnt_ptr = 
        (uint32_t *)(R_PU_TRG_MMIN_BASEADDR + 0xC);

    if (*pcie_int_cnt_ptr) 
    {
        *pcie_int_dec_ptr = 1;
    }

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

    /* TODO: Need to decide the base address for memory
    (32-bit or 64-bit) based on memory type. */
    
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
*       Host_Iface_Get_VQ_Base_Addr
*  
*   DESCRIPTION
*
*       Obtain the Submission Queue base address
*
*   INPUTS
*
*       vq_type     Virtuql Queue Type
*       vq_id       Virtual Queue ID
*
*   OUTPUTS
*
*       vq_cb_t*    Pointer to Virtual queue base
*
***********************************************************************/
vq_cb_t* Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type, uint8_t vq_id)
{
    vq_cb_t* retval=0;

    if(vq_type == SQ)
    {
        retval = &Host_SQs.vqueues[vq_id]; 
    }
    else if(vq_type == CQ)
    {
        retval = &Host_CQs.vqueues[vq_id];
    }
    else
    {
	 Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "HostIface:ERROR:Failed to obtain VQ base address, bad vq_id\r\n");
    }
    
    return retval;
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

    /* Initialize the Completion vqueues control block 
    based on build configuration mm_config.h */
    Host_CQs.vqueues_base = MM_CQS_BASE_ADDRESS;
    Host_CQs.per_vqueue_size = MM_CQ_SIZE;

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
    return Host_Iface_Interrupt_Flag;
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
int8_t Host_Iface_Peek_SQ_Cmd_Hdr(uint8_t sq_id, void* cmd)
{
    (void) sq_id;
    (void) cmd;

 
    return 0;
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
                "CQ:ERROR: Host notification Failed. (Error code: )", 
                status, "\r\n");
        }
    } 
    else 
    {
        Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
            "CQ:ERROR: Circbuff Push Failed. (Error code: )", 
            status, "\r\n");
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
    uint8_t sq_id;   
    bool status;

    if(Host_Iface_Interrupt_Flag)
    {
        Host_Iface_Interrupt_Flag = false;
        asm volatile("fence"); 
    }

    /* Scan all SQs for available command */
    for (sq_id = 0; sq_id < MM_SQ_COUNT; sq_id++)    
    {
        status = VQ_Data_Avail(&Host_SQs.vqueues[sq_id]);

        if(status == true)
        {
            Log_Write(LOG_LEVEL_DEBUG, 
            "%s%d%s", "HostIfaceProcessing:Notifying:SQW_IDX=", sq_id, "\r\n");

            /* Dispatch work to SQ Worker associated with this SQ */
            SQW_Notify(sq_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s%d%s", 
                "HostIfaceProcessing:NoData on SQ=", sq_id, "\r\n");
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
