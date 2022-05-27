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
#include <etsoc/drivers/pmu/pmu.h>

/* mm_rt_helpers */
#include "error_codes.h"
#include "syscall_internal.h"

/* mm specific headers */
#include "services/log.h"
#include "services/sw_timer.h"
#include "services/trace.h"
#include "config/mm_config.h"
#include "workers/statw.h"
#include "workers/kw.h"

#define UNUSED_SYSCALL_ARGS 0

/*! \typedef statw_cb
    \brief Device statistics worker control block
*/
typedef struct {
    uint64_t cm_utilization[STATW_CMA_SAMPLE_COUNT];
    uint64_t dma_read_utilization[STATW_CMA_SAMPLE_COUNT];
    uint64_t dma_write_utilization[STATW_CMA_SAMPLE_COUNT];
    uint32_t sampling_flag;
    uint8_t pad[4];
} statw_cb;

/*! \var STATW_CB
    \brief Global Stat Worker Control Block
    \warning Not thread safe!
*/
static statw_cb STATW_CB __attribute__((aligned(CACHE_LINE_SIZE))) = { 0 };

static void statw_sample_device_stats_callback(uint8_t arg)
{
    (void)arg;

    /* Set the flag to sample device stats. */
    atomic_store_local_32(&STATW_CB.sampling_flag, 1);
}

