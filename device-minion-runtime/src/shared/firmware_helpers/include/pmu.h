
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

#ifndef __PMU_H__
#define __PMU_H__

#include "device-common/esr_defines.h"
// PMU support: Defines and basic API

// ETSoC-1 PMU hardware
#define PMU_FIRST_HPM      3
#define PMU_LAST_CORE_HPM  6
#define PMU_LAST_HPM       8
#define PMU_NR_HPM         (PMU_LAST_HPM - PMU_FIRST_HPM + 1)

// User-Level CSRs :: Counters/Timers
#define PMU_USER_CYCLE   0xC00ULL
#define PMU_USER_TIME    0xC01ULL
#define PMU_USER_INSTRET 0xC02ULL
#define PMU_HPMCOUNTER3  0xC03ULL
#define PMU_HPMCOUNTER4  0xC04ULL
#define PMU_HPMCOUNTER5  0xC05ULL
#define PMU_HPMCOUNTER6  0xC06ULL
#define PMU_HPMCOUNTER7  0xC07ULL
#define PMU_HPMCOUNTER8  0xC08ULL

// Machine-Level CSRs :: Counters Setup
#define PMU_MHPMEVENT3 0x323ULL
#define PMU_MHPMEVENT4 0x324ULL
#define PMU_MHPMEVENT5 0x325ULL
#define PMU_MHPMEVENT6 0x326ULL
#define PMU_MHPMEVENT7 0x327ULL
#define PMU_MHPMEVENT8 0x328ULL

// Machine-Level CSRs :: Counters/Timers
#define PMU_MHPMCOUNTER3 0xB03
#define PMU_MHPMCOUNTER4 0xB04
#define PMU_MHPMCOUNTER5 0xB05
#define PMU_MHPMCOUNTER6 0xB06
#define PMU_MHPMCOUNTER7 0xB07
#define PMU_MHPMCOUNTER8 0xB08

// Constants used to index inside PMC configuration buffer
// NOTE: THIS IS DEPRECATED. IMPLEMENTATION DETAILS OF THE OLD PMC TRACING CODE
#define PMU_MINION_COUNTERS_PER_HART 4
#define PMU_NEIGH_COUNTERS_PER_HART  2
#define PMU_SC_COUNTERS_PER_BANK     3
#define PMU_EVENT_SHIRE_AREA \
    (PMU_MINION_COUNTERS_PER_HART * 2 + PMU_NEIGH_COUNTERS_PER_HART * 2 + PMU_SC_COUNTERS_PER_BANK)
#define NUM_SHIRES_PMC 33
#define PMU_EVENT_MEMSHIRE_OFFSET (PMU_EVENT_SHIRE_AREA * NUM_SHIRES_PMC)
#define PMU_MS_COUNTERS_PER_MS    5

// Thread in neigh that does all the PMC configuration, starting / topping and sampling
// There is a 1-1 neigh-bank mapping. Only 1 hread needs to do that to avoid races as the CTL_STATUS register is shared
// among counters / events
#define NEIGH_HART_SC 12
#define NEIGH_HART_MS 14

// Values to reset and start counting shire cache and memshire PMCs
// For the time we ignore overflows and disable interrupts.
// This could be moved in the configuration buffer but we need to add support for
// overflows, etc in the firmware in that case.
#define PMU_SC_START_CTRL_VAL 0x00060033ULL
#define PMU_SC_STOP_CTRL_VAL ~PMU_SC_START_CTRL_VAL
#define PMU_MS_START_CTRL_VAL 0x00060033ULL
#define PMU_MS_STOP_CTRL_VAL ~PMU_MS_START_CTRL_VAL

// Events

