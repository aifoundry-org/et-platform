
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "etsoc/isa/esr_defines.h"

// PMU support: Defines and basic API
typedef uint8_t hpm_counter_e;

enum hpm_counter {
    HPM_COUNTER_3 = 0,
    HPM_COUNTER_4,
    HPM_COUNTER_5,
    HPM_COUNTER_6,
    HPM_COUNTER_7,
    HPM_COUNTER_8
};

typedef struct shire_pmc_cnt_ {
    uint64_t cycle;
    uint64_t pmc0;
    uint64_t pmc1;
    bool cycle_overflow;
    bool pmc0_overflow;
    bool pmc1_overflow;
} shire_pmc_cnt_t;

// ETSoC-1 PMU hardware
#define PMU_FIRST_HPM     3
#define PMU_LAST_CORE_HPM 6
#define PMU_LAST_HPM      8
#define PMU_NR_HPM        (PMU_LAST_HPM - PMU_FIRST_HPM + 1)

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

// Constants used in PMU APIs
#define PMU_MINION_COUNTERS_PER_HART 4
#define PMU_NEIGH_COUNTERS_PER_HART  2
#define PMU_SC_COUNTERS_PER_BANK     3
#define PMU_MS_COUNTERS_PER_MS       3
#define PMU_MS_COUNTERS_CONTROL_SHIRE \
    0 /* ID of the Minion Shire responsible for configuration and reset on MS PMCs */
#define PMU_MEM_SHIRE_COUNT 8

// Thread in neigh that does all the PMC configuration, starting / topping and sampling
// There is a 1-1 neigh-bank mapping. Only 1 hread needs to do that to avoid races as the CTL_STATUS register is shared
// among counters / events
#define NEIGH_HART_SC 12
#define NEIGH_HART_MS 14

// Values to reset and start counting shire cache and memshire PMCs
// For the time we ignore overflows and disable interrupts.
// This could be moved in the configuration buffer but we need to add support for
// overflows, etc in the firmware in that case.
#define PMU_SC_START_ALL_CTRL_VAL 0x00060033ULL
#define PMU_SC_STOP_ALL_CTRL_VAL  ~PMU_SC_START_ALL_CTRL_VAL
#define PMU_MS_START_ALL_CTRL_VAL 0x00060033ULL
#define PMU_MS_STOP_ALL_CTRL_VAL  ~PMU_MS_START_ALL_CTRL_VAL

#define PMU_SC_START_CYCLE_CNT_CTRL (0x1ULL << 0)
#define PMU_SC_START_P0_CTRL        (0x1ULL << 4)
#define PMU_SC_START_P1_CTRL        (0x1ULL << 17)

#define PMU_SC_RESET_CYCLE_CNT_CTRL (0x2ULL << 0)
#define PMU_SC_RESET_P0_CTRL        (0x2ULL << 4)
#define PMU_SC_RESET_P1_CTRL        (0x2ULL << 17)

#define PMU_SC_SO_CYCLE_CNT_BIT  33
#define PMU_SC_SO_P0_BIT         35
#define PMU_SC_SO_P1_BIT         37
#define PMU_SC_SO_CYCLE_CNT_CTRL (0x1ULL << PMU_SC_SO_CYCLE_CNT_BIT)
#define PMU_SC_SO_P0_CTRL        (0x1ULL << PMU_SC_SO_P0_BIT)
#define PMU_SC_SO_P1_CTRL        (0x1ULL << PMU_SC_SO_P1_BIT)

#define PMU_MS_START_CYCLE_CNT_CTRL (0x1ULL << 0)
#define PMU_MS_START_P0_CTRL        (0x1ULL << 4)
#define PMU_MS_START_P1_CTRL        (0x1ULL << 17)

#define PMU_MS_RESET_CYCLE_CNT_CTRL (0x2ULL << 0)
#define PMU_MS_RESET_P0_CTRL        (0x2ULL << 4)
#define PMU_MS_RESET_P1_CTRL        (0x2ULL << 17)

