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

#ifndef SHIRE_CACHE_H
#define SHIRE_CACHE_H

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#define SC_NUM_BANKS 4

#define SC_CACHEOP_OPCODE_ALL_INV   0
#define SC_CACHEOP_OPCODE_L2_INV    1
#define SC_CACHEOP_OPCODE_L2_FLUSH  2
#define SC_CACHEOP_OPCODE_L2_EVICT  3
#define SC_CACHEOP_OPCODE_L3_INV    4
#define SC_CACHEOP_OPCODE_L3_FLUSH  5
#define SC_CACHEOP_OPCODE_L3_EVICT  6
#define SC_CACHEOP_OPCODE_DBG_READ  7
#define SC_CACHEOP_OPCODE_DBG_WRITE 8
#define SC_CACHEOP_OPCODE_SCP_ZERO  9
#define SC_CACHEOP_OPCODE_CB_INV    10

#define SC_CACHEOP_STATE_RESET   (1 << 0)
#define SC_CACHEOP_STATE_ALL_INV (1 << 1)
#define SC_CACHEOP_STATE_IDLE    (1 << 2)
#define SC_CACHEOP_STATE_ACTIVE  (1 << 3)
#define SC_CACHEOP_STATE_CB_INV  (1 << 4)
#define SC_CACHEOP_STATE_DBG     (1 << 5)
#define SC_CACHEOP_STATE_SYNC    (1 << 6)

static inline void sc_idx_cop_sm_ctl_wait_idle(volatile const uint64_t *const addr)
{
    uint64_t state;

    do {
        state = (*addr >> 24) & 0xFF;
    } while (state != SC_CACHEOP_STATE_IDLE);
}

static inline void sc_idx_cop_sm_ctl_go(volatile uint64_t *const addr, uint64_t opcode)
{
    *addr = (1ULL << 0) | // Go bit = 1
            ((opcode & 0xF) << 8); // Opcode
}

static inline void sc_idx_cop_sm_ctl_all_banks_go(uint64_t shire, uint64_t opcode)
{
    // Broadcast to all 4 banks
    volatile uint64_t *const addr = (volatile uint64_t *)ESR_CACHE(shire, 0xF, SC_IDX_COP_SM_CTL);
    sc_idx_cop_sm_ctl_go(addr, opcode);
}

static inline void sc_idx_cop_sm_ctl_all_banks_wait_idle(uint64_t shire)
{
    for (uint64_t i = 0; i < SC_NUM_BANKS; i++) {
        volatile uint64_t *const addr = (volatile uint64_t *)ESR_CACHE(shire, i, SC_IDX_COP_SM_CTL);
        sc_idx_cop_sm_ctl_wait_idle(addr);
    }
}

static inline void sc_cache_bank_op(uint64_t shire, uint64_t bank, uint64_t op)
{
    volatile uint64_t *const addr = (volatile uint64_t *)ESR_CACHE(shire, bank, SC_IDX_COP_SM_CTL);

    sc_idx_cop_sm_ctl_wait_idle(addr);
    sc_idx_cop_sm_ctl_go(addr, op);
    sc_idx_cop_sm_ctl_wait_idle(addr);
}

#endif // ! SHIRE_CACHE_H
