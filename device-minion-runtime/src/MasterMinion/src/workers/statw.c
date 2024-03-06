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
    Design notes:
    The stats are logged from host's perspective. That means, the device's
    DMA read is DMA write and DMA write is DMA read from host's perspective.

    Public interfaces:
        STATW_Launch
        STATW_Get_Minion_Freq
        STATW_Get_MM_Stats
        STATW_Reset_MM_Stats
        STATW_Add_New_Sample_Atomically

*/
/***********************************************************************/

#include <math.h>

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
#include "services/sp_iface.h"
#include "config/mm_config.h"
#include "workers/statw.h"
#include "workers/kw.h"
#include "workers/dmaw.h"

/*! \def STATW_RECALC_MIN_MAX(resource, current_sample)
    \brief It re-calculates the min, and max each time.
    NOTE: Uses local writes.
*/
#define STATW_RECALC_MIN_MAX(resource, current_sample) \
    resource.min = MIN(resource.min, current_sample);  \
    resource.max = MAX(resource.max, current_sample);

/*! \def STATW_RECALC_CMA_MIN_MAX(resource, current_sample, sample_count)
    \brief Helper macro to add new sample into stats. It re-calculates the average, min, and max each time.
    NOTE: Uses local writes.
*/
#define STATW_RECALC_CMA_MIN_MAX(resource, current_sample, sample_count)              \
    /* Calculate commutative moving average. */                                       \
    resource.avg = statw_recalculate_cma(resource.avg, current_sample, sample_count); \
    STATW_RECALC_MIN_MAX(resource, current_sample)

/*! \def STATW_PMU_REQ_COUNT_TO_MBPS
    \brief Helper macro to convert request count to PMU to MB/Sec.
    Every request is 64 bytes long.
*/
#define STATW_PMU_REQ_COUNT_TO_MBPS(req_count, count_freq_mhz, cycles_consumed) \
    (cycles_consumed == 0) ? 0 :                                                \
                             (((req_count)*CACHE_LINE_SIZE * count_freq_mhz) / (cycles_consumed))

/*! \def STATW_PMU_RESET_ON_OVERFLOW(overflow, prev_val)
    \brief Macro that checks for overflow in PMC value and resets the previous value.
*/
#define STATW_PMU_RESET_ON_OVERFLOW(overflow, prev_val) \
    {                                                   \
        if (overflow)                                   \
        {                                               \
            prev_val = 0;                               \
        }                                               \
    }

/*! \def STATW_UTIL_BAD_PMC_CHECK(util_type_str, shire_id, bank_id, prev_val, curr_val)
    \brief Macro that checks for bad PMC values.
*/
#define STATW_UTIL_BAD_PMC_CHECK(util_type_str, shire_id, bank_id, prev_val, curr_val)                               \
    {                                                                                                                \
        if (prev_val > curr_val)                                                                                     \
        {                                                                                                            \
            Log_Write(LOG_LEVEL_DEBUG,                                                                               \
                "statw: %s: shire: %ld: bank: %ld: Previous greater than current: prev_val: %ld: curr_val: %ld\r\n", \
                util_type_str, shire_id, bank_id, prev_val, curr_val);                                               \
            /* TODO: For now, just make both values equal. Need to check and fix this case. */                       \
            curr_val = prev_val;                                                                                     \
        }                                                                                                            \
    }

/*! \def CHECK_BIT_SET(data, pos)
    \brief Macro that checks for set bits in mask with position.
*/
#define CHECK_BIT_SET(data, pos) (data & (1U << pos))

/*! \def STATW_SAMPLING_FLAG_SET
    \brief Helper macro to set sampling flag
*/
#define STATW_SAMPLING_FLAG_SET 1U

/*! \def STATW_SAMPLING_FLAG_CLEAR
    \brief Helper macro to clear sampling flag
*/
#define STATW_SAMPLING_FLAG_CLEAR 0U

/*! \def STATW_PERCENTAGE_MULTIPLIER
    \brief Multiplier to convert percentage from float to a whole decimal numbers.
    WARNING: When this is 100 or less, then max perectage will go to 100.
*/
#define STATW_PERCENTAGE_MULTIPLIER 100

/*! \def UNUSED_SYSCALL_ARGS
    \brief Dummy value for unused arguments for a syscall.
*/
#define UNUSED_SYSCALL_ARGS 0