#define PMU_MS_SO_CYCLE_CNT_BIT  33
#define PMU_MS_SO_P0_BIT         35
#define PMU_MS_SO_P1_BIT         37
#define PMU_MS_SO_CYCLE_CNT_CTRL (0x1ULL << PMU_MS_SO_CYCLE_CNT_BIT)
#define PMU_MS_SO_P0_CTRL        (0x1ULL << PMU_MS_SO_P0_BIT)
#define PMU_MS_SO_P1_CTRL        (0x1ULL << PMU_MS_SO_P1_BIT)

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

// Neighborhood events
#define PMU_NEIGH_EVENT_NONE                         0
#define PMU_NEIGH_EVENT_MINION_ETLINK_REQ            1
#define PMU_NEIGH_EVENT_MINION_ETLINK_RSP            2
#define PMU_NEIGH_EVENT_COOP_LOAD_REQ                3
#define PMU_NEIGH_EVENT_INTER_COOP_LOAD_REQ          4
#define PMU_NEIGH_EVENT_COOP_LOAD_RSP                5
#define PMU_NEIGH_EVENT_COOP_STORE_REQ               6
#define PMU_NEIGH_EVENT_COOP_STORE_RSP               7
#define PMU_NEIGH_EVENT_MINION_ICACHE_REQ            8
#define PMU_NEIGH_EVENT_MINION_ICACHE_RSP            9
#define PMU_NEIGH_EVENT_MINION_PTW_REQ               10
#define PMU_NEIGH_EVENT_MINION_PTW_RSP               11
#define PMU_NEIGH_EVENT_MINION_FLN_MSG               12
#define PMU_NEIGH_EVENT_ICACHE_ETLINK_REQ            13
#define PMU_NEIGH_EVENT_ICACHE_ETLINK_RSP            14
#define PMU_NEIGH_EVENT_ICACHE_L1_SRAM_REQ           15
#define PMU_NEIGH_EVENT_ICACHE_L1_SRAM_RSP           16
#define PMU_NEIGH_EVENT_PTW_ETLINK_REQ               17
#define PMU_NEIGH_EVENT_PTW_ETLINK_RSP               18
#define PMU_NEIGH_EVENT_INTER_FIFO_PUSH_ETLINK_REQ   21
#define PMU_NEIGH_EVENT_BANK_UC_FIFO_PUSH_ETLINK_REQ 22
#define PMU_NEIGH_EVENT_SC_UC_ETLINK_RSP             23

/******************************************* SC PMCs Event mode 0 qual spec ******************************************/
/* Link: https://docs.google.com/document/d/1zMfIFV49HJY2qijftTB9n8vMKYFr-7iupdJ2q657Wps/edit#heading=h.u87axsdehb9h */
/*                                                                                                                   */
/*********************************************************************************************************************/

/* SC default config values. These values are Hardware ESRs of PMC component. */
#define PMU_SC_CTL_STATUS_MASK \
    0x280144 /* Enables p0 and p1 counters in event mode 0, overflow bits for cycle, p0 and p1. */
#define PMU_SC_L2_READS  0x7FF0
#define PMU_SC_L2_WRITES 0xBFF0
#define PMU_SC_MSG_SEND  0x4

/* MS default config values */
#define PMU_MS_CTL_STATUS_MASK 0xC80644

/*! \def PMU_MS_QUAL_ALL_MESH_READS
    \brief Enable memory shire reads for all Minion Shires, IOShire, and PShire.
*/
#define PMU_MS_QUAL_ALL_MESH_READS 0x00001FDULL

/*! \def PMU_MS_QUAL_MINIONS_MESH_READS
    \brief Enable memory shire reads for Minion Shires only.
*/
#define PMU_MS_QUAL_MINIONS_MESH_READS 0x000013DULL

/*! \def PMU_MS_QUAL_PSHIRE_MESH_READS
    \brief Enable memory shire reads for PShire only.
*/
#define PMU_MS_QUAL_PSHIRE_MESH_READS 0x00000BDULL

/*! \def PMU_MS_QUAL_IOSHIRE_MESH_READS
    \brief Enable memory shire reads for IOShire only.
*/
#define PMU_MS_QUAL_IOSHIRE_MESH_READS 0x000007DULL

/*! \def PMU_MS_QUAL_ALL_MESH_WRITES
    \brief Enable memory shire writes for all Minion Shires, IOShire, and PShire.
*/
#define PMU_MS_QUAL_ALL_MESH_WRITES 0x00001FEULL

