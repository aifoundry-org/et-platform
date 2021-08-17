/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "esr_defines.h"
#include "hwinc/hal_device.h"
#include "config/mgmt_build_config.h"
#include "interrupt.h"
#include "pcie_int.h"
#include "bl2_asset_trk.h"
#include "bl2_firmware_update.h"
#include "bl2_thermal_power_monitor.h"
#include "bl2_link_mgmt.h"
#include "bl2_error_control.h"
#include "bl2_historical_extreme.h"
#include "minion_configuration.h"
#include "bl2_perf.h"
#include "bl2_timer.h"
#include "trace.h"
#include "command_dispatcher.h"

#include "sp_host_iface.h"

#include "mm_iface.h"
#include "sp_mm_comms_spec.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "dm.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"

static TaskHandle_t g_pc_vq_task_handle;
static StackType_t g_pc_vq_task_stack[VQUEUE_STACK_SIZE];
static StaticTask_t g_pc_vq_task_ptr;

static TaskHandle_t g_mm_cmd_hdlr_handle;
static StackType_t g_mm_cmd_hdlr_stack[VQUEUE_STACK_SIZE];
static StaticTask_t g_mm_cmd_hdlr_ptr;

static void mm_cmd_hdlr_task(void *pvParameters);
static void pc_vq_task(void *pvParameters);