// Minion events
#define PMU_MINION_EVENT_NONE             0
#define PMU_MINION_EVENT_CYCLES           1
#define PMU_MINION_EVENT_RETIRED_INST0    2
#define PMU_MINION_EVENT_RETIRED_INST1    3
#define PMU_MINION_EVENT_BRANCHES0        4
#define PMU_MINION_EVENT_BRANCHES1        5
#define PMU_MINION_EVENT_DCACHE_ACCESS0   6
#define PMU_MINION_EVENT_DCACHE_ACCESS1   7
#define PMU_MINION_EVENT_DCACHE_MISSES0   8
#define PMU_MINION_EVENT_DCACHE_MISSES1   9
#define PMU_MINION_EVENT_L2_MISS_REQ      10
#define PMU_MINION_EVENT_L2_MISS_REQ_REJ  11
#define PMU_MINION_EVENT_L2_EVICT_REQ     12
#define PMU_MINION_EVENT_L2_EVICT_REQ_REJ 13
#define PMU_MINION_EVENT_TL_INST          14
#define PMU_MINION_EVENT_TL_OPS           15
#define PMU_MINION_EVENT_TS_INST          16
#define PMU_MINION_EVENT_TS_OPS           17
#define PMU_MINION_EVENT_TFMA_WAIT_TENB   18
#define PMU_MINION_EVENT_TIMA_OPS         19
#define PMU_MINION_EVENT_TXFMA_3216_OPS   20
#define PMU_MINION_EVENT_TXFMA_32_OPS     21
#define PMU_MINION_EVENT_TXFMA_INT_OPS    22
#define PMU_MINION_EVENT_TRANS_OPS        23
#define PMU_MINION_EVENT_SHORT_OPS        24
#define PMU_MINION_EVENT_MASK_OPS         25
#define PMU_MINION_EVENT_TFMA_INST        26
#define PMU_MINION_EVENT_TREDUCE_INST     27
#define PMU_MINION_EVENT_TQUANT_INST      28

// Indices of shire cache and memshire PMCs.
#define PMU_SC_CYCLE_PMC 0
#define PMU_SC_PMC0      1
#define PMU_SC_PMC1      2

#define PMU_MS_CYCLE_PMC 0
#define PMU_MS_PMC0      1
#define PMU_MS_PMC1      2

#define PMU_INCORRECT_COUNTER 0xFFFFFFFFFFFFFFFFULL

// Configure a core (minion and neighborhood) event counter
static inline int64_t pmu_core_event_configure(uint64_t evt_reg, uint64_t evt)
{
    switch (evt_reg) {
    case PMU_MHPMEVENT3:
        __asm__ __volatile__("csrw %[csr], %[event]\n"
                             :
                             : [event] "r"(evt), [csr] "i"(PMU_MHPMEVENT3));
        break;
    case PMU_MHPMEVENT4:
        __asm__ __volatile__("csrw %[csr], %[event]\n"
                             :
                             : [event] "r"(evt), [csr] "i"(PMU_MHPMEVENT4));
        break;
    case PMU_MHPMEVENT5:
        __asm__ __volatile__("csrw %[csr], %[event]\n"
                             :
                             : [event] "r"(evt), [csr] "i"(PMU_MHPMEVENT5));
        break;
    case PMU_MHPMEVENT6:
        __asm__ __volatile__("csrw %[csr], %[event]\n"
                             :
                             : [event] "r"(evt), [csr] "i"(PMU_MHPMEVENT6));
        break;
    case PMU_MHPMEVENT7:
        __asm__ __volatile__("csrw %[csr], %[event]\n"
                             :
                             : [event] "r"(evt), [csr] "i"(PMU_MHPMEVENT7));
        break;
    case PMU_MHPMEVENT8:
        __asm__ __volatile__("csrw %[csr], %[event]\n"
                             :
                             : [event] "r"(evt), [csr] "i"(PMU_MHPMEVENT8));
        break;
    default:
        return -1;
    }
    return 0;
}

// Write to core (minion and neighborhood) event counter
static inline int64_t pmu_core_counter_write(uint64_t pmc, uint64_t val)
{
    switch (pmc) {
    case PMU_MHPMCOUNTER3:
        __asm__ __volatile__("csrw %[csr], %[init]\n"
                             :
                             : [init] "r"(val), [csr] "i"(PMU_MHPMCOUNTER3));
        break;
    case PMU_MHPMCOUNTER4:
        __asm__ __volatile__("csrw %[csr], %[init]\n"
                             :
                             : [init] "r"(val), [csr] "i"(PMU_MHPMCOUNTER4));
        break;
    case PMU_MHPMCOUNTER5:
        __asm__ __volatile__("csrw %[csr], %[init]\n"
                             :
                             : [init] "r"(val), [csr] "i"(PMU_MHPMCOUNTER5));
        break;
    case PMU_MHPMCOUNTER6:
        __asm__ __volatile__("csrw %[csr], %[init]\n"
                             :
                             : [init] "r"(val), [csr] "i"(PMU_MHPMCOUNTER6));
        break;
    case PMU_MHPMCOUNTER7:
        __asm__ __volatile__("csrw %[csr], %[init]\n"
                             :
                             : [init] "r"(val), [csr] "i"(PMU_MHPMCOUNTER7));
        break;
    case PMU_MHPMCOUNTER8:
        __asm__ __volatile__("csrw %[csr], %[init]\n"
                             :
                             : [init] "r"(val), [csr] "i"(PMU_MHPMCOUNTER8));
        break;
    default:
        return -1;
    }
    return 0;
}