/*! \def PMU_MS_QUAL_MINIONS_MESH_WRITES
    \brief Enable memory shire writes for Minion Shires only.
*/
#define PMU_MS_QUAL_MINIONS_MESH_WRITES 0x000013EULL

/*! \def PMU_MS_QUAL_PSHIRE_MESH_WRITES
    \brief Enable memory shire writes for PShire only.
*/
#define PMU_MS_QUAL_PSHIRE_MESH_WRITES 0x00000BEULL

/*! \def PMU_MS_QUAL_IOSHIRE_MESH_WRITES
    \brief Enable memory shire writes for IOShire only.
*/
#define PMU_MS_QUAL_IOSHIRE_MESH_WRITES 0x000007EULL

// Indices of shire cache and memshire PMCs.
#define PMU_SC_CYCLE_PMC 0
#define PMU_SC_PMC0      1
#define PMU_SC_PMC1      2
#define PMU_SC_ALL       3

#define PMU_MS_CYCLE_PMC 0
#define PMU_MS_PMC0      1
#define PMU_MS_PMC1      2
#define PMU_MS_ALL       3

#define PMU_INCORRECT_COUNTER 0xFFFFFFFFFFFFFFFFULL

// Configure a core (minion and neighborhood) event counter
static inline int64_t pmu_core_event_configure(uint64_t evt_reg, uint64_t evt)
{
    switch (evt_reg)
    {
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
    switch (pmc)
    {
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

// Read a core (minion and neighborhood) M-mode event counter
// Return -1 on incorrect counter
static inline uint64_t pmu_core_counter_read_priv(uint64_t pmc)
{
    uint64_t val = 0;
    switch (pmc)
    {
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

/* Read a core (minion and neighborhood) U-mode and S-mode event counters */
static inline uint64_t pmu_core_counter_read_unpriv(hpm_counter_e pmc)
{
    uint64_t value = 0;

    switch (pmc)
    {
        case HPM_COUNTER_3:
            __asm__ __volatile__("csrr %0, hpmcounter3\n" : "=r"(value));
            break;
        case HPM_COUNTER_4:
            __asm__ __volatile__("csrr %0, hpmcounter4\n" : "=r"(value));
            break;
        case HPM_COUNTER_5:
            __asm__ __volatile__("csrr %0, hpmcounter5\n" : "=r"(value));
            break;
        case HPM_COUNTER_6:
            __asm__ __volatile__("csrr %0, hpmcounter6\n" : "=r"(value));
            break;
        case HPM_COUNTER_7:
            __asm__ __volatile__("csrr %0, hpmcounter7\n" : "=r"(value));
            break;
        case HPM_COUNTER_8:
            __asm__ __volatile__("csrr %0, hpmcounter8\n" : "=r"(value));
            break;
        default:
            break;
    }

    return value;
}

// Configure an event for a shire cache perf counter
static inline int64_t pmu_shire_cache_event_configure(
    uint64_t shire_id, uint64_t b, uint64_t evt_reg, uint64_t val)
{
    uint64_t *sc_bank_qualevt_addr = 0;
    if (evt_reg == 0)
    {
        sc_bank_qualevt_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P0_QUAL);
    }
    else if (evt_reg == 1)
    {
        sc_bank_qualevt_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P1_QUAL);
    }
    else
    {
        return -1;
    }
    *sc_bank_qualevt_addr = val;
    return 0;
}

// Set a shire cache PMC to a value
static inline void pmu_shire_cache_counter_set(uint64_t shire_id, uint64_t b, uint64_t val)
{
    uint64_t *sc_bank_perfctrl_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_CTL_STATUS);
    *sc_bank_perfctrl_addr = val;
}

// Reset a shire cache PMC
// It directly sets the counter value to zero. It does not make use of reset bit in ESR control register
// because to take affect it needs to start the counter
static inline void pmu_shire_cache_counter_reset(uint64_t shire_id, uint64_t bank_id, uint64_t pmc)
{
    uint64_t *sc_bank_pmc_addr = 0;
    if (pmc == PMU_SC_CYCLE_PMC)
    {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_CYC_CNTR);
        *sc_bank_pmc_addr = 0;
    }
    else if (pmc == PMU_SC_PMC0)
    {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_P0_CNTR);
        *sc_bank_pmc_addr = 0;
    }
    else if (pmc == PMU_SC_PMC1)
    {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_P1_CNTR);
        *sc_bank_pmc_addr = 0;
    }
}

