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
#include "decode.h"
#include "emu.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "esrs.h"
#include "log.h"
#include "memmap.h"
#include "memop.h"
#include "mmu.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "tensor_error.h"
#include "tensor_mask.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


extern std::array<Processor,EMU_NUM_THREADS> cpu;


// Tensor extension
extern void clear_l1scp(unsigned);


// True if a cacheline is locked
bool scp_locked[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];

// Which PA a locked cacheline is mapped to
uint64_t scp_trans[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];


void dcache_change_mode(uint8_t oldval, uint8_t newval)
{
    bool all_change = (oldval ^ newval) & 1;
    bool scp_change = (oldval ^ newval) & 2;
    bool scp_enabled = (newval & 2);

    if (!all_change && !scp_change)
        return;

    unsigned current_minion = current_thread / EMU_THREADS_PER_MINION;

    // clear locks
    if (all_change) {
        for (int i = 0; i < L1D_NUM_SETS; ++i) {
            for (int j = 0; j < L1D_NUM_WAYS; ++j) {
                scp_locked[current_minion][i][j] = false;
                scp_trans[current_minion][i][j] = 0;
            }
        }
    }
    else if (scp_change) {
        for (int i = 0; i < L1D_NUM_SETS - 2; ++i) {
            for (int j = 0; j < L1D_NUM_WAYS; ++j) {
                scp_locked[current_minion][i][j] = false;
                scp_trans[current_minion][i][j] = 0;
            }
        }
    }

    // clear L1SCP
    if (scp_change && scp_enabled) {
        clear_l1scp(current_minion);
    }
}


void dcache_evict_flush_set_way(bool evict, bool tm, int dest, int set, int way, int numlines)
{
    // Skip all if dest is L1, or if set is outside the cache limits
    if ((dest == 0) || (set >= L1D_NUM_SETS))
        return;

    for (int i = 0; i < numlines; i++)
    {
        // skip if masked or evicting and hard-locked
        if ((!tm || tmask_pass(i)) && !(evict && scp_locked[current_thread>>1][set][way]))
        {
            // NB: Hardware sets TensorError[7] if the PA in the set/way
            // corresponds to L2SCP and dest > L2, but we do not keep track of
            // unlocked cache lines.
            if ((dest >= 2) && scp_locked[current_thread>>1][set][way] && paddr_is_scratchpad(scp_trans[current_thread>>1][set][way]))
            {
                LOG(DEBUG, "\tFlushSW: Set: %d, Way: %d, DestLevel: %d cannot flush L2 scratchpad address 0x%016" PRIx64,
                    set, way, dest, scp_trans[current_thread>>1][set][way]);
                update_tensor_error(1 << 7);
                return;
            }
            LOG(DEBUG, "\tDoing %s: Set: %d, Way: %d, DestLevel: %d",
                evict ? "EvictSW" : "FlushSW", set, way, dest);
#ifdef SYS_EMU
            if(sys_emu::get_mem_check())
            {
                unsigned shire = current_thread / EMU_THREADS_PER_SHIRE;
                unsigned minion = (current_thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE;
                if(evict) sys_emu::get_mem_checker().l1_evict_sw(shire, minion, set, way);
                else      sys_emu::get_mem_checker().l1_flush_sw(shire, minion, set, way);
            }
#endif

        }
        // Increment set and way with wrap-around
        if (++set >= L1D_NUM_SETS)
        {
            if (++way >= L1D_NUM_WAYS)
            {
                way = 0;
            }
            set = 0;
        }
    }
}


void dcache_evict_flush_vaddr(bool evict, bool tm, int dest, uint64_t vaddr, int numlines, int id, uint64_t stride)
{
    (void)(id);

    // Skip all if dest is L1
    if (dest == 0)
        return;

    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            cacheop_type cop = CacheOp_None;
            if(evict)
            {
                if     (dest == 1) cop = CacheOp_EvictL2;
                else if(dest == 2) cop = CacheOp_EvictL3;
                else if(dest == 3) cop = CacheOp_EvictDDR;
            }
            else
            {
                if     (dest == 1) cop = CacheOp_FlushL2;
                else if(dest == 2) cop = CacheOp_FlushL3;
                else if(dest == 3) cop = CacheOp_FlushDDR;
            }
            paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, mreg_t(-1), cop);
        }
        catch (const sync_trap_t& t)
        {
            LOG(DEBUG, "\t%s: %016" PRIx64 ", DestLevel: %01x generated exception (suppressed)",
                evict ? "EvictVA" : "FlushVA", vaddr, dest);
            update_tensor_error(1 << 7);
            return;
        }
        LOG(DEBUG, "\tDoing %s: %016" PRIx64 " (%016" PRIx64 "), DestLevel: %01x",
            evict ? "EvictVA" : "FlushVA", vaddr, paddr, dest);
    }
}