static void vqueue_pcie_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    const uint32_t ipi_trigger = *(volatile uint32_t *)R_PU_TRG_PCIE_BASEADDR;
    volatile uint32_t *const ipi_clear_ptr = (volatile uint32_t *)(R_PU_TRG_PCIE_SP_BASEADDR);

    *ipi_clear_ptr = ipi_trigger;

    xTaskNotifyFromISR(g_pc_vq_task_handle, ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void vqueue_mm_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    const uint32_t ipi_trigger = *(volatile uint32_t *)R_PU_TRG_MMIN_BASEADDR;
    volatile uint32_t *const ipi_clear_ptr = (volatile uint32_t *)(R_PU_TRG_MMIN_SP_BASEADDR);

    *ipi_clear_ptr = ipi_trigger;

    xTaskNotifyFromISR(g_mm_cmd_hdlr_handle, ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void create_pc_vq_task(void)
{
    /* Create PCIe VQueue task */
    g_pc_vq_task_handle = xTaskCreateStatic(pc_vq_task, "SP_PC_VQueue_Task", VQUEUE_STACK_SIZE,
                                            NULL, VQUEUE_TASK_PRIORITY, g_pc_vq_task_stack,
                                            &g_pc_vq_task_ptr);
    if (g_pc_vq_task_handle == NULL) {
        Log_Write(LOG_LEVEL_ERROR, "xTaskCreateStatic(pc_vq_task) failed!\r\n");
    }
}

static void create_mm_cmd_hdlr_task(void)
{
    /* Create MM_CMD_Handle_Task */
    g_mm_cmd_hdlr_handle = xTaskCreateStatic(mm_cmd_hdlr_task, "MM_CMD_Handle_Task", VQUEUE_STACK_SIZE,
                                            NULL, VQUEUE_TASK_PRIORITY, g_mm_cmd_hdlr_stack,
                                            &g_mm_cmd_hdlr_ptr);
    if (g_mm_cmd_hdlr_handle == NULL) {
        Log_Write(LOG_LEVEL_ERROR, "xTaskCreateStatic(mm_cmd_hdlr_task) failed!\r\n");
    }
}

static inline int8_t pc_vq_process_pending_command(vq_cb_t *vq_cached, vq_cb_t *vq_shared,
    void* shared_mem_ptr, uint64_t vq_used_space)
{
    static uint8_t buffer[SP_HOST_SQ_MAX_ELEMENT_SIZE] __attribute__((aligned(8))) = { 0 };
    tag_id_t tag_id;
    msg_id_t msg_id;
    int8_t status = STATUS_SUCCESS;

    /* Pop a command from SP<->PC VQueue */
    int32_t pop_ret_val =
        VQ_Pop_Optimized(vq_cached, vq_used_space, shared_mem_ptr, buffer);

    /* Update the tail offset in VQ shared memory (SRAM) so that host is able to push new commands */
    SP_Host_Iface_Optimized_SQ_Update_Tail(vq_shared, vq_cached);

    /* Check for valid data */
    if(pop_ret_val > 0)
    {
        const dev_mgmt_cmd_header_t *const hdr = (void *)buffer;
        tag_id = hdr->cmd_hdr.tag_id;
        msg_id = hdr->cmd_hdr.msg_id;

        Log_Write(LOG_LEVEL_INFO, "pc_vq_process_pending_command TagID: %d MsgID:%d\r\n",tag_id, msg_id);

        /* Process new message */
        switch (msg_id)
        {
        case DM_CMD_GET_MODULE_MANUFACTURE_NAME ... DM_CMD_GET_MODULE_MEMORY_TYPE:
            /* Process asset tracking service request cmd */
            asset_tracking_process_request(tag_id, msg_id);
            break;
        case DM_CMD_GET_FUSED_PUBLIC_KEYS ... DM_CMD_SET_FIRMWARE_VALID:
            /* Process firmware service request cmd */
            firmware_service_process_request(tag_id, msg_id, (void *)buffer);
            break;
        case DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS ... DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES:
            thermal_power_monitoring_process(tag_id, msg_id, (void *)buffer);
            break;
        case DM_CMD_SET_PCIE_RESET ... DM_CMD_SET_PCIE_RETRAIN_PHY:
            link_mgmt_process_request(tag_id, msg_id, (void *)buffer);
            break;
        case DM_CMD_SET_DDR_ECC_COUNT ... DM_CMD_SET_SRAM_ECC_COUNT:
            /* Process set error control cmd */
            error_control_process_request(tag_id, msg_id);
            break;
        case DM_CMD_GET_MODULE_MAX_TEMPERATURE ... DM_CMD_GET_MAX_MEMORY_ERROR:
            historical_extreme_value_request(tag_id, msg_id);
            break;
        case DM_CMD_GET_MM_ERROR_COUNT:
            Minion_State_Host_Iface_Process_Request(tag_id, msg_id);
            break;
        case DM_CMD_GET_ASIC_FREQUENCIES ... DM_CMD_GET_ASIC_LATENCY:
            process_performance_request(tag_id, msg_id);
            break;
        case DM_CMD_GET_DEVICE_ERROR_EVENTS:
#ifdef TEST_EVENT_GEN
            start_test_events(tag_id, msg_id);
#endif
            break;
        case DM_CMD_SET_DM_TRACE_RUN_CONTROL:
        case DM_CMD_SET_DM_TRACE_CONFIG:
            Trace_Process_CMD(tag_id, msg_id, (void *)buffer);
            break;
        case DM_CMD_RESET_ETSOC:
            /* Process firmware service request cmd */
            firmware_service_process_request(tag_id, msg_id, (void *)buffer);
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "[PC VQ] Invalid message id: %d\r\n", msg_id);
            Log_Write(LOG_LEVEL_ERROR, "message length: %d, buffer:\r\n", pop_ret_val);
            // TODO:
            // Implement error handler
            break;
        }
    }
    else if(pop_ret_val < 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "pc_vq_task:ERROR:Host cmd processing failed:%d\r\n", pop_ret_val);

        status = GENERAL_ERROR;
    }

    return status;
}

