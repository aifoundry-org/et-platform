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
#include "bl2_timer.h"
#include "vqueue.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

// Pending Device Management header to be generated
//#include "device_mngt_api.h"
#include "dm.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define VQUEUE_TASK_PRIORITY 2
#define VQUEUE_STACK_SIZE    256

static TaskHandle_t g_pc_vq_task_handle;
static StackType_t g_pc_vq_task_stack[VQUEUE_STACK_SIZE];
static StaticTask_t g_pc_vq_task_ptr;

static TaskHandle_t g_mm_vq_task_handle;
static StackType_t g_mm_vq_task_stack[VQUEUE_STACK_SIZE];
static StaticTask_t g_mm_vq_task_ptr;

static void init_pc_vq(uint64_t vq_size);
static void init_mm_vq(void);
static void create_pc_vq_task(void);
static void create_mm_vq_task(void);
static void pc_vq_task(void *pvParameters);
static void mm_vq_task(void *pvParameters);

static vqueue_info_intern_t pc_vq_info __attribute__((section(".data")));
static vqueue_info_intern_t mm_vq_info __attribute__((section(".data")));

static void init_pc_vq(uint64_t vq_size)
{
    struct vqueue_info *vq = DEVICE_VQUEUE_BASE;
    void *sbuffer_addr, *cbuffer_addr;

    // Submission Queue start address
    sbuffer_addr = DEVICE_SQUEUE_BASE(vq_size);
    // Completion Queue start address
    cbuffer_addr = DEVICE_CQUEUE_BASE(vq_size);

    // Init the producer and consumer locks
    pc_vq_info.producer_lock = 0U;
    pc_vq_info.consumer_lock = 0U;

    // Query the pre-allocated interrupt vector ID
    pc_vq_info.notify_int = get_service_processor_dev_intf_reg()->sp_vq.interrupt_vector;

    CIRCBUFFER_init(&(vq->sq_header), sbuffer_addr, vq_size);
    CIRCBUFFER_init(&(vq->cq_header), cbuffer_addr, vq_size);
}

