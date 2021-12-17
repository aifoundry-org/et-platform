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
        Host_Iface_Optimized_SQ_Update_Tail
        Host_Iface_CQ_Push_Cmd
        Host_Iface_Interrupt_Status
        Host_Iface_Processing
        Host_Iface_SQs_Deinit
        Host_Iface_CQs_Deinit
*/
/***********************************************************************/
/* mm_rt_svcs */
#include <transports/vq/vq.h>
#include <etsoc/drivers/pcie/pcie_int.h>

/* etsoc_hal */
#include "hwinc/hal_device.h"

/* mm specific headers */
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/log.h"
#include "workers/sqw.h"
#include "workers/sqw_hp.h"
#include "drivers/plic.h"

/*! \struct host_iface_sqs_hp_cb_t
    \brief Host interface control block that manages
    high priority submissions queues
*/
typedef struct host_iface_sqs_hp_cb_ {
    uint32_t vqueues_base;
    uint32_t per_vqueue_size;
    vq_cb_t vqueues[MM_SQ_HP_COUNT];
} host_iface_sqs_hp_cb_t;

/*! \struct host_iface_sqs_cb_t
    \brief Host interface control block that manages
    submissions queues
*/
typedef struct host_iface_sqs_cb_ {
    uint32_t vqueues_base; /* This is a 32 bit offset from 64 dram base */
    uint32_t per_vqueue_size;
    vq_cb_t vqueues[MM_SQ_COUNT];
} host_iface_sqs_cb_t;

/*! \struct host_iface_cqs_cb_t
    \brief Host interface control block that manages
    completion queues
*/
typedef struct host_iface_cqs_cb_ {
    uint32_t vqueues_base; /* This is a 32 bit offset from 64 dram base */
    uint32_t per_vqueue_size;
    spinlock_t vqueue_locks[MM_CQ_COUNT];
    vq_cb_t vqueues[MM_CQ_COUNT];
} host_iface_cqs_cb_t;

/*! \var host_iface_sqs_hp_cb_t Host_SQs_HP
    \brief Global Host to MM high priority submission
    queues interface
    \warning Not thread safe!
*/
static host_iface_sqs_hp_cb_t Host_SQs_HP __attribute__((aligned(64))) = { 0 };

/*! \var host_iface_sqs_cb_t Host_SQs
    \brief Global Host to MM submission
    queues interface
    \warning Not thread safe!
*/
static host_iface_sqs_cb_t Host_SQs __attribute__((aligned(64))) = { 0 };

/*! \var host_iface_cqs_cb_t Host_CQs
    \brief Global MM to Host Minion completion
    queues interface
    \warning Not thread safe!
*/
static host_iface_cqs_cb_t Host_CQs __attribute__((aligned(64))) = { 0 };

/*! \var bool Host_Iface_Interrupt_Flag
    \brief Global Submission vqueues Control Block
    \warning Not thread safe!
*/
static volatile bool Host_Iface_Interrupt_Flag __attribute__((aligned(64))) = false;

/* Local fn proptotypes */
static void host_iface_rxisr(uint32_t intID);

