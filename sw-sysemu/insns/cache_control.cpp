/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "cache.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "esrs.h"
#include "insn_util.h"
#include "log.h"
#include "memmap.h"
#include "mmu.h"
#include "system.h"
#include "traps.h"
#include "utility.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

#ifdef SYS_EMU
#define SYS_EMU_PTR cpu.chip->emu()
#endif

namespace bemu {


// Tensor extension
extern void clear_l1scp(Hart&);


void dcache_change_mode(Hart& cpu, uint8_t newval)
{
    uint8_t oldval = cpu.core->mcache_control;
    bool all_change = (oldval ^ newval) & 1;
    bool scp_change = (oldval ^ newval) & 2;
    bool scp_enabled = (newval & 2);

    if (!all_change && !scp_change)
        return;

    // clear locks
    if (all_change) {
        for (int i = 0; i < L1D_NUM_SETS; ++i) {
            for (int j = 0; j < L1D_NUM_WAYS; ++j) {
                cpu.core->scp_lock[i][j] = false;
                cpu.core->scp_addr[i][j] = 0;
            }
        }
    }
    else if (scp_change) {
        for (int i = 0; i < L1D_NUM_SETS - 2; ++i) {
            for (int j = 0; j < L1D_NUM_WAYS; ++j) {
                cpu.core->scp_lock[i][j] = false;
                cpu.core->scp_addr[i][j] = 0;
            }
        }
    }

    // clear L1SCP
    if (scp_change && scp_enabled) {
        clear_l1scp(cpu);
    }
}


void dcache_evict_flush_set_way(Hart& cpu, bool evict, uint64_t value)
{
    bool tm    = (value >> 63) & 0x1;
    int  dest  = (value >> 58) & 0x3;
    int  set   = (value >> 14) & 0xF;
    int  way   = (value >>  6) & 0x3;
    int  count = (value & 0xF) + 1;

    if (tm) {
        LOG_TENSOR_MASK(":");
    }

    // Skip all if dest is L1, or if set is outside the cache limits
    if ((dest == 0) || (set >= L1D_NUM_SETS))
        return;

    if ((dest != 1) && (cpu.mhartid == IO_SHIRE_SP_HARTID)) {
        LOG_HART(WARN, cpu, "\t%s with DestLevel: %d has undefined behavior on the SP",
            evict ? " EvictSW" : "FlushSW", dest);
        return;
    }

    for (int i = 0; i < count; ++i) {
        // skip if masked or evicting and hard-locked
        if ((!tm || cpu.tensor_mask[i]) && !(evict && cpu.core->scp_lock[set][way])) {
            // NB: Hardware raises a bus error if the PA in the set/way
            // corresponds to L2SCP and dest > L2, but we do not keep track of
            // unlocked cache lines.
            if ((dest >= 2) && cpu.core->scp_lock[set][way] && paddr_is_scratchpad(cpu.core->scp_addr[set][way])) {
                LOG_HART(DEBUG, cpu, "\t%s: Set: %d, Way: %d, DestLevel: %d cannot flush L2 scratchpad address 0x%016" PRIx64,
                    evict ? "EvictSW" : "FlushSW", set, way, dest, cpu.core->scp_addr[set][way]);
                throw memory_error(cpu.core->scp_addr[set][way]);
            }
            LOG_HART(DEBUG, cpu, "\tDoing %s: Set: %d, Way: %d, DestLevel: %d",
                evict ? "EvictSW" : "FlushSW", set, way, dest);
#ifdef SYS_EMU
            if (SYS_EMU_PTR->get_mem_check()) {
                unsigned thread = hart_index(cpu);
                unsigned shire = thread / EMU_THREADS_PER_SHIRE;
                unsigned minion = (thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE;
                if (evict) {
                    SYS_EMU_PTR->get_mem_checker().l1_evict_sw(shire, minion, set, way);
                } else {
                    SYS_EMU_PTR->get_mem_checker().l1_flush_sw(shire, minion, set, way);
                }
            }
#endif
        }
        // Increment set and way with wrap-around
        if (++set >= L1D_NUM_SETS) {
            if (++way >= L1D_NUM_WAYS) {
                way = 0;
            }
            set = 0;
        }
    }
}


void dcache_evict_flush_vaddr(Hart& cpu, bool evict, uint64_t value)
{
    bool     tm     = (value >> 63) & 0x1;
    int      dest   = (value >> 58) & 0x3;
    uint64_t vaddr  = value & 0x0000FFFFFFFFFFC0ULL;
    int      count  = (value & 0x0F) + 1;
    uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
    //int      id     = X31 & 0x0000000000000001ULL;

    LOG_REG(":", 31);
    if (tm) {
        LOG_TENSOR_MASK(":");
    }

    // Skip all if dest is L1
    if (dest == 0)
        return;

    if ((dest != 1) && (cpu.mhartid == IO_SHIRE_SP_HARTID)) {
        LOG_HART(WARN, cpu, "\t%s with DestLevel: %d has undefined behavior on the SP",
            evict ? " EvictVA" : "FlushVA", dest);
        return;
    }

    cacheop_type cop = CacheOp_None;
    switch (dest) {
    case 1: cop = evict ? CacheOp_EvictL2  : CacheOp_FlushL2; break;
    case 2: cop = evict ? CacheOp_EvictL3  : CacheOp_FlushL3; break;
    case 3: cop = evict ? CacheOp_EvictDDR : CacheOp_FlushDDR; break;
    default: break;
    }

    for (int i = 0; i < count; ++i) {
        if (!tm || cpu.tensor_mask[i]) {
            try {
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, cop);
                LOG_HART(DEBUG, cpu, "\tDoing %s: 0x%016" PRIx64 " (0x%016" PRIx64 "), DestLevel: %d",
                         evict ? "EvictVA" : "FlushVA", vaddr, paddr, dest);
                if ((dest >= 2) && paddr_is_scratchpad(paddr)) {
                    throw memory_error(paddr);
                }
            }
            catch (const Exception&) {
                LOG_HART(DEBUG, cpu, "\t%s: 0x%016" PRIx64 ", DestLevel: %d generated exception (suppressed)",
                         evict ? "EvictVA" : "FlushVA", vaddr, dest);
                update_tensor_error(cpu, 1 << 7);
                return;
            }
        } else {
            LOG_HART(DEBUG, cpu, "\tSkipping %s: 0x%016" PRIx64 ", DestLevel: %d" PRIx64,
                     evict ? "EvictVA" : "FlushVA", vaddr, dest);
        }
        vaddr += stride;
    }
}


void dcache_prefetch_vaddr(Hart& cpu, uint64_t value)
{
    bool tm         = (value >> 63) & 0x1;
    int  dest       = (value >> 58) & 0x3;
    uint64_t vaddr  = value & 0x0000FFFFFFFFFFC0ULL;
    int  count      = (value & 0xF) + 1;
    uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
    //int      id   = X31 & 0x0000000000000001ULL;

    LOG_REG(":", 31);
    if (tm) {
        LOG_TENSOR_MASK(":");
    }

    // Skip all if dest is MEM
    if (dest == 3)
        return;

    cacheop_type cop;
    switch (dest) {
    case  1: cop = CacheOp_PrefetchL2; break;
    case  2: cop = CacheOp_PrefetchL3; break;
    default: cop = CacheOp_PrefetchL1; break;
    }

    for (int i = 0; i < count; ++i) {
        if (!tm || cpu.tensor_mask[i]) {
            try {
                cache_line_t tmp;
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_Prefetch, cop);
                if (paddr_is_scratchpad(paddr) && dest != 0) {
                    throw memory_error(paddr);
                }
                cpu.chip->memory.read(cpu, paddr, L1D_LINE_SIZE, tmp.u32.data());
                LOG_MEMREAD512(paddr, tmp.u32);
            }
            catch (const Exception&) {
                update_tensor_error(cpu, 1 << 7);
                return;
            }
        }
        vaddr += stride;
    }
}


