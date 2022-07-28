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

#ifndef __CACHEOPS_UMODE_H
#define __CACHEOPS_UMODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "etsoc/isa/utils.h"
#include "etsoc/isa/syscall.h"

enum cop_dest { to_L1 = 0x0ULL, to_L2 = 0x1ULL, to_L3 = 0x2ULL, to_Mem = 0x3ULL };

enum l1d_mode { l1d_shared, l1d_split, l1d_scp };

#define CALC_CACHE_OPS_VAL(use_tmask, dst, way, set, num_lines)              \
    (((((uint64_t)use_tmask) & 1) << 63) | ((((uint64_t)dst) & 0x3) << 58) | \
        ((((uint16_t)set) & 0xF) << 14) | ((way & 0x3) << 6) | (((num_lines)&0xF)))

#define CALC_CSR_ENC_VAL(use_tmask, dst, addr, num_lines)                   \
    ((((uint64_t)use_tmask) & 1) << 63) |                                   \
        ((((uint64_t)dst) & 0x3 /* 00=L1, 01=L2, 10=L3, 11=MEM */) << 58) | \
        ((((uint64_t)addr) & 0xFFFFFFFFFFC0ULL)) | (((num_lines)&0xF))
//-------------------------------------------------------------------------------------------------
//   Privledged U-Mode cache operations
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_priv_evict_sw
//
//   This function evicts the specified set and way from the cache hierarchy, up to the provided
//   destination. Optionally, a repeat count can be specified to evict more adjacent cache lines.
//   Optionally, each potential line eviction can be gated by the value of the TensorMask CSR.
//
#define cache_ops_priv_evict_sw(use_tmask, dst, way, set, num_lines)                             \
    syscall(SYSCALL_CACHE_OPS_EVICT_SW, CALC_CACHE_OPS_VAL(use_tmask, dst, way, set, num_lines), \
        0, 0);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_priv_flush_sw
//
//   This function writes back the specified set and way up to the provided cached level, if the
//   line is dirty. Optionally, a repeat count can be specified to flush more adjacent cache lines.
//   Optionally, each potential line flush can be gated by the value of the TensorMask CSR.
//
#define cache_ops_priv_flush_sw(use_tmask, dst, way, set, num_lines)                             \
    syscall(SYSCALL_CACHE_OPS_FLUSH_SW, CALC_CACHE_OPS_VAL(use_tmask, dst, way, set, num_lines), \
        0, 0);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_priv_l1_cache_lock_sw
//
//   Hard-lock and zero a particular set-way in the L1 data cache
//
#define cache_ops_priv_l1_cache_lock_sw(way, phy_addr) \
    syscall(SYSCALL_CACHE_OPS_LOCK_SW,                 \
        (((((uint64_t)way) & 0x3) << 55) | ((((uint64_t)phy_addr) & 0xFFFFFFFFC0ULL))), 0, 0);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_priv_l1_cache_unlock_sw
//
//   Hard-unlock and zero a particular set-way in the L1 data cache
//
#define cache_ops_priv_l1_cache_unlock_sw(way, set)                                              \
    syscall(SYSCALL_CACHE_OPS_UNLOCK_SW, (((((uint64_t)way) & 0x3) << 55) | ((set & 0xF) << 6)), \
        0, 0);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_priv_cache_invalidate
//
//   This function invalidates various cache structures used by the minion core.
//
#define cache_ops_priv_cache_invalidate(inval_instr_cache, inval_TLBs_and_PTW) \
    syscall(SYSCALL_CACHE_OPS_INVALIDATE,                                      \
        ((((uint64_t)inval_TLBs_and_PTW & 1)) | (((uint64_t)inval_instr_cache & 1) << 1)), 0, 0);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_priv_evict_l1
//
//   This function invalidates the L1 of the minion core to the desired cache destination level.
//   The user can optionally use the tensor mask to decide which sets to evict.
//
#define cache_ops_priv_evict_l1(use_tmask, dest_level) \
    syscall(SYSCALL_CACHE_OPS_EVICT_L1, (uint64_t)use_tmask, (uint64_t)dest_level, 0);