/*! \def BANKS_PER_SC
    \brief Number of memory banks per shire.
*/
#define BANKS_PER_SC 4

/*! \def DDR_FREQUENCY
    \brief DDR frequency.
    TODO: add a new MM2SP command to query this frequency.
*/
#define DDR_FREQUENCY 933UL

/*! \def STATW_PMU_SAMPLING_STATE_TIMEOUT
    \brief Timeout value for PMU state change.
*/
#define STATW_PMU_SAMPLING_STATE_TIMEOUT 100

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
    struct resource_value cm_bw; /* Reserve whole cache line for this to reduce the serialization
                                         among Worker Harts reading/writing on same cache line */
    uint64_t pad3[5];
    uint64_t saved_trace_entry;
    uint32_t sampling_flag;
    uint32_t reset_sampling_flag;
    uint32_t minion_freq_mhz;
    uint32_t pmu_sampling_state;
    uint32_t pmu_sampling_timeout_flag;
} __attribute__((packed, aligned(8))) statw_cb;

/*! \typedef pmc_prev_counters
    \brief Struct to hold previous counter values for PMU
*/
typedef struct {
    uint64_t prev_ddr_cycle_counter[NUM_MEM_SHIRES];
    uint64_t prev_ddr_read_counter[NUM_MEM_SHIRES];
    uint64_t prev_ddr_write_counter[NUM_MEM_SHIRES];
    uint64_t prev_l2_l3_cycle_counter[NUM_SHIRES][BANKS_PER_SC];
    uint64_t prev_l2_l3_read_counter[NUM_SHIRES][BANKS_PER_SC];
    uint64_t prev_l2_l3_write_counter[NUM_SHIRES][BANKS_PER_SC];
} __attribute__((packed, aligned(8))) pmc_prev_counters;

typedef struct {
    shire_pmc_cnt_t ms_pmcs[NUM_MEM_SHIRES];
    shire_pmc_cnt_t avg_ms_pmcs;
    shire_pmc_cnt_t sc_pmcs[NUM_SHIRES][BANKS_PER_SC];
    shire_pmc_cnt_t avg_sc_pmcs;
} pmc_current_counters;

/*! \var STATW_CB
    \brief Global Stat Worker Control Block
    \warning Not thread safe!
*/
static statw_cb STATW_CB __attribute__((aligned(CACHE_LINE_SIZE))) = { 0 };

static inline uint64_t statw_recalculate_cma(
    uint64_t old_value, uint64_t current_value, uint64_t sample_count)
{
    if (current_value > old_value)
    {
        double cma_temp = ceil(((double)current_value + (double)(old_value * (sample_count - 1))) /
                               (double)sample_count);
        return (uint64_t)cma_temp;
    }
    else if (current_value < old_value)
    {
        double cma_temp = floor(((double)current_value + (double)(old_value * (sample_count - 1))) /
                                (double)sample_count);
        return (uint64_t)cma_temp;
    }
    return old_value;
}

static void statw_sample_pmc_counters(uint64_t shire_mask, pmc_current_counters *pmc_cur)
{
    for (uint64_t shire_id = 0; shire_id < NUM_MEM_SHIRES; shire_id++)
    {
        /* Sample PMC MS Counter 0 and 1 (reads, writes). */
        syscall(SYSCALL_PMC_MS_SAMPLE_ALL_INT, shire_id, (uintptr_t) & (pmc_cur->ms_pmcs[shire_id]),
            UNUSED_SYSCALL_ARGS);
    }

    for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id++)
    {
        if (!CHECK_BIT_SET(shire_mask, shire_id))
        {
            continue;
        }

        for (uint64_t bank_id = 0; bank_id < BANKS_PER_SC; bank_id++)
        {
            /* Sample PMC SC Counter 0 and 1 (reads, writes). */
            syscall(SYSCALL_PMC_SC_SAMPLE_ALL_INT, shire_id, bank_id,
                (uintptr_t) & (pmc_cur->sc_pmcs[shire_id][bank_id]));
        }
    }
}

static uint64_t statw_calculate_average(uint64_t new_value, uint64_t average, uint64_t samples)
{
    double avg =
        round(((double)samples * (double)average + (double)new_value) / (double)(samples + 1));
    return (uint64_t)avg;
}

