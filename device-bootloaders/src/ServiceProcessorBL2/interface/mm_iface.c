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
*       MM_Iface_Init
*       MM_Iface_Send_Echo_Cmd
*       MM_Iface_Update_MM_Heartbeat
*
***********************************************************************/
#include <stdio.h>
#include "log.h"
#include "mm_iface.h"
#include "sp_mm_comms_spec.h"
#include "sp_mm_shared_config.h"
#include "interrupt.h"
#include "semphr.h"
#include "error.h"
#include "config/mgmt_build_config.h"

typedef struct mm_iface_cb_ {
    bool mm_wd_initialized;
} mm_iface_cb_t;

static mm_iface_cb_t mm_iface_cb_g __attribute__((aligned(64))) = {0};

static void mm2sp_notification_isr(void);
static SemaphoreHandle_t mm_cmd_lock = NULL;
static StaticSemaphore_t xSemaphoreBuffer;

static void mm2sp_notification_isr(void)
{
    Log_Write(LOG_LEVEL_INFO, "Received SP_MM_Iface interrupt notification from MM..");
}

static bool mm2sp_wait_for_response(bool enable_timeout)
{
    bool wait_for_rsp = true;
    TickType_t start_tick = xTaskGetTickCount();

    /* Wait for response from MM. */
    do
    {
        /* Check if response from MM is received. */
        if(SP_MM_Iface_Data_Available(SP_CQ) == true)
        {
            return true;
        }
        else
        {
            /* Data not available, suspend the task for a fix time. and then check again.
                This suspension time (in ticks) is a temporary number can be made
                dynamic based on caller timeout. */
            vTaskDelay(50);
        }

        /* If timeout was enabled */
        if(enable_timeout)
        {
            wait_for_rsp = (xTaskGetTickCount() - start_tick) < SP2MM_CMD_TIMEOUT;
        }
    } while(wait_for_rsp);

    return false;
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
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, SP2MM_CMD_ECHO,
    sizeof(struct sp2mm_echo_cmd_t), SP2MM_CMD_NOTIFY_HART)

    cmd.payload = 0xDEADBEEF;

    if(xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if(0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void*)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if(mm2sp_wait_for_response(true))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Cmd_From_SP2MM_CQ(&rsp);

            if ((status > 0) &&
                ((rsp.msg_hdr.msg_id == SP2MM_RSP_ECHO) && (cmd.payload == rsp.payload)))
            {
                status = ERROR_NO_ERROR;
                Log_Write(LOG_LEVEL_ERROR, "SP2MM:ECHO:Response: %x\r\n", rsp.payload);
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

    if(xSemaphoreTake(mm_cmd_lock, SP2MM_CMD_TIMEOUT) == pdTRUE)
    {
        /* Send command to MM. */
        if(0 != MM_Iface_Push_Cmd_To_SP2MM_SQ((void*)&cmd, sizeof(cmd)))
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_SP2MM_SQ: CQ push error!\r\n");
            xSemaphoreGive(mm_cmd_lock);
            return MM_IFACE_SP2MM_CMD_PUSH_ERROR;
        }

        /* Wait for response from MM with default timeout. */
        if(mm2sp_wait_for_response(true))
        {
            /* Get response from MM. */
            status = MM_Iface_Pop_Cmd_From_SP2MM_CQ(&rsp);

            if ((status > 0) &&
                (rsp.msg_hdr.msg_id == SP2MM_RSP_GET_DRAM_BW))
            {
                *read_bw = rsp.read_bw;
                *write_bw = rsp.write_bw;
                status = ERROR_NO_ERROR;
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
*       MM_Iface_Update_MM_Heartbeat
*
*   DESCRIPTION
*
*       This function updates and checks the MM heartbeat for its
*       correctness.
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
int8_t MM_Iface_Update_MM_Heartbeat(void)
{
    int8_t status = STATUS_SUCCESS;

    /* First time we get a heartbeat, we register watchdog timer */
    if (!mm_iface_cb_g.mm_wd_initialized)
    {
        /* TODO: SW-8081: Watchdog initialization here. Also register a callback to reset MM FW */
        mm_iface_cb_g.mm_wd_initialized = true;
    }
    else
    {
        /* TODO: SW-8081: Re-kick the watchdog timer here */
    }

    return status;
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
    configASSERT( mm_cmd_lock );

    /* Initialize the interface */
    status = SP_MM_Iface_Init();

    if (status == STATUS_SUCCESS)
    {
        /* Reset MM heartbeat WD init flag */
        mm_iface_cb_g.mm_wd_initialized = false;
    }

    /* Register interrupt handler */
    INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1,
        mm2sp_notification_isr);

    return status;
}