void dcache_prefetch_vaddr(uint64_t val)
{
    bool tm         = (val >> 63) & 0x1;
    int  dest       = (val >> 58) & 0x3;
    uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
    int  count      = (val & 0xF) + 1;
    uint64_t stride = X31 & 0x0000FFFFFFFFFFC0ULL;
    //int      id   = X31 & 0x0000000000000001ULL;

    LOG_REG(":", 31);

    // Skip all if dest is MEM
    if (dest == 3)
        return;

    cacheop_type       cop = CacheOp_PrefetchL1;
    if     (dest == 1) cop = CacheOp_PrefetchL2;
    else if(dest == 2) cop = CacheOp_PrefetchL3;

    for (int i = 0; i < count; i++, vaddr += stride)
    {
        if (!tm || tmask_pass(i))
        {
            try {
                cache_line_t tmp;
                uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_Prefetch, mreg_t(-1), cop);
                bemu::pmemread512(paddr, tmp.u32.data());
                LOG_MEMREAD512(paddr, tmp.u32);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(1 << 7);
                return;
            }
            catch (const bemu::memory_error&) {
                raise_bus_error_interrupt(current_thread, 0);
            }
        }
    }
}


void dcache_lock_paddr(int way, uint64_t paddr)
{
    if (!mmu_check_cacheop_access(paddr, CacheOp_Lock))
    {
        LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(1 << 7);
        return;
    }

    unsigned set = dcache_index(paddr, cpu[current_thread].core->mcache_control, current_thread, EMU_THREADS_PER_MINION);

    // Check if paddr already locked in the cache
    int nlocked = 0;
    for (int w = 0; w < L1D_NUM_WAYS; ++w)
    {
        if (scp_locked[current_thread >> 1][set][w])
        {
            ++nlocked;
            if ((w == way) || (scp_trans[current_thread >> 1][set][w] == paddr))
            {
                // Requested PA already locked in a different way, or requested
                // way already locked with a different PA; stop the operation.
                // NB: Hardware sets TensorError[5] also when the PA is
                // in the L1 cache on a different set/way but we do not keep
                // track of unlocked cache lines.
                LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d double-locking on way %d (addr: 0x%016" PRIx64 ")",
                    paddr, way, w, scp_trans[current_thread >> 1][set][w]);
                update_tensor_error(1 << 5);
                return;
            }
        }
    }

    // Cannot lock any more lines in this set; stop the operation
    if (nlocked >= (L1D_NUM_WAYS-1))
    {
        update_tensor_error(1 << 5);
        return;
    }

    try {
        cache_line_t tmp;
        std::fill_n(tmp.u64.data(), tmp.u64.size(), 0);
        bemu::pmemwrite512(paddr, tmp.u32.data());
        LOG_MEMWRITE512(paddr, tmp.u32);
    }
    catch (const sync_trap_t&) {
        LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(1 << 7);
        return;
    }
    catch (const bemu::memory_error&) {
        raise_bus_error_interrupt(current_thread, 0);
        return;
    }
    scp_locked[current_thread >> 1][set][way] = true;
    scp_trans[current_thread >> 1][set][way] = paddr;
    LOG(DEBUG, "\tDoing LockSW: (%016" PRIx64 "), Way: %d, Set: %d", paddr, way, set);
}


void dcache_unlock_set_way(int set, int way)
{
    if ((set < L1D_NUM_SETS) && (way < L1D_NUM_WAYS))
    {
        scp_locked[current_thread >> 1][set][way] = false;
    }
}


void dcache_lock_vaddr(bool tm, uint64_t vaddr, int numlines, int id __attribute__((unused)), uint64_t stride)
{
    cache_line_t tmp;
    std::fill_n(tmp.u64.data(), tmp.u64.size(), 0);

    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        try {
            // LockVA is a hint, so no need to model soft-locking of the cache.
            // We just need to make sure we zero the cache line.
            uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, mreg_t(-1), CacheOp_Lock);
            bemu::pmemwrite512(paddr, tmp.u32.data());
            LOG_MEMWRITE512(paddr, tmp.u32);
            LOG(DEBUG, "\tDoing LockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
        }
        catch (const sync_trap_t& t) {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tLockVA 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return;
        }
        catch (const bemu::memory_error&) {
            raise_bus_error_interrupt(current_thread, 0);
        }
    }
}


void dcache_unlock_vaddr(bool tm, uint64_t vaddr, int numlines, int id __attribute__((unused)), uint64_t stride)
{
    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i)) {
            LOG(DEBUG, "\tSkipping UnlockVA: 0x%016" PRIx64, vaddr);
            continue;
        }

        try {
            // Soft-locking of the cache is not modeled, so there is nothing more to do here.
            uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, mreg_t(-1), CacheOp_Unlock);
            LOG(DEBUG, "\tDoing UnlockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
        }
        catch (const sync_trap_t& t) {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tUnlockVA: 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return;
        }
    }
}


} // namespace bemu
