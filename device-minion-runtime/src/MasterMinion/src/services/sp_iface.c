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
/*! \file sp_iface.c
    \brief A C module that implements the Service Processor Interface
    services

    Public interfaces:
        SP_Iface_Init
        SP_Iface_Processing
        SP_Iface_Get_Shire_Mask
        SP_Iface_Get_Boot_Freq
        SP_Iface_Report_Error
*/
/***********************************************************************/
#include "sync.h"
#include "syscall_internal.h"
#include "layout.h"
#include "device-common/hart.h"
#include "riscv_encoding.h"
#include "services/sp_iface.h"
#include "services/sw_timer.h"
#include "services/log.h"
#include "sp_mm_comms_spec.h"

/*! \struct sp_iface_sq_cb_t
    \brief SP interface control block that manages
    submission queue
*/
typedef struct sp_iface_sq_cb_ {
    spinlock_t vq_lock;
    uint32_t timeout_flag;
} sp_iface_sq_cb_t;

/*! \var sp_iface_sq_cb_t SP_SQ_CB
    \brief Global SP to MM submission
    queue interface
    \warning Not thread safe!
*/
static sp_iface_sq_cb_t SP_SQ_CB __attribute__((aligned(64))) = {0};

/* Local prototypes */
static int8_t sp_command_handler(void* cmd_buffer);
static int8_t wait_for_response_from_service_processor(void);

/************************************************************************
*
*   FUNCTION
*
*       sp_iface_mm_heartbeat_cb
*
*   DESCRIPTION
*
*       Callback to generate MM->SP heartbeat after every interval
*
***********************************************************************/
static void sp_iface_mm_heartbeat_cb(uint8_t param)
{
    struct mm2sp_heartbeat_event_t event;
    (void)param;

    /* Initialize event header */
    SP_MM_IFACE_INIT_MSG_HDR(&event.msg_hdr, MM2SP_EVENT_HEARTBEAT,
        sizeof(struct mm2sp_heartbeat_event_t),
        (int32_t) get_hart_id());

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send event to Service Processor */
    if(SP_Iface_Push_Cmd_To_MM2SP_SQ(&event, sizeof(event)) != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing event to MM to SP SQ");
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);
}