static void statw_process_pmc_counters(
    uint64_t shire_mask, pmc_prev_counters *pmc_cnt, pmc_current_counters *pmc_cur)
{
    for (uint64_t shire_id = 0; shire_id < NUM_MEM_SHIRES; shire_id++)
    {
        /* Check for overflow */
        STATW_PMU_RESET_ON_OVERFLOW(
            pmc_cur->ms_pmcs[shire_id].cycle_overflow, pmc_cnt->prev_ddr_cycle_counter[shire_id])
        STATW_PMU_RESET_ON_OVERFLOW(
            pmc_cur->ms_pmcs[shire_id].pmc0_overflow, pmc_cnt->prev_ddr_read_counter[shire_id])
        STATW_PMU_RESET_ON_OVERFLOW(
            pmc_cur->ms_pmcs[shire_id].pmc1_overflow, pmc_cnt->prev_ddr_write_counter[shire_id])

        pmc_cur->avg_ms_pmcs.cycle = statw_calculate_average(
            pmc_cur->ms_pmcs[shire_id].cycle - pmc_cnt->prev_ddr_cycle_counter[shire_id],
            pmc_cur->avg_ms_pmcs.cycle, shire_id);
        pmc_cur->avg_ms_pmcs.pmc0 = statw_calculate_average(
            pmc_cur->ms_pmcs[shire_id].pmc0 - pmc_cnt->prev_ddr_read_counter[shire_id],
            pmc_cur->avg_ms_pmcs.pmc0, shire_id);
        pmc_cur->avg_ms_pmcs.pmc1 = statw_calculate_average(
            pmc_cur->ms_pmcs[shire_id].pmc1 - pmc_cnt->prev_ddr_write_counter[shire_id],
            pmc_cur->avg_ms_pmcs.pmc1, shire_id);

        /* Update the previous values */
        pmc_cnt->prev_ddr_cycle_counter[shire_id] = pmc_cur->ms_pmcs[shire_id].cycle;
        pmc_cnt->prev_ddr_read_counter[shire_id] = pmc_cur->ms_pmcs[shire_id].pmc0;
        pmc_cnt->prev_ddr_write_counter[shire_id] = pmc_cur->ms_pmcs[shire_id].pmc1;
    }

    unsigned int cnt = 0;
    for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id++)
    {
        if (!CHECK_BIT_SET(shire_mask, shire_id))
        {
            continue;
        }

        for (uint64_t bank_id = 0; bank_id < BANKS_PER_SC; bank_id++)
        {
            /* Check for overflow */
            STATW_PMU_RESET_ON_OVERFLOW(pmc_cur->sc_pmcs[shire_id][bank_id].cycle_overflow,
                pmc_cnt->prev_l2_l3_cycle_counter[shire_id][bank_id])
            STATW_PMU_RESET_ON_OVERFLOW(pmc_cur->sc_pmcs[shire_id][bank_id].pmc0_overflow,
                pmc_cnt->prev_l2_l3_read_counter[shire_id][bank_id])
            STATW_PMU_RESET_ON_OVERFLOW(pmc_cur->sc_pmcs[shire_id][bank_id].pmc1_overflow,
                pmc_cnt->prev_l2_l3_write_counter[shire_id][bank_id])

            /* Add checks for bad values */
            STATW_UTIL_BAD_PMC_CHECK("SC_Cycle", shire_id, bank_id,
                pmc_cnt->prev_l2_l3_cycle_counter[shire_id][bank_id],
                pmc_cur->sc_pmcs[shire_id][bank_id].cycle)
            STATW_UTIL_BAD_PMC_CHECK("SC_Read", shire_id, bank_id,
                pmc_cnt->prev_l2_l3_read_counter[shire_id][bank_id],
                pmc_cur->sc_pmcs[shire_id][bank_id].pmc0)
            STATW_UTIL_BAD_PMC_CHECK("SC_Write", shire_id, bank_id,
                pmc_cnt->prev_l2_l3_write_counter[shire_id][bank_id],
                pmc_cur->sc_pmcs[shire_id][bank_id].pmc1)

            pmc_cur->avg_sc_pmcs.cycle =
                statw_calculate_average(pmc_cur->sc_pmcs[shire_id][bank_id].cycle -
                                            pmc_cnt->prev_l2_l3_cycle_counter[shire_id][bank_id],
                    pmc_cur->avg_sc_pmcs.cycle, cnt);
            pmc_cur->avg_sc_pmcs.pmc0 =
                statw_calculate_average(pmc_cur->sc_pmcs[shire_id][bank_id].pmc0 -
                                            pmc_cnt->prev_l2_l3_read_counter[shire_id][bank_id],
                    pmc_cur->avg_sc_pmcs.pmc0, cnt);
            pmc_cur->avg_sc_pmcs.pmc1 =
                statw_calculate_average(pmc_cur->sc_pmcs[shire_id][bank_id].pmc1 -
                                            pmc_cnt->prev_l2_l3_write_counter[shire_id][bank_id],
                    pmc_cur->avg_sc_pmcs.pmc1, cnt);
            cnt++;

            /* Update the previous values */
            pmc_cnt->prev_l2_l3_cycle_counter[shire_id][bank_id] =
                pmc_cur->sc_pmcs[shire_id][bank_id].cycle;
            pmc_cnt->prev_l2_l3_read_counter[shire_id][bank_id] =
                pmc_cur->sc_pmcs[shire_id][bank_id].pmc0;
            pmc_cnt->prev_l2_l3_write_counter[shire_id][bank_id] =
                pmc_cur->sc_pmcs[shire_id][bank_id].pmc1;
        }
    }
}

