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

static TaskHandle_t gs_taskHandle;
static StackType_t gs_stack[VQUEUE_STACK_SIZE];
static StaticTask_t gs_taskPtr;

static void init_task(void);
static void vqueue_task(void *pvParameters);
static void init_vqueue(uint64_t vq_size);

static volatile vqueue_info_intern_t vq_info __attribute__((section(".data")));

static void init_vqueue(uint64_t vq_size)
{
    struct vqueue_info *vq = DEVICE_VQUEUE_BASE;
    void *sbuffer_addr, *cbuffer_addr;

    // Submission Queue start address
    sbuffer_addr = DEVICE_SQUEUE_BASE(vq_size);
    // Completion Queue start address
    cbuffer_addr = DEVICE_CQUEUE_BASE(vq_size);

    // Init the producer and consumer locks
    vq_info.producer_lock = 0U;
    vq_info.consumer_lock = 0U;

    // Query the pre-allocated interrupt vector ID
    vq_info.notify_int = get_service_processor_dev_intf_reg()->sp_vq.interrupt_vector;

    CIRCBUFFER_init(&(vq->sq_header), sbuffer_addr, vq_size);
    CIRCBUFFER_init(&(vq->cq_header), cbuffer_addr, vq_size);
}

static void vqueue_pcie_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    const uint32_t ipi_trigger = *(volatile uint32_t *)R_PU_TRG_PCIE_BASEADDR;
    volatile uint32_t *const ipi_clear_ptr = (volatile uint32_t *)(R_PU_TRG_PCIE_SP_BASEADDR);

    xTaskNotifyFromISR(gs_taskHandle, ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void init_task(void)
{
    // Create Vqueue task
    gs_taskHandle = xTaskCreateStatic(vqueue_task, "SP_Vqueue_Task", VQUEUE_STACK_SIZE,
                                    NULL, VQUEUE_TASK_PRIORITY, gs_stack, &gs_taskPtr);
    if (gs_taskHandle == NULL) {
        printf("xTaskCreateStatic(vqueue_task) failed!\r\n");
    }
}

static void vqueue_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t notificationValue;

    static uint8_t buffer[CIRCBUFFER_SIZE] __attribute__((aligned(8))) = { 0 };

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    // ISRs set notification bits per ipi_trigger in case we want them - not currently using them
    xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

    int64_t length;
    struct cmd_header_t *hdr;

    // Pop command from SP Vqueue 
    length = VQUEUE_pop(SQ_TYPE, SP_VQUEUE_INDEX, buffer, sizeof(buffer));
        
    if (length > 0) {
          hdr = (void *)&buffer[0];

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
            case GET_MODULE_MEMORY_TYPE:{
                // Process asset tracking service request cmd
                // asset_tracking_process_request(mbox, (uint32_t)*message_id);
                break;
            }
            case SET_FIRMWARE_UPDATE:
            case GET_MODULE_FIRMWARE_REVISIONS: 
            case GET_FIRMWARE_BOOT_STATUS:
            case SET_SP_BOOT_ROOT_CERT:
            case SET_SW_BOOT_ROOT_CERT:
            case RESET_ETSOC: {
                // Process firmware service request cmd
                //firmware_service_process_request(mbox, (uint32_t)*message_id, (void *)buffer);
                break;
            }
            default: {
	        // TODO:
	        // Implement error handler
            }
          }
        }
}

void VQUEUE_init(void)
{
    init_vqueue(VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE);
    init_task();
    INT_enableInterrupt(SPIO_PLIC_MBOX_HOST_INTR, 1, vqueue_pcie_isr);
}
