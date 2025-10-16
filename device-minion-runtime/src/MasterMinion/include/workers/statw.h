/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/***********************************************************************/
/*! \file statw.h
    \brief A C header that defines the device statistics sampler
    worker's interface.
*/
/***********************************************************************/

#ifndef _STATW_
#define _STATW_

#include "services/trace.h"

typedef uint8_t statw_resource_type_e;

enum statw_resource_type {
    STATW_RESOURCE_DMA_READ,
    STATW_RESOURCE_DMA_WRITE,
    STATW_RESOURCE_CM,
    STATW_RESOURCE_DDR_READ,
    STATW_RESOURCE_DDR_WRITE,
    STATW_RESOURCE_L2_L3_READ,
    STATW_RESOURCE_L2_L3_WRITE,
};

enum statw_pmu_sampling_state {
    STATW_PMU_SAMPLING_START,
    STATW_PMU_SAMPLING_RESET_AND_START,
    STATW_PMU_SAMPLING_STOP_ASYNC,
    STATW_PMU_SAMPLING_STOP_SYNC,
    STATW_PMU_SAMPLING_STOPPED,
};

/*! \def STATW_SAMPLING_INTERVAL
    \brief Device statistics sampling interval of 1 millisecond
    WARNING: Assumption is timer granularity is one millisecond
*/
#define STATW_SAMPLING_INTERVAL 1UL

/*! \def STATW_UTILIZATION_CMA_SAMPLE_COUNT
    \brief Device utilization statistics moving average sample count.
*/
#define STATW_UTILIZATION_CMA_SAMPLE_COUNT 5UL

/*! \def STATW_BW_CMA_SAMPLE_COUNT
    \brief Device bandwidth statistics moving average sample count.
*/
#define STATW_BW_CMA_SAMPLE_COUNT 100UL

/*! \def STATW_RESOURCE_DEFAULT_MIN
    \brief Default minimum value for device bandwidth resource stat.
*/
#define STATW_RESOURCE_DEFAULT_MIN 0UL

/*! \def STATW_RESOURCE_DEFAULT_MAX
    \brief Default maximum for device resource stat.
*/
#define STATW_RESOURCE_DEFAULT_MAX 0UL

/*! \def STATW_RESOURCE_DEFAULT_AVG
    \brief Default average for device resource stat.
*/
#define STATW_RESOURCE_DEFAULT_AVG 0UL

/*! \def STATW_1MB
    \brief A macro that define number of bytes in 1 megabytes.
*/
#define STATW_1MB 0x100000UL

/*! \def STATW_NUM_OF_MS_IN_SEC
    \brief A macro that define number of milliseconds in 1 second.
*/
#define STATW_NUM_OF_MS_IN_SEC 1000UL

/*! \def MAX(x,y)
    \brief Returns max
*/
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*! \def MIN(x,y)
    \brief Returns min
*/
#define MIN(x, y)                         \
    ((y) == STATW_RESOURCE_DEFAULT_MIN) ? \
        (x) :                             \
        ((x) == STATW_RESOURCE_DEFAULT_MIN) ? (y) : ((x) < (y) ? (x) : (y))

/*! \def STATW_CHECK_FOR_CONTINUED_EXEC_TRANSACTION
    \brief This is check if transaction is continued past the sampling interval end then
    end cycles will be equal to start cycles because there are no total dma transaction cycles.
    In this case total cycles will be the interval cycles.
*/
#define STATW_CHECK_FOR_CONTINUED_EXEC_TRANSACTION(interval_start, interval_end,        \
    exec_start_cycles, exec_end_cycles, dma_tans_cycles, prev_cycles, chan_prev_cycles) \
    (exec_start_cycles == exec_end_cycles) ?                                            \
        (atomic_add_local_64(chan_prev_cycles, (interval_end - interval_start)),        \
            (interval_end - interval_start)) :                                          \
        (atomic_add_local_64(chan_prev_cycles, (dma_tans_cycles - prev_cycles)),        \
            (dma_tans_cycles - prev_cycles))

/*! \def STATW_CHECK_FOR_TRANS_COMPLETION_WRT_SAMPLING_INTERVAL
    \brief This is check for scenario in which completed transaction ended after sampling interval
*/
#define STATW_CHECK_FOR_TRANS_COMPLETION_WRT_SAMPLING_INTERVAL(                      \
    exec_start_cycles, exec_end_cycles, interval_start, interval_end, tans_cycles)   \
    ((exec_start_cycles < interval_end) && (exec_start_cycles > interval_start) &&   \
        (exec_end_cycles > interval_end)) ?                                          \
        (interval_end - exec_start_cycles) :                                         \
        ((exec_start_cycles < interval_start) && (exec_end_cycles < interval_end) && \
            (exec_end_cycles > interval_start)) ?                                    \
        (exec_end_cycles - interval_start) :                                         \
        ((exec_start_cycles < interval_start) && (exec_end_cycles > interval_end) && \
            (exec_end_cycles > interval_start)) ?                                    \
        (tans_cycles + (interval_end - exec_end_cycles)) :                           \
        0

/*! \def STATW_CHECK_FOR_TRANS_COMPLETION_BEFORE_SAMPLING_INTERVAL
    \brief This is check for scenario where transaction started before sampling interval.
*/
#define STATW_CHECK_FOR_TRANS_COMPLETION_BEFORE_SAMPLING_INTERVAL(               \
    exec_start_cycles, exec_end_cycles, interval_start, kw_tans_cycles)          \
    ((exec_start_cycles < interval_start) && (exec_end_cycles < interval_end) && \
        (exec_end_cycles > interval_start)) ?                                    \
        (exec_end_cycles - interval_start) :                                     \
        kw_tans_cycles

/*! \fn void STATW_Launch(uint32_t sqw_idx)
    \brief Initialize Device Stat Workers, used by dispatcher
    \param hart_id HART ID on which the Stat Worker should be launched
    \return none
*/
void STATW_Launch(uint32_t hart_id);

/*! \fn uint32_t STATW_Add_New_Sample_Atomically(statw_resource_type_e resource_type, uint64_t current_sample)
    \brief This functions adds new sample for resource utilization data.
    \param resource_type Resource for which new sample is to be added.
    \param current_sample Sample data
    \return None.
*/
void STATW_Add_New_Sample_Atomically(statw_resource_type_e resource_type, uint64_t current_sample);

/*! \fn uint32_t STATW_Get_Minion_Freq(void)
    \brief Returns the Minion frequency in MHz.
    \return Frequency value in mega hertz.
*/
uint32_t STATW_Get_Minion_Freq(void);

/*! \fn int32_t STATW_Get_MM_Stats(struct compute_resources_sample *sample)
    \brief Get the current MM stats.
    \param sample Pointer to sample to populate.
    \return status success or error.
*/
int32_t STATW_Get_MM_Stats(struct compute_resources_sample *sample);

/*! \fn int32_t STATW_Reset_MM_Stats(void)
    \brief Reset the current MM stats.
    \return status success or error.
*/
int32_t STATW_Reset_MM_Stats(void);

/*! \fn int32_t STATW_Update_PMU_Sampling_State(enum statw_pmu_sampling_state pmu_state)
    \brief This function sets the state of PMU sampling.
    \param pmu_state   sampling state to be changed
    \return status success or error.
*/
int32_t STATW_Update_PMU_Sampling_State(enum statw_pmu_sampling_state pmu_state);

#endif