static void pc_vq_task(void *pvParameters)
{
    (void)pvParameters;

    circ_buff_cb_t circ_buff_cached __attribute__((aligned(64)));
    void* shared_mem_ptr;
    vq_cb_t vq_cached;
    uint64_t tail_prev;

    /* Declare a pointer to the VQ control block that is pointing to the VQ attributes in
    global memory and the actual Circular Buffer CB in SRAM (which is shared with host) */
    vq_cb_t *vq_shared = SP_Host_Iface_Get_VQ_Base_Addr(SQ);

    /* Make a copy of the VQ CB in global data to cached L1 stack variable */
    memcpy(&vq_cached, vq_shared, sizeof(vq_cached));

    /* Make a copy of the Circular Buffer CB in shared SRAM to cached L1 stack variable */
    memcpy(&circ_buff_cached, vq_cached.circbuff_cb, sizeof(circ_buff_cached));

    /* Save the shared memory pointer to data buffer */
    shared_mem_ptr = (void*)vq_cached.circbuff_cb->buffer_ptr;

    /* Update the local VQ CB to point to the cached L1 stack variable */
    vq_cached.circbuff_cb = &circ_buff_cached;

    /* Disable buffering on stdout */
    setbuf(stdout, NULL);

    while (1)
    {
        uint32_t notificationValue;

        /* ISRs set notification bits per ipi_trigger in case we want them - not currently using them */
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

        Log_Write(LOG_LEVEL_INFO, "Host_Iface: Received DM request from host.\n");

        /* Get the cached tail pointer */
        tail_prev = VQ_Get_Tail_Offset(&vq_cached);

        /* Refresh the cached VQ CB - Get updated head and tail values from SRAM */
        VQ_Get_Head_And_Tail(vq_shared, &vq_cached);

        /* Verify that the tail value read from memory is equal to previous tail value */
        if(tail_prev != VQ_Get_Tail_Offset(&vq_cached))
        {
            /* If this condition occurs, there's definitely some corruption in VQs */
            Log_Write(LOG_LEVEL_ERROR,
            "pc_vq_task:FATAL_ERROR:Tail Mismatch:Cached: %ld, Shared Memory: %ld Using cached value as fallback mechanism\r\n",
            tail_prev, VQ_Get_Tail_Offset(&vq_cached));

            /* Fallback mechanism: use the cached copy of SQ tail */
            vq_cached.circbuff_cb->tail_offset = tail_prev;
        }

        /* Calculate the total number of bytes available in the VQ */
        uint64_t vq_used_space = VQ_Get_Used_Space(&vq_cached, CIRCBUFF_FLAG_NO_READ);

        /* Process as many new messages as possible */
        while (vq_used_space)
        {
            /* Process the command */
            if(STATUS_SUCCESS !=
                pc_vq_process_pending_command(&vq_cached, vq_shared, shared_mem_ptr, vq_used_space))
            {
                break;
            }

            /* Re-calculate the total number of bytes available in the VQ */
            vq_used_space = VQ_Get_Used_Space(&vq_cached, CIRCBUFF_FLAG_NO_READ);
        }
    }
}

static void mm2sp_echo_cmd_handler(const void *cmd_buffer)
{
    const struct mm2sp_echo_cmd_t *cmd = cmd_buffer;
    struct mm2sp_echo_rsp_t rsp;

    Log_Write(LOG_LEVEL_INFO, "MM2SP_CMD_ECHO: \n");

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_ECHO,
    sizeof(struct mm2sp_echo_rsp_t), cmd->msg_hdr.issuing_hart_id)

    rsp.payload = cmd->payload;

    if(0 != MM_Iface_Push_Rsp_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
    {
        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Rsp_To_MM2SP_CQ: Cqueue push error!\n");
    }
}

static void mm2sp_get_active_shire_mask_cmd_handler(const void *cmd_buffer)
{
    const struct mm2sp_get_active_shire_mask_cmd_t *cmd = cmd_buffer;
    struct mm2sp_get_active_shire_mask_rsp_t rsp;

    Log_Write(LOG_LEVEL_INFO,
        "MM2SP_CMD_GET_ACTIVE_SHIRE_MASK: response going to  MM HART: %d\n",
        cmd->msg_hdr.issuing_hart_id);

    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_GET_ACTIVE_SHIRE_MASK,
    sizeof(struct mm2sp_get_active_shire_mask_rsp_t),
    cmd->msg_hdr.issuing_hart_id)

    rsp.active_shire_mask = Minion_State_MM_Iface_Get_Active_Shire_Mask();

    if(0 != MM_Iface_Push_Rsp_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
    {
        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Rsp_To_MM2SP_CQ: Cqueue push error!\n");
    }
}

