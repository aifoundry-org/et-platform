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
*       MM_Iface_MM_Command_Shell
*       MM_Iface_Send_Echo_Cmd
*       MM_Iface_Get_DRAM_BW
*       MM_Iface_Get_MM_Stats
*       MM_Iface_MM_Stats_Run_Control
*       MM_Iface_Pop_Cmd_From_MM2SP_SQ
*       MM_Iface_Init
*
***********************************************************************/
#include <stdio.h>
#include "log.h"
#include "mm_iface.h"
#include "transports/sp_mm_iface/sp_mm_comms_spec.h"
#include "transports/sp_mm_iface/sp_mm_shared_config.h"
#include "interrupt.h"
#include "semphr.h"
#include "bl_error_code.h"
#include "config/mgmt_build_config.h"

static void mm2sp_notification_isr(void);
static SemaphoreHandle_t mm_cmd_lock = NULL;
static StaticSemaphore_t xSemaphoreBuffer;

static void mm2sp_notification_isr(void)
{
    Log_Write(LOG_LEVEL_INFO, "Received SP_MM_Iface interrupt notification from MM..");
}

static bool mm2sp_wait_for_response(uint32_t timeout_ms)
{
    bool wait_for_rsp = true;
    uint64_t milliseconds_elapsed = 0;

    /* Wait for response from MM. */
    do
    {
        /* Check if response from MM is received. */
        if (SP_MM_Iface_Data_Available(MM_CQ) == true)
        {
            return true;
        }
        else
        {
            Log_Write(LOG_LEVEL_INFO, "MM2SP:Waiting for response...\r\n");
            /* Data not available, suspend the task for 1 ms time. and then check again. */
            vTaskDelay(pdMS_TO_TICKS(1));
            milliseconds_elapsed++;
        }

        /* If timeout was enabled */
        if (timeout_ms)
        {
            wait_for_rsp = milliseconds_elapsed < timeout_ms;
        }
    } while (wait_for_rsp);

    return false;
}

