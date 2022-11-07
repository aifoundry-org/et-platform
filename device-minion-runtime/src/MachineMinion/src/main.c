/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file consists the main method that serves as the entry
*       point for the the c runtime.
*
*   FUNCTIONS
*
*       main
*
***********************************************************************/
#include <stdint.h>

/* minion_bl */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>

/* Machine minion specific headers */
#include "config/mm_config.h"

static inline void initialize_scp(uint32_t shire_id)
{
    /* Setup cache op state machine to zero out the SCP region of the given shire */
    __asm__ __volatile__("li t0, 0x00000901\n");

    for (uint64_t bank = 0; bank < 4; bank++)
    {
        /* Setup the ESR address */
        uint64_t esr = 0x01c0300030U | (shire_id << 22U) | (bank << 13U);
        __asm__ __volatile__("mv t1, %[esr]\n"
                             "sd t0, 0(t1)\n"
                             :
                             : [esr] "r"(esr));
    }
    __asm__ __volatile__("fence iorw, iorw\n");
}

static inline void mm_setup_default_pmcs(uint32_t shire_id, uint32_t hart_id)
{
    /* Enable counter in 1st core of the neighborhood only. */
    if ((hart_id % 16 == 0) || (hart_id % 16 == 1))
    {
        /* Configure mhpmevent3 for each neighborhood to count cycles.
        Enabling it for all cores will accumulate the cycles from all
        the cores and won't give us any advantage */
        pmu_core_event_configure(PMU_MHPMEVENT3, PMU_MINION_EVENT_CYCLES);

        /* Configure mhpmevent7 for each neighborhood to count minion icache requests */
        pmu_core_event_configure(PMU_MHPMEVENT7, PMU_NEIGH_EVENT_MINION_ICACHE_REQ);

        /* Configure mhpmevent8 for each neighborhood to count icache etlink requests */
        pmu_core_event_configure(PMU_MHPMEVENT8, PMU_NEIGH_EVENT_ICACHE_ETLINK_REQ);

        /* A single hart each neighborhood configures SC PMCs */
        if (hart_id % 16 == 0)
        {
            configure_sc_pmcs(PMU_SC_CTL_STATUS_MASK, PMU_SC_L2_READS, PMU_SC_L2_WRITES);

            uint64_t neigh_id = (hart_id >> 4) & 0x3;

            /* Start Shire Cache PMC counters */
            pmu_shire_cache_counter_start(shire_id, neigh_id, PMU_SC_ALL);
        }
    }

    /* A single hart programs the PMCs of mem shires 0-7 */
    if ((shire_id == PMU_MS_COUNTERS_CONTROL_SHIRE) && (get_neighborhood_id() == 3) &&
        ((hart_id & 0xF) == NEIGH_HART_MS))
    {
        for (uint8_t ms_idx = 0; ms_idx < PMU_MEM_SHIRE_COUNT; ms_idx++)
        {
            configure_ms_pmcs(ms_idx, PMU_MS_CTL_STATUS_MASK, PMU_MS_QUAL_ALL_MESH_READS,
                PMU_MS_QUAL_ALL_MESH_WRITES, 0, 0);

            /* Start the counters */
            pmu_memshire_event_start(ms_idx, PMU_MS_ALL);
        }
    }

    /********************************************/
    /* Every hart configures the counters below */
    /********************************************/

    /* Configure mhpmevent4 for each hart to count retired inst0 */
    pmu_core_event_configure(PMU_MHPMEVENT4, PMU_MINION_EVENT_RETIRED_INST0);

    /* Configure mhpmevent5 for each hart to count retired inst1 */
    pmu_core_event_configure(PMU_MHPMEVENT5, PMU_MINION_EVENT_RETIRED_INST1);

    /* Configure mhpmevent6 for each hart to count L2 miss req */
    pmu_core_event_configure(PMU_MHPMEVENT6, PMU_MINION_EVENT_L2_MISS_REQ);

    /* Reset all the PMCs. Details of which hart resets which PMC can be found in PMU component */
    reset_minion_neigh_pmcs_all();
    reset_sc_pmcs_all();
    reset_ms_pmcs_all();
}

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    uint32_t hart_id = get_hart_id();
    uint32_t shire_id = get_shire_id();

    // "Upon reset, a hart's privilege mode is set to M. The mstatus fields MIE and MPRV are reset to 0.
    // The pc is set to an implementation-defined reset vector. The mcause register is set to a value
    // indicating the cause of the reset. All other hart state is undefined."

    asm volatile(
        "la    %0, trap_handler     \n" // Setup machine mode trap handler
        "csrw  mtvec, %0            \n"
        "li    %0, 0x800333         \n" // Delegate supervisor and user software, timer, bus error and external interrupts to supervisor mode
        "csrs  mideleg, %0          \n"
        "li    %0, 0x5C00B1F7       \n" // Delegate all RISC-V and ET specific relevant exceptions to supervisor mode
        "csrs  medeleg, %0          \n"
        "csrwi menable_shadows, 0x3 \n" // Enable shadow registers for hartid and sleep txfma
        "li    %0, 0x800008         \n" // Enable machine software interrupts, ET Bus Error Interrupt[23]
        "csrs  mie, %0              \n"
        "csrsi mstatus, 0x8         \n" // Enable interrupts
        : "=&r"(temp));

    asm volatile(
        "li    %0, 0x1020  \n" // Setup for mret into S-mode: bitmask for mstatus MPP[1] and SPIE
        "csrc  mstatus, %0 \n" // clear mstatus MPP[1] = supervisor mode, SPIE = interrupts disabled
        "li    %0, 0x800   \n" // bitmask for mstatus MPP[0]
        "csrs  mstatus, %0 \n" // set mstatus MPP[0] = supervisor mode
        : "=&r"(temp));

    // Enable all available PMU counters to be sampled in S-mode
    asm volatile("csrw mcounteren, %0\n" : : "r"(((1u << PMU_NR_HPM) - 1) << PMU_FIRST_HPM));

    /* Setup the default events for PMCs */
    mm_setup_default_pmcs(shire_id, hart_id);

    /* First HART every shire, master or worker */
    if (hart_id % 64 == 0)
    {
        // Block user-level PC redirection
        volatile uint64_t *const ipi_redirect_filter_ptr =
            (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, IPI_REDIRECT_FILTER);
        *ipi_redirect_filter_ptr = 0;

        /* Initialize Shire L2 SCP */
        initialize_scp(shire_id);
    }

    /* Master shire non-sync minions (lower 16) */
    if ((shire_id == MM_SHIRE_ID) && ((get_minion_id() & 0x1F) < 16))
    {
        const uint64_t *const master_entry = (uint64_t *)FW_MASTER_SMODE_ENTRY;

        // Jump to master firmware in supervisor mode
        asm volatile("csrw  mepc, %0 \n" // write return address
                     "mret           \n" // return in S-mode
                     :
                     : "r"(master_entry));
    }
    else
    {
        // Worker shire and Master shire sync-minions (upper 16)
        const uint64_t *const worker_entry = (uint64_t *)FW_WORKER_SMODE_ENTRY;

        // Jump to worker firmware in supervisor mode
        asm volatile("csrw  mepc, %0 \n" // write return address
                     "mret           \n" // return in S-mode
                     :
                     : "r"(worker_entry));
    }

    while (1)
    {
        // Should never get here
    }
}
