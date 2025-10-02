#ifndef SYNC_MINIONS_H
#define SYNC_MINIONS_H

#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"

#define ALL_BANKS_MASK 0xFUL;
#define OPCODE_FLUSH_CB 0x0A01UL;

// Custom user-level functions that sync up minions inside a kernel
// This is currently used only for directed tests

// SCB (Store coalescing buffer) drain.
// Sync up minions in a shire with an FLB.
static inline void drain_scb(uint64_t shire_id, uint64_t minion_id, uint64_t flb_id)
{
    uint64_t barrier_result;
    WAIT_FLB(32, flb_id, barrier_result);
    if (barrier_result == 1) {

        // The last minion to reach this barrier flushes the CB and sends a credit to all others to continue
        // Having more than one minions flush the CB at the same time may end up in having one of the flushes
        // dropped by the shire cache
	uint64_t sc_bank_mask = ALL_BANKS_MASK;
        uint64_t flush_cb_opcode = OPCODE_FLUSH_CB;
        volatile uint64_t *cb_flush_addr = (volatile uint64_t *)ESR_CACHE(shire_id, sc_bank_mask, SC_IDX_COP_SM_CTL_USER);
        store((uint64_t) cb_flush_addr, flush_cb_opcode);

        __asm__ __volatile__ ("fence\n");

	// You will need to poll each bank separately and make sure flushing has completed before you proceed
        uint64_t cb_busy = 0;
        while (cb_busy != 0x4) {
            uint64_t cb_busy_bank[4];
            for (uint64_t b=0; b < 4; b++) {
                cb_flush_addr = (volatile uint64_t *)ESR_CACHE(shire_id, b, SC_IDX_COP_SM_CTL_USER);
                cb_busy_bank[b] = ((*cb_flush_addr) >> 24) & 0x4;
            }
            cb_busy = cb_busy_bank[0] | cb_busy_bank[1] | cb_busy_bank[2] | cb_busy_bank[3];
        }

	// Flushing is done send a credit to other minions
        uint64_t target_min_mask = 0xFFFFFFFFUL;
        target_min_mask = target_min_mask & (~(1ULL << (minion_id & 0x1f)));
        SEND_FCC(shire_id, 0, 0, target_min_mask);
    }

    // If you are not the last minion to reach barrier, wait for a credit.
    else {
        WAIT_FCC(0);
    }
}


// Loose synchronization across all minions in every shire, without involving master shire
// Minions in all shires block waiting for a credit. The only minions that do not block and
// send credit to others are minions whose minion id is the same as their shire id.
// So minion 0, shire 0 sends credits to all minion 0's in all other shires
// Minion 1, shire 1, sends credits to all minion 1's in all other shires
// After that each shire does and FLB synchronization
// This synchronization is not perfect. Some shires may end up ahead of others, but it is good enough
// to sync up minions when they are thousands of cycles apart.
// It currently works only for even harts
static inline void sync_up_all_minions(uint64_t minion_id, uint32_t num_shires, uint64_t flb_id)
{
    // Sync up minions across shires.
    // Minion with minion_id = shire_id sends credit to all other shires to minions with same minion_id.
    uint64_t local_minion_id = minion_id & 0x1F;
    uint64_t target_min_mask = 1ULL << local_minion_id;
    uint64_t shire_id = (minion_id >> 5) & 0x1F;
    if (local_minion_id == shire_id) {
      for (uint64_t target_shire=0; target_shire < num_shires; target_shire++) {
        if (shire_id == target_shire) continue;
        SEND_FCC(target_shire, 0, 0, target_min_mask);
      }
    } else {
      WAIT_FCC(0);
    }

    // Synchronize all minions in shires now
    uint64_t barrier_result;
    WAIT_FLB(32, flb_id, barrier_result);
    if (barrier_result == 1) {
      target_min_mask = 0xFFFFFFFFUL;
      target_min_mask = target_min_mask & (~(1ULL << minion_id));
      SEND_FCC(shire_id, 0, 1, target_min_mask);
    } else {
      WAIT_FCC(1);
    }
    FENCE;
}
#endif