static inline int32_t mm_command_handler_shell_process_rsp(char *rsp, uint32_t *rsp_size,
                                                           uint32_t timeout_ms)
{
    int32_t retval = SUCCESS;

    Log_Write(LOG_LEVEL_INFO, "SP2MM:MM_Iface_MM_Command_Shell: Waiting on response...\r\n");

    /* Wait for response from MM with timeout. */
    if (mm2sp_wait_for_response(timeout_ms))
    {
        /* Get response from MM. */
        retval = MM_Iface_Pop_Rsp_From_SP2MM_CQ(rsp);

        Log_Write(LOG_LEVEL_INFO, "SP2MM:MM_Iface_MM_Command_Shell: Got response size = %d \r\n",
                  retval);

        if (retval <= 0)
        {
            retval = MM_IFACE_SP2MM_INVALID_RESPONSE;
            *rsp_size = 0;
        }
        else
        {
            *rsp_size = (uint32_t)retval;
            retval = SUCCESS;
        }
    }
    else
    {
        retval = MM_IFACE_MM2SP_TIMEOUT_ERROR;

        Log_Write(LOG_LEVEL_INFO,
                  "SP2MM:MM_Iface_MM_Command_Shell:Timed-out waiting for response\r\n");
    }

    return retval;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_MM_Command_Shell
*
*   DESCRIPTION
*
*       This interface allows SP to push MM device api commands to
*       MM FW.
*
*   INPUTS
*
*       cmd         Pointer to MM device-api command
*       cmd_size    Size of MM device-api command
*       rsp         Pointer to receive the command's response
*       rsp_size    Size of reponse received
*       timeout_ms  Timeout for the MM command's response
*       num_of_rsp  Number of responses to expect
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_MM_Command_Shell(const void *cmd, uint32_t cmd_size, char *rsp, uint32_t *rsp_size,
                                  uint32_t timeout_ms, uint8_t num_of_rsp)
{
    int32_t retval = SUCCESS;

    Log_Write(LOG_LEVEL_INFO, "SP2MM:MM_Iface_MM_Command_Shell.\r\n");

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ(cmd, cmd_size))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        while (num_of_rsp--)
        {
            /* Process the response */
            retval = mm_command_handler_shell_process_rsp(rsp, rsp_size, timeout_ms);
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return retval;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Send_Echo_Cmd
*
*   DESCRIPTION
*
*       This sends Echo command to Master Minion. It is a blocking call
*       and it waits for responce for a given time.
*       This is an example to SP2MM command.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_Send_Echo_Cmd(void)
{
    int32_t status = MM_IFACE_SP2MM_CMD_ERROR;
    struct sp2mm_echo_cmd_t cmd;
    struct sp2mm_echo_rsp_t rsp;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_ECHO, sizeof(struct sp2mm_echo_cmd_t),
                             SP2MM_CMD_NOTIFY_HART)

    cmd.payload = 0xDEADBEEF;

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void *)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if (mm2sp_wait_for_response(SP2MM_CMD_TIMEOUT))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Rsp_From_SP2MM_CQ(&rsp);

            if ((status > 0) &&
                ((rsp.msg_hdr.msg_id == SP2MM_RSP_ECHO) && (cmd.payload == rsp.payload)))
            {
                status = SUCCESS;
                Log_Write(LOG_LEVEL_CRITICAL, "SP2MM:ECHO:Response: %x\r\n", rsp.payload);
            }
            else
            {
                status = MM_IFACE_SP2MM_INVALID_RESPONSE;
            }
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Get_DRAM_BW
*
*   DESCRIPTION
*
*       This sends Get DRAM BW command to Master Minion. It is a blocking call
*       and it waits for response for a given time.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_Get_DRAM_BW(uint32_t *read_bw, uint32_t *write_bw)
{
    int32_t status = MM_IFACE_SP2MM_CMD_ERROR;
    struct sp2mm_get_dram_bw_cmd_t cmd;
    struct sp2mm_get_dram_bw_rsp_t rsp;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_GET_DRAM_BW,
                             sizeof(struct sp2mm_get_dram_bw_cmd_t), SP2MM_CMD_NOTIFY_HART)

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void *)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if (mm2sp_wait_for_response(SP2MM_CMD_TIMEOUT))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Rsp_From_SP2MM_CQ(&rsp);

            if ((status > 0) && (rsp.msg_hdr.msg_id == SP2MM_RSP_GET_DRAM_BW))
            {
                *read_bw = rsp.read_bw;
                *write_bw = rsp.write_bw;
                status = SUCCESS;
            }
            else
            {
                status = MM_IFACE_SP2MM_INVALID_RESPONSE;
            }
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Get_MM_Stats
*
*   DESCRIPTION
*
*       This sends Get MM_Stats command to Master Minion. It is a blocking call
*       and it waits for response for a given time.
*
*   INPUTS
*
*       stats         Pointer to receive the mm stats
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_Get_MM_Stats(struct compute_resources_sample *stats)
{
    int32_t status = MM_IFACE_SP2MM_CMD_ERROR;
    struct sp2mm_get_mm_stats_cmd_t cmd;
    struct sp2mm_get_mm_stats_rsp_t rsp;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_GET_MM_STATS,
                             sizeof(struct sp2mm_get_mm_stats_cmd_t), SP2MM_CMD_NOTIFY_HART)

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void *)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if (mm2sp_wait_for_response(SP2MM_CMD_TIMEOUT))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Rsp_From_SP2MM_CQ(&rsp);

            if ((status <= 0) || (rsp.msg_hdr.msg_id != SP2MM_RSP_GET_MM_STATS))
            {
                xSemaphoreGive(mm_cmd_lock);
                return MM_IFACE_SP2MM_INVALID_RESPONSE;
            }

            status = rsp.status;
            if (status != 0)
            {
                Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Get_MM_Stats: response status %d!\r\n",
                          status);
            }
            else
            {
                *stats = rsp.sample;
            }
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_MM_Stats_Run_Control
*
*   DESCRIPTION
*
*       This sends MM_Stats run control command to Master Minion. It is a blocking call
*       and it waits for response for a given time.
*
*   INPUTS
*
*       control		Operation to be performed on MM Stats.
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_MM_Stats_Run_Control(sp2mm_stats_control_e control)
{
    int32_t status = MM_IFACE_SP2MM_CMD_ERROR;
    struct sp2mm_mm_stats_run_control_cmd_t cmd = { 0 };
    struct sp2mm_mm_stats_run_control_rsp_t rsp = { 0 };

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_MM_STATS_RUN_CONTROL, sizeof(cmd),
                             SP2MM_CMD_NOTIFY_HART)
    cmd.control = control;

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void *)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if (mm2sp_wait_for_response(SP2MM_CMD_TIMEOUT))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Rsp_From_SP2MM_CQ(&rsp);

            if ((status <= 0) || (rsp.msg_hdr.msg_id != SP2MM_RSP_MM_STATS_RUN_CONTROL))
            {
                xSemaphoreGive(mm_cmd_lock);
                return MM_IFACE_SP2MM_INVALID_RESPONSE;
            }

            status = rsp.status;
            if (status != 0)
            {
                Log_Write(LOG_LEVEL_ERROR, "MM_Iface_MM_Stats_Run_Control: response status %d!\r\n",
                          status);
            }
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Send_Abort_All_Cmd
*
*   DESCRIPTION
*
*       This sends Get Abort command to Master Minion Firmware.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_Send_Abort_All_Cmd(void)
{
    int32_t status = MM_IFACE_SP2MM_CMD_ERROR;
    struct sp2mm_mm_abort_all_cmd_t cmd;
    struct sp2mm_mm_abort_all_rsp_t rsp;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_MM_ABORT_ALL,
                             sizeof(struct sp2mm_mm_abort_all_cmd_t), SP2MM_CMD_NOTIFY_HART)

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void *)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if (mm2sp_wait_for_response(SP2MM_CMD_TIMEOUT))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Rsp_From_SP2MM_CQ(&rsp);

            if ((status > 0) && (rsp.msg_hdr.msg_id == SP2MM_RSP_MM_ABORT_ALL))
            {
                status = rsp.status;
            }
            else
            {
                status = MM_IFACE_SP2MM_INVALID_RESPONSE;
            }
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Wait_For_CM_Boot_Cmd
*
*   DESCRIPTION
*
*       This sends warm reset to CM and wait for them to boot.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t  Success or error code.
*
***********************************************************************/
int32_t MM_Iface_Wait_For_CM_Boot_Cmd(uint64_t shire_mask)
{
    int32_t status = MM_IFACE_SP2MM_CMD_ERROR;
    struct sp2mm_cm_reset_cmd_t cmd;
    struct sp2mm_cm_reset_rsp_t rsp;

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_CM_RESET, sizeof(struct sp2mm_cm_reset_cmd_t),
                             SP2MM_CMD_NOTIFY_HART)
    cmd.shire_mask = shire_mask;

    if (xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if (0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void *)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if (mm2sp_wait_for_response(SP2MM_CMD_TIMEOUT))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Rsp_From_SP2MM_CQ(&rsp);

            if ((status >= 0) && (rsp.msg_hdr.msg_id == SP2MM_RSP_CM_RESET))
            {
                status = rsp.status;
            }
            else
            {
                status = MM_IFACE_SP2MM_INVALID_RESPONSE;
            }
        }

        xSemaphoreGive(mm_cmd_lock);
    }
    else
    {
        return MM_IFACE_SP2MM_TIMEOUT_ERROR;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Push_Rsp_To_MM2SP_CQ
*
*   DESCRIPTION
*
*       Pushes response to Master Minion (MM) to Service Processor (SP)
*       Completion Queue(CQ)
*
*   INPUTS
*
*       p_rsp       Pointer to cmd buffer
*       rsp_size    Size of the data to push
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t MM_Iface_Push_Rsp_To_MM2SP_CQ(const void *p_rsp, uint32_t rsp_size)
{
    int8_t status;

    /* Enter critical section - Prevents the calling task to not to schedule out.
    Context switching messes the shared VQ regions, hence this is required for now. */
    portENTER_CRITICAL();

    status = SP_MM_Iface_Push(SP_CQ, p_rsp, rsp_size);

    /* Exit critical section */
    portEXIT_CRITICAL();

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Pop_Cmd_From_MM2SP_SQ
*
*   DESCRIPTION
*
*       This function is used to pop a command from MM2SP submission queue.
*
*   INPUTS
*
*       rx_buff    Pointer to rx buffer to copy popped data.
*
*   OUTPUTS
*
*       int32_t    Negative value - error
*                  zero - No Data
*                  Positive value - Number of bytes popped
*
***********************************************************************/
int32_t MM_Iface_Pop_Cmd_From_MM2SP_SQ(void *rx_buff)
{
#ifdef SP_MM_VERIFY_VQ_TAIL
    if (SP_MM_Iface_Verify_Tail(SP_SQ) == SP_MM_IFACE_ERROR_VQ_BAD_TAIL)
    {
        Log_Write(
            LOG_LEVEL_WARNING,
            "MM_Iface_Pop_Cmd_From_MM2SP_SQ:FATAL_ERROR:Tail Mismatch! Using cached value as fallback mechanism\r\n");
    }
#endif
    return SP_MM_Iface_Pop(SP_SQ, rx_buff);
}

/************************************************************************
*
*   FUNCTION
*
*       MM_Iface_Init
*
*   DESCRIPTION
*
*       This function initialise SP to Master Minion interface.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t  Success or error code.
*
***********************************************************************/
int8_t MM_Iface_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    /* Initialize the mutex */
    mm_cmd_lock = xSemaphoreCreateMutexStatic(&xSemaphoreBuffer);
    configASSERT(mm_cmd_lock);

    /* Initialize the interface */
    status = SP_MM_Iface_Init();

    if (status == STATUS_SUCCESS)
    {
        /* Register interrupt handler */
        INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1, mm2sp_notification_isr);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s :  MM Interface initialization failed\n", __func__);
    }
    return status;
}
