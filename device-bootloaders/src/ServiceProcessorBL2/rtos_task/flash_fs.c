#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "bl2_task_priorities.h"

#include "bl2_flashfs_driver.h"
#include "bl2_flash_fs.h"

#pragma GCC push_options
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define FLASHFS_DRIVER_TASK_STACK_SIZE     4096
#define FLASHFS_DRIVER_REQUEST_QUEUE_SIZE  4
#define FLASHFS_DRIVER_RESPONSE_QUEUE_SIZE 4

typedef enum FLASHFS_DRIVER_REQUEST_e {
    FLASHFS_DRIVER_REQUEST_INVALID,
    FLASHFS_DRIVER_REQUEST_GET_CONFIG_DATA,
    FLASHFS_DRIVER_REQUEST_GET_FILE_SIZE,
    FLASHFS_DRIVER_REQUEST_READ_FILE,
    FLASHFS_DRIVER_REQUEST_RESET_BOOT_COUNTERS,
    FLASHFS_DRIVER_REQUEST_INCREMENT_COMPLETED_BOOT_COUNT,
    _FLASHFS_DRIVER_REQUEST_COUNT_
} FLASHFS_DRIVER_REQUEST_t;

typedef struct FLASHFS_DRIVER_REQUEST_MESSAGE_s {
    uint64_t id;
    FLASHFS_DRIVER_REQUEST_t request;
    union {
        struct {
            void *buffer;
        } get_config_data;
        struct {
            ESPERANTO_FLASH_REGION_ID_t region_id;
        } get_file_size;
        struct {
            ESPERANTO_FLASH_REGION_ID_t region_id;
            uint32_t offset;
            void *buffer;
            uint32_t buffer_size;
        } read_file;
    } args;
} FLASHFS_DRIVER_REQUEST_MESSAGE_t;

typedef struct FLASHFS_DRIVER_RESPONSE_MESSAGE_s {
    uint64_t id;
    FLASHFS_DRIVER_REQUEST_t request;
    int status_code;
    union {
        struct {
            uint32_t size;
        } get_file_size;
    } args;
} FLASHFS_DRIVER_RESPONSE_MESSAGE_t;

static QueueHandle_t gs_flashfs_driver_reqeust_queue;
static uint8_t
    gs_flashfs_driver_reqeust_queue_storage_buffer[FLASHFS_DRIVER_REQUEST_QUEUE_SIZE *
                                                   sizeof(FLASHFS_DRIVER_REQUEST_MESSAGE_t)];
static StaticQueue_t gs_flashfs_driver_reqeust_queue_buffer;

static QueueHandle_t gs_flashfs_driver_response_queue;
static uint8_t
    gs_flashfs_driver_response_queue_storage_buffer[FLASHFS_DRIVER_RESPONSE_QUEUE_SIZE *
                                                    sizeof(FLASHFS_DRIVER_RESPONSE_MESSAGE_t)];
static StaticQueue_t gs_flashfs_driver_response_queue_buffer;

static TaskHandle_t gs_flashfs_driver_task_handle;
static StackType_t gs_flashfs_driver_task_stack_buffer[FLASHFS_DRIVER_TASK_STACK_SIZE];
static StaticTask_t gs_flashfs_driver_task_buffer;

static uint64_t gs_next_request_id;

static uint64_t get_next_request_id(void)
{
    return ++gs_next_request_id;
}

static void flashfs_driver_task(void *pvParameters)
{
    (void)pvParameters;
    FLASHFS_DRIVER_REQUEST_MESSAGE_t req_msg;
    FLASHFS_DRIVER_RESPONSE_MESSAGE_t rsp_msg;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1) {
        if (pdTRUE != xQueueReceive(gs_flashfs_driver_reqeust_queue, &req_msg, portMAX_DELAY)) {
            Log_Write(LOG_LEVEL_ERROR, "flashfs_driver_task:  xQueueReceive() failed!\r\n");
            continue;
        }

        rsp_msg.id = req_msg.id;
        rsp_msg.request = req_msg.request;

        switch (req_msg.request) {
            // case FLASHFS_DRIVER_REQUEST_GET_CONFIG_DATA:
            //     rsp_msg.status_code = flash_fs_get_config_data(req_msg.args.get_config_data.buffer);
            //     break;

        case FLASHFS_DRIVER_REQUEST_GET_FILE_SIZE:
            rsp_msg.status_code = flash_fs_get_file_size(req_msg.args.get_file_size.region_id,
                                                         &rsp_msg.args.get_file_size.size);
            break;

        case FLASHFS_DRIVER_REQUEST_READ_FILE:
            rsp_msg.status_code = flash_fs_read_file(req_msg.args.read_file.region_id,
                                                     req_msg.args.read_file.offset,
                                                     req_msg.args.read_file.buffer,
                                                     req_msg.args.read_file.buffer_size);
            break;

        case FLASHFS_DRIVER_REQUEST_INCREMENT_COMPLETED_BOOT_COUNT:
            rsp_msg.status_code = flash_fs_increment_completed_boot_count();
            break;

        case FLASHFS_DRIVER_REQUEST_RESET_BOOT_COUNTERS:
        default:
            Log_Write(LOG_LEVEL_ERROR, "flashfs_driver_task: invalid or not supported request code %u!\r\n",
                   req_msg.request);
            rsp_msg.status_code = -1;
        }

        if (pdTRUE != xQueueSend(gs_flashfs_driver_response_queue, &rsp_msg, portMAX_DELAY)) {
            Log_Write(LOG_LEVEL_ERROR, "flashfs_driver_task:  xQueueSend() failed!\r\n");
            continue;
        }
    }
}