static void init_mm_vq(void)
{
    /* TODO */
    memset(&mm_vq_info, 0, sizeof(mm_vq_info));
}

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

    xTaskNotifyFromISR(g_mm_vq_task_handle, ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void create_pc_vq_task(void)
{
    // Create PCIe VQueue task
    g_pc_vq_task_handle = xTaskCreateStatic(pc_vq_task, "SP_PC_VQueue_Task", VQUEUE_STACK_SIZE,
                                            NULL, VQUEUE_TASK_PRIORITY, g_pc_vq_task_stack,
                                            &g_pc_vq_task_ptr);
    if (g_pc_vq_task_handle == NULL) {
        printf("xTaskCreateStatic(pc_vq_task) failed!\r\n");
    }
}

static void create_mm_vq_task(void)
{
    // Create MM VQueue task
    g_mm_vq_task_handle = xTaskCreateStatic(mm_vq_task, "SP_MM_VQueue_Task", VQUEUE_STACK_SIZE,
                                            NULL, VQUEUE_TASK_PRIORITY, g_mm_vq_task_stack,
                                            &g_mm_vq_task_ptr);
    if (g_mm_vq_task_handle == NULL) {
        printf("xTaskCreateStatic(mm_vq_task) failed!\r\n");
    }
}

static void pc_vq_task(void *pvParameters)
{
    (void)pvParameters;

    static uint8_t buffer[CIRCBUFFER_SIZE] __attribute__((aligned(8))) = { 0 };

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1) {
        uint32_t notificationValue;

        // ISRs set notification bits per ipi_trigger in case we want them - not currently using them
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

        // Process as many new messages as possible
        while (1) {
            // Pop a command from SP<->PC VQueue
            int64_t length = VQUEUE_pop(SQ_TYPE, SP_VQUEUE_INDEX, buffer, sizeof(buffer));

            // No new messages
            if (length <= 0) {
                break;
            }

            // Message with an invalid size
            if ((size_t)length < sizeof(struct cmd_header_t)) {
                printf("Invalid message: length = %" PRId64 ", min length %" PRIu64 "\r\n", length,
                       sizeof(struct cmd_header_t));
                break;
            }

            const struct cmd_header_t *const hdr = (void *)buffer;

            // Process new message
            switch (hdr->cmd_hdr.msg_id) {
            case GET_MODULE_MANUFACTURE_NAME:
            case GET_MODULE_PART_NUMBER:
            case GET_MODULE_SERIAL_NUMBER:
            case GET_ASIC_CHIP_REVISION:
            case GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED:
            case GET_MODULE_REVISION:
            case GET_MODULE_FORM_FACTOR:
            case GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
            case GET_MODULE_MEMORY_SIZE_MB:
            case GET_MODULE_MEMORY_TYPE:
                // Process asset tracking service request cmd
                // asset_tracking_process_request(mbox, (uint32_t)*message_id);
                break;
            case SET_FIRMWARE_UPDATE:
            case GET_MODULE_FIRMWARE_REVISIONS:
            case GET_FIRMWARE_BOOT_STATUS:
            case SET_SP_BOOT_ROOT_CERT:
            case SET_SW_BOOT_ROOT_CERT:
            case RESET_ETSOC:
                // Process firmware service request cmd
                //firmware_service_process_request(mbox, (uint32_t)*message_id, (void *)buffer);
                break;
            default:
                printf("[PC VQ] Invalid message id: %" PRIu16 "\r\n", hdr->cmd_hdr.msg_id);
                printf("message length: %" PRIi64 ", buffer:\r\n", length);
                for (int64_t i = 0; i < length; ++i) {
                    if (i % 8 == 0 && i != 0)
                        printf("\r\n");
                    printf("%02x ", buffer[i]);
                }
                // TODO:
                // Implement error handler
                break;
            }
        }
    }
}

static void mm_vq_task(void *pvParameters)
{
    (void)pvParameters;

    static uint8_t buffer[CIRCBUFFER_SIZE] __attribute__((aligned(8))) = { 0 };

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1) {
        uint32_t notificationValue;

        // ISRs set notification bits per ipi_trigger in case we want them - not currently using them
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

        // Process as many new messages as possible
        while (1) {
            // Pop a command from SP<->MM VQueue
            int64_t length = 0; // TODO: VQUEUE_pop(SP_MM_SQ_TYPE, SP_MM_VQUEUE_INDEX, buffer, sizeof(buffer));

            // No new messages
            if (length <= 0) {
                break;
            }

            // Message with an invalid size
            if ((size_t)length < sizeof(struct cmd_header_t)) {
                printf("Invalid message: length = %" PRId64 ", min length %" PRIu64 "\r\n", length,
                       sizeof(struct cmd_header_t));
                break;
            }

            const struct cmd_header_t *const hdr = (void *)buffer;

            // Process new message
            switch (hdr->cmd_hdr.msg_id) {
            default:
                printf("[MM VQ] Invalid message id: %" PRIu16 "\r\n", hdr->cmd_hdr.msg_id);
                printf("message length: %" PRIi64 ", buffer:\r\n", length);
                for (int64_t i = 0; i < length; ++i) {
                    if (i % 8 == 0 && i != 0)
                        printf("\r\n");
                    printf("%02x ", buffer[i]);
                }
                // TODO:
                // Implement error handler
                break;
            }
        }
    }
}

void VQUEUE_init(void)
{
    init_pc_vq(VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE);
    init_mm_vq();
    create_pc_vq_task();
    create_mm_vq_task();
    INT_enableInterrupt(SPIO_PLIC_MBOX_HOST_INTR, 1, vqueue_pcie_isr);
    INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1, vqueue_mm_isr);
}