//-------------------------------------------------------------------------------------------------
//   Instructions available to U-Mode, S-Mode, and M-Mode
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_evict_va
//
//   This function evicts the specified virtual address from the cache hierarchy, up to the provided
//   cache level. Optionally, a repeat count can be specified to evict more lines, whose addresses
//   are calculated using the provided stride.
//   Optionally, each potential line eviction can be gated by the value of the TensorMask CSR.
//
#define cache_ops_evict_va(use_tmask, dst, addr, num_lines, stride, id)                   \
    {                                                                                     \
        uint64_t csr_enc = CALC_CSR_ENC_VAL(use_tmask, dst, addr, num_lines);             \
        register uint64_t x31_enc asm("x31") = (((uint64_t)stride) & 0xFFFFFFFFFFC0ULL) | \
                                               (id & 0x1);                                \
        __asm__ __volatile__("csrw 0x89f, %[csr_enc]\n"                                   \
                             :                                                            \
                             : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));           \
    }

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_evict
//
//   This function evicts all cache lines from address to address+size up to the provided
//   cache level.
//
#define cache_ops_evict(dest, address, size)                               \
    cache_ops_evict_va(0, (enum cop_dest)dest, (volatile uint64_t)address, \
        (((uint64_t)address & 0x3F) + (uint64_t)size) >> 6, 64, 0);

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_flush_va
//
//   This function flushes the specified virtual address from the cache hierarchy, if it is present
//   and dirty, up to the provided cache level. Optionally, a repeat count can be specified to flush
//   more lines, whose addresses are calculated using the provided stride.
//   Optionally, each potential line flush can be gated by the value of the TensorMask CSR.
//
#define cache_ops_flush_va(use_tmask, dst, addr, num_lines, stride, id)                   \
    {                                                                                     \
        uint64_t csr_enc = CALC_CSR_ENC_VAL(use_tmask, dst, addr, num_lines);             \
        register uint64_t x31_enc asm("x31") = (((uint64_t)stride) & 0xFFFFFFFFFFC0ULL) | \
                                               (id & 0x1);                                \
        __asm__ __volatile__("csrw 0x8bf, %[csr_enc]\n"                                   \
                             :                                                            \
                             : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));           \
    }
//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_prefetch_va
//
//   This function prefetches the provided virtual address to the specified cache level.
//   Optionally, a repeat count can be provided to pretech more lines, whose addresses are
//   calculated using the provided stride.
//   Optionally, each line prefetch can be gated by the value of the TensorMask CSR.
//
#define cache_ops_prefetch_va(use_tmask, dst, addr, num_lines, stride, id)                \
    {                                                                                     \
        uint64_t csr_enc = CALC_CSR_ENC_VAL(use_tmask, dst, addr, num_lines);             \
        register uint64_t x31_enc asm("x31") = (((uint64_t)stride) & 0xFFFFFFFFFFC0ULL) | \
                                               (id & 0x1);                                \
        __asm__ __volatile__("csrw 0x81f, %[csr_enc]\n"                                   \
                             :                                                            \
                             : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));           \
    }

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_lock_va
//
//   This function soft-locks the provided virtual address in the L1, meaning that it will never
//   be chosen for line replacement if other non-locked lines are present in the same set.
//   Optionally, a repeat count can be provided to soft-lock more lines, whose addresses are
//   calculated using the provided stride.
//   Optionally, each line lock can be gated by the value of the TensorMask CSR.
//
#define cache_ops_lock_va(use_tmask, addr, num_lines, stride, id)                          \
    {                                                                                      \
        uint64_t csr_enc = ((((uint64_t)use_tmask) & 1) << 63) |                           \
                           ((((uint64_t)addr) & 0xFFFFFFFFFFC0ULL)) | (((num_lines)&0xF)); \
        register uint64_t x31_enc asm("x31") = (((uint64_t)stride) & 0xFFFFFFFFFFC0ULL) |  \
                                               (id & 0x1);                                 \
        __asm__ __volatile__("csrw 0x8df, %[csr_enc]\n"                                    \
                             :                                                             \
                             : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));            \
    }

