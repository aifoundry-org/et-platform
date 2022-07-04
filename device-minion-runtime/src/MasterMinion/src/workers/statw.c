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

/*! \def STATW_RECALC_CMA_MIN_MAX(resource, current_sample)
    \brief Helper macro to add new sample into stats. It re-calculates the average, min, and max each time.
    NOTE: It writes into L1 memory.
*/
#define STATW_RECALC_CMA_MIN_MAX(resource, current_sample)                                         \
    /* Calculate commutative moving average. */                                                    \
    resource.avg =                                                                                 \
        (current_sample + (STATW_CMA_SAMPLE_COUNT * resource.avg)) / (STATW_CMA_SAMPLE_COUNT + 1); \
    resource.min = MIN(resource.min, current_sample);                                              \
    resource.max = MAX(resource.max, current_sample);

/*! \def STATW_PMU_REQ_COUNT_TO_MBPS
    \brief Helper macro to convert request count to PMU to MB/Sec.
           It assumes that every request is 64 bytes long. And sampling interval unit is milliseconds.
*/
#define STATW_PMU_REQ_COUNT_TO_MBPS(req_count)                \
    (((req_count)*CACHE_LINE_SIZE * STATW_NUM_OF_MS_IN_SEC) / \
        (STATW_SAMPLING_INTERVAL * STATW_NUM_OF_BYTES_IN_1MB))

/*! \def STATW_SAMPLING_FLAG_SET
    \brief Helper macro to set sampling flag
*/
#define STATW_SAMPLING_FLAG_SET 1U

/*! \def STATW_SAMPLING_FLAG_CLEAR
    \brief Helper macro to clear sampling flag
*/
#define STATW_SAMPLING_FLAG_CLEAR 0U

#define UNUSED_SYSCALL_ARGS 0

/*! \typedef statw_cb
    \brief Device statistics worker control block
*/
typedef struct {
    struct resource_value
        pcie_dma_read_bw; /* Reserve whole cache line for this to reduce the serialization
                                         among Worker Harts reading/writing on same cache line */
    uint64_t pad1[5];
    struct resource_value
        pcie_dma_write_bw; /* Reserve whole cache line for this to reduce the serialization
                                         among Worker Harts reading/writing on same cache line */
    uint64_t pad2[5];
    struct resource_value
        cm_utilization; /* Reserve whole cache line for this to reduce the serialization
                                         among Worker Harts reading/writing on same cache line */
    uint64_t pad3[5];
    uint32_t sampling_flag;
    uint8_t pad[4];
} __attribute__((packed, aligned(8))) statw_cb;

/*! \var STATW_CB
    \brief Global Stat Worker Control Block
    \warning Not thread safe!
*/
static statw_cb STATW_CB __attribute__((aligned(CACHE_LINE_SIZE))) = { 0 };

static void statw_sample_device_stats_callback(uint8_t arg)
{
    (void)arg;

    /* Set the flag to sample device stats. */
    atomic_store_local_32(&STATW_CB.sampling_flag, STATW_SAMPLING_FLAG_SET);
}

