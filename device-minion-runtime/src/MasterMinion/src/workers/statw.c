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
#include <system/layout.h>

/* mm_rt_helpers */
#include "error_codes.h"
#include "syscall_internal.h"

/* mm specific headers */
#include "services/log.h"
#include "services/sw_timer.h"
#include "services/trace.h"
#include "config/mm_config.h"
#include "workers/statw.h"

/*! \def MAX(x,y)
    \brief Returns max
*/
#define MAX(x, y) x > y ? x : y

/*! \def MIN(x,y)
    \brief Returns min
*/
#define MIN(x, y) y == 0 ? x : x < y ? x : y

/*! \typedef statw_cb
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
    resource_value ddr_read_bw;
    resource_value ddr_write_bw;
    resource_value ls_l3_read_bw;
    resource_value ls_l3_write_bw;
} __attribute__((packed, aligned(8))) compute_resources_sample;

static void statw_sample_device_stats_callback(uint8_t arg)
{
    (void)arg;

    /* Set the flag to sample device stats. */
    atomic_store_local_32(&STATW_CB.sampling_flag, 1);
}

static void calculate_avg_min_max(
    resource_value *resource, const int16_t *sample_array, uint32_t sample_count)
{
    int64_t sum = 0;

    if (sample_count > 0)
    {
        for (uint32_t sample = 0; sample < sample_count; sample++)
        {
            sum = sum + sample_array[sample];
            resource->max = MAX(resource->max, sample_array[sample]);
            resource->min = MIN(resource->min, sample_array[sample]);
        }

        resource->avg = (int16_t)(sum / sample_count);
        resource->current = sample_array[sample_count];
    }
    else
    {
        resource->max = 0;
        resource->min = 0;
        resource->avg = 0;
        resource->current = 0;
    }
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
    compute_resources_sample data_sample;
    int16_t ms_pmc0_samples[NUM_MEM_SHIRES];
    int16_t ms_pmc1_samples[NUM_MEM_SHIRES];
    int16_t sc_pmc0_samples[NUM_SHIRES][NEIGH_PER_SHIRE];
    int16_t sc_pmc1_samples[NUM_SHIRES][NEIGH_PER_SHIRE];

    Log_Write(LOG_LEVEL_INFO, "STATW:H[%d]\r\n", hart_id);

    /* Initialize the flag to sample device stats. Set the flag to log first sample at the start. */
    atomic_store_local_32(&STATW_CB.sampling_flag, 1);

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
            data_sample = (compute_resources_sample){ .cm_utilization.avg = 1,
                .cm_utilization.min = 2,
                .cm_utilization.max = 3,
                .cm_utilization.current = 4,
                .pcie_dma_bw.avg = 5,
                .pcie_dma_bw.min = 4,
                .pcie_dma_bw.max = 3,
                .pcie_dma_bw.current = 2 };

            for (uint64_t shire_id = 0; shire_id < NUM_MEM_SHIRES; shire_id++)
            {
                /* Sample PMC MS Counter 0 and 1 (PMU_MS_MESH_READS, PMU_MS_MESH_WRITES). */
                ms_pmc0_samples[shire_id] =
                    (int16_t)syscall(SYSCALL_PMC_MS_SAMPLE_INT, shire_id, 0, 0);
                ms_pmc1_samples[shire_id] =
                    (int16_t)syscall(SYSCALL_PMC_MS_SAMPLE_INT, shire_id, 1, 0);
            }

            for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id++)
            {
                for (uint64_t neigh_id = 0; neigh_id < NEIGH_PER_SHIRE; neigh_id++)
                {
                    /* Sample PMC SC Counter 0 and 1 (PMU_SC_L2_READS and PMU_SC_L2_WRITES). */
                    sc_pmc0_samples[shire_id][neigh_id] =
                        (int16_t)syscall(SYSCALL_PMC_SC_SAMPLE_INT, shire_id, neigh_id, 0);
                    sc_pmc1_samples[shire_id][neigh_id] =
                        (int16_t)syscall(SYSCALL_PMC_SC_SAMPLE_INT, shire_id, neigh_id, 1);
                }
            }

            /* Compute Average, Min, Max amongst the Shires/Banks value for Counter 0/1 */
            calculate_avg_min_max(&data_sample.ddr_read_bw, ms_pmc0_samples, NUM_MEM_SHIRES);
            calculate_avg_min_max(&data_sample.ddr_write_bw, ms_pmc1_samples, NUM_MEM_SHIRES);
            calculate_avg_min_max(
                &data_sample.ls_l3_read_bw, &sc_pmc0_samples[0][0], NUM_SHIRES * NEIGH_PER_SHIRE);
            calculate_avg_min_max(
                &data_sample.ls_l3_write_bw, &sc_pmc1_samples[0][0], NUM_SHIRES * NEIGH_PER_SHIRE);

            Trace_Custom_Event(Trace_Get_MM_Stats_CB(), TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES,
                (const uint8_t *)&data_sample, sizeof(data_sample));

            Trace_Evict_Buffer_MM_Stats();
        }
    }
}