//-------------------------------------------------------------------------------------------------
//
// FUNCTION: cache_ops_unlock_va
//
//   This function removes the soft-lock of the provided virtual address.
//   Optionally, a repeat count can be provided to unlock more lines, whose addresses are
//   calculated using the provided stride.
//   Optionally, each unlock can be gated by the value of the TensorMask CSR.
//
#define cache_ops_unlock_va(use_tmask, addr, num_lines, stride, id)                        \
    {                                                                                      \
        uint64_t csr_enc = ((((uint64_t)use_tmask) & 1) << 63) |                           \
                           ((((uint64_t)addr) & 0xFFFFFFFFFFC0ULL)) | (((num_lines)&0xF)); \
        register uint64_t x31_enc asm("x31") = (((uint64_t)stride) & 0xFFFFFFFFFFC0ULL) |  \
                                               (id & 0x1);                                 \
        __asm__ __volatile__("csrw 0x8ff, %[csr_enc]\n"                                    \
                             :                                                             \
                             : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));            \
    }

//
// UCACHE_CONTROL
//
#define cache_ops_ucache_control(scp_en, cacheop_rate, cacheop_max)                          \
    {                                                                                        \
        uint64_t csr_enc = ((((uint64_t)cacheop_max) & 0x1F) << 6) |                         \
                           ((((uint64_t)cacheop_rate) & 0x7) << 2) |                         \
                           ((((uint64_t)scp_en) & 0x1) << 1);                                \
        __asm__ __volatile__("csrw 0x810, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "x31"); \
    }

#define cache_ops_get_l1d_mode                                                                    \
    ({                                                                                            \
        uint64_t csr_enc;                                                                         \
        __asm__ __volatile__("csrr %[csr_enc], 0x810\n" : [csr_enc] "=r"(csr_enc) : :);           \
        (((csr_enc & 0x3) == 0x3) ? l1d_scp : ((csr_enc & 0x3) == 0x1) ? l1d_split : l1d_shared); \
    })

#define cache_ops_scp(warl, DEscratchpad)                                                    \
    {                                                                                        \
        /* Hard partition L1 Data cache between the harts */                                 \
        FENCE;                                                                               \
        WAIT_CACHEOPS;                                                                       \
        /* Enable scratchpad */                                                              \
        uint64_t csr_enc = ((((uint64_t)warl) & 0x7FFFFFFFFFFFFFFF) << 1) |                  \
                           ((((uint64_t)DEscratchpad) & 0x1));                               \
        __asm__ __volatile__("csrw 0x810, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "x31"); \
    }

#define cache_ops_cb_drain(drain_shire, drain_bank)                                               \
    {                                                                                             \
        /* Drain the coalescing buffer of shire cache bank */                                     \
        /* 1. Write the CB invalidate (assumes FSM always available) */                           \
        volatile uint64_t *sc_idx_cop_sm_ctl_addr = (volatile uint64_t *)ESR_CACHE(               \
            ((uint64_t)drain_shire), ((uint64_t)drain_bank), SC_IDX_COP_SM_CTL_USER);             \
        /* 2. Checks done */                                                                      \
        uint64_t state;                                                                           \
        do                                                                                        \
        {                                                                                         \
            state = (*sc_idx_cop_sm_ctl_addr >> 24) & 0xFF;                                       \
        } while (state != 4);                                                                     \
        *sc_idx_cop_sm_ctl_addr = (1 << 0) | /* Go bit = 1  */                                    \
                                  (10 << 8); /* Opcode = CB_Inv (Coalescing buffer invalidate) */ \
        /* 3. Checks done */                                                                      \
        do                                                                                        \
        {                                                                                         \
            state = (*sc_idx_cop_sm_ctl_addr >> 24) & 0xFF;                                       \
        } while (state != 4);                                                                     \
    }

#ifdef __cplusplus
}
#endif

#endif // ! __CACHEOPS_UMODE_H
