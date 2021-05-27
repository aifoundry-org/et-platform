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
#include "hal_device.h"
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


#define VQUEUE_TASK_PRIORITY 2
#define VQUEUE_STACK_SIZE    256

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
    BaseType_t xHigherPriorityTaskWoken;
    const uint32_t ipi_trigger = *(volatile uint32_t *)R_PU_TRG_PCIE_BASEADDR;
    volatile uint32_t *const ipi_clear_ptr = (volatile uint32_t *)(R_PU_TRG_PCIE_SP_BASEADDR);

    xTaskNotifyFromISR(g_pc_vq_task_handle, ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void vqueue_mm_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    const uint32_t ipi_trigger = *(volatile uint32_t *)R_PU_TRG_MMIN_BASEADDR;
    volatile uint32_t *const ipi_clear_ptr = (volatile uint32_t *)(R_PU_TRG_MMIN_SP_BASEADDR);

    xTaskNotifyFromISR(g_mm_cmd_hdlr_handle, ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

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

static void pc_vq_task(void *pvParameters)
{
    (void)pvParameters;

    static uint8_t buffer[SP_HOST_SQ_MAX_ELEMENT_SIZE] __attribute__((aligned(8))) = { 0 };
    tag_id_t tag_id;
    msg_id_t msg_id;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1) {
        uint32_t notificationValue;

        // ISRs set notification bits per ipi_trigger in case we want them - not currently using them
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

        // Process as many new messages as possible
        while (1) {

            // Pop a command from SP<->PC VQueue
            uint32_t length = SP_Host_Iface_SQ_Pop_Cmd(&buffer);

            // No new messages
            if (length == 0) {
                break;
            }

            // Message with an invalid size
            if ((size_t)length < sizeof(struct cmd_header_t)) {
                Log_Write(LOG_LEVEL_ERROR, "Invalid message: length = %d, min length %ld\r\n", length,
                       sizeof(struct cmd_header_t));
                break;
            }

            const dev_mgmt_cmd_header_t *const hdr = (void *)buffer;
            tag_id = hdr->cmd_hdr.tag_id;
            msg_id = hdr->cmd_hdr.msg_id;
            // Process new message
            switch (msg_id) {
            case DM_CMD_GET_MODULE_MANUFACTURE_NAME:
            case DM_CMD_GET_MODULE_PART_NUMBER:
            case DM_CMD_GET_MODULE_SERIAL_NUMBER:
            case DM_CMD_GET_ASIC_CHIP_REVISION:
            case DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED:
            case DM_CMD_GET_MODULE_REVISION:
            case DM_CMD_GET_MODULE_FORM_FACTOR:
            case DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
            case DM_CMD_GET_MODULE_MEMORY_SIZE_MB:
            case DM_CMD_GET_MODULE_MEMORY_TYPE:
                // Process asset tracking service request cmd
                asset_tracking_process_request(tag_id, msg_id);
                break;
            case DM_CMD_SET_FIRMWARE_UPDATE:
            case DM_CMD_GET_MODULE_FIRMWARE_REVISIONS:
            case DM_CMD_GET_FIRMWARE_BOOT_STATUS:
            case DM_CMD_SET_SP_BOOT_ROOT_CERT:
            case DM_CMD_SET_SW_BOOT_ROOT_CERT:
            case DM_CMD_RESET_ETSOC:
                // Process firmware service request cmd
                firmware_service_process_request(tag_id, msg_id, (void *)buffer);
                break;
            case DM_CMD_GET_MODULE_POWER_STATE:
            case DM_CMD_SET_MODULE_POWER_STATE:
            case DM_CMD_GET_MODULE_STATIC_TDP_LEVEL:
            case DM_CMD_SET_MODULE_STATIC_TDP_LEVEL:
            case DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS:
            case DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS:
            case DM_CMD_GET_MODULE_CURRENT_TEMPERATURE:
            case DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES:
            case DM_CMD_GET_MODULE_POWER:
            case DM_CMD_GET_MODULE_VOLTAGE:
            case DM_CMD_GET_MODULE_UPTIME:
                thermal_power_monitoring_process(tag_id, msg_id, (void *)buffer);
                break;
            case DM_CMD_SET_PCIE_RESET:
            case DM_CMD_SET_PCIE_MAX_LINK_SPEED:
            case DM_CMD_SET_PCIE_LANE_WIDTH:
            case DM_CMD_SET_PCIE_RETRAIN_PHY:
            case DM_CMD_GET_MODULE_PCIE_ECC_UECC:
            case DM_CMD_GET_MODULE_DDR_ECC_UECC:
            case DM_CMD_GET_MODULE_SRAM_ECC_UECC:
            case DM_CMD_GET_MODULE_DDR_BW_COUNTER:
                link_mgmt_process_request(tag_id, msg_id, (void *)buffer);
                break;
            case DM_CMD_SET_DDR_ECC_COUNT:
            case DM_CMD_SET_PCIE_ECC_COUNT:
            case DM_CMD_SET_SRAM_ECC_COUNT:
                // Process set error control cmd
                error_control_process_request(tag_id, msg_id);
                break;
            case DM_CMD_GET_MAX_MEMORY_ERROR:
            case DM_CMD_GET_MODULE_MAX_DDR_BW:
            case DM_CMD_GET_MODULE_MAX_THROTTLE_TIME:
            case DM_CMD_GET_MODULE_MAX_TEMPERATURE:
                historical_extreme_value_request(tag_id, msg_id);
                break;
            case DM_CMD_GET_MM_ERROR_COUNT:
                Minion_State_Host_Iface_Process_Request(tag_id, msg_id);
                break;
            case DM_CMD_GET_ASIC_FREQUENCIES:
            case DM_CMD_GET_DRAM_BANDWIDTH:
            case DM_CMD_GET_DRAM_CAPACITY_UTILIZATION:
            case DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION:
            case DM_CMD_GET_ASIC_UTILIZATION:
            case DM_CMD_GET_ASIC_STALLS:
            case DM_CMD_GET_ASIC_LATENCY:
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
            default:
                Log_Write(LOG_LEVEL_ERROR, "[PC VQ] Invalid message id: %d\r\n", msg_id);
                Log_Write(LOG_LEVEL_ERROR, "message length: %d, buffer:\r\n", length);
                // TODO:
                // Implement error handler
                break;
            }
        }
    }
}

static void mm_cmd_hdlr_task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t buffer[SP_MM_CQ_MAX_ELEMENT_SIZE] __attribute__((aligned(8))) = { 0 };
    struct dev_cmd_hdr_t *hdr;
    int64_t cmd_length = 0;

    /* Disable buffering on stdout */
    setbuf(stdout, NULL);

    while (1)
    {
        uint32_t notificationValue;

        /* ISRs set notification bits per ipi_trigger in case we want them - not currently using them */
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

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
                {
                    struct mm2sp_echo_cmd_t *cmd = (void *)buffer;
                    struct mm2sp_echo_rsp_t rsp;

                    /* Initialize command header */
                    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_ECHO,
                    sizeof(struct mm2sp_echo_rsp_t), cmd->msg_hdr.issuing_hart_id);

                    rsp.payload = cmd->payload;

                    if(0 != MM_Iface_Push_Cmd_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
                    {
                        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_MM2SP_CQ: Cqueue push error!\n");
                    }

                    break;
                }
                case MM2SP_CMD_GET_ACTIVE_SHIRE_MASK:
                {
                    struct mm2sp_get_active_shire_mask_cmd_t *cmd = (void *)buffer;
                    struct mm2sp_get_active_shire_mask_rsp_t rsp;

                    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_GET_ACTIVE_SHIRE_MASK,
                    sizeof(struct mm2sp_get_active_shire_mask_rsp_t),
                    cmd->msg_hdr.issuing_hart_id);

                    rsp.active_shire_mask = Minion_State_MM_Iface_Get_Active_Shire_Mask();

                    if(0 != MM_Iface_Push_Cmd_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
                    {
                        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_MM2SP_CQ: Cqueue push error!\n");
                    }

                    break;
                }
                case MM2SP_CMD_GET_CM_BOOT_FREQ:
                {
                    struct mm2sp_get_cm_boot_freq_cmd_t *cmd = (void *)buffer;
                    struct mm2sp_get_cm_boot_freq_rsp_t rsp;

                    SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, MM2SP_RSP_GET_CM_BOOT_FREQ,
                    sizeof(struct mm2sp_get_cm_boot_freq_rsp_t),
                    cmd->msg_hdr.issuing_hart_id);

                    get_pll_frequency(PLL_ID_SP_PLL_4, &rsp.cm_boot_freq);

                    if(0 != MM_Iface_Push_Cmd_To_MM2SP_CQ((void*)&rsp, sizeof(rsp)))
                    {
                        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Push_Cmd_To_MM2SP_CQ: Cqueue push error!\n");
                    }

                    break;
                }
                case MM2SP_EVENT_REPORT_ERROR:
                {
                    struct mm2sp_report_error_event_t *event = (void *)buffer;

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

                    break;
                }
                case MM2SP_EVENT_HEARTBEAT:
                {
                    struct mm2sp_heartbeat_event_t *event = (void *)buffer;

                    /* TODO: Register watchdog timer for MM->SP heartbeats in MM_Iface_Update_MM_Heartbeat().
                    Whenever a heatbeat is received from MM, SP will compare the minion cycles with
                    the previously saved value, reset WD timer and if the cycles match, we have a hang.
                    Also, if the heartbeat is not received within specified time interval of SP WD timer
                    (error threshold of timer should be incorporated), SP detects a MM hang and resets
                    the MM FW with taking approriate measures */
                    if(MM_Iface_Update_MM_Heartbeat(event->minion_cycles) != 0)
                    {
                        Log_Write(LOG_LEVEL_ERROR, "MM2SP:Valid Heartbeat not found!\r\n");
                    }

                    break;
                }
                default:
                {
                    Log_Write(LOG_LEVEL_ERROR, "[MM VQ] Invalid message id: %" PRIu16 "\r\n", hdr->msg_id);
                    Log_Write(LOG_LEVEL_ERROR, "message length: %" PRIi64 ", buffer:\r\n", cmd_length);
                    break;
                }
            }
        }
    }
}

void sp_intf_init(void)
{
    /* Setup and Initialize the SP -> Host Transport layer*/
    SP_Host_Iface_SQ_Init();
    SP_Host_Iface_CQ_Init();

    /* MM Iface initialization */
    MM_Iface_Init();
}

void launch_command_dispatcher(void)
{
    /* Launch the SP-HOST Command Dispatcher*/
    create_pc_vq_task();
    INT_enableInterrupt(SPIO_PLIC_MBOX_HOST_INTR, 1, vqueue_pcie_isr);

    /* Launch the SP-MM Command Dispatcher*/
    create_mm_cmd_hdlr_task();

    INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1, vqueue_mm_isr);
}



