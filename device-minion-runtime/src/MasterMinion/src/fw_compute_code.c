/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

// Global
#include <stdio.h>

// Local
#include "shire.h"
#include "atomic_ipi.h"
#include "trap_handler.h"
#include "fw_compute_code.h"

///////////////////////////////////////////////////////////////////////////////
// This is the function called after the threads finish the bootrom. They set
// up the mask, barriers and scratchpad and do a WFI and wait for the
// Master Minion to send an IPI with the PC of the operation to execute
///////////////////////////////////////////////////////////////////////////////

void fw_compute_code(void)
{
    // Gets the minion id
    unsigned int minion_id = get_minion_id();
    unsigned int thread_id = get_thread_id();

    if (thread_id == 0)
    {
        // Enable the scratchpad and allocate ways 0, 1 and 2 for all sets
        // This is done in M-mode
        __asm__ __volatile__ (
            "csrwi 0x7e0, 0x1\n" // First enable split
            "csrwi 0x7e0, 0x3\n" // Then enable scratchpad
            :
            :
            : "a1", "a3", "a4", "a5"
        );
    }

    // configure trap vector and move to user mode
    __asm__ __volatile__ (
        "la t0, mtrap_vector\n"
        "csrw mtvec, t0\n"
        "li t0, 0x1800\n"
        "csrrc x0, mstatus, t0\n"  // clear mstatus.mpp
        "la t0, 1f\n"
        "csrw mepc, t0\n"          // set mepc to user-mode entry point
        "mret\n"                   // go to user mode
        "1:\n"                     // label to referr to user-mode
        :
        :
        : "t0"
    );

    minion_id = minion_id & 0x1F;

    // If minion0 within shire, initialize barriers
    if ((minion_id == 0) && (thread_id == 0))
    {
        // Sets the global variables
        uint64_t barrier_t0 = ATOMIC_REGION;

        // Resets the barriers
        __asm__ __volatile__ (
            // Set
            "add       x31, %[barrier_t0], zero\n"
            "addi      x30, x0, 32\n"
            "init_loop:\n"
            "sd        zero, 0(x31)\n"
            "addi      x31, x31, 8\n"
            "addi      x30, x30, -1\n"
            "bne       x30, x0, init_loop\n"
            "fence"
            :
            : [barrier_t0] "r" (barrier_t0)
            : "x30", "x31"
        );

        // Wake up the other threads 0 but current one
        uint64_t fcc_t0_mask = ~0x1ULL;
        uint64_t fcc_t0_addr = IPI_THREAD0;
        uint64_t fcc_t1_mask = ~0x0ULL;
        uint64_t fcc_t1_addr = IPI_THREAD1;

        __asm__ __volatile__ (
            "sd %[fcc_t0_mask], 0(%[fcc_t0_addr])\n"
            "sd %[fcc_t1_mask], 0(%[fcc_t1_addr])\n"
            :
            : [fcc_t0_mask] "r" (fcc_t0_mask),
            [fcc_t0_addr] "r" (fcc_t0_addr),
            [fcc_t1_mask] "r" (fcc_t1_mask),
            [fcc_t1_addr] "r" (fcc_t1_addr)
            :
        );
    }
    else
    {
        // Other minions go to sleep and wait for initialization
        __asm__ __volatile__ (
            "csrw 0x821, x0\n"
        );
    }

    __asm__ __volatile__ (
        // Enables 4 elements of FPU
        "mov.m.x m0, zero, 0x0f\n"
        // Set RM to 0 (round near even)
        "csrrwi x0, frm, 0\n"
    );

    // Barrier to send a ready message to master shire (fcc1)
    uint64_t fcc_to_master = FCC1_MASTER;
    __asm__ __volatile__ (
        "li    x1, 2016\n"                  // Do a barrier on #0 with 64 threads
        "csrrw x1, 0x820, x1\n"
        "beq   x1, x0, no_fcc_ready\n"
        "li    x1, 1\n"                     // FCC to master minion only
        "sd    x1, 0(%[fcc_to_master])\n"
        "no_fcc_ready:\n"
        :
        : [fcc_to_master] "r" (fcc_to_master)
        : "x1"
    );

    // Go to sleep and wait for someone to provide a new PC
    __asm__ __volatile__ (
        "stall_loop:\n"
        "csrw 0x822, x0\n"
        //"csrwi 0x821, 1\n"
        "beq  zero, zero, stall_loop\n"
    );
}