void dcache_lock_paddr(Hart& cpu, uint64_t value)
{
    int      way   = (value >> 55) & 0x3;
    uint64_t paddr = value & 0x000000FFFFFFFFC0ULL;

    bool cacheop_ok = mmu_check_cacheop_access(cpu, paddr, CacheOp_Lock);

    // Here we just check if there would be an access fault, but do not actually stop.
    // We still want to check whether the PA can _actually_ be locked.
    // This way we can set TensorError[5] and TensorError[7] for the same instruction.
    if (!cacheop_ok) {
        LOG_HART(DEBUG, cpu, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(cpu, 1 << 7);
    }

    unsigned set = dcache_index(paddr, cpu.core->mcache_control, cpu.mhartid % EMU_THREADS_PER_MINION);

    // Check if paddr already locked in the cache
    int nlocked = 0;
    for (int w = 0; w < L1D_NUM_WAYS; ++w) {
        if (cpu.core->scp_lock[set][w]) {
            ++nlocked;
            if ((w == way) || (cpu.core->scp_addr[set][w] == paddr)) {
                // Requested PA already locked in a different way, or requested
                // way already locked with a different PA; stop the operation.
                // NB: Hardware sets TensorError[5] also when the PA is
                // in the L1 cache on a different set/way but we do not keep
                // track of unlocked cache lines.
                LOG_HART(DEBUG, cpu, "\tLockSW: 0x%016" PRIx64 ", Way: %d double-locking on way %d (addr: 0x%016" PRIx64 ")",
                    paddr, way, w, cpu.core->scp_addr[set][w]);
                update_tensor_error(cpu, 1 << 5);
                return;
            }
        }
    }

    // Cannot lock any more lines in this set; stop the operation
    if (nlocked >= (L1D_NUM_WAYS-1)) {
        update_tensor_error(cpu, 1 << 5);
        return;
    }

    // Finally stop if this would cause an access fault
    if (!cacheop_ok) {
        return;
    }

    try {
        cache_line_t tmp;
        std::fill_n(tmp.u64.data(), tmp.u64.size(), 0);
        cpu.chip->memory.write(cpu, paddr, L1D_LINE_SIZE, tmp.u32.data());
        LOG_MEMWRITE512(paddr, tmp.u32);
    }
    catch (const Exception&) {
        LOG_HART(DEBUG, cpu, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(cpu, 1 << 7);
        return;
    }
    cpu.core->scp_lock[set][way] = true;
    cpu.core->scp_addr[set][way] = paddr;
    LOG_HART(DEBUG, cpu, "\tDoing LockSW: (0x%016" PRIx64 "), Way: %d, Set: %d", paddr, way, set);
}


void dcache_unlock_set_way(Hart& cpu, uint64_t value)
{
    int way = (value >> 55) & 0x3;
    int set = (value >>  6) & 0xF;

    if ((set < L1D_NUM_SETS) && (way < L1D_NUM_WAYS)) {
        cpu.core->scp_lock[set][way] = false;
    }
}


void dcache_lock_vaddr(Hart& cpu, uint64_t value)
{
    bool     tm     = (value >> 63) & 0x1;
    uint64_t vaddr  = value & 0x0000FFFFFFFFFFC0ULL;
    int      count  = (value & 0xF) + 1;
    uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
    //int      id     = X31 & 0x0000000000000001ULL;

    LOG_REG(":", 31);
    if (tm) {
        LOG_TENSOR_MASK(":");
    }

    cache_line_t tmp;
    std::fill_n(tmp.u64.data(), tmp.u64.size(), 0);

    for (int i = 0; i < count; ++i) {
        if (!tm || cpu.tensor_mask[i]) {
            try {
                // LockVA is a hint, so no need to model soft-locking of the cache.
                // We just need to make sure we zero the cache line.
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, CacheOp_Lock);
                cpu.chip->memory.write(cpu, paddr, L1D_LINE_SIZE, tmp.u32.data());
                LOG_MEMWRITE512(paddr, tmp.u32);
                LOG_HART(DEBUG, cpu, "\tDoing LockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
            }
            catch (const Exception&) {
                // Stop the operation if there is an exception
                LOG_HART(DEBUG, cpu, "\tLockVA 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
                update_tensor_error(cpu, 1 << 7);
                return;
            }
        } else {
            LOG_HART(DEBUG, cpu, "\tSkipping LockVA: 0x%016" PRIx64, vaddr);
        }
        vaddr += stride;
    }
}


void dcache_unlock_vaddr(Hart& cpu, uint64_t value)
{
    bool     tm     = (value >> 63) & 0x1;
    uint64_t vaddr  = value & 0x0000FFFFFFFFFFC0ULL;
    int      count  = (value & 0xF) + 1;
    uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
    //int      id     = X31 & 0x0000000000000001ULL;

    LOG_REG(":", 31);
    if (tm) {
        LOG_TENSOR_MASK(":");
    }

    for (int i = 0; i < count; ++i) {
        if (!tm || cpu.tensor_mask[i]) {
            try {
                // Soft-locking of the cache is not modeled, so there is nothing more to do here.
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, CacheOp_Unlock);
                LOG_HART(DEBUG, cpu, "\tDoing UnlockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
            }
            catch (const Exception&) {
                // Stop the operation if there is an exception
                LOG_HART(DEBUG, cpu, "\tUnlockVA: 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
                update_tensor_error(cpu, 1 << 7);
                return;
            }
        } else {
            LOG_HART(DEBUG, cpu, "\tSkipping UnlockVA: 0x%016" PRIx64, vaddr);
        }
        vaddr += stride;
    }
}


} // namespace bemu