int flashfs_drv_init(FLASH_FS_BL2_INFO_t *restrict flash_fs_bl2_info,
                     const FLASH_FS_BL1_INFO_t *restrict flash_fs_bl1_info)
{
    gs_next_request_id = 0;

    if (0 != flash_fs_init(flash_fs_bl2_info, flash_fs_bl1_info)) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_init:  flash_fs_init() failed!\r\n");
        return -1;
    }

    gs_flashfs_driver_reqeust_queue = xQueueCreateStatic(
        FLASHFS_DRIVER_REQUEST_QUEUE_SIZE, sizeof(FLASHFS_DRIVER_REQUEST_MESSAGE_t),
        gs_flashfs_driver_reqeust_queue_storage_buffer, &gs_flashfs_driver_reqeust_queue_buffer);
    if (NULL == gs_flashfs_driver_reqeust_queue) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_init:  xQueueCreateStatic(request_queue) failed!\r\n");
        return -1;
    }

    gs_flashfs_driver_response_queue = xQueueCreateStatic(
        FLASHFS_DRIVER_RESPONSE_QUEUE_SIZE, sizeof(FLASHFS_DRIVER_RESPONSE_MESSAGE_t),
        gs_flashfs_driver_response_queue_storage_buffer, &gs_flashfs_driver_response_queue_buffer);
    if (NULL == gs_flashfs_driver_response_queue) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_init:  xQueueCreateStatic(response_queue) failed!\r\n");
        return -1;
    }

    gs_flashfs_driver_task_handle =
        xTaskCreateStatic(flashfs_driver_task, "FLASHFS_DRV_TASK", FLASHFS_DRIVER_TASK_STACK_SIZE,
                          NULL, // pvParameters
                          FLASHFS_DRIVER_TASK_PRIORITY, gs_flashfs_driver_task_stack_buffer,
                          &gs_flashfs_driver_task_buffer);
    if (NULL == gs_flashfs_driver_task_handle) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_init:  xTaskCreateStatic() failed!\r\n");
        return -1;
    }

    return 0;
}

static int queue_request_and_wait_for_response(const FLASHFS_DRIVER_REQUEST_MESSAGE_t *req_msg,
                                               FLASHFS_DRIVER_RESPONSE_MESSAGE_t *rsp_msg)
{
    if (pdTRUE != xQueueSend(gs_flashfs_driver_reqeust_queue, req_msg, portMAX_DELAY)) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  xQueueSend() failed!\r\n");
        return -1;
    }

    if (pdTRUE != xQueueReceive(gs_flashfs_driver_response_queue, rsp_msg, portMAX_DELAY)) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  xQueueSend() failed!\r\n");
        return -1;
    }

    if (req_msg->id != rsp_msg->id) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  id mismatch!\r\n");
        return -1;
    }

    if (req_msg->request != rsp_msg->request) {
        Log_Write(LOG_LEVEL_ERROR, "queue_request_and_wait_for_response:  request mismatch!\r\n");
        return -1;
    }

    return 0;
}

int flashfs_drv_get_config_data(void *buffer)
{
    FLASHFS_DRIVER_REQUEST_MESSAGE_t req;
    FLASHFS_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = FLASHFS_DRIVER_REQUEST_GET_CONFIG_DATA;
    req.args.get_config_data.buffer = buffer;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_config_data: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_config_data: flash_fs_get_config_data() failed!\r\n");
        return -1;
    }

    return 0;
}

int flashfs_drv_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size)
{
    FLASHFS_DRIVER_REQUEST_MESSAGE_t req;
    FLASHFS_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = FLASHFS_DRIVER_REQUEST_GET_FILE_SIZE;
    req.args.get_file_size.region_id = region_id;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_config_data: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_config_data: flash_fs_get_file_size() failed!\r\n");
        return -1;
    }

    *size = rsp.args.get_file_size.size;
    return 0;
}

int flashfs_drv_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                          uint32_t buffer_size)
{
    FLASHFS_DRIVER_REQUEST_MESSAGE_t req;
    FLASHFS_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = FLASHFS_DRIVER_REQUEST_READ_FILE;
    req.args.read_file.region_id = region_id;
    req.args.read_file.offset = offset;
    req.args.read_file.buffer = buffer;
    req.args.read_file.buffer_size = buffer_size;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_config_data: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_config_data: flash_fs_read_file() failed!\r\n");
        return -1;
    }

    return 0;
}

int flashfs_drv_reset_boot_counters(void)
{
    return -1;
}

int flashfs_drv_increment_completed_boot_count(void)
{
    FLASHFS_DRIVER_REQUEST_MESSAGE_t req;
    FLASHFS_DRIVER_RESPONSE_MESSAGE_t rsp;

    req.id = get_next_request_id();
    req.request = FLASHFS_DRIVER_REQUEST_INCREMENT_COMPLETED_BOOT_COUNT;

    if (0 != queue_request_and_wait_for_response(&req, &rsp)) {
        Log_Write(LOG_LEVEL_ERROR, 
            "flashfs_drv_increment_completed_boot_count: queue_request_and_wait_for_response() failed!\r\n");
        return -1;
    }

    if (0 != rsp.status_code) {
        Log_Write(LOG_LEVEL_ERROR, 
            "flashfs_drv_increment_completed_boot_count: flash_fs_increment_completed_boot_count() failed!\r\n");
        return -1;
    }

    return 0;
}

#pragma GCC pop_options