static void mm2sp_get_fw_version_cmd_handler(const void *cmd_buffer)
{
    const struct mm2sp_get_fw_version_t *cmd = cmd_buffer;
    struct mm2sp_get_fw_version_rsp_t rsp;

    Log_Write(LOG_LEVEL_INFO, "MM2SP_CMD_GET_FW_VERSION: \n");

    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_GET_FW_VERSION,
    sizeof(struct mm2sp_get_fw_version_rsp_t),
    cmd->msg_hdr.issuing_hart_id)

    Log_Write(LOG_LEVEL_DEBUG, "mm_cmd_hldr: MM requesting FW type: %d\n", cmd->fw_type);

    /* Call the API to get the FW version */
    if(cmd->fw_type == MM2SP_MASTER_MINION_FW)
    {
        Log_Write(LOG_LEVEL_INFO, "MM2SP_MASTER_MINION_FW: \n");

        /* Request firmware service for version */
        firmware_service_get_mm_version(&rsp.major, &rsp.minor, &rsp.revision);

        Log_Write(LOG_LEVEL_DEBUG, "mm_cmd_hldr: MM FW version:major:%d:minor:%d:revision:%d\n",
            rsp.major, rsp.minor, rsp.revision);
    }
    else if(cmd->fw_type == MM2SP_MACHINE_MINION_FW)
    {
        Log_Write(LOG_LEVEL_INFO, "MM2SP_MACHINE_MINION_FW: \n");

        /* Request firmware service for version */
        firmware_service_get_machm_version(&rsp.major, &rsp.minor, &rsp.revision);

        Log_Write(LOG_LEVEL_DEBUG, "mm_cmd_hldr: Machine FW version:major:%d:minor:%d:revision:%d\n",
            rsp.major, rsp.minor, rsp.revision);
    }
    else if(cmd->fw_type == MM2SP_WORKER_MINION_FW)
    {
        Log_Write(LOG_LEVEL_INFO, "MM2SP_WORKER_MINION_FW: \n");

        /* Request firmware service for version */
        firmware_service_get_wm_version(&rsp.major, &rsp.minor, &rsp.revision);

        Log_Write(LOG_LEVEL_DEBUG, "mm_cmd_hldr: Worker FW version:major:%d:minor:%d:revision:%d\n",
            rsp.major, rsp.minor, rsp.revision);
    }
    else
    {
        /* Unknown FW type */
        Log_Write(LOG_LEVEL_ERROR, "mm_cmd_hldr: ERROR:Unknown FW type:%d\n", cmd->fw_type);
    }

    if(0 != MM_Iface_Push_Rsp_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
    {
        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Rsp_To_MM2SP_CQ: Cqueue push error!\n");
    }
}

static void mm2sp_get_cm_boot_freq_cmd_handler(const void *cmd_buffer)
{
    const struct mm2sp_get_cm_boot_freq_cmd_t *cmd = cmd_buffer;
    struct mm2sp_get_cm_boot_freq_rsp_t rsp;

    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_GET_CM_BOOT_FREQ,
    sizeof(struct mm2sp_get_cm_boot_freq_rsp_t),
    cmd->msg_hdr.issuing_hart_id)

    get_pll_frequency(PLL_ID_SP_PLL_4, &rsp.cm_boot_freq);

    if(0 != MM_Iface_Push_Rsp_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
    {
        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Rsp_To_MM2SP_CQ: Cqueue push error!\n");
    }
}

