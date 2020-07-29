/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#include "esperanto/device-api/device_api.h"
#include "syscall.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "log.h"
#include "layout.h"
#include "hart.h"
#include "cacheops.h"
#include "syscall_internal.h"
#include "message.h"

#if ENABLE_DEVICEFW_TRACE

int trace_init_buffer(void);

int trace_init_master(void)
{
    struct trace_control_region_t *cntrl = (struct trace_control_region_t *)DEVICE_MRT_TRACE_BASE;

    // Enable all groups
    for (unsigned int i = 0; i<(TRACE_GROUPS_MAX/(sizeof(uint64_t) * BITS_PER_BYTE) + 1); i++)
        cntrl->group_knobs[i] = 0xFFFFFFFFFFFFFFFF;

    // Initialize Event knobs
    for (unsigned int i = 0; i<TRACE_EVENT_ID_MAX; i++) {
        cntrl->event_knobs[i].log_level = LOG_LEVELS_TRACE;
        cntrl->event_knobs[i].uart_en = 0;
    }

    // Intialize trace buffer size
    cntrl->buffer_size = DEVICE_MRT_DEFAULT_BUFFER_SIZE;

    // Evict control region
    evict_trace_control();

    // send message to workers
    message_t message;
    message.id = MESSAGE_ID_UPDATE_TRACE_CONTROL;
    broadcast_message_send_master(0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF, &message);

    // Init buffer associated with master
    trace_init_buffer();

    return TRACE_STATUS_SUCCESS;
}

int trace_init_buffer(void)
{
    // Init buffer
    uint64_t hart_id = get_hart_id();
    struct trace_control_region_t *cntrl = (struct trace_control_region_t *)DEVICE_MRT_TRACE_BASE;
    struct buffer_header_t *buf_head = (struct buffer_header_t *)(ALIGN(DEVICE_MRT_TRACE_BASE +
                                        sizeof(struct trace_control_region_t), TRACE_CONTROL_REGION_ALIGNEMNT) + cntrl->buffer_size * hart_id);

    // Init buffer header
    buf_head->hart_id = (uint16_t)hart_id;
    buf_head->head = 0;
    buf_head->tail = 0;
    return TRACE_STATUS_SUCCESS;
}

void evict_trace_buffer(void)
{
    uint64_t hart_id = get_hart_id();
    struct trace_control_region_t *cntrl = (struct trace_control_region_t *)DEVICE_MRT_TRACE_BASE;

    // Get the buffer address for the given hart id
    struct buffer_header_t *buf_head = (struct buffer_header_t *)(ALIGN(DEVICE_MRT_TRACE_BASE +
                                        sizeof(struct trace_control_region_t), TRACE_CONTROL_REGION_ALIGNEMNT) + cntrl->buffer_size * hart_id);

    asm volatile ("fence");
    evict(to_L3, buf_head, cntrl->buffer_size);
    WAIT_CACHEOPS
}

void evict_trace_control(void)
{
    struct trace_control_region_t *cntrl = (struct trace_control_region_t *)DEVICE_MRT_TRACE_BASE;

    // If called by master, it should evict dirty control region cache entries into l3
    // If called by workers, it should invalidate trace control region, assuming it is not updated by worker
    asm volatile ("fence");
    evict(to_L3, cntrl, sizeof(struct trace_control_region_t));
    WAIT_CACHEOPS
}

#endif