static void statw_update_cma(
    struct compute_resources_sample *data_sample, pmc_current_counters *pmc_cur)
{
    /* Calculate CMA, min and max */
    /* For DDR BW, multiply the request count by 2 due to double data rate in DDR */
    STATW_RECALC_CMA_MIN_MAX(data_sample->ddr_read_bw,
        STATW_PMU_REQ_COUNT_TO_MBPS(
            pmc_cur->avg_ms_pmcs.pmc0 * 2, DDR_FREQUENCY, pmc_cur->avg_ms_pmcs.cycle),
        STATW_BW_CMA_SAMPLE_COUNT)
    STATW_RECALC_CMA_MIN_MAX(data_sample->ddr_write_bw,
        STATW_PMU_REQ_COUNT_TO_MBPS(
            pmc_cur->avg_ms_pmcs.pmc1 * 2, DDR_FREQUENCY, pmc_cur->avg_ms_pmcs.cycle),
        STATW_BW_CMA_SAMPLE_COUNT)

    uint32_t minion_freq_mhz = atomic_load_local_32(&STATW_CB.minion_freq_mhz);
    STATW_RECALC_CMA_MIN_MAX(data_sample->l2_l3_read_bw,
        STATW_PMU_REQ_COUNT_TO_MBPS(
            pmc_cur->avg_sc_pmcs.pmc0, minion_freq_mhz, pmc_cur->avg_sc_pmcs.cycle),
        STATW_BW_CMA_SAMPLE_COUNT)
    STATW_RECALC_CMA_MIN_MAX(data_sample->l2_l3_write_bw,
        STATW_PMU_REQ_COUNT_TO_MBPS(
            pmc_cur->avg_sc_pmcs.pmc1, minion_freq_mhz, pmc_cur->avg_sc_pmcs.cycle),
        STATW_BW_CMA_SAMPLE_COUNT)
}