static void host_iface_rxisr(uint32_t intID)
{
    (void)intID;

    Log_Write(LOG_LEVEL_DEBUG, "Dispatcher:PCIe interrupt!\r\n");

    Host_Iface_Interrupt_Flag = true;

    /* TODO: Move this to a interrupt ack API within
    the driver abstraction, if ack mechanism is generic
    across interrupts this ack API can go to interrupts.c */
    volatile uint32_t *const pcie_int_dec_ptr = (uint32_t *)(R_PU_TRG_MMIN_BASEADDR + 0x8);
    volatile uint32_t *const pcie_int_cnt_ptr = (uint32_t *)(R_PU_TRG_MMIN_BASEADDR + 0xC);

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
*       Initiliaze normal and high priority SQs used by Host to post
*       commands to MM
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
    uint64_t temp = 0;

    /* TODO: Need to decide the base address for memory
    (32-bit or 64-bit) based on memory type. */

    /* Initialize High Priority Submission vqueues control block
    based on build configuration mm_config.h */
    temp = ((MM_SQ_HP_SIZE << 32) | MM_SQS_HP_BASE_ADDRESS);
    atomic_store_local_64((uint64_t *)&Host_SQs_HP, temp);

    for (uint32_t i = 0; (i < MM_SQ_HP_COUNT); i++)
    {
        /* Initialize the High Priority SQ circular buffer */
        status = VQ_Init(&Host_SQs_HP.vqueues[i],
            VQ_CIRCBUFF_BASE_ADDR(MM_SQS_HP_BASE_ADDRESS, i, MM_SQ_HP_SIZE), MM_SQ_HP_SIZE, 0,
            sizeof(cmd_size_t), MM_SQ_MEM_TYPE);

        /* Check for error */
        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "ERROR: Unable to initialize Host to MM HP SQs. (Error code: %d)\r\n", status);
            break;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /* Initialize the Submission vqueues control block
        based on build configuration mm_config.h */
        temp = ((MM_SQ_SIZE << 32) | MM_SQS_BASE_ADDRESS);
        atomic_store_local_64((uint64_t *)&Host_SQs, temp);

        for (uint32_t i = 0; (i < MM_SQ_COUNT); i++)
        {
            /* Initialize the SQ circular buffer */
            status = VQ_Init(&Host_SQs.vqueues[i],
                VQ_CIRCBUFF_BASE_ADDR(MM_SQS_BASE_ADDRESS, i, MM_SQ_SIZE), MM_SQ_SIZE, 0,
                sizeof(cmd_size_t), MM_SQ_MEM_TYPE);

            /* Check for error */
            if (status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "ERROR: Unable to initialize Host to MM SQs. (Error code: %d)\r\n", status);
                break;
            }
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /* Register host interface interrupt service routine to the host
        PCIe interrupt that is used to notify MM runtime of host's push
        to any of the submission vqueues */
        PLIC_RegisterHandler(PU_PLIC_PCIE_MESSAGE_INTR_ID, HIFACE_INT_PRIORITY, host_iface_rxisr);
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
vq_cb_t *Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type, uint8_t vq_id)
{
    vq_cb_t *retval = 0;

    if (vq_type == SQ)
    {
        retval = &Host_SQs.vqueues[vq_id];
    }
    else if (vq_type == CQ)
    {
        retval = &Host_CQs.vqueues[vq_id];
    }
    else if (vq_type == SQ_HP)
    {
        retval = &Host_SQs_HP.vqueues[vq_id];
    }
    else
    {
        Log_Write(
            LOG_LEVEL_ERROR, "HostIface:ERROR:Obtaining VQ base address, bad vq_id: %d\r\n", vq_id);
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
    uint64_t temp = 0;

    /* Initialize the Completion vqueues control block
    based on build configuration mm_config.h */
    temp = (((uint64_t)MM_CQ_SIZE << 32) | MM_CQS_BASE_ADDRESS);
    atomic_store_local_64((uint64_t *)&Host_CQs, temp);

    for (uint32_t i = 0; (i < MM_CQ_COUNT) && (status == STATUS_SUCCESS); i++)
    {
        /* Initialize the spinlock */
        init_local_spinlock(&Host_CQs.vqueue_locks[i], 0);

        /* Initialize the CQ circular buffer */
        status = VQ_Init(&Host_CQs.vqueues[i],
            VQ_CIRCBUFF_BASE_ADDR(MM_CQS_BASE_ADDRESS, i, MM_CQ_SIZE), MM_CQ_SIZE, 0,
            sizeof(cmd_size_t), MM_CQ_MEM_TYPE);
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
    VQ_Peek(&Host_SQs.vqueues[sq_id], (void *)&command_size, 0, sizeof(cmd_size_t));

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
*       int8_t     Status indicating success or negative error
*
***********************************************************************/
int8_t Host_Iface_Peek_SQ_Cmd_Hdr(uint8_t sq_id, void *cmd)
{
    (void)sq_id;
    (void)cmd;

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
*       cmd_size   Command size
*
*   OUTPUTS
*
*       int8_t     Status indicating success or negative error
*
***********************************************************************/
int8_t Host_Iface_CQ_Push_Cmd(uint8_t cq_id, void *p_cmd, uint32_t cmd_size)
{
    int8_t status;

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&Host_CQs.vqueue_locks[cq_id]);

    /* TODO: SW-5781 Polling here until we are able to push response */
    do
    {
        /* Push the response to circular buffer */
        status = VQ_Push(&Host_CQs.vqueues[cq_id], p_cmd, cmd_size);
        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_WARNING, "CQ[%d] push warning: status code: %d\n", cq_id, status);
        }
    } while (status == CIRCBUFF_ERROR_FULL);

    if (status == STATUS_SUCCESS)
    {
        asm volatile("fence");
        /* TODO: Using MSI idx 0 for single CQ model */
        status = (int8_t)pcie_interrupt_host(1);

        /* Release the lock */
        release_local_spinlock(&Host_CQs.vqueue_locks[cq_id]);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(
                LOG_LEVEL_ERROR, "HostIface:CQ_Push:ERROR: Notification (Error: %d)\r\n", status);
        }
    }
    else
    {
        /* Release the lock */
        release_local_spinlock(&Host_CQs.vqueue_locks[cq_id]);

        Log_Write(LOG_LEVEL_ERROR, "HostIface:CQ_Push:ERROR: VQ Push (Error: %d)\r\n", status);
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
*       int32_t   Status indicating success or negative error
*
***********************************************************************/
int32_t Host_Iface_SQ_Pop_Cmd(uint8_t sq_id, void *rx_buff)
{
    int32_t pop_ret_val;

    /* Pop the command from circular buffer */
    pop_ret_val = VQ_Pop(&Host_SQs.vqueues[sq_id], rx_buff);

    if (pop_ret_val < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "HostIface:SQ_Pop:ERROR:VQ pop failed.(Error code: %d)\r\n",
            pop_ret_val);
    }

    return pop_ret_val;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_Iface_Optimized_SQ_Update_Tail
*
*   DESCRIPTION
*
*       This function is used to update the value of tail from cached VQ CB
*       to shared VQ CB.
*
*   INPUTS
*
*       sq_shared  Pointer to shared VQ CB.
*       sq_cached  Pointer to cached VQ CB.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Host_Iface_Optimized_SQ_Update_Tail(vq_cb_t *sq_shared, vq_cb_t *sq_cached)
{
    /* Update tail value in VQ memory */
    VQ_Set_Tail_Offset(sq_shared, VQ_Get_Tail_Offset(sq_cached));

    /* TODO: We need to send event to the host when 25% of space is avialable is SQ.
    Should be build time macro */
    /* TODO: Use seperate MSI for notifiying host that SQ has freed up space */
    (void)pcie_interrupt_host(1);
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

    if (Host_Iface_Interrupt_Flag)
    {
        Host_Iface_Interrupt_Flag = false;
        asm volatile("fence");
    }

    /* Scan all SQs for available command */
    for (sq_id = 0; sq_id < MM_SQ_COUNT; sq_id++)
    {
        status = VQ_Data_Avail(&Host_SQs.vqueues[sq_id]);

        if (status == true)
        {
            Log_Write(LOG_LEVEL_DEBUG, "HostIfaceProcessing:Notifying:SQW_IDX:%d\r\n", sq_id);

            /* Dispatch work to SQ Worker associated with this SQ */
            SQW_Notify(sq_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "HostIfaceProcessing:NoData:SQ_IDX:%d\r\n", sq_id);
        }
    }

    /* Scan all HP SQs for available command */
    for (sq_id = 0; sq_id < MM_SQ_HP_COUNT; sq_id++)
    {
        status = VQ_Data_Avail(&Host_SQs_HP.vqueues[sq_id]);

        if (status == true)
        {
            Log_Write(LOG_LEVEL_DEBUG, "HostIfaceProcessing:Notifying:SQW_HP_IDX:%d\r\n", sq_id);

            /* Dispatch work to HP SQ Worker associated with this HP SQ */
            SQW_HP_Notify(sq_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "HostIfaceProcessing:NoData:SQ_HP_IDX:%d\r\n", sq_id);
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
    PLIC_UnregisterHandler(PU_PLIC_PCIE_MESSAGE_INTR_ID);

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
    PLIC_UnregisterHandler(PU_PLIC_PCIE_MESSAGE_INTR_ID);

    return 0;
}