/************************************************************************
*
*   FUNCTION
*
*       sp_iface_sp_response_timeout_cb
*
*   DESCRIPTION
*
*       Callback for SP response timer
*
***********************************************************************/
static void sp_iface_sp_response_timeout_cb(uint8_t thread_id)
{
    /* Set the timeout flag */
    atomic_store_local_32(&SP_SQ_CB.timeout_flag, 1);

    /* Trigger IPI to respective hart */
    syscall(SYSCALL_IPI_TRIGGER_INT, (1ull << thread_id), MASTER_SHIRE, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       wait_for_response_from_service_processor
*
*   DESCRIPTION
*
*       Helper to wait for notification events from service processor
*
***********************************************************************/
static int8_t wait_for_response_from_service_processor(void)
{
    uint64_t sip;
    int8_t sw_timer_idx;
    int8_t status = STATUS_SUCCESS;

    /* Create timeout for SP response */
    sw_timer_idx = SW_Timer_Create_Timeout(
        &sp_iface_sp_response_timeout_cb, (get_hart_id() & (HARTS_PER_SHIRE - 1)),
        TIMEOUT_SP_IFACE_RESPONSE);

    /* If the timer was successfully registered */
    if(sw_timer_idx >= 0)
    {
        Log_Write(LOG_LEVEL_DEBUG, "Waiting on SP\r\n");

        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        Log_Write(LOG_LEVEL_DEBUG,
            "Waiting on SP: received an interrupt 0x%" PRIx64 "\r\n", sip);

        if(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* Clear IPI pending interrupt */
            asm volatile("csrc sip, %0" : : "r"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Free the registered SW Timeout slot */
            SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

            /* Check for timeout flag */
            if (atomic_compare_and_exchange_local_32(&SP_SQ_CB.timeout_flag, 1, 0) == 1)
            {
                status = SP_IFACE_SP_RSP_TIMEDOUT;
                Log_Write(LOG_LEVEL_ERROR,
                    "Error:Timed-out waiting for response from SP\r\n");
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR,
                "Error:unexpected interrupt while waiting for IPI from SP\r\n");
        }
    }
    else
    {
        status = SP_IFACE_TIMER_REGISTER_FAILED;
        Log_Write(LOG_LEVEL_ERROR,
            "ERROR: Unable to register SP response timeout!\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       sp_command_handler
*
*   DESCRIPTION
*
*       A local scope helper function to process supported SP to MM
*       commands
*
***********************************************************************/
static int8_t sp_command_handler(void* cmd_buffer)
{
    int8_t status = STATUS_SUCCESS;
    struct dev_cmd_hdr_t *hdr = cmd_buffer;

    Log_Write(LOG_LEVEL_DEBUG, "SP_Command_Handler:hdr:%s%d%s%d%s",
            ":msg_id:",hdr->msg_id,
            ":msg_size:",hdr->msg_size, "\r\n");

    switch (hdr->msg_id)
    {
        case SP2MM_CMD_ECHO:
        {
            struct sp2mm_echo_cmd_t *echo_cmd = (void*) hdr;
            struct sp2mm_echo_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG,
                    "SP_Command_Handler:Echo:%s%d%s%d%s0x%x%s",
                    ":msg_id:",hdr->msg_id,
                    ":msg_size:",hdr->msg_size,
                    ":payload", echo_cmd->payload,"\r\n");

            /* Initialize response header */
            SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, SP2MM_RSP_ECHO,
                sizeof(struct sp2mm_echo_rsp_t), echo_cmd->msg_hdr.issuing_hart_id);

            rsp.payload = echo_cmd->payload;

            status = SP_Iface_Push_Cmd_To_SP2MM_CQ((void*)&rsp, sizeof(rsp));
            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push success!\r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push error!\r\n");
            }

            break;
        }
        case SP2MM_CMD_UPDATE_FREQ:
        {
            struct sp2mm_update_freq_cmd_t *update_active_freq_cmd =
                (void*) hdr;

            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Command_Handler:UpdateActiveFreq:%s%d%s%d%s0x%x%s",
                ":msg_id:",hdr->msg_id,
                ":msg_size:",hdr->msg_size,
                ":payload", update_active_freq_cmd->freq,"\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_TEARDOWN_MM:
        {
            /* struct sp2mm_teardown_mm_cmd_t *teardown_mm_cmd = (void*) hdr; */

            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Command_Handler:TearDownMM:%s%d%s%d%s",
                ":msg_id:",hdr->msg_id,
                ":msg_size:",hdr->msg_size, "\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_QUIESCE_TRAFFIC:
        {
            /* struct sp2mm_quiesce_traffic_cmd_t *quiese_traffic_cmd = (void*) hdr; */

            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Command_Handler:QuieseTraffic:%s%d%s%d%s",
                ":msg_id:",hdr->msg_id,
                ":msg_size:",hdr->msg_size, "\r\n");

            /* Implement functionality here .. */
            break;
        }
        default:
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SP_Command_Handler:UnsupportedCommandID.\r\n");

            break;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Init
*
*   DESCRIPTION
*
*       Initialize the SP interface for communication.
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
int8_t SP_Iface_Init(void)
{
    int8_t status;

    status = SP_MM_Iface_Init();

    if (status == STATUS_SUCCESS)
    {
        /* Initialize the lock for SP SQ */
        init_local_spinlock(&SP_SQ_CB.vq_lock, 0);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Processing
*
*   DESCRIPTION
*
*       Process MM2SP CQ response messages, and SP2MM SQ command messages
*       from Service Processor (SP)
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
int8_t SP_Iface_Processing(void)
{
    static uint8_t cmd_buff[64] __attribute__((aligned(64))) = { 0 };
    uint64_t cmd_length = 0;
    int8_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "SP_Iface_Processing..\r\n");

    do
    {
        cmd_length = (uint64_t) SP_Iface_Pop_Cmd_From_SP2MM_SQ(&cmd_buff[0]);

        if(cmd_length != 0)
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Iface_Processing:cmd_length: 0x%" PRIx64 "\r\n", cmd_length);

            status = sp_command_handler(cmd_buff);
        }
    } while(cmd_length != 0);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Get_Shire_Mask
*
*   DESCRIPTION
*
*       A blocking call to obtain shire mask from SP
*
*   INPUTS
*
*       shire_mask  reference to shire mask to be used as return arg
*
*   OUTPUTS
*
*       int8_t      status success or failure of Interface initialization
*
***********************************************************************/
int8_t SP_Iface_Get_Shire_Mask(uint64_t *shire_mask)
{
    static uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    struct dev_cmd_hdr_t *hdr;
    uint64_t rsp_length = 0;
    struct mm2sp_get_active_shire_mask_cmd_t cmd;
    int8_t status = STATUS_SUCCESS;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, MM2SP_CMD_GET_ACTIVE_SHIRE_MASK,
        sizeof(struct mm2sp_get_active_shire_mask_cmd_t),
        (int32_t) get_hart_id());

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send command to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&cmd, sizeof(cmd));

    if(status == STATUS_SUCCESS)
    {
        /* Wait for response from Service Processor */
        status = wait_for_response_from_service_processor();

        if(status == STATUS_SUCCESS)
        {
            /* Pop response from MM to SP completion queue */
            rsp_length = (uint64_t) SP_Iface_Pop_Cmd_From_MM2SP_CQ(&rsp_buff[0]);

            /* Process response and fetch shire mask */
            if(rsp_length != 0)
            {
                hdr = (void*)rsp_buff;
                if(hdr->msg_id == MM2SP_RSP_GET_ACTIVE_SHIRE_MASK)
                {
                    struct mm2sp_get_active_shire_mask_rsp_t *rsp = (void *)rsp_buff;
                    *shire_mask = rsp->active_shire_mask;
                }
                else
                {
                    status = SP_IFACE_INVALID_SHIRE_MASK;
                    Log_Write(LOG_LEVEL_ERROR,
                        "ERROR: Received unexpected response, for shire mask from SP\r\n");
                }
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "ERROR: Received a notification from SP with no data\r\n");
            }
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing command to MM to SP SQ\r\n");
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Get_Boot_Freq
*
*   DESCRIPTION
*
*       A blocking call to obtain compute minions boot frequency from SP
*
*   INPUTS
*
*       shire_mask  Reference to boot frequency variable to be used as
*                   return arg
*
*   OUTPUTS
*
*       int8_t      status success or failure of Interface initialization
*
***********************************************************************/
int8_t SP_Iface_Get_Boot_Freq(uint32_t *boot_freq)
{
    static uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    struct dev_cmd_hdr_t *hdr;
    uint64_t rsp_length = 0;
    struct mm2sp_get_cm_boot_freq_cmd_t cmd;
    int8_t status = STATUS_SUCCESS;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, MM2SP_CMD_GET_CM_BOOT_FREQ,
        sizeof(struct mm2sp_get_cm_boot_freq_cmd_t),
        (int32_t) get_hart_id());

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send command to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&cmd, sizeof(cmd));

    if(status == STATUS_SUCCESS)
    {
        /* Wait for response from Service Processor */
        status = wait_for_response_from_service_processor();

        if(status == STATUS_SUCCESS)
        {
            /* Pop response from MM to SP completion queue */
            rsp_length = (uint64_t) SP_Iface_Pop_Cmd_From_MM2SP_CQ(&rsp_buff[0]);

            /* Process response and fetch boot frequency */
            if(rsp_length != 0)
            {
                hdr = (void*)rsp_buff;
                if(hdr->msg_id == MM2SP_RSP_GET_CM_BOOT_FREQ)
                {
                    struct mm2sp_get_cm_boot_freq_rsp_t *rsp = (void *)rsp_buff;
                    *boot_freq = rsp->cm_boot_freq;
                }
                else
                {
                    status = SP_IFACE_INVALID_BOOT_FREQ;
                    Log_Write(LOG_LEVEL_ERROR,
                        "ERROR: Received unexpected response, for boot frequency from SP \r\n");
                }
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "ERROR: Received a notification from SP and no data");
            }
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing command to MM to SP SQ\r\n");
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Report_Error
*
*   DESCRIPTION
*
*       A non-blocking call to report error to Service Processor
*
*   INPUTS
*
*       error_type   Type of error
*       error_code   Error code to report
*
*   OUTPUTS
*
*       int8_t       status success or failure
*
***********************************************************************/
int8_t SP_Iface_Report_Error(enum mm2sp_error_type_e error_type, int16_t error_code)
{
    struct mm2sp_report_error_event_t event;
    int8_t status = STATUS_SUCCESS;

    /* Initialize event header */
    SP_MM_IFACE_INIT_MSG_HDR(&event.msg_hdr, MM2SP_EVENT_REPORT_ERROR,
        sizeof(struct mm2sp_report_error_event_t),
        (int32_t) get_hart_id());
    event.error_type = error_type;
    event.error_code = error_code;

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send event to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&event, sizeof(event));
    if(status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing event to MM to SP SQ");
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Setup_MM_HeartBeat
*
*   DESCRIPTION
*
*       This function initializes the MM->SP heartbeat with a periodic
*       timer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t       status success or failure
*
***********************************************************************/
int8_t SP_Iface_Setup_MM_HeartBeat(void)
{
    int8_t sw_timer_idx;
    int8_t status = STATUS_SUCCESS;

    /* Create timer for MM heartbeat */
    /* TODO: Fine tune the heartbeat interval */
    sw_timer_idx = SW_Timer_Create_Timeout(&sp_iface_mm_heartbeat_cb, 0,
        SP_IFACE_MM_HEARTBEAT_INTERVAL);

    /* If the timer was not successfully registered */
    if(sw_timer_idx < 0)
    {
        status = SP_IFACE_TIMER_REGISTER_FAILED;
        Log_Write(LOG_LEVEL_ERROR,
            "ERROR: Unable to register MM->SP Heartbeat Timer!\r\n");
    }

    return status;
}
