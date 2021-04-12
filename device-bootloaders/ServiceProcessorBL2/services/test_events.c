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
#include <inttypes.h>
#include <stdio.h>
#include "dm_event_control.h"
#include "sp_host_iface.h"


#ifdef TEST_EVENT_GEN

void start_test_events(tag_id_t tag_id, msg_id_t msg_id)
{
    struct event_message_t message;
    uint64_t req_start_time = timer_get_ticks_count();

    printf("Generating error event\r\n");
    
    /* Generate PCIE Correctable Error */
    FILL_EVENT_HEADER(&message.header, PCIE_CE,
            sizeof(struct event_message_t) );
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 1024, 1, 0);
    pcie_event_callback(CORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate PCIE Un-Correctable Error */
    FILL_EVENT_HEADER(&message.header, PCIE_UCE,
            sizeof(struct event_message_t) );
    FILL_EVENT_PAYLOAD(&message.payload, FATAL, 100, 16, 0);
    pcie_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate DRAM Correctable Error */
    FILL_EVENT_HEADER(&message.header, DRAM_CE,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 100, 1, 0);
    ddr_event_callback(CORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate DRAM Un-Correctable Error */
    FILL_EVENT_HEADER(&message.header, DRAM_UCE,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, FATAL, 100, 1, 0);
    ddr_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate SRAM Correctable Error */
    FILL_EVENT_HEADER(&message.header, SRAM_CE,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 100, 1, 0);
    sram_event_callback(CORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate SRAM UN-Correctable Error */
    FILL_EVENT_HEADER(&message.header, SRAM_UCE,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, FATAL, 100, 1, 0);
    sram_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate Low Temperature Error */
    FILL_EVENT_HEADER(&message.header, THERMAL_LOW,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, WARNING, 1, 50, 0);
    power_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate High Temperature Error */
    FILL_EVENT_HEADER(&message.header, THERMAL_HIGH,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, FATAL, 65, 100, 0);
    power_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate Watchdog Timeout Error */
    FILL_EVENT_HEADER(&message.header, WDOG_TIMEOUT,
            sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 65, 1, 0);
    wdog_timeout_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Generate Thermal Throttling Error */
     FILL_EVENT_HEADER(&message.header, THROTTLE_TIME,
             sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, WARNING, 65, 5000, 0);
    power_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10));     

    /* Generate Minion Exception threshold Error */
     FILL_EVENT_HEADER(&message.header, MINION_EXCEPT_TH,
             sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, WARNING, 20, 3, 0);
    minion_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10)); 

    /* Generate Minion hang threshold Error */
     FILL_EVENT_HEADER(&message.header, MINION_HANG_TH,
             sizeof(struct event_message_t));
    FILL_EVENT_PAYLOAD(&message.payload, WARNING, 80, 2, 0);
    minion_event_callback(UNCORRECTABLE, &message);
    vTaskDelay(pdMS_TO_TICKS(10)); 

    struct device_mgmt_default_rsp_t dm_rsp;

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, 0);

    dm_rsp.payload = 0;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        printf("send_status_response: Cqueue push error!\n");
    }
}

#endif

