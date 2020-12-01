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

#include "broadcast.h"
#include "cacheops.h"
#include "device-mrt-trace.h"
#include "fcc.h"
#include "hart.h"
#include "layout.h"
#include "message.h"
#include "ring_buffer.h"
#include "minion_fw_boot_config.h"

void TRACE_init_master(uint64_t functional_shires)
{
    struct trace_control_t *cntrl =
        (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;
    trace_groups_e grp_dwrd;
    trace_events_e evnt_dwrd;

    // Enable all groups
    for (trace_groups_e i = TRACE_GROUP_ID_NONE + 1UL; i < TRACE_GROUP_ID_LAST;
         i++) {
        grp_dwrd = i / (sizeof(uint64_t) * 8UL);
        cntrl->group_knobs[grp_dwrd] |= 1ULL << i;
    }

    // Initialize Event knobs
    for (trace_events_e i = TRACE_EVENT_ID_NONE + 1UL; i < TRACE_EVENT_ID_LAST;
         i++) {
        evnt_dwrd = i / (sizeof(uint64_t) * 8UL);
        cntrl->event_knobs[evnt_dwrd] |= 1ULL << i;
    }

    // Intialize trace buffer size
    cntrl->buffer_size = DEVICE_MRT_DEFAULT_BUFFER_SIZE;

    // Enable trace
    cntrl->log_level = LOG_LEVELS_TRACE;
    cntrl->uart_en = 0;
    cntrl->trace_en = 1;

    // Set default masks
    cntrl->shire_mask = functional_shires;
    cntrl->harts_mask = DEVICE_MRT_DEFAULT_HARTS_MASK;

    // Evict control region
    TRACE_update_control();

    // Init master's trace buffer
    TRACE_init_buffer();
}

void TRACE_init_worker(void)
{
    // TODO: hack to keep workers waiting until master inits system.
    // Depends on SW-3589. Once done, remove the while loop and also
    // struct trace_control_t *cntrl
    struct trace_control_t *cntrl =
        (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

    // TODO: FIXME: HACK: This assumes there's no garbage in memory!!!!!
    while (cntrl->buffer_size != DEVICE_MRT_DEFAULT_BUFFER_SIZE)
      // Evict control region
      TRACE_update_control();

    // Init worker's trace buffer
    TRACE_init_buffer();
}

void TRACE_init_buffer(void)
{
    // Init buffer
    uint64_t hart_id = get_hart_id();
    struct trace_control_t *cntrl =
        (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

    struct buffer_header_t *buf_head = DEVICE_MRT_BUFFER_BASE(hart_id, cntrl->buffer_size);

    // Init buffer header
    buf_head->hart_id = (uint16_t)hart_id;
    buf_head->head = 0;
    buf_head->tail = 0;
}

void TRACE_evict_buffer(void)
{
    uint64_t hart_id = get_hart_id();
    struct trace_control_t *cntrl =
        (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;
    struct buffer_header_t *buf_head =
        DEVICE_MRT_BUFFER_BASE(hart_id, cntrl->buffer_size);

    asm volatile("fence");
    evict(to_L3, buf_head, cntrl->buffer_size);
    WAIT_CACHEOPS
}

void TRACE_update_control(void)
{
    struct trace_control_t *cntrl =
        (struct trace_control_t *)DEVICE_MRT_TRACE_BASE;

    // If called by master, Evicts dirty control region cache entries into l3
    // If called by workers, Invalidates trace control region, assuming
    // it is not updated by worker
    asm volatile("fence");
    evict(to_L3, cntrl, sizeof(struct trace_control_t));
    WAIT_CACHEOPS
}