#define pmu_core_counter_reset(pmc) pmu_core_counter_write(pmc, 0)

// Read a core (minion and neighborhood) event counter
// Return -1 on incorrect counter
static inline uint64_t pmu_core_counter_read(uint64_t pmc)
{
    uint64_t val = 0;
    switch (pmc) {
    case PMU_MHPMCOUNTER3:
        __asm__ __volatile__("csrr %[res], %[csr]\n"
                             : [res] "=r"(val)
                             : [csr] "i"(PMU_MHPMCOUNTER3));
        break;
    case PMU_MHPMCOUNTER4:
        __asm__ __volatile__("csrr %[res], %[csr]\n"
                             : [res] "=r"(val)
                             : [csr] "i"(PMU_MHPMCOUNTER4));
        break;
    case PMU_MHPMCOUNTER5:
        __asm__ __volatile__("csrr %[res], %[csr]\n"
                             : [res] "=r"(val)
                             : [csr] "i"(PMU_MHPMCOUNTER5));
        break;
    case PMU_MHPMCOUNTER6:
        __asm__ __volatile__("csrr %[res], %[csr]\n"
                             : [res] "=r"(val)
                             : [csr] "i"(PMU_MHPMCOUNTER6));
        break;
    case PMU_MHPMCOUNTER7:
        __asm__ __volatile__("csrr %[res], %[csr]\n"
                             : [res] "=r"(val)
                             : [csr] "i"(PMU_MHPMCOUNTER7));
        break;
    case PMU_MHPMCOUNTER8:
        __asm__ __volatile__("csrr %[res], %[csr]\n"
                             : [res] "=r"(val)
                             : [csr] "i"(PMU_MHPMCOUNTER8));
        break;
    default:
        return PMU_INCORRECT_COUNTER;
    }
    return val;
}

// Configure an event for a shire cache perf counter
static inline int64_t pmu_shire_cache_event_configure(uint64_t shire_id, uint64_t b, uint64_t evt_reg,
                                         uint64_t val)
{
    uint64_t *sc_bank_qualevt_addr = 0;
    int64_t ret = 0;
    if (evt_reg == 0) {
        sc_bank_qualevt_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P0_QUAL);
    } else if (evt_reg == 1) {
        sc_bank_qualevt_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P1_QUAL);
    } else {
        ret = -1;
    }
    *sc_bank_qualevt_addr = val;
    return ret;
}

// Set a shire cache PMC to a value
static inline void pmu_shire_cache_counter_set(uint64_t shire_id, uint64_t b, uint64_t val)
{
    uint64_t *sc_bank_perfctrl_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_CTL_STATUS);
    *sc_bank_perfctrl_addr = val;
}

// Reset and start a shire cache PMC
static inline void pmu_shire_cache_counter_reset(uint64_t shire_id, uint64_t b)
{
    uint64_t *sc_bank_perfctrl_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_CTL_STATUS);
    uint64_t init_val = *sc_bank_perfctrl_addr;
    *sc_bank_perfctrl_addr = PMU_SC_START_CTRL_VAL | init_val;
}

// Stop a shire cache PMC
static inline void pmu_shire_cache_counter_stop(uint64_t shire_id, uint64_t b)
{
    uint64_t *sc_bank_perfctrl_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_CTL_STATUS);
    uint64_t init_val = *sc_bank_perfctrl_addr;
    *sc_bank_perfctrl_addr = init_val & PMU_SC_STOP_CTRL_VAL;
}