// Start a shire cache PMC
static inline void pmu_shire_cache_counter_start(uint64_t shire_id, uint64_t bank_id, uint64_t pmc)
{
    uint64_t *sc_bank_perfctrl_addr =
        (uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_CTL_STATUS);

    if (pmc == PMU_SC_ALL)
    {
        *sc_bank_perfctrl_addr |=
            (PMU_SC_START_CYCLE_CNT_CTRL | PMU_SC_START_P0_CTRL | PMU_SC_START_P1_CTRL);
    }
    else if (pmc == PMU_SC_CYCLE_PMC)
    {
        *sc_bank_perfctrl_addr |= PMU_SC_START_CYCLE_CNT_CTRL;
    }
    else if (pmc == PMU_SC_PMC0)
    {
        *sc_bank_perfctrl_addr |= PMU_SC_START_P0_CTRL;
    }
    else if (pmc == PMU_SC_PMC1)
    {
        *sc_bank_perfctrl_addr |= PMU_SC_START_P1_CTRL;
    }
}

// Stop a shire cache PMC
static inline void pmu_shire_cache_counter_stop(uint64_t shire_id, uint64_t bank_id, uint64_t pmc)
{
    uint64_t *sc_bank_perfctrl_addr =
        (uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_CTL_STATUS);

    if (pmc == PMU_SC_ALL)
    {
        *sc_bank_perfctrl_addr &=
            ~(PMU_SC_START_CYCLE_CNT_CTRL | PMU_SC_START_P0_CTRL | PMU_SC_START_P1_CTRL);
    }
    else if (pmc == PMU_SC_CYCLE_PMC)
    {
        *sc_bank_perfctrl_addr &= ~PMU_SC_START_CYCLE_CNT_CTRL;
    }
    else if (pmc == PMU_SC_PMC0)
    {
        *sc_bank_perfctrl_addr &= ~PMU_SC_START_P0_CTRL;
    }
    else if (pmc == PMU_SC_PMC1)
    {
        *sc_bank_perfctrl_addr &= ~PMU_SC_START_P1_CTRL;
    }
}

// Read a shire cache PMC. Return -1 on incorrect counter
static inline uint64_t pmu_shire_cache_counter_sample(uint64_t shire_id, uint64_t b, uint64_t pmc)
{
    uint64_t val = 0;
    uint64_t *sc_bank_pmc_addr = 0;
    if (pmc == PMU_SC_CYCLE_PMC)
    {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_CYC_CNTR);
    }
    else if (pmc == PMU_SC_PMC0)
    {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P0_CNTR);
    }
    else if (pmc == PMU_SC_PMC1)
    {
        sc_bank_pmc_addr = (uint64_t *)ESR_CACHE(shire_id, b, SC_PERFMON_P1_CNTR);
    }
    else
    {
        return PMU_INCORRECT_COUNTER;
    }
    val = *sc_bank_pmc_addr;
    return val;
}

// Read a shire cache PMC. Return -1 on error
static inline int32_t pmu_shire_cache_counter_sample_all(
    uint64_t shire_id, uint64_t bank_id, shire_pmc_cnt_t *val)
{
    if (val != NULL)
    {
        uint64_t sc_bank_perfctrl =
            *((uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_CTL_STATUS));

        // Check for overflow and reset counter. Is reset really required
        // or the counter is automatically reset when started after overflow?
        val->cycle_overflow = (sc_bank_perfctrl >> PMU_SC_SO_CYCLE_CNT_BIT) & 1;
        val->pmc0_overflow = (sc_bank_perfctrl >> PMU_SC_SO_P0_BIT) & 1;
        val->pmc1_overflow = (sc_bank_perfctrl >> PMU_SC_SO_P1_BIT) & 1;
        if (val->cycle_overflow)
        {
            pmu_shire_cache_counter_reset(shire_id, bank_id, PMU_SC_CYCLE_PMC);
        }
        if (val->pmc0_overflow)
        {
            pmu_shire_cache_counter_reset(shire_id, bank_id, PMU_SC_PMC0);
        }
        if (val->pmc1_overflow)
        {
            pmu_shire_cache_counter_reset(shire_id, bank_id, PMU_SC_PMC1);
        }
        val->cycle = *((uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_CYC_CNTR));
        val->pmc0 = *((uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_P0_CNTR));
        val->pmc1 = *((uint64_t *)ESR_CACHE(shire_id, bank_id, SC_PERFMON_P1_CNTR));
        return 0;
    }
    return -1;
}