/************************************************************************
*
*   FUNCTION
*
*       read_resource_stats_atomically
*
*   DESCRIPTION
*
*       This functions reads back the device stats atomically.
*       It should only used for stats stored global and L2 memory.
*
*   INPUTS
*
*       resource_type   Resource for which new sample is to be added.
*       current_sample  Sample data
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void read_resource_stats_atomically(
    statw_resource_type_e resource_type, struct resource_value *resource_dest)
{
    const struct resource_value *resource;

    if (resource_type == STATW_RESOURCE_CM)
    {
        resource = &STATW_CB.cm_utilization;
    }
    else if (resource_type == STATW_RESOURCE_DMA_READ)
    {
        resource = &STATW_CB.pcie_dma_read_bw;
    }
    else
    {
        resource = &STATW_CB.pcie_dma_write_bw;
    }

    resource_dest->avg = atomic_load_local_64(&resource->avg);
    resource_dest->min = atomic_load_local_64(&resource->min);
    resource_dest->max = atomic_load_local_64(&resource->max);
}

/************************************************************************
*
*   FUNCTION
*
*       statw_init
*
*   DESCRIPTION
*
*       This functions adds new sample for resource utilization data.
*
*   INPUTS
*
*       resource_type   Resource for which new sample is to be added.
*       current_sample  Sample data
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void STATW_Add_New_Sample_Atomically(statw_resource_type_e resource_type, uint64_t current_sample)
{
    struct resource_value *resource;

    if (resource_type == STATW_RESOURCE_CM)
    {
        resource = &STATW_CB.cm_utilization;
    }
    else if (resource_type == STATW_RESOURCE_DMA_READ)
    {
        resource = &STATW_CB.pcie_dma_read_bw;
    }
    else
    {
        resource = &STATW_CB.pcie_dma_write_bw;
    }

    /* Calculate commutative moving average. */
    uint64_t avg = atomic_load_local_64(&resource->avg);
    avg = (current_sample + (STATW_CMA_SAMPLE_COUNT * avg)) / (STATW_CMA_SAMPLE_COUNT + 1);
    atomic_store_local_64(&resource->avg, avg);

    /* TODO: Use ET HW functionality of atomic Min and Max to reduce the number of atomic operations below. */
    uint64_t prev_min = atomic_load_local_64(&resource->min);
    uint64_t prev_max = atomic_load_local_64(&resource->max);
    atomic_store_local_64(&resource->min, MIN(prev_min, current_sample));
    atomic_store_local_64(&resource->max, MAX(prev_max, current_sample));
}

