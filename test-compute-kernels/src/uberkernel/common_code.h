/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 *
 ************************************************************
 * ---------------------------------------------------------
 * This code is Auto-Generated. Please DON'T MODIFY IT.
 * ---------------------------------------------------------
 ************************************************************
 *
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------
 */

#ifndef _COMMON_CODE_H_
#define _COMMON_CODE_H_

// Global
#include <inttypes.h>

// FW syscall IDs
#include "etsoc/isa/syscall.h"

// Shared
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"

// Helper thread masks
#define SYNC_SHIRE_ID             32
#define COMPUTE_THREADS           0xFFFFFFFFULL
#define HELPER_ACTIVATION_THREADS 0x0000FFFFULL
#define HELPER_WEIGHTS_THREADS    0x0F0F0000ULL
#define HELPER_DRAIN_THREADS      0x30300000ULL
#define HELPER_CODE_THREADS       0x40000000ULL
#define HELPER_DDR_THREADS        0x80000000ULL
#define SYNC_MINIONS              0xFFFF0000ULL

// Some defines
#define THREAD_0 0
#define THREAD_1 1
#define FCC_0    0
#define FCC_1    1
#define MASTER_SHIRE 32

#define SC_CACHEOP_L2_EVICT 0x3

// Global functions

// "Device common lib" helpers
#define flbarrier(barrier_id, num_threads)            ({ uint64_t res; WAIT_FLB(num_threads + 1, barrier_id, res); res; })
#define fcc_send(prv, shire_id, thread, fcc_id, mask) ({ SEND_FCC(shire_id, thread, fcc_id, mask); })
#define fcc(fcc_id)                                   ({ wait_fcc(fcc_id); })

// This function sends one FCC to a sync minion when the last hart gets
// to the barrier
static inline void global_barrier_starter(
                            uint64_t num_harts, // Harts doing barrier in the source shires
                            uint64_t flb_num,   // FLB to be used in the source shire for the barrier
                            uint64_t shire_id,  // Source shire id
                            uint64_t fcc)       // Which FCC to send the shire ready signal
{
    volatile uint64_t * sync_minion_addr = (uint64_t * ) (
                                           (1ULL << 32)          // ESR
                                         + (32ULL << 22)         // Going to master shire
                                         + (0x1AULL << 17)       // Shire other ESRs
                                         + 0xC0ULL               // FCC ESRs
                                         + ((shire_id & 1) * 16) // Which thread is going to
                                         + (fcc * 8));           // FCC destination
    uint64_t sync_minion_data = 1ULL << ((shire_id / 2) + 16); // Send FCC to according sync minion
    uint64_t flb_result = flbarrier(flb_num, num_harts - 1);
    if(flb_result == 1)
        * sync_minion_addr = sync_minion_data;
}

// This function wiats for one FCC to a sync minion when the last hart gets
// to the barrier
static inline void global_barrier_receiver(
                            uint64_t fcc_wait,         // Which FCC to wait for
                            uint64_t flb_num,          // FLB to be used in the source shire for the barrier
                            uint64_t minion_id,        // Sync minion id
                            uint64_t thread_id,        // Sync thread id
                            uint64_t thread_dest,      // Thread of the FCC dest
                            uint64_t fcc_dest,         // FCC for dest
                            uint64_t minion_mask_dest,  // Mask of minions in dest shire to receive FCC
                            uint64_t n_compute_shires   //number of compute Shires
                            ) // Mask of minions in dest shire to receive FCC
{
    // Waits for its associated shire FCC and do FLB
    fcc(fcc_wait);
    uint64_t flb_result = flbarrier(flb_num, n_compute_shires - 1);

    // If last wake up other sync minions
    if(flb_result == 1)
    {
        // Send to FCC1
        fcc_send(PRV_U, SYNC_SHIRE_ID, THREAD_0, FCC_1, SYNC_MINIONS);
        fcc_send(PRV_U, SYNC_SHIRE_ID, THREAD_1, FCC_1, SYNC_MINIONS);
    }

    // Waits for FCC that last sync minion got FCC
    fcc(FCC_1);

    // Sends FCC to destination
    fcc_send(PRV_U, ((minion_id - 16) * 2) + thread_id, thread_dest, fcc_dest, minion_mask_dest);
}

static inline void ecall_l1_evict_all(uint64_t use_tmask, uint64_t dest_level)
{
    syscall(SYSCALL_CACHE_OPS_EVICT_L1, use_tmask, dest_level, 0);
}

static inline void ecall_shire_cache_bank_op(uint64_t shire, uint64_t bank, uint64_t op)
{
    syscall(SYSCALL_SHIRE_CACHE_BANK_OP, shire, bank, op);
}

#endif