// Configure an event for memshire counters
static inline int64_t pmu_memshire_event_configure(uint64_t ms_id, uint64_t evt_reg, uint64_t val)
{
    uint64_t *ms_pmc_ctrl_addr = 0;
    if (evt_reg == 0)
    {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_QUAL);
    }
    else if (evt_reg == 1)
    {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_QUAL);
    }
    else if (evt_reg == 2)
    {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_QUAL2);
    }
    else if (evt_reg == 3)
    {
        ms_pmc_ctrl_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_QUAL2);
    }
    else
    {
        return -1;
    }
    *ms_pmc_ctrl_addr = val;
    return 0;
}

// Set a memshire PMC to a value
static inline void pmu_memshire_event_set(uint64_t ms_id, uint64_t val)
{
    uint64_t *ms_perfctrl_addr =
        (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);
    *ms_perfctrl_addr = val;
}

// Reset a memshire PMC
// It directly sets the counter value to zero. It does not make use of reset bit in ESR control register
// because to take affect it needs to start the counter
static inline void pmu_memshire_event_reset(uint64_t ms_id, uint64_t pmc)
{
    uint64_t *ms_pmc_addr = 0;
    if (pmc == PMU_MS_CYCLE_PMC)
    {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CYC_CNTR);
        *ms_pmc_addr = 0;
    }
    else if (pmc == PMU_MS_PMC0)
    {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_CNTR);
        *ms_pmc_addr = 0;
    }
    else if (pmc == PMU_MS_PMC1)
    {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_CNTR);
        *ms_pmc_addr = 0;
    }
}

// Start a memshire PMC
static inline void pmu_memshire_event_start(uint64_t ms_id, uint64_t pmc)
{
    uint64_t *ms_pmc_ctrl_addr =
        (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);

    if (pmc == PMU_MS_ALL)
    {
        *ms_pmc_ctrl_addr |=
            (PMU_MS_START_CYCLE_CNT_CTRL | PMU_MS_START_P0_CTRL | PMU_MS_START_P1_CTRL);
    }
    else if (pmc == PMU_MS_CYCLE_PMC)
    {
        *ms_pmc_ctrl_addr |= PMU_MS_START_CYCLE_CNT_CTRL;
    }
    else if (pmc == PMU_MS_PMC0)
    {
        *ms_pmc_ctrl_addr |= PMU_MS_START_P0_CTRL;
    }
    else if (pmc == PMU_MS_PMC1)
    {
        *ms_pmc_ctrl_addr |= PMU_MS_START_P1_CTRL;
    }
}

// Stop a memshire PMC
static inline void pmu_memshire_event_stop(uint64_t ms_id, uint64_t pmc)
{
    uint64_t *ms_pmc_ctrl_addr =
        (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);

    if (pmc == PMU_MS_ALL)
    {
        *ms_pmc_ctrl_addr &=
            ~(PMU_MS_START_CYCLE_CNT_CTRL | PMU_MS_START_P0_CTRL | PMU_MS_START_P1_CTRL);
    }
    else if (pmc == PMU_MS_CYCLE_PMC)
    {
        *ms_pmc_ctrl_addr &= ~PMU_MS_START_CYCLE_CNT_CTRL;
    }
    else if (pmc == PMU_MS_PMC0)
    {
        *ms_pmc_ctrl_addr &= ~PMU_MS_START_P0_CTRL;
    }
    else if (pmc == PMU_MS_PMC1)
    {
        *ms_pmc_ctrl_addr &= ~PMU_MS_START_P1_CTRL;
    }
}