static void calculate_avg_min_max(
    resource_value *resource, const uint64_t *sample_array, uint32_t sample_count)
{
    uint64_t sum = 0;

    if (sample_count > 0)
    {
        /* Initialize min and max with first sample. */
        resource->max = sample_array[0];
        resource->min = sample_array[0];

        /* Calculate sum, min, max on rest of samples */
        for (uint32_t sample = 0; sample < sample_count; sample++)
        {
            sum = sum + sample_array[sample];
            resource->max = MAX(resource->max, sample_array[sample]);
            resource->min = MIN(resource->min, sample_array[sample]);
        }

        resource->avg = sum / sample_count;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       calculate_avg_min_max_atomically
*
*   DESCRIPTION
*
*       This functions is used to get resource utilization stats. It reads
*       the sample data atomically.
*
*   INPUTS
*
*       resource_type       Resource for which new sample is to be added.
*       resource            Place to save stats
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void calculate_avg_min_max_atomically(
    statw_resource_type_e resource_type, resource_value *resource)
{
    uint64_t sum;
    uint64_t current_sample;

    const uint64_t *sample_src;

    /* Get base address of resource data/ */
    if (resource_type == STATW_RESOURCE_CM)
    {
        sample_src = &STATW_CB.cm_utilization[STATW_CMA_SAMPLE_INDEX_START];
    }
    else if (resource_type == STATW_RESOURCE_DMA_READ)
    {
        sample_src = &STATW_CB.dma_read_utilization[STATW_CMA_SAMPLE_INDEX_START];
    }
    else
    {
        sample_src = &STATW_CB.dma_write_utilization[STATW_CMA_SAMPLE_INDEX_START];
    }

    /* Initialize min and max with first sample. */
    resource->max = atomic_load_local_64(&sample_src[STATW_CMA_SAMPLE_INDEX_START]);
    resource->min = resource->max;
    sum = resource->min;

    /* Calculate sum, min, max on rest of samples */
    for (uint32_t sample = STATW_CMA_SAMPLE_INDEX_START + 1; sample < STATW_CMA_SAMPLE_COUNT;
         sample++)
    {
        current_sample = atomic_load_local_64(&sample_src[sample]);
        sum = sum + current_sample;
        resource->max = MAX(resource->max, current_sample);
        resource->min = MIN(resource->min, current_sample);
    }

    resource->avg = sum / STATW_CMA_SAMPLE_COUNT;
}

/************************************************************************
*
*   FUNCTION
*
*       STATW_Add_Resource_Utilization_Sample
*
*   DESCRIPTION
*
*       This functions adds new sample for resource utilization data.
*       It puts data into a circular buffer. It automatically overrides
*       the oldest values in the buffer.
*
*   INPUTS
*
*       resource_type       Resource for which new sample is to be added.
*       current_sample      Sample data
*       index               Index of circular buffer.
*
*
*   OUTPUTS
*
*       uint32_t            Next index of circular buffer.
*
***********************************************************************/
uint32_t STATW_Add_Resource_Utilization_Sample(
    statw_resource_type_e resource_type, uint64_t current_sample, uint32_t index)
{
    uint32_t next_index = index + 1;
    uint64_t *sample_dest;

    /* Get base address of resource data/ */
    if (resource_type == STATW_RESOURCE_CM)
    {
        sample_dest = &STATW_CB.cm_utilization[STATW_CMA_SAMPLE_INDEX_START];
    }
    else if (resource_type == STATW_RESOURCE_DMA_READ)
    {
        sample_dest = &STATW_CB.dma_read_utilization[STATW_CMA_SAMPLE_INDEX_START];
    }
    else
    {
        sample_dest = &STATW_CB.dma_write_utilization[STATW_CMA_SAMPLE_INDEX_START];
    }

    /* Put the current data sample into circular buffer. */
    if (index < STATW_CMA_SAMPLE_INDEX_END)
    {
        atomic_store_local_64(&sample_dest[index], current_sample);
    }
    else if (index == STATW_CMA_SAMPLE_INDEX_END)
    {
        atomic_store_local_64(&sample_dest[STATW_CMA_SAMPLE_INDEX_END], current_sample);
        next_index = STATW_CMA_SAMPLE_INDEX_START;
    }
    else
    {
        /* This will be done only once for first sample after bootup.
           Initialize the the whole array with very first sample.
           This is done to optimize the number of atomic load/stores.*/
        for (uint32_t i = 0; i < STATW_CMA_SAMPLE_COUNT; i++)
        {
            atomic_store_local_64(&sample_dest[i], current_sample);
        }
        next_index = STATW_CMA_SAMPLE_INDEX_START + 1;
    }

    return next_index;
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
    uint64_t ms_pmc0_samples[NUM_MEM_SHIRES];
    uint64_t ms_pmc1_samples[NUM_MEM_SHIRES];
    uint64_t sc_pmc0_samples[NUM_SHIRES][NEIGH_PER_SHIRE];
    uint64_t sc_pmc1_samples[NUM_SHIRES][NEIGH_PER_SHIRE];

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
            for (uint64_t shire_id = 0; shire_id < NUM_MEM_SHIRES; shire_id++)
            {
                /* Sample PMC MS Counter 0 and 1 (PMU_MS_MESH_READS, PMU_MS_MESH_WRITES). */
                ms_pmc0_samples[shire_id] = (uint64_t)syscall(
                    SYSCALL_PMC_MS_SAMPLE_INT, shire_id, PMU_MS_PMC0, UNUSED_SYSCALL_ARGS);
                ms_pmc1_samples[shire_id] = (uint64_t)syscall(
                    SYSCALL_PMC_MS_SAMPLE_INT, shire_id, PMU_MS_PMC1, UNUSED_SYSCALL_ARGS);
            }

            for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id++)
            {
                for (uint64_t neigh_id = 0; neigh_id < NEIGH_PER_SHIRE; neigh_id++)
                {
                    /* Sample PMC SC Counter 0 and 1 (PMU_SC_L2_READS and PMU_SC_L2_WRITES). */
                    sc_pmc0_samples[shire_id][neigh_id] = (uint64_t)syscall(
                        SYSCALL_PMC_SC_SAMPLE_INT, shire_id, neigh_id, PMU_SC_PMC0);
                    sc_pmc1_samples[shire_id][neigh_id] = (uint64_t)syscall(
                        SYSCALL_PMC_SC_SAMPLE_INT, shire_id, neigh_id, PMU_SC_PMC1);
                }
            }

            /* Compute Average, Min, Max of all resources. */
            calculate_avg_min_max(&data_sample.ddr_read_bw, ms_pmc0_samples, NUM_MEM_SHIRES);
            calculate_avg_min_max(&data_sample.ddr_write_bw, ms_pmc1_samples, NUM_MEM_SHIRES);
            calculate_avg_min_max(
                &data_sample.l2_l3_read_bw, &sc_pmc0_samples[0][0], NUM_SHIRES * NEIGH_PER_SHIRE);
            calculate_avg_min_max(
                &data_sample.l2_l3_write_bw, &sc_pmc1_samples[0][0], NUM_SHIRES * NEIGH_PER_SHIRE);
            calculate_avg_min_max_atomically(STATW_RESOURCE_CM, &data_sample.cm_utilization);
            calculate_avg_min_max_atomically(
                STATW_RESOURCE_DMA_READ, &data_sample.pcie_dma_read_bw);
            calculate_avg_min_max_atomically(
                STATW_RESOURCE_DMA_WRITE, &data_sample.pcie_dma_write_bw);

            Trace_Custom_Event(Trace_Get_MM_Stats_CB(), TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES,
                (const uint8_t *)&data_sample, sizeof(data_sample));

            Trace_Evict_Buffer_MM_Stats();
        }
    }
}