static void statw_init_pmc_cnt(uint64_t shire_mask, pmc_prev_counters *pmc_cnt)
{
    /* Initialize the first sample for delta calculations */
    for (uint64_t shire_id = 0; shire_id < NUM_MEM_SHIRES; shire_id++)
    {
        shire_pmc_cnt_t ms_pmcs = { 0 };
        /* Sample PMC MS Counter 0 and 1 (reads, writes). */
        syscall(SYSCALL_PMC_MS_SAMPLE_ALL_INT, shire_id, (uintptr_t)&ms_pmcs, UNUSED_SYSCALL_ARGS);
        pmc_cnt->prev_ddr_cycle_counter[shire_id] = ms_pmcs.cycle_overflow ? 0 : ms_pmcs.cycle;
        pmc_cnt->prev_ddr_read_counter[shire_id] = ms_pmcs.pmc0_overflow ? 0 : ms_pmcs.pmc0;
        pmc_cnt->prev_ddr_write_counter[shire_id] = ms_pmcs.pmc1_overflow ? 0 : ms_pmcs.pmc1;
    }

    for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id++)
    {
        if (!CHECK_BIT_SET(shire_mask, shire_id))
        {
            continue;
        }

        for (uint64_t bank_id = 0; bank_id < BANKS_PER_SC; bank_id++)
        {
            shire_pmc_cnt_t sc_pmcs = { 0 };
            syscall(SYSCALL_PMC_SC_SAMPLE_ALL_INT, shire_id, bank_id, (uintptr_t)&sc_pmcs);
            pmc_cnt->prev_l2_l3_cycle_counter[shire_id][bank_id] =
                sc_pmcs.cycle_overflow ? 0 : sc_pmcs.cycle;
            pmc_cnt->prev_l2_l3_read_counter[shire_id][bank_id] =
                sc_pmcs.pmc0_overflow ? 0 : sc_pmcs.pmc0;
            pmc_cnt->prev_l2_l3_write_counter[shire_id][bank_id] =
                sc_pmcs.pmc1_overflow ? 0 : sc_pmcs.pmc1;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       statw_update_pmc_stats
*
*   DESCRIPTION
*
*       This functions updates pmu sampling stats by reading mem shire
*       and shire cache read/write cycles.
*
*   INPUTS
*
*       shire_mask  Shire mask of the available shires
*       data_sample data struct containing sampled data
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void statw_update_pmc_stats(
    uint64_t shire_mask, struct compute_resources_sample *data_sample)
{
    static pmc_prev_counters pmc_cnt = { 0 };
    static pmc_current_counters pmc_cur = { 0 };
    /* Check the flag to sample device stats. */
    switch (atomic_load_local_32(&STATW_CB.pmu_sampling_state))
    {
        case STATW_PMU_SAMPLING_START:
            memset(&pmc_cur, 0, sizeof(pmc_cur));
            statw_sample_pmc_counters(shire_mask, &pmc_cur);
            statw_process_pmc_counters(shire_mask, &pmc_cnt, &pmc_cur);
            statw_update_cma(data_sample, &pmc_cur);
            break;
        case STATW_PMU_SAMPLING_RESET_AND_START:
            statw_init_pmc_cnt(shire_mask, &pmc_cnt);
            /* Start PMU sampling */
            STATW_Update_PMU_Sampling_State(STATW_PMU_SAMPLING_START);
            break;
        case STATW_PMU_SAMPLING_STOP_ASYNC:
        case STATW_PMU_SAMPLING_STOP_SYNC:
            atomic_store_local_32(&STATW_CB.pmu_sampling_state, STATW_PMU_SAMPLING_STOPPED);
            break;
        case STATW_PMU_SAMPLING_STOPPED:
            /* Nothing need to be done for this case */
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "statw_update_pmc_stats: Invalid PMU sampling state\r\n");
            break;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       statw_sample_pmu_state_timeout_callback
*
*   DESCRIPTION
*
*       Callback for stats worker state change wait timeout.
*
*   INPUTS
*
*       arg    optional argument
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void statw_sample_pmu_state_timeout_callback(uint8_t arg)
{
    (void)arg;

    /* Set the pmu sampling state change timeout flag */
    atomic_store_local_32(&STATW_CB.pmu_sampling_timeout_flag, 1U);
}

/************************************************************************
*
*   FUNCTION
*
*       statw_sample_device_stats_callback
*
*   DESCRIPTION
*
*       Callback for periodic sampling of stats.
*
*   INPUTS
*
*       arg    optional argument
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
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
*       resource_dest   Sample data
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void read_resource_stats_atomically(
    statw_resource_type_e resource_type, struct resource_value *resource_dest)
{
    const struct resource_value *resource;

    if (resource_type == STATW_RESOURCE_CM)
    {
        resource = &STATW_CB.cm_bw;
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
*       STATW_Get_Minion_Freq
*
*   DESCRIPTION
*
*       This function returns the Minion frequency in MHz.
*
*   INPUTS
*
*       void
*
*   OUTPUTS
*
*       Frequency value in MHz.
*
***********************************************************************/
uint32_t STATW_Get_Minion_Freq(void)
{
    return atomic_load_local_32(&STATW_CB.minion_freq_mhz);
}

/************************************************************************
*
*   FUNCTION
*
*       STATW_Get_MM_Stats
*
*   DESCRIPTION
*
*       This function returns the current MM stats.
*
*   INPUTS
*
*       sample  pointer to compute_resource_sample to populate
*
*   OUTPUTS
*
*       success or error
*
***********************************************************************/
int32_t STATW_Get_MM_Stats(struct compute_resources_sample *sample)
{
    int32_t status = STATUS_SUCCESS;
    uint64_t trace_entry = atomic_load_local_64(&STATW_CB.saved_trace_entry);

    if (!sample)
    {
        Log_Write(LOG_LEVEL_ERROR, "STATW_Get_MM_Stats: invalid null sample argument\n");
        status = STATW_ERROR_GET_MM_STATS_INVALID_ARG;
    }
    else if (!trace_entry)
    {
        Log_Write(LOG_LEVEL_WARNING, "STATW_Get_MM_Stats: unexpected startup race condition\n");
        memset(sample, 0, sizeof(*sample));
    }
    else
    {
        struct dst_type_t {
            struct trace_custom_event_t event;
            struct compute_resources_sample sample;
        } __attribute__((packed, aligned(8)));
        struct dst_type_t dst;
        struct trace_entry_header_t *src = (struct trace_entry_header_t *)trace_entry;

        status = Trace_Event_Copy(Trace_Get_MM_Stats_CB(), src, &dst, sizeof(dst));

        /* If the trace CB is invalid, or trace is disabled, just return null data */
        if (status == TRACE_INVALID_CB)
        {
            memset(sample, 0, sizeof(*sample));
            status = STATUS_SUCCESS;
        }
        if (status != TRACE_STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "STATW_Get_MM_Stats: Trace_Event_Copy failed %d!\n", status);
        }
        else if (dst.event.header.type != TRACE_TYPE_CUSTOM_EVENT ||
                 dst.event.custom_type != TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES)
        {
            Log_Write(LOG_LEVEL_ERROR, "STATW_Get_MM_Stats: type mismatch %d %d (expect %d %d)!\n",
                dst.event.header.type, dst.event.custom_type, TRACE_TYPE_CUSTOM_EVENT,
                TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES);
            status = STATW_ERROR_GET_MM_STATS_INVALID_EVENT;
        }
        else
        {
            *sample = dst.sample;
            status = STATUS_SUCCESS;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       STATW_Reset_MM_Stats
*
*   DESCRIPTION
*
*       This function sets the flag to reset the MM stats.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       success or error
*
***********************************************************************/
int32_t STATW_Reset_MM_Stats(void)
{
    atomic_store_local_32(&STATW_CB.reset_sampling_flag, STATW_SAMPLING_FLAG_SET);
    return STATUS_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       STATW_Add_New_Sample_Atomically
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
        resource = &STATW_CB.cm_bw;
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
    avg = statw_recalculate_cma(avg, current_sample, STATW_BW_CMA_SAMPLE_COUNT);
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
*       statw_fill_utilization_stats
*
*   DESCRIPTION
*
*       This functions fills the utilization stats int the given data
*       sample struct.
*
*   INPUTS
*
*       data_sample          Pointer to the compute resources
*       prev_timestamp       Timestamp of previously sampled utilization
*
*
*   OUTPUTS
*
*       current_timestamp    Timestamp at which the stats were calculated
*
***********************************************************************/
static uint64_t statw_fill_utilization_stats(
    struct compute_resources_sample *data_sample, uint64_t prev_timestamp)
{
    /* Percent utilization = trasnsaction accumulated cycles * 100 / Cycles in sampling interval. */
    uint64_t current_timestamp = PMC_Get_Current_Cycles();
    uint64_t total_cycles = current_timestamp - prev_timestamp;
    uint64_t dma_write_util_cycles =
        DMAW_Get_Average_Exec_Cycles(DMA_CHAN_TYPE_READ, prev_timestamp, current_timestamp);
    uint64_t dma_read_util_cycles =
        DMAW_Get_Average_Exec_Cycles(DMA_CHAN_TYPE_WRITE, prev_timestamp, current_timestamp);
    uint64_t cm_util_cycles = KW_Get_Average_Exec_Cycles(prev_timestamp, current_timestamp);

    /* Caclulate the instantaneous average for utilization. */
    uint64_t instant_average = (dma_read_util_cycles * STATW_PERCENTAGE_MULTIPLIER) / total_cycles;
    STATW_RECALC_CMA_MIN_MAX(
        data_sample->pcie_dma_read_utilization, instant_average, STATW_UTILIZATION_CMA_SAMPLE_COUNT)

    instant_average = (dma_write_util_cycles * STATW_PERCENTAGE_MULTIPLIER) / total_cycles;
    STATW_RECALC_CMA_MIN_MAX(data_sample->pcie_dma_write_utilization, instant_average,
        STATW_UTILIZATION_CMA_SAMPLE_COUNT)

    instant_average = (cm_util_cycles * STATW_PERCENTAGE_MULTIPLIER) / total_cycles;
    STATW_RECALC_CMA_MIN_MAX(
        data_sample->cm_utilization, instant_average, STATW_UTILIZATION_CMA_SAMPLE_COUNT)

    return current_timestamp;
}

/************************************************************************
*
*   FUNCTION
*
*       statw_sample_init
*
*   DESCRIPTION
*
*       Initialize sample stats
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
static void statw_sample_init(struct compute_resources_sample *local_stats_cb)
{
    atomic_store_local_64(&STATW_CB.cm_bw.avg, STATW_RESOURCE_DEFAULT_AVG);
    atomic_store_local_64(&STATW_CB.cm_bw.max, STATW_RESOURCE_DEFAULT_MAX);
    atomic_store_local_64(&STATW_CB.cm_bw.min, STATW_RESOURCE_DEFAULT_MIN);
    atomic_store_local_64(&STATW_CB.pcie_dma_read_bw.avg, STATW_RESOURCE_DEFAULT_AVG);
    atomic_store_local_64(&STATW_CB.pcie_dma_read_bw.max, STATW_RESOURCE_DEFAULT_MAX);
    atomic_store_local_64(&STATW_CB.pcie_dma_read_bw.min, STATW_RESOURCE_DEFAULT_MIN);
    atomic_store_local_64(&STATW_CB.pcie_dma_write_bw.avg, STATW_RESOURCE_DEFAULT_AVG);
    atomic_store_local_64(&STATW_CB.pcie_dma_write_bw.max, STATW_RESOURCE_DEFAULT_MAX);
    atomic_store_local_64(&STATW_CB.pcie_dma_write_bw.min, STATW_RESOURCE_DEFAULT_MIN);

    local_stats_cb->ddr_read_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->ddr_read_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->ddr_read_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->pcie_dma_read_utilization.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->pcie_dma_read_utilization.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->pcie_dma_read_utilization.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->ddr_write_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->ddr_write_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->ddr_write_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->pcie_dma_write_utilization.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->pcie_dma_write_utilization.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->pcie_dma_write_utilization.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->l2_l3_read_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->l2_l3_read_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->l2_l3_read_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->l2_l3_write_bw.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->l2_l3_write_bw.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->l2_l3_write_bw.min = STATW_RESOURCE_DEFAULT_MIN;
    local_stats_cb->cm_utilization.avg = STATW_RESOURCE_DEFAULT_AVG;
    local_stats_cb->cm_utilization.max = STATW_RESOURCE_DEFAULT_MAX;
    local_stats_cb->cm_utilization.min = STATW_RESOURCE_DEFAULT_MIN;
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
    /* Store the frequency in global data.
    TODO: Doesn't take dynamically changing frequency in account */
    atomic_store_local_32(&STATW_CB.minion_freq_mhz, MM_Config_Get_Minion_Boot_Freq());
    atomic_store_local_64(&STATW_CB.saved_trace_entry, 0);

    statw_sample_init(local_stats_cb);
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
    uint64_t prev_timestamp;
    uint64_t shire_mask;
    struct trace_custom_event_t *entry;

    statw_init(&data_sample);
    Log_Write(LOG_LEVEL_INFO, "STATW:H[%d]\r\n", hart_id);

    /* Initialize the flag to sample device stats. Set the flag to log first sample at the start. */
    atomic_store_local_32(&STATW_CB.sampling_flag, STATW_SAMPLING_FLAG_SET);

    /* Initialize the flag to reset sample device stats. Set the flag to clear at the start. */
    atomic_store_local_32(&STATW_CB.reset_sampling_flag, STATW_SAMPLING_FLAG_CLEAR);

    /* Initialize the flag to reset and start PMU sample device stats. */
    atomic_store_local_32(&STATW_CB.pmu_sampling_state, STATW_PMU_SAMPLING_RESET_AND_START);

    /* Create a timer for periodic sampling of stats */
    int sw_timer_idx =
        SW_Timer_Create_Timeout(&statw_sample_device_stats_callback, 0, STATW_SAMPLING_INTERVAL);

    if (sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_WARNING,
            "STATW: Unable to register sampler timer! It may not log device stats.\r\n");
    }

    /* Obtain the CM shire mask */
    shire_mask = MM_Config_Get_CM_Shire_Mask();

    /* Set the bit for MM shire */
    shire_mask = MASK_SET_BIT(shire_mask, MASTER_SHIRE);

    /* Take the initial timestamp */
    prev_timestamp = PMC_Get_Current_Cycles();

    while (1)
    {
        /* Check the flag to sample device stats. */
        if (atomic_compare_and_exchange_local_32(
                &STATW_CB.sampling_flag, STATW_SAMPLING_FLAG_SET, STATW_SAMPLING_FLAG_CLEAR))
        {
            if (atomic_compare_and_exchange_local_32(&STATW_CB.reset_sampling_flag,
                    STATW_SAMPLING_FLAG_SET, STATW_SAMPLING_FLAG_CLEAR))
            {
                statw_sample_init(&data_sample);
            }

            /* Fill stats based on PMC stats (Shire Cache and DDR) */
            statw_update_pmc_stats(shire_mask, &data_sample);

            /* Read global stats populated by other harts/workers. */
            read_resource_stats_atomically(STATW_RESOURCE_CM, &data_sample.cm_bw);
            read_resource_stats_atomically(STATW_RESOURCE_DMA_READ, &data_sample.pcie_dma_read_bw);
            read_resource_stats_atomically(
                STATW_RESOURCE_DMA_WRITE, &data_sample.pcie_dma_write_bw);

            /* Fill the utilization stats */
            prev_timestamp = statw_fill_utilization_stats(&data_sample, prev_timestamp);

            /* Log the event in trace */
            entry = (struct trace_custom_event_t *)Trace_Custom_Event(Trace_Get_MM_Stats_CB(),
                TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES, (const uint8_t *)&data_sample,
                sizeof(data_sample));

            if (entry != NULL)
            {
                /* Evict last Trace event which is logged above */
                Trace_Evict_Event_MM_Stats(
                    entry, (sizeof(struct trace_custom_event_t) + sizeof(data_sample)));

                /* Save the trace entry for retrieval in STATW_Get_MM_Stats via Trace_Event_Copy */
                atomic_store_local_64(&STATW_CB.saved_trace_entry, (uint64_t)entry);
            }
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       STATW_Update_PMU_Sampling_State
*
*   DESCRIPTION
*
*       This function sets the state of PMU sampling.
*
*   INPUTS
*
*       pmu_state   sampling state to be changed
*
*   OUTPUTS
*
*       status      success or error
*
***********************************************************************/
int32_t STATW_Update_PMU_Sampling_State(enum statw_pmu_sampling_state pmu_state)
{
    int32_t status = STATUS_SUCCESS;

    /* Change PMU sampling state and wait for state to be changed if it's a stop sync */
    if (pmu_state != atomic_load_local_32(&STATW_CB.pmu_sampling_state))
    {
        /* Update PMU sampling state */
        atomic_store_local_32(&STATW_CB.pmu_sampling_state, pmu_state);

        /* Force the Stats worker to do sampling in order to transition the state */
        atomic_store_local_32(&STATW_CB.sampling_flag, STATW_SAMPLING_FLAG_SET);

        /* Check if state is stop sync */
        if (pmu_state == STATW_PMU_SAMPLING_STOP_SYNC)
        {
            int32_t sw_timer_idx;
            uint32_t timeout_flag;

            /* Create timeout to wait for PMU sampling to be completely stopped */
            sw_timer_idx = SW_Timer_Create_Timeout(
                &statw_sample_pmu_state_timeout_callback, 0, STATW_PMU_SAMPLING_STATE_TIMEOUT);

            if (sw_timer_idx < 0)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "STATW: Unable to register PMU sampling stop wait timeout!\r\n");
            }
            else
            {
                /* Wait for state to be completely stopped */
                do
                {
                    /* Read the timeout flag */
                    timeout_flag = atomic_compare_and_exchange_local_32(
                        &STATW_CB.pmu_sampling_timeout_flag, 1, 0);
                } while ((atomic_load_local_32(&STATW_CB.pmu_sampling_state) ==
                             STATW_PMU_SAMPLING_STOP_SYNC) &&
                         (timeout_flag == 0));

                /* Check for timeout */
                if (timeout_flag == 1)
                {
                    status = STATW_ERROR_UPDATE_PMU_SAMPLING_STATE_TIMEOUT;
                }
                /* Free the registered SW Timeout slot */
                SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);
            }
        }
    }

    return status;
}
