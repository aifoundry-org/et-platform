/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file statw.c
    \brief A C module that implements the device statistics sampler
    worker.

    Public interfaces:
        STATW_Launch

*/
/***********************************************************************/

/* mm_rt_svcs */
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/sync.h>
#include <etsoc/common/common_defs.h>

/* mm_rt_helpers */
#include "error_codes.h"
#include "syscall_internal.h"

/* mm specific headers */
#include "services/log.h"
#include "services/sw_timer.h"
#include "services/trace.h"
#include "config/mm_config.h"
#include "workers/statw.h"

/*! \typedef resource_value
    \brief Device statistics worker control block
*/
typedef struct {
    uint32_t sampling_flag;
    uint8_t pad[4];
} statw_cb;

/*! \var STATW_CB
    \brief Global Stat Worker Control Block
    \warning Not thread safe!
*/
static statw_cb STATW_CB __attribute__((aligned(CACHE_LINE_SIZE))) = { 0 };

/*! \typedef resource_value
    \brief Device statistics sample structure
*/
typedef struct {
    int16_t avg;
    int16_t min;
    int16_t max;
    int16_t current;
} __attribute__((packed, aligned(8))) resource_value;

/*! \typedef compute_resources_sample
    \brief Device computer resource structure
*/
typedef struct {
    resource_value cm_utilization;
    resource_value pcie_dma_bw;
    resource_value ddr_bw;
    resource_value ls_l3_bw;
} __attribute__((packed, aligned(8))) compute_resources_sample;

static void statw_sample_device_stats_callback(uint8_t arg)
{
    (void)arg;

    /* Set the flag to sample device stats. */
    atomic_store_local_32(&STATW_CB.sampling_flag, 1);
}

/************************************************************************
*
*   FUNCTION
*
*       STATW_Launch
*
*   DESCRIPTION
*
*       Initialize Device Stat Workers, used by dispatcher
*
*   INPUTS
*
*       hart_id     HART ID on which the Stat Worker should be launched
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
__attribute__((noreturn)) void STATW_Launch(uint32_t hart_id)
{
    compute_resources_sample dummy_sample;

    Log_Write(LOG_LEVEL_INFO, "STATW:H[%d]\r\n", hart_id);

    /* Initialize the flag to sample device stats. */
    atomic_store_local_32(&STATW_CB.sampling_flag, 0);

    /* Create timeout to wait for all Compute Workers to boot up */
    int sw_timer_idx =
        SW_Timer_Create_Timeout(&statw_sample_device_stats_callback, 0, STATW_SAMPLING_INTERVAL);

    if (sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_WARNING,
            "STATW: Unable to register sampler timer! It may not log device stats.\r\n");
    }

    while (1)
    {
        /* Check the flag to sample device stats. */
        if (atomic_compare_and_exchange_local_32(&STATW_CB.sampling_flag, 1, 0))
        {
            /* Create dummy sample data. */
            dummy_sample = (compute_resources_sample){ .cm_utilization.avg = 1,
                .cm_utilization.min = 2,
                .cm_utilization.max = 3,
                .cm_utilization.current = 4,
                .ddr_bw.avg = 5,
                .ddr_bw.min = 6,
                .ddr_bw.max = 7,
                .ddr_bw.current = 8,
                .ls_l3_bw.avg = 9,
                .ls_l3_bw.min = 8,
                .ls_l3_bw.max = 7,
                .ls_l3_bw.current = 6,
                .pcie_dma_bw.avg = 5,
                .pcie_dma_bw.min = 4,
                .pcie_dma_bw.max = 3,
                .pcie_dma_bw.current = 2 };

            Trace_Custom_Event(Trace_Get_MM_Stats_CB(), MM_TRACE_CUSTOM_ID_COMPUTE_RESOURCES,
                (const uint8_t *)&dummy_sample, sizeof(dummy_sample));

            Trace_Evict_Buffer_MM_Stats();
        }
    }
}