/************************************************************************
*
*   FUNCTION
*
*       statw_init
*
*   DESCRIPTION
*
*       Initialize Device Stat Worker
*
*   INPUTS
*
*       local_stats_cb  Local device stats pointer in L1 memory.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void statw_init(struct compute_resources_sample *local_stats_cb)
{
    atomic_store_local_64(&STATW_CB.cm_utilization.avg, STATW_RESOURCE_DEFAULT_AVG);
    atomic_store_local_64(&STATW_CB.cm_utilization.max, STATW_RESOURCE_DEFAULT_MAX);
    atomic_store_local_64(&STATW_CB.cm_utilization.min, STATW_RESOURCE_DEFAULT_MIN);
    atomic_store_local_64(&STATW_CB.pcie_dma_read_bw.avg, STATW_RESOURCE_DEFAULT_AVG);
    atomic_store_local_64(&STATW_CB.pcie_dma_read_bw.max, STATW_RESOURCE_DEFAULT_MAX);
    atomic_store_local_64(&STATW_CB.pcie_dma_read_bw.min, STATW_RESOURCE_DEFAULT_MIN);
    atomic_store_local_64(&STATW_CB.pcie_dma_write_bw.avg, STATW_RESOURCE_DEFAULT_AVG);
    atomic_store_local_64(&STATW_CB.pcie_dma_write_bw.max, STATW_RESOURCE_DEFAULT_MAX);
    atomic_store_local_64(&STATW_CB.pcie_dma_write_bw.min, STATW_RESOURCE_DEFAULT_MIN);

    local_stats_cb->ddr_read_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->ddr_read_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->ddr_read_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->ddr_write_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->ddr_write_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->ddr_write_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->l2_l3_read_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->l2_l3_read_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->l2_l3_read_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->l2_l3_write_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->l2_l3_write_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->l2_l3_write_bw.min = STATW_RESOURCE_DEFAULT_MIN;
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
    struct compute_resources_sample data_sample = { 0 };
    uint64_t current_counter_value;
    uint64_t prev_ddr_read_counter[NUM_MEM_SHIRES] = { 0 };
    uint64_t prev_ddr_write_counter[NUM_MEM_SHIRES] = { 0 };
    uint64_t prev_l2_l3_read_counter[NUM_SHIRES][NEIGH_PER_SHIRE] = { 0 };
    uint64_t prev_l2_l3_write_counter[NUM_SHIRES][NEIGH_PER_SHIRE] = { 0 };

    statw_init(&data_sample);
    Log_Write(LOG_LEVEL_INFO, "STATW:H[%d]\r\n", hart_id);

    /* Initialize the flag to sample device stats. Set the flag to log first sample at the start. */
    atomic_store_local_32(&STATW_CB.sampling_flag, STATW_SAMPLING_FLAG_SET);

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
        if (atomic_compare_and_exchange_local_32(
                &STATW_CB.sampling_flag, STATW_SAMPLING_FLAG_SET, STATW_SAMPLING_FLAG_CLEAR))
        {
            for (uint64_t shire_id = 0; shire_id < NUM_MEM_SHIRES; shire_id++)
            {
                /* Sample PMC MS Counter 0 and 1 (reads, writes). */
                current_counter_value = (uint64_t)syscall(
                    SYSCALL_PMC_MS_SAMPLE_INT, shire_id, PMU_MS_PMC0, UNUSED_SYSCALL_ARGS);
                STATW_RECALC_CMA_MIN_MAX(data_sample.ddr_read_bw,
                    STATW_PMU_REQ_COUNT_TO_MBPS(
                        current_counter_value - prev_ddr_read_counter[shire_id]))
                prev_ddr_read_counter[shire_id] = current_counter_value;

                current_counter_value = (uint64_t)syscall(
                    SYSCALL_PMC_MS_SAMPLE_INT, shire_id, PMU_MS_PMC1, UNUSED_SYSCALL_ARGS);
                STATW_RECALC_CMA_MIN_MAX(data_sample.ddr_write_bw,
                    STATW_PMU_REQ_COUNT_TO_MBPS(
                        current_counter_value - prev_ddr_write_counter[shire_id]))
                prev_ddr_write_counter[shire_id] = current_counter_value;
            }

            for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id++)
            {
                for (uint64_t neigh_id = 0; neigh_id < NEIGH_PER_SHIRE; neigh_id++)
                {
                    /* Sample PMC SC Counter 0 and 1 (reads, writes). */
                    current_counter_value = (uint64_t)syscall(
                        SYSCALL_PMC_SC_SAMPLE_INT, shire_id, neigh_id, PMU_SC_PMC0);
                    STATW_RECALC_CMA_MIN_MAX(data_sample.l2_l3_read_bw,
                        STATW_PMU_REQ_COUNT_TO_MBPS(
                            current_counter_value - prev_l2_l3_read_counter[shire_id][neigh_id]))
                    prev_l2_l3_read_counter[shire_id][neigh_id] = current_counter_value;

                    current_counter_value = (uint64_t)syscall(
                        SYSCALL_PMC_SC_SAMPLE_INT, shire_id, neigh_id, PMU_SC_PMC1);
                    STATW_RECALC_CMA_MIN_MAX(data_sample.l2_l3_write_bw,
                        STATW_PMU_REQ_COUNT_TO_MBPS(
                            current_counter_value - prev_l2_l3_write_counter[shire_id][neigh_id]))
                    prev_l2_l3_write_counter[shire_id][neigh_id] = current_counter_value;
                }
            }

            /* Read global stats populated by other harts/workers. */
            read_resource_stats_atomically(STATW_RESOURCE_CM, &data_sample.cm_utilization);
            read_resource_stats_atomically(STATW_RESOURCE_DMA_READ, &data_sample.pcie_dma_read_bw);
            read_resource_stats_atomically(
                STATW_RESOURCE_DMA_WRITE, &data_sample.pcie_dma_write_bw);

            Trace_Custom_Event(Trace_Get_MM_Stats_CB(), TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES,
                (const uint8_t *)&data_sample, sizeof(data_sample));

            Trace_Evict_Buffer_MM_Stats();
        }
    }
}