// Read a shire cache PMC. Return -1 on incorrect counter
static inline uint64_t pmu_shire_cache_counter_sample(uint64_t shire_id, uint64_t b, uint64_t pmc)
{
    uint64_t val = 0;
    uint64_t *sc_bank_pmc_addr = 0;
    if (pmc == PMU_SC_CYCLE_PMC) {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_CYC_CNTR);
    } else if (pmc == PMU_SC_PMC0) {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P0_CNTR);
    } else if (pmc == PMU_SC_PMC1) {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P1_CNTR);
    } else {
        return PMU_INCORRECT_COUNTER;
    }
    val = *sc_bank_pmc_addr;
    return val;
}

// Configure an event for memshire counters
static inline int64_t pmu_memshire_event_configure(uint64_t ms_id, uint64_t evt_reg, uint64_t val)
{
    uint64_t *ms_pmc_ctrl_addr = 0;
    int64_t ret = 0;
    if (evt_reg == 0) {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_QUAL);
    } else if (evt_reg == 1) {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_QUAL);
    } else if (evt_reg == 2) {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_QUAL2);
    } else if (evt_reg == 3) {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_QUAL2);
    } else {
        ret = -1;
    }
    *ms_pmc_ctrl_addr = val;
    return ret;
}

// Set a memshire PMC to a value
static inline void pmu_memshire_event_set(uint64_t ms_id, uint64_t val)
{
    uint64_t *ms_perfctrl_addr =
        (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);
    *ms_perfctrl_addr = val;
}

// Reset and start a memshire PMC
static inline void pmu_memshire_event_reset(uint64_t ms_id)
{
    uint64_t *ms_pmc_ctrl_addr =
        (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);
    uint64_t init_val = *ms_pmc_ctrl_addr;
    *ms_pmc_ctrl_addr = init_val | PMU_MS_START_CTRL_VAL;
}

// Stop a memshire PMC
static inline void pmu_memshire_event_stop(uint64_t ms_id)
{
    uint64_t *ms_pmc_ctrl_addr =
        (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);
    uint64_t init_val = *ms_pmc_ctrl_addr;
    *ms_pmc_ctrl_addr = init_val & PMU_MS_STOP_CTRL_VAL;
}

// Read a memshire PMC. Return -1 on incorrect counter
static inline uint64_t pmu_memshire_event_sample(uint64_t ms_id, uint64_t pmc)
{
    uint64_t val = 0;
    uint64_t *ms_pmc_addr = 0;
    if (pmc == PMU_MS_CYCLE_PMC) {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CYC_CNTR);
    } else if (pmc == PMU_MS_PMC0) {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_CNTR);
    } else if (pmc == PMU_MS_PMC1) {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_CNTR);
    } else {
        return PMU_INCORRECT_COUNTER;
    }
    val = *ms_pmc_addr;
    return val;
}

// NOTE: THIS IS DEPRECATED
int64_t configure_pmcs(uint64_t reset_counters, uint64_t conf_area_addr);
int64_t sample_pmcs(uint64_t reset_counters, uint64_t log_buffer_addr);
int64_t reset_pmcs(void);

/*TODO: This time(cycles) stamping infrastructure does not account for
64 bit wrap at this time, should be improved, a proper time_stamping API
with required wrap tracking and time scaling logic should be implemented.
This will suffice for initial profiling needs for now, but this should
be improved */

/*! \def PMC_RESET_CYCLES_COUNTER
    \brief A macro to reset PMC minion cycles counter
*/
#define PMC_RESET_CYCLES_COUNTER   (uint64_t)pmu_core_counter_reset(PMU_MHPMEVENT3)

/*! \fn PMC_Get_Current_Cycles
    \brief A function to get current minion cycles based on PMC Counter 3 which
    setup by default to count the Minion cycles
*/
static inline uint64_t PMC_Get_Current_Cycles(void)  {
    uint64_t val;
    __asm__ __volatile__("csrr %[res], %[csr]\n" \
                          : [res] "=r"(val)      \
                          : [csr] "i"(PMU_HPMCOUNTER3)); \
    return val;
}

/*! \def PMC_GET_LATENCY
    \brief A macro to calculate latency. Uses PMC Counter 3 to get current cycle
    minus start_cycle(argument)
*/
#define PMC_GET_LATENCY(x) (uint32_t)(PMC_Get_Current_Cycles() - x)

#endif
