
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

// PMU support: Defines and basic API

// PMU Minions and Neigh event register addresses
#define PMU_MHPMEVENT3 0x323ULL
#define PMU_MHPMEVENT4 0x324ULL
#define PMU_MHPMEVENT5 0x325ULL
#define PMU_MHPMEVENT6 0x326ULL
#define PMU_MHPMEVENT7 0x327ULL
#define PMU_MHPMEVENT8 0x328ULL

// PMU Minions and Neigh event counter addresses
#define PMU_MHPMCOUNTER3 0xB03
#define PMU_MHPMCOUNTER4 0xB04
#define PMU_MHPMCOUNTER5 0xB05
#define PMU_MHPMCOUNTER6 0xB06
#define PMU_MHPMCOUNTER7 0xB07
#define PMU_MHPMCOUNTER8 0xB08

// Constants used to index inside PMC configuration buffer
#define PMU_MINION_COUNTERS_PER_HART 4
#define PMU_NEIGH_COUNTERS_PER_HART 2
#define PMU_SC_COUNTERS_PER_BANK 2
#define PMU_EVENT_SHIRE_AREA (PMU_MINION_COUNTERS_PER_HART*2 + PMU_NEIGH_COUNTERS_PER_HART*2 + PMU_SC_COUNTERS_PER_BANK)
#define PMU_EVENT_MEMSHIRE_OFFSET PMU_EVENT_SHIRE_AREA * 32
#define PMU_MS_COUNTERS_PER_MS 4

// Values to reset and start counting shire cache and memshire PMCs
#define PMU_SC_START_CTRL_VAL 0x00060033ULL
#define PMU_MS_START_CTRL_VAL 0x40DE06FFULL
#define PMU_SC_STOP_CTRL_VAL 0
#define PMU_MS_STOP_CTRL_VAL 0

// Indices of shire cache and memshire PMCs.
#define PMU_SC_CYCLE_PMC 0
#define PMU_SC_PMC0 1
#define PMU_SC_PMC1 2

#define PMU_MS_CYCLE_PMC 0
#define PMU_MS_PMC0 1
#define PMU_MS_PMC1 2

#define PMU_INCORRECT_COUNTER 0xFFFFFFFFFFFFFFFFULL

// Configure a neighborhood event counter (both minion and neigh events are handled here)
static inline int64_t configure_neigh_event(uint64_t evt, uint64_t evt_reg)
{
    switch (evt_reg) {
        case PMU_MHPMEVENT3:
            __asm__ __volatile__ ( "csrrw zero, %[csr], %[event]\n" : : [event] "r" (evt), [csr] "i" (PMU_MHPMEVENT3));
            break;
        case PMU_MHPMEVENT4:
            __asm__ __volatile__ ( "csrrw zero, %[csr], %[event]\n" : : [event] "r" (evt), [csr] "i" (PMU_MHPMEVENT4));
            break;
        case PMU_MHPMEVENT5:
            __asm__ __volatile__ ( "csrrw zero, %[csr], %[event]\n" : : [event] "r" (evt), [csr] "i" (PMU_MHPMEVENT5));
            break;
        case PMU_MHPMEVENT6:
            __asm__ __volatile__ ( "csrrw zero, %[csr], %[event]\n" : : [event] "r" (evt), [csr] "i" (PMU_MHPMEVENT6));
            break;
        case PMU_MHPMEVENT7:
            __asm__ __volatile__ ( "csrrw zero, %[csr], %[event]\n" : : [event] "r" (evt), [csr] "i" (PMU_MHPMEVENT7));
            break;
        case PMU_MHPMEVENT8:
            __asm__ __volatile__ ( "csrrw zero, %[csr], %[event]\n" : : [event] "r" (evt), [csr] "i" (PMU_MHPMEVENT8));
            break;
        default:
            return -1;
    }
    return 0;
}

// Write a neighborhood event counter (both minion and neigh events are handled here) 
static inline int64_t write_neigh_pmc(uint64_t pmc, uint64_t val)
{
    switch (pmc) {
        case PMU_MHPMCOUNTER3:
            __asm__ __volatile__ ( "csrw %[csr], %[init]\n" : : [init] "r" (val), [csr] "i" (PMU_MHPMCOUNTER3));
            break;
        case PMU_MHPMCOUNTER4:
            __asm__ __volatile__ ( "csrw %[csr], %[init]\n" : : [init] "r" (val), [csr] "i" (PMU_MHPMCOUNTER4));
            break;
        case PMU_MHPMCOUNTER5:
            __asm__ __volatile__ ( "csrw %[csr], %[init]\n" : : [init] "r" (val), [csr] "i" (PMU_MHPMCOUNTER5));
            break;
        case PMU_MHPMCOUNTER6:
            __asm__ __volatile__ ( "csrw %[csr], %[init]\n" : : [init] "r" (val), [csr] "i" (PMU_MHPMCOUNTER6));
            break;
        case PMU_MHPMCOUNTER7:
            __asm__ __volatile__ ( "csrw %[csr], %[init]\n" : : [init] "r" (val), [csr] "i" (PMU_MHPMCOUNTER7));
            break;
        case PMU_MHPMCOUNTER8:
            __asm__ __volatile__ ( "csrw %[csr], %[init]\n" : : [init] "r" (val), [csr] "i" (PMU_MHPMCOUNTER8));
            break;
        default:
            return -1;
    }
    return 0;
}

#define reset_neigh_pmc(pmc) write_neigh_pmc(pmc, 0)


