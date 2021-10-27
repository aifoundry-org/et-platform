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

/* minion_rt_helpers */
#include "layout.h"

/* Machine minion specific headers */
#include "config/mm_config.h"

/* Global variable to keep track of Machine Minions boot */
static spinlock_t MM_Thread_Boot_Counter[NUM_SHIRES] = { 0 };

static inline void initialize_scp(void)
{
    /* setup cache op state machine to zero out the SCP region */
    __asm__ __volatile__(
        "li t0, 0x00000901\n"

        "li t1, 0x01c0300030\n"
        "sd t0, 0(t1)\n"

        "li t1, 0x01c0302030\n"
        "sd t0, 0(t1)\n"

        "li t1, 0x01c0304030\n"
        "sd t0, 0(t1)\n"

        "li t1, 0x01c0306030\n"
        "sd t0, 0(t1)\n"

        "fence iorw, iorw\n"
            : :
    );
}

static inline void mm_setup_default_pmcs(uint32_t hart_id)
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
            configure_sc_pmcs(PMU_SC_CTL_STATUS_MASK, PMU_SC_L2_READS, PMU_SC_MSG_SEND);
        }
    }

    /* A single hart in shires 0-7 programs the PMCs of mem shires 0-7 */
    if (((hart_id & 0xF) == NEIGH_HART_MS) && (get_shire_id() < 8) && (get_neighborhood_id() == 3))
    {
        configure_ms_pmcs(PMU_MS_CTL_STATUS_MASK, PMU_MS_MESH_READS, PMU_MS_MESH_WRITES, 0, 0);
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

    /* Reset the PMCs. Detatils of which hart resets which PMC can be found in PMU component */
    reset_pmcs();
}

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    uint32_t hart_id = get_hart_id();

    // "Upon reset, a hart's privilege mode is set to M. The mstatus fields MIE and MPRV are reset to 0.
    // The pc is set to an implementation-defined reset vector. The mcause register is set to a value
    // indicating the cause of the reset. All other hart state is undefined."

    /* TODO: Use medeleg to delegate U-mode exceptions directly to S-mode such as illegal instruction, etc)
     *       this way if a kernel has an exception it will be processed faster. Right now U-mode
     *       exceptions trap to M-mode, so M-mode is manually delegating them to S-mode, which
     *       introduces a latency/performance hit... (check trap_routine) */

    asm volatile(
        "la    %0, trap_handler     \n" // Setup machine mode trap handler
        "csrw  mtvec, %0            \n"
        "li    %0, 0x333            \n" // Delegate supervisor and user software, timer and external interrupts to supervisor mode
        "csrs  mideleg, %0          \n"
        "li    %0, 0x100            \n" // Delegate user environment calls to supervisor mode
        "csrs  medeleg, %0          \n"
        "csrwi menable_shadows, 0x3 \n" // Enable shadow registers for hartid and sleep txfma
        "csrsi mie, 0x8             \n" // Enable machine software interrupts
        "csrsi mstatus, 0x8         \n" // Enable interrupts
        : "=&r"(temp));

    asm volatile(
        "li    %0, 0x1020  \n" // Setup for mret into S-mode: bitmask for mstatus MPP[1] and SPIE
        "csrc  mstatus, %0 \n" // clear mstatus MPP[1] = supervisor mode, SPIE = interrupts disabled
        "li    %0, 0x800   \n" // bitmask for mstatus MPP[0]
        "csrs  mstatus, %0 \n" // set mstatus MPP[0] = supervisor mode
        : "=&r"(temp));

    // Enable all available PMU counters to be sampled in S-mode
    asm volatile("csrw mcounteren, %0\n"
        : : "r"(((1u << PMU_NR_HPM) - 1) << PMU_FIRST_HPM));

    /* Init global console lock */
    if (hart_id == MM_DISPATCHER_HART_ID)
    {
        init_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR, 0);
    }

    /* Setup the default events for PMCs */
    mm_setup_default_pmcs(hart_id);

    if (hart_id % 64 == 0) { // First HART every shire, master or worker
        // Block user-level PC redirection
        volatile uint64_t *const ipi_redirect_filter_ptr =
            (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, IPI_REDIRECT_FILTER);
        *ipi_redirect_filter_ptr = 0;

        /* Initialize Shire L2 SCP */
        initialize_scp();
    }

    if ((get_shire_id() == 32) &&
        ((get_minion_id() & 0x1F) < 16)) { // Master shire non-sync minions (lower 16)
        const uint64_t *const master_entry = (uint64_t *)FW_MASTER_SMODE_ENTRY;
        const uint32_t minion_mask = 0xFFFFU;

        // First HART in each neighborhood
        if (hart_id % 16 == 0) {
            const uint64_t neighborhood_id = get_neighborhood_id();

            volatile uint64_t *const mprot_ptr =
                (volatile uint64_t *)ESR_NEIGH(THIS_SHIRE, neighborhood_id, MPROT);
            uint64_t mprot = *mprot_ptr;
            // Clear io_access_mode, disable_pcie_access, disable_osbox_access
            mprot &= ~0x4Fu;
            // Set secure memory permissions (M/S RX/RW regions), and allow I/O accesses at S-mode
            mprot |= 0x41;
            if (neighborhood_id != 0) {
                // For Neighborhoods 1-3 in master shire: disable access to PCI-E region
                mprot |= 0x04;
            }
            *mprot_ptr = mprot;

            /* Wait for MPROT config on all 2 non-sync neighborhoods
            to complete before any thread can continue. */
            if (atomic_add_local_32(&MM_Thread_Boot_Counter[MASTER_SHIRE].flag, 1U) == 1)
            {
                /* Reset the thread boot counter */
                init_local_spinlock(&MM_Thread_Boot_Counter[MASTER_SHIRE], 0);

                /* Last neighborhood to configure MPROT sends FCC0 to all HARTs (other than sync-minions)
                in this shire minion thread1s aren't enabled yet, so send FCC0 to 16 thread0s. */
                SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
            }
        }

        // Only thread0s participate in the initial MRPOT config rendezvous
        // thread1s boot up later, long after MPROT has been configured per neighborhood
        if (get_thread_id() == 0) {
            WAIT_FCC(0);
        }

        // Jump to master firmware in supervisor mode
        asm volatile("csrw  mepc, %0 \n" // write return address
                     "mret           \n" // return in S-mode
                     :
                     : "r"(master_entry));
    } else { // Worker shire and Master shire sync-minions (upper 16)
        const uint64_t *const worker_entry = (uint64_t *)FW_WORKER_SMODE_ENTRY;
        const uint32_t minion_mask = (get_shire_id() == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;

        // First HART in each neighborhood
        if (hart_id % 16 == 0) {
            const uint64_t neighborhood_id = get_neighborhood_id();

            volatile uint64_t *const mprot_ptr =
                (volatile uint64_t *)ESR_NEIGH(THIS_SHIRE, neighborhood_id, MPROT);
            uint64_t mprot = *mprot_ptr;
            // Clear io_access_mode, disable_pcie_access, disable_osbox_access
            mprot &= ~0x4Fu;
            // Set enable_secure_memory, disable_pcie_access and io_access_mode = b11 (M-mode only)
            mprot |= 0x47;
            *mprot_ptr = mprot;

            // minion thread1s aren't enabled yet, so send FCC0 to all thread0s
            SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
        }

        // Only thread0s participate in the initial MRPOT config rendezvous
        // thread1s boot up later, long after MPROT has been configured per neighborhood
        if (get_thread_id() == 0) {
            WAIT_FCC(0);
        }

        // Jump to worker firmware in supervisor mode
        asm volatile("csrw  mepc, %0 \n" // write return address
                     "mret           \n" // return in S-mode
                     :
                     : "r"(worker_entry));
    }

    while (1) {
        // Should never get here
    }
}
