/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
*************************************************************************/
/*! \file firmware_update.c
    \brief A C module that implements the master minion state service.

    Public interfaces:
        Minion_State_Host_Iface_Process_Request
*/
/***********************************************************************/

#include "minion_state.h"

/*!
 * @struct struct minion_event_control_block
 * @brief Minion error event mgmt control block
 */
struct minion_event_control_block {
    uint32_t except_count;       /**< Exception error count. */
    uint32_t hang_count;         /**< Hang error count. */
    uint32_t except_threshold;   /**< Exception error count threshold. */
    uint32_t hang_threshold;     /**< Hang error count threshold. */
    uint64_t active_shire_mask;  /**< active shire number mask. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/* The driver can populate this structure with the defaults that will be used during the init
    phase.*/

static struct minion_event_control_block event_control_block __attribute__((section(".data")));

static uint64_t g_active_shire_mask = 0;

static int get_mm_error_count(struct mm_error_count_t *mm_error_count)
{
    // TODO : Get the thread state from MM.
    // https://esperantotech.atlassian.net/browse/SW-6744
    // Currently providing dummy response.
    mm_error_count->hang_count = 0;
    mm_error_count->exception_count = 0;

    return 0;
}

void Minion_State_Init(uint64_t active_shire_mask)
{
    g_active_shire_mask = active_shire_mask;
}

void Minion_State_Host_Iface_Process_Request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time;
    int32_t status;
    req_start_time = timer_get_ticks_count();

    switch (msg_id) {
        case DM_CMD_GET_MM_ERROR_COUNT: {
            struct device_mgmt_mm_state_rsp_t dm_rsp;

            status = get_mm_error_count(&dm_rsp.mm_error_count);

            if (0 != status) {
                Log_Write(LOG_LEVEL_ERROR, " mm state svc error: get_mm_error_count()\r\n");
            }

            FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MM_ERROR_COUNT,
                            timer_get_ticks_count() - req_start_time, status);

            if (0 !=
                SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_mm_state_rsp_t))) {
                Log_Write(LOG_LEVEL_ERROR, "Minion_State_Host_Iface_Process_Request: Cqueue push error!\n");
            }
            break;
        }
    }
}

void Minion_State_MM_Iface_Process_Request(uint16_t msg_id)
{
    switch (msg_id) {
        case MM2SP_CMD_GET_ACTIVE_SHIRE_MASK: {
            struct mm2sp_get_active_shire_mask_rsp_t rsp;
            rsp.msg_hdr.msg_id = MM2SP_RSP_GET_ACTIVE_SHIRE_MASK;
            rsp.active_shire_mask = (uint64_t)(g_active_shire_mask & 0xFFFFFFFF); // Compute shires

            if(0 != MM_Iface_Push_Cmd_To_MM2SP_CQ((char *)&rsp, sizeof(rsp)))
            {
                Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_MM2SP_CQ: Cqueue push error!\n");
            }
            break;
        }

        case MM2SP_CMD_REPORT_EXCEPTION_ERROR: {
            
            /* update error count for Exception type */
            minion_error_update_count(EXCEPTION);
            break;
        }
        case MM2SP_CMD_REPORT_HANG_ERROR: {

            /* update error count for Hang type */
            minion_error_update_count(HANG);
            break;
        }    
    }
}


int32_t minion_error_control_init(dm_event_isr_callback event_cb)
{
    /* register event callback */
    event_control_block.event_cb = event_cb;

    /* set default thershold values */
    event_control_block.except_threshold = 5;
    event_control_block.hang_threshold = 1;

    return 0;
}

int32_t minion_error_control_deinit(void)
{
    event_control_block.event_cb = NULL;
    return 0;
}

int32_t minion_set_except_threshold(uint32_t th_value)
{
    /* set errors count threshold */
    event_control_block.except_threshold = th_value;
    return 0;
}

int32_t minion_set_hang_threshold(uint32_t th_value)
{
    /* set errors count threshold */
    event_control_block.hang_threshold = th_value;
    return 0;
}

int32_t minion_get_except_err_count(uint32_t *err_count)
{
    /* get exceptionerrors count */
    *err_count = event_control_block.except_count;
    return 0;
}

int32_t minion_get_hang_err_count(uint32_t *err_count)
{
    /* get hang errors count */
    *err_count = event_control_block.hang_count;
    return 0;
}

void minion_error_update_count(uint8_t error_type)
{
    // TODO: This is just an example implementation.
    // The final driver implementation will read these values from the
    // hardware, create a message and invoke call back with message and error type as parameters.
    
    struct event_message_t message;

    switch (error_type)
    {
        case EXCEPTION:
            if(++event_control_block.except_count > event_control_block.except_threshold) {
                
                /* add details in message header and fill payload */
                FILL_EVENT_HEADER(&message.header, MINION_EXCEPT_TH,
                                    sizeof(struct event_message_t));
                FILL_EVENT_PAYLOAD(&message.payload, WARNING, 1024, 1, 0);

                /* call the callback function and post message */
                event_control_block.event_cb(CORRECTABLE, &message);                                          
            }
            break;
        
        case HANG:
            if(++event_control_block.hang_count > event_control_block.hang_threshold) {

                /* add details in message header and fill payload */
                FILL_EVENT_HEADER(&message.header, MINION_HANG_TH,
                                    sizeof(struct event_message_t));
                FILL_EVENT_PAYLOAD(&message.payload, WARNING, 1020, 1, 0); 

                /* call the callback function and post message */
                event_control_block.event_cb(CORRECTABLE, &message);
            }

        default:
            break;
        }
}