// Read a neighborhood event counter (both minion and neigh events are handled here)
// Return -1 on incorrect counter
static inline uint64_t read_neigh_pmc(uint64_t pmc)
{
    uint64_t val = 0;
    switch (pmc) {
        case PMU_MHPMCOUNTER3:
            __asm__ __volatile__ ( "csrr %[res], %[csr]\n" : [res] "=r" (val) : [csr] "i" (PMU_MHPMCOUNTER3));
            break;
        case PMU_MHPMCOUNTER4:
            __asm__ __volatile__ ( "csrr %[res], %[csr]\n" : [res] "=r" (val) : [csr] "i" (PMU_MHPMCOUNTER4));
            break;
        case PMU_MHPMCOUNTER5:
            __asm__ __volatile__ ( "csrr %[res], %[csr]\n" : [res] "=r" (val) : [csr] "i" (PMU_MHPMCOUNTER5));
            break;
        case PMU_MHPMCOUNTER6:
            __asm__ __volatile__ ( "csrr %[res], %[csr]\n" : [res] "=r" (val) : [csr] "i" (PMU_MHPMCOUNTER6));
            break;
        case PMU_MHPMCOUNTER7:
            __asm__ __volatile__ ( "csrr %[res], %[csr]\n" : [res] "=r" (val) : [csr] "i" (PMU_MHPMCOUNTER7));
            break;
        case PMU_MHPMCOUNTER8:
            __asm__ __volatile__ ( "csrr %[res], %[csr]\n" : [res] "=r" (val) : [csr] "i" (PMU_MHPMCOUNTER8));
            break;
        default:
            return PMU_INCORRECT_COUNTER;
  }
  return 0;
}


// Configure an event for a shire cache perf counter
static inline int64_t configure_sc_event(uint64_t shire_id, uint64_t b, uint64_t evt_reg, uint64_t val)
{
    uint64_t *sc_bank_qualevt_addr = 0;
    int64_t ret = 0;
    if (evt_reg == 0) {
        sc_bank_qualevt_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_P0_QUAL);
    } else if (evt_reg == 1) {
        sc_bank_qualevt_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_P1_QUAL);
    } else {
        ret = -1;
    }
    *sc_bank_qualevt_addr = val;
    return ret;
}


// Reset and start a shire cache PMC
static inline void reset_sc_pmcs(uint64_t shire_id, uint64_t b)
{
    uint64_t *sc_bank_perfctrl_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_CTL_STATUS);
    *sc_bank_perfctrl_addr = PMU_SC_START_CTRL_VAL;
}


// Stop a shire cache PMC
static inline void stop_sc_pmcs(uint64_t shire_id, uint64_t b)
{
    uint64_t *sc_bank_perfctrl_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_CTL_STATUS);
    *sc_bank_perfctrl_addr = PMU_SC_STOP_CTRL_VAL;
}


// Read a shire cache PMC. Return -1 on incorrect counter
static uint64_t sample_sc_pmcs(uint64_t shire_id, uint64_t b, uint64_t pmc)
{
    uint64_t val = 0;
    uint64_t *sc_bank_pmc_addr = 0;
    if (pmc == PMU_SC_CYCLE_PMC) {
        sc_bank_pmc_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_CYC_CNTR);
    } else if (pmc == PMU_SC_PMC0) {
        sc_bank_pmc_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_P0_CNTR);
    } else if (pmc == PMU_SC_PMC1) {
        sc_bank_pmc_addr = (uint64_t *) ESR_CACHE(shire_id, b, SC_PERFMON_P1_CNTR);
    } else {
        return PMU_INCORRECT_COUNTER;
    }
    val = *sc_bank_pmc_addr;
    return val;
}


// Configure an event for memshire counters
static inline int64_t configure_ms_event(uint64_t ms_id, uint64_t evt_reg, uint64_t val)
{
    uint64_t *ms_pmc_ctrl_addr = 0;
    int64_t ret = 0;
    if (evt_reg == 0) {
        ms_pmc_ctrl_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_QUAL);
    } else if (evt_reg == 1) {
        ms_pmc_ctrl_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_QUAL);
    } else if (evt_reg == 2) {
        ms_pmc_ctrl_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_QUAL2);
    } else if (evt_reg == 3) {
        ms_pmc_ctrl_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_QUAL2);
    } else {
        ret = -1;
    }
    *ms_pmc_ctrl_addr = val;
    return ret;
}


// Reset and start a memshire PMC
static inline void reset_ms_pmcs(uint64_t ms_id)
{
    uint64_t *ms_pmc_ctrl_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);
    *ms_pmc_ctrl_addr = PMU_MS_START_CTRL_VAL;
}


// Stop a memshire PMC
static inline void stop_ms_pmcs(uint64_t ms_id)
{
    uint64_t *ms_pmc_ctrl_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CTL_STATUS);
    *ms_pmc_ctrl_addr = PMU_MS_STOP_CTRL_VAL;
}


// Read a memshire PMC. Return -1 on incorrect counter
static inline uint64_t sample_ms_pmcs(uint64_t ms_id, uint64_t pmc)
{
    uint64_t val = 0;
    uint64_t *ms_pmc_addr = 0;
    if (pmc == PMU_MS_CYCLE_PMC) {
        ms_pmc_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_CYC_CNTR);
    } else if (pmc == PMU_MS_PMC0) {
        ms_pmc_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P0_CNTR);
    } else if (pmc == PMU_MS_PMC1) {
        ms_pmc_addr = (uint64_t *) ESR_DDRC(MEMSHIRE_SHIREID(ms_id), DDRC_PERFMON_P1_CNTR);
    } else {
        return PMU_INCORRECT_COUNTER;
    }
    val = *ms_pmc_addr;
    return val;
}


#endif