static void mm2sp_report_error_event_handler(const void *cmd_buffer)
{
    const struct mm2sp_report_error_event_t *event = cmd_buffer;

    Log_Write(LOG_LEVEL_INFO, "MM_Error_Reported: Code = %d \r\n", event->error_code);

    if (event->error_type == MM_RECOVERABLE)
    {
        Minion_State_MM_Error_Handler(event->error_code);
    }
    else if (event->error_type == SP_RECOVERABLE)
    {
        /* Restart the Master Minion */
    }
    else
    {
        /* Not supported */
        Log_Write(LOG_LEVEL_ERROR, "MM2SP:Unsupported error type!\r\n");
    }
}

static void mm2sp_heartbeat_event_handler(const void *cmd_buffer)
{
    (void)cmd_buffer;
    /* TODO: SW-8081: Register watchdog timer for MM->SP heartbeats in MM_Iface_Update_MM_Heartbeat().
    Whenever a heatbeat is received from MM, SP will reset WD timer. If the heartbeat is not
    received within specified time interval of SP WD timer (error threshold of timer should
    be incorporated), SP detects a MM hang and resets the MM FW with taking approriate measures */
    if(0 != MM_Iface_Update_MM_Heartbeat())
    {
        Log_Write(LOG_LEVEL_ERROR, "MM2SP:MM Heartbeat update error!\r\n");
    }
}

static void mm_cmd_hdlr_task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t buffer[SP_MM_CQ_MAX_ELEMENT_SIZE] __attribute__((aligned(8))) = { 0 };
    const struct dev_cmd_hdr_t *hdr;
    int64_t cmd_length = 0;

    /* Disable buffering on stdout */
    setbuf(stdout, NULL);

    while (1)
    {
        uint32_t notificationValue;

        /* ISRs set notification bits per ipi_trigger in case we want them - not currently using them */
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

        Log_Write(LOG_LEVEL_INFO, "MM request received: %s\n",__func__);

        /* Process as many new messages as possible */
        while (1)
        {
            /* Pop a command from SP<->MM VQueue */
            cmd_length = MM_Iface_Pop_Cmd_From_MM2SP_SQ(&buffer[0]);

            if(cmd_length == 0)
            {
                break;
            }

            hdr = (void *)&buffer[0];

            /* Process new message */
            switch (hdr->msg_id)
            {
                case MM2SP_CMD_ECHO:
                    mm2sp_echo_cmd_handler(buffer);
                    break;

                case MM2SP_CMD_GET_ACTIVE_SHIRE_MASK:
                    mm2sp_get_active_shire_mask_cmd_handler(buffer);
                    break;

                case MM2SP_CMD_GET_FW_VERSION:
                    mm2sp_get_fw_version_cmd_handler(buffer);
                    break;

                case MM2SP_CMD_GET_CM_BOOT_FREQ:
                    mm2sp_get_cm_boot_freq_cmd_handler(buffer);
                    break;

                case MM2SP_EVENT_REPORT_ERROR:
                    mm2sp_report_error_event_handler(buffer);
                    break;

                case MM2SP_EVENT_HEARTBEAT:
                    mm2sp_heartbeat_event_handler(buffer);
                    break;

                default:
                    Log_Write(LOG_LEVEL_ERROR, "[MM VQ] Invalid message id: %" PRIu16 "\r\n", hdr->msg_id);
                    Log_Write(LOG_LEVEL_ERROR, "message length: %" PRIi64 ", buffer:\r\n", cmd_length);
                    break;
            }
        }
    }
}

void launch_host_sp_command_handler(void)
{
    /* Launch the SP-HOST Command Dispatcher*/
    create_pc_vq_task();
    INT_enableInterrupt(SPIO_PLIC_MBOX_HOST_INTR, 1, vqueue_pcie_isr);
}

void launch_mm_sp_command_handler(void)
{
    /* Launch the SP-MM Command Dispatcher*/
    create_mm_cmd_hdlr_task();

    INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1, vqueue_mm_isr);
}