// Read a memshire PMC. Return -1 on incorrect counter
static inline uint64_t pmu_memshire_event_sample(uint64_t ms_id, uint64_t pmc)
{
    uint64_t val = 0;
    uint64_t *ms_pmc_addr = 0;
    if (pmc == PMU_MS_CYCLE_PMC)
    {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CYC_CNTR);
    }
    else if (pmc == PMU_MS_PMC0)
    {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_CNTR);
    }
    else if (pmc == PMU_MS_PMC1)
    {
        ms_pmc_addr = (uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_CNTR);
    }
    else
    {
        return PMU_INCORRECT_COUNTER;
    }
    val = *ms_pmc_addr;
    return val;
}

// Read a memshire PMC. Return -1 on error
static inline int32_t pmu_memshire_event_sample_all(uint64_t ms_id, shire_pmc_cnt_t *val)
{
    if ((val != NULL) && (ms_id < PMU_MEM_SHIRE_COUNT))
    {
        uint64_t ms_pmc_ctrl =
            *((uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS));

        // Check for overflow and reset counter. Is reset really required
        // or the counter is automatically reset when started after overflow?
        val->cycle_overflow = (ms_pmc_ctrl >> PMU_MS_SO_CYCLE_CNT_BIT) & 1;
        val->pmc0_overflow = (ms_pmc_ctrl >> PMU_MS_SO_P0_BIT) & 1;
        val->pmc1_overflow = (ms_pmc_ctrl >> PMU_MS_SO_P1_BIT) & 1;
        if (val->cycle_overflow)
        {
            pmu_memshire_event_reset(ms_id, PMU_MS_CYCLE_PMC);
        }
        if (val->pmc0_overflow)
        {
            pmu_memshire_event_reset(ms_id, PMU_MS_PMC0);
        }
        if (val->pmc1_overflow)
        {
            pmu_memshire_event_reset(ms_id, PMU_MS_PMC1);
        }
        val->cycle = *((uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CYC_CNTR));
        val->pmc0 = *((uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_CNTR));
        val->pmc1 = *((uint64_t *)ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_CNTR));
        return 0;
    }

    return -1;
}

int64_t configure_sc_pmcs(uint64_t ctl_status_cfg, uint64_t pmc0_cfg, uint64_t pmc1_cfg);
int64_t configure_ms_pmcs(uint64_t ms_id, uint64_t ctl_status_cfg, uint64_t ddrc_perfmon_p0_qual,
    uint64_t ddrc_perfmon_p1_qual, uint64_t ddrc_perfmon_p0_qual2, uint64_t ddrc_perfmon_p1_qual2);
uint64_t sample_sc_pmcs(uint64_t shire_id, uint64_t neigh_id, uint64_t pmc);
uint64_t sample_ms_pmcs(uint64_t ms_id, uint64_t pmc);
int32_t sample_sc_pmcs_all(uint64_t shire_id, uint64_t bank_id, shire_pmc_cnt_t *val);
int32_t sample_ms_pmcs_all(uint64_t ms_id, shire_pmc_cnt_t *val);
int64_t reset_minion_neigh_pmcs_all(void);
int64_t reset_sc_pmcs_all(void);
int64_t reset_ms_pmcs_all(void);

/*TODO: This time(cycles) stamping infrastructure does not account for
64 bit wrap at this time, should be improved, a proper time_stamping API
with required wrap tracking and time scaling logic should be implemented.
This will suffice for initial profiling needs for now, but this should
be improved */

/*! \fn PMC_Get_Current_Cycles
    \brief A function to get current minion cycles based on PMC Counter 3 which
    setup by default to count the Minion cycles
*/
static inline uint64_t PMC_Get_Current_Cycles(void)
{
    uint64_t val;
    __asm__ __volatile__("csrr %0, hpmcounter3\n" : "=r"(val));
    return val;
}

/*! \def PMC_GET_LATENCY
    \brief A macro to calculate latency. Uses PMC Counter 3 to get current cycle
    minus start_cycle(argument)
*/
#define PMC_GET_LATENCY(x) (PMC_Get_Current_Cycles() - x)

#ifdef __cplusplus
}
#endif

